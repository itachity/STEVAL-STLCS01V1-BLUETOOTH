/**
 ******************************************************************************
 * @file    ble_app.c
 * @brief   Minimal BLE application that advertises a SensorTile-style
 *          Environmental service and streams a repeating 1..10 counter at
 *          10 Hz to any connected STBLESensor client.
 *
 * How it shows up in the STBLESensor app:
 *   - The device advertises itself as "STLBLE1" (7 chars is the protocol
 *     requirement enforced by STBLESensor).
 *   - Manufacturer data byte 17 carries the feature bitmap 0x04 (1 temp
 *     sensor) so the app knows to render an Environmental tab.
 *   - The Environmental characteristic is 4 bytes:
 *       [timestamp LE : 2][temperature LE : 2]
 *     temperature is expressed in tenths of a degree Celsius. We load
 *     (counter * 10) into it, so the app displays 1.0, 2.0, ..., 10.0 °C.
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "ble_app.h"
#include "hci_tl_interface.h"
#include "uart_console.h"

#include "hci.h"
#include "hci_const.h"
#include "hci_le.h"
#include "bluenrg_def.h"
#include "bluenrg_gap.h"
#include "bluenrg_gap_aci.h"
#include "bluenrg_gatt_aci.h"
#include "bluenrg_gatt_server.h"
#include "bluenrg_hal_aci.h"
#include "bluenrg_aci_const.h"
#include "bluenrg_utils.h"
#include "sm.h"          /* MITM_PROTECTION_REQUIRED, OOB_AUTH_DATA_ABSENT,
                            USE_FIXED_PIN_FOR_PAIRING, BONDING              */

#include "uuid_ble_service.h"

/* --- Forward decls ------------------------------------------------------- */
static int  BLE_Stack_Init(void);
static int  BLE_AddEnvService(void);
static void BLE_SetConnectable(void);
static void BLE_HandleEvent(void *pckt);
static void BLE_SendCounter(uint8_t value);

/* --- State --------------------------------------------------------------- */
/* Advertised device name: STBLESensor treats the first 7 bytes of the local
 * name as the "board name". Must be exactly 7 chars. */
#define STLBLE_LOCAL_NAME "STLBLE1"

static uint16_t env_service_handle;
static uint16_t env_char_handle;

static volatile uint8_t  g_connected       = 0;
static volatile uint8_t  g_set_connectable = 1;
static volatile uint8_t  g_env_notify_on   = 0;   /* set when central subscribes */

static uint8_t g_bdaddr[6];

/* Counter state */
static uint8_t  g_counter     = 1;
static uint32_t g_last_emit_ms = 0;

/* ============================================================= */
/*                    Public API                                 */
/* ============================================================= */

int BLE_App_Init(void)
{
  /* 0) Bring up the serial console first so any later printf() is visible */
  Console_Init();
  printf("\r\n[BLE_App] boot\r\n");

  /* 1) Register HCI IO bus + EXTI callback */
  hci_tl_lowlevel_init();

  /* 2) Bring up BlueNRG stack, configure addr + GAP/GATT */
  if (BLE_Stack_Init() != 0) {
    return -1;
  }

  /* 3) Add our service + characteristic */
  if (BLE_AddEnvService() != 0) {
    return -2;
  }

  /* 4) Start advertising */
  BLE_SetConnectable();
  g_set_connectable = 0;

  g_last_emit_ms = HAL_GetTick();
  return 0;
}

void BLE_App_Run(void)
{
  /* Pump BlueNRG events */
  if (HCI_ProcessEvent) {
    HCI_ProcessEvent = 0;
    hci_user_evt_proc();
  }

  /* Re-arm advertising after disconnect */
  if (g_set_connectable) {
    BLE_SetConnectable();
    g_set_connectable = 0;
  }

  /* 10 Hz counter tick — always prints; pushes BLE notify when subscribed */
  {
    uint32_t now = HAL_GetTick();
    if ((now - g_last_emit_ms) >= 100U) {
      g_last_emit_ms = now;

      printf("%u\r\n", (unsigned)g_counter);

      if (g_connected && g_env_notify_on) {
        BLE_SendCounter(g_counter);
      }

      g_counter++;
      if (g_counter > 10) g_counter = 1;
    }
  }
}

void BLE_App_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == HCI_TL_SPI_EXTI_PIN) {
    hci_tl_lowlevel_isr();
  }
}

/* ============================================================= */
/*                    Stack init                                  */
/* ============================================================= */

static int BLE_Stack_Init(void)
{
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  uint8_t  hwVersion;
  uint16_t fwVersion;
  tBleStatus ret;

  hci_init(BLE_HandleEvent, NULL);

  /* Fetch version (also exercises the SPI transport early) */
  getBlueNRGVersion(&hwVersion, &fwVersion);

  /* Full reset so that aci_hal_write_config_data can be the first cmd */
  hci_reset();
  HAL_Delay(100);

  /* Derive a stable random BD addr from the STM32's unique ID */
  {
    const uint32_t *uid = (uint32_t *)0x1FFF7590;
    g_bdaddr[0] = (uid[1] >> 24) & 0xFF;
    g_bdaddr[1] = (uid[0]      ) & 0xFF;
    g_bdaddr[2] = (uid[2] >> 8 ) & 0xFF;
    g_bdaddr[3] = (uid[0] >> 16) & 0xFF;
    g_bdaddr[4] = 0xE1;   /* arbitrary — 2 chars from the name suffix */
    g_bdaddr[5] = 0xC0;   /* top 2 bits = 11 => Static Random addr */
  }

  ret = aci_gatt_init();
  if (ret) return -1;

  ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07,
                             &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (ret != BLE_STATUS_SUCCESS) return -2;

  ret = hci_le_set_random_address(g_bdaddr);
  if (ret) return -3;

  /* Set the GAP device-name characteristic to our 7-char local name */
  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0,
                                   7, (uint8_t *)STLBLE_LOCAL_NAME);
  if (ret) return -4;

  ret = aci_gap_set_auth_requirement(MITM_PROTECTION_REQUIRED,
                                     OOB_AUTH_DATA_ABSENT,
                                     NULL, 7, 16,
                                     USE_FIXED_PIN_FOR_PAIRING, 123456,
                                     BONDING);
  if (ret != BLE_STATUS_SUCCESS) return -5;

  aci_hal_set_tx_power_level(1, 4);
  return 0;
}

/* ============================================================= */
/*            Service + characteristic definition                 */
/* ============================================================= */

static int BLE_AddEnvService(void)
{
  tBleStatus ret;
  uint8_t uuid[16];

  /* Hardware (features) service */
  COPY_HW_SENS_W2ST_SERVICE_UUID(uuid);
  ret = aci_gatt_add_serv(UUID_TYPE_128, uuid, PRIMARY_SERVICE, 1 + 3 * 1,
                          &env_service_handle);
  if (ret != BLE_STATUS_SUCCESS) return -1;

  /* Environmental characteristic with the "1 temp sensor" flag set */
  COPY_ENVIRONMENTAL_W2ST_CHAR_UUID(uuid);
  uuid[14] |= 0x04;   /* feature-mask byte: 0x04 = one temperature channel */

  /* payload = 2B timestamp + 2B temp = 4 bytes */
  ret = aci_gatt_add_char(env_service_handle, UUID_TYPE_128, uuid, 4,
                          CHAR_PROP_NOTIFY | CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
                          16, 0, &env_char_handle);
  if (ret != BLE_STATUS_SUCCESS) return -2;

  return 0;
}

/* ============================================================= */
/*            Advertising                                         */
/* ============================================================= */

static void BLE_SetConnectable(void)
{
  /* 1-byte AD_TYPE + 7 chars = 8-byte local-name blob */
  char local_name[8] = { AD_TYPE_COMPLETE_LOCAL_NAME,
                         'S','T','L','B','L','E','1' };

  /* ST BLE Sensor manuf-data format (per ST protocol spec):
   *   [len=2][0x0A = TX Power][0x00]
   *   [len=8][0x09 = Complete Local Name][7 bytes]
   *   [len=13][0xFF = Manuf Specific]
   *     [0x01 = SDK version]
   *     [0x02]
   *     [manuf_data[15] : board type / flags]
   *     [manuf_data[16] : features hi  — 0x20 bit = LED]
   *     [manuf_data[17] : features lo  — 0x04 = 1 temp sensor]
   *     [manuf_data[18] : features    ]
   *     [manuf_data[19] : features    ]
   *     [6 bytes BLE MAC (reverse order)]
   */
  uint8_t manuf_data[26] = {
    2, 0x0A, 0x00,
    8, 0x09, 'S','T','L','B','L','E','1',
    13, 0xFF,
    0x01, 0x02,
    0x00,
    0x00,
    0x04,   /* <-- 1 temperature sensor */
    0x00,
    0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* MAC placeholder */
  };

  manuf_data[20] = g_bdaddr[5];
  manuf_data[21] = g_bdaddr[4];
  manuf_data[22] = g_bdaddr[3];
  manuf_data[23] = g_bdaddr[2];
  manuf_data[24] = g_bdaddr[1];
  manuf_data[25] = g_bdaddr[0];

  hci_le_set_scan_resp_data(0, NULL);

  aci_gap_set_discoverable(ADV_IND, 0, 0,
                           STATIC_RANDOM_ADDR,
                           NO_WHITE_LIST_USE,
                           sizeof(local_name), local_name,
                           0, NULL, 0, 0);

  aci_gap_update_adv_data(26, manuf_data);
}

/* ============================================================= */
/*            Counter -> GATT notification                        */
/* ============================================================= */

static void BLE_SendCounter(uint8_t value)
{
  uint8_t buff[4];
  int16_t temp_tenths = (int16_t)value * 10;   /* 1 -> 10, 2 -> 20, ... */

  STORE_LE_16(buff,     (HAL_GetTick() >> 3));  /* 8 ms tick-based timestamp */
  STORE_LE_16(buff + 2, temp_tenths);

  aci_gatt_update_char_value(env_service_handle, env_char_handle, 0, 4, buff);
}

/* ============================================================= */
/*            HCI event demultiplexer                             */
/* ============================================================= */

static void BLE_HandleEvent(void *pckt)
{
  hci_uart_pckt  *hci_pckt   = pckt;
  hci_event_pckt *event_pckt = (hci_event_pckt *)hci_pckt->data;

  if (hci_pckt->type != HCI_EVENT_PKT) return;

  switch (event_pckt->evt) {
    case EVT_DISCONN_COMPLETE:
      g_connected       = 0;
      g_env_notify_on   = 0;
      g_set_connectable = 1;
      break;

    case EVT_LE_META_EVENT: {
      evt_le_meta_event *evt = (void *)event_pckt->data;
      if (evt->subevent == EVT_LE_CONN_COMPLETE) {
        g_connected = 1;
      }
      break;
    }

    case EVT_VENDOR: {
      evt_blue_aci *blue_evt = (void *)event_pckt->data;
      if (blue_evt->ecode == EVT_BLUE_GATT_ATTRIBUTE_MODIFIED) {
        evt_gatt_attr_modified_IDB05A1 *m =
            (evt_gatt_attr_modified_IDB05A1 *)blue_evt->data;

        /* The CCCD for our env characteristic sits at env_char_handle + 2.
         * att_data[0] == 1 -> notifications enabled, 0 -> disabled. */
        if (m->attr_handle == env_char_handle + 2) {
          g_env_notify_on = (m->att_data[0] == 1);
          /* NOTE: we used to reset g_counter here so STBLESensor always saw
           *       a clean 1..10 cycle, but that would also jump the terminal
           *       output. Counter now runs free; subscribers join mid-cycle. */
        }
      }
      break;
    }

    default:
      break;
  }
}
