/**
 ******************************************************************************
 * @file    hci_tl_interface.c
 * @brief   BlueNRG-MS HCI Transport Layer — SPI1 glue for the SensorTile.
 *
 * Adapted from STMicro's STSW-STLKT01_V2.5.0 BLE_SampleApp. The only change
 * vs. the reference is that BSP_SPI1_* is replaced with BLE_BSP_SPI1_* so this
 * file is self-contained (no dependency on SensorTile_bus.c).
 ******************************************************************************
 */
#include "hci_tl.h"
#include "hci_tl_interface.h"
#include "stm32l4xx_hal_exti.h"

#define HCI_TL
#define HEADER_SIZE       5U
#define MAX_BUFFER_SIZE   255U
#define TIMEOUT_DURATION  15U

EXTI_HandleTypeDef          hexti5;
volatile uint32_t           HCI_ProcessEvent;

/* --- IO Operation functions registered with the BlueNRG stack ------------ */

int32_t HCI_TL_SPI_Init(void *pConf)
{
  (void)pConf;
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  HAL_PWREx_EnableVddIO2();              /* PH0 lives in VddIO2 domain */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* CS idle high before we drive it */
  HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_SET);

  /* BlueNRG IRQ line (PC5) as EXTI rising */
  GPIO_InitStruct.Pin   = HCI_TL_SPI_EXTI_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(HCI_TL_SPI_EXTI_PORT, &GPIO_InitStruct);

  /* RESET (PH0) as push-pull output */
  GPIO_InitStruct.Pin   = HCI_TL_RST_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HCI_TL_RST_PORT, &GPIO_InitStruct);

  /* CS (PB2) as push-pull output */
  GPIO_InitStruct.Pin   = HCI_TL_SPI_CS_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HCI_TL_SPI_CS_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_SET);

  return BLE_BSP_SPI1_Init();
}

int32_t HCI_TL_SPI_DeInit(void)
{
  HAL_GPIO_DeInit(HCI_TL_SPI_EXTI_PORT, HCI_TL_SPI_EXTI_PIN);
  HAL_GPIO_DeInit(HCI_TL_SPI_CS_PORT,   HCI_TL_SPI_CS_PIN);
  HAL_GPIO_DeInit(HCI_TL_RST_PORT,      HCI_TL_RST_PIN);
  return 0;
}

int32_t HCI_TL_SPI_Reset(void)
{
  HAL_GPIO_WritePin(HCI_TL_RST_PORT, HCI_TL_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(HCI_TL_RST_PORT, HCI_TL_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(5);
  return 0;
}

int32_t HCI_TL_SPI_Receive(uint8_t *buffer, uint16_t size)
{
  uint16_t byte_count;
  uint8_t  len = 0;
  uint8_t  char_ff = 0xff;
  volatile uint8_t read_char;

  uint8_t header_master[HEADER_SIZE] = {0x0b, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_RESET);

  BLE_BSP_SPI1_SendRecv(header_master, header_slave, HEADER_SIZE);

  if (header_slave[0] == 0x02) {
    byte_count = (header_slave[4] << 8) | header_slave[3];
    if (byte_count > 0) {
      if (byte_count > size) {
        byte_count = size;
      }
      for (len = 0; len < byte_count; len++) {
        BLE_BSP_SPI1_SendRecv(&char_ff, (uint8_t *)&read_char, 1);
        buffer[len] = read_char;
      }
    }
  }

  HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_SET);
  return len;
}

int32_t HCI_TL_SPI_Send(uint8_t *buffer, uint16_t size)
{
  int32_t result;
  uint8_t header_master[HEADER_SIZE] = {0x0a, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];
  static uint8_t read_char_buf[MAX_BUFFER_SIZE];
  uint32_t tickstart = HAL_GetTick();

  do {
    result = 0;
    HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_RESET);
    BLE_BSP_SPI1_SendRecv(header_master, header_slave, HEADER_SIZE);
    if (header_slave[0] == 0x02) {
      if (header_slave[1] >= size) {
        BLE_BSP_SPI1_SendRecv(buffer, read_char_buf, size);
      } else {
        result = -2;
      }
    } else {
      result = -1;
    }
    HAL_GPIO_WritePin(HCI_TL_SPI_CS_PORT, HCI_TL_SPI_CS_PIN, GPIO_PIN_SET);

    if ((HAL_GetTick() - tickstart) > TIMEOUT_DURATION) {
      result = -3;
      break;
    }
  } while (result < 0);

  return result;
}

static int32_t IsDataAvailable(void)
{
  return (HAL_GPIO_ReadPin(HCI_TL_SPI_EXTI_PORT, HCI_TL_SPI_EXTI_PIN) == GPIO_PIN_SET);
}

void hci_tl_lowlevel_init(void)
{
  tHciIO fops;
  fops.Init    = HCI_TL_SPI_Init;
  fops.DeInit  = HCI_TL_SPI_DeInit;
  fops.Send    = HCI_TL_SPI_Send;
  fops.Receive = HCI_TL_SPI_Receive;
  fops.Reset   = HCI_TL_SPI_Reset;
  fops.GetTick = BLE_BSP_GetTick;
  hci_register_io_bus(&fops);

  /* Tie the EXTI5 line to our handler so hci_tl_lowlevel_isr fires on BLE IRQ */
  HAL_EXTI_GetHandle(&hexti5, EXTI_LINE_5);
  HAL_EXTI_RegisterCallback(&hexti5, HAL_EXTI_COMMON_CB_ID, hci_tl_lowlevel_isr);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void hci_tl_lowlevel_isr(void)
{
  while (IsDataAvailable()) {
    if (hci_notify_asynch_evt(NULL)) {
      return;
    }
    HCI_ProcessEvent = 1;
  }
}
