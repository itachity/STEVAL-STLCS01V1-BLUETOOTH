/**
 ******************************************************************************
 * @file    hci_tl_interface.h
 * @brief   BlueNRG-MS HCI Transport Layer interface for SensorTile
 *
 * SensorTile BlueNRG-MS pin map:
 *   SPI1_SCK   PA5   (AF5)
 *   SPI1_MISO  PA6   (AF5)
 *   SPI1_MOSI  PA7   (AF5)
 *   BLE CS     PB2   (output, push-pull, idle HIGH)
 *   BLE IRQ    PC5   (EXTI, rising edge)
 *   BLE RESET  PH0   (output, push-pull, idle HIGH)
 ******************************************************************************
 */
#ifndef __BLE_HCI_TL_INTERFACE_H
#define __BLE_HCI_TL_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

extern volatile uint32_t HCI_ProcessEvent;

#define HCI_TL_SPI_EXTI_PORT  GPIOC
#define HCI_TL_SPI_EXTI_PIN   GPIO_PIN_5
#define HCI_TL_SPI_EXTI_IRQn  EXTI9_5_IRQn

#define HCI_TL_SPI_IRQ_PORT   GPIOC
#define HCI_TL_SPI_IRQ_PIN    GPIO_PIN_5

#define HCI_TL_SPI_CS_PORT    GPIOB
#define HCI_TL_SPI_CS_PIN     GPIO_PIN_2

#define HCI_TL_RST_PORT       GPIOH
#define HCI_TL_RST_PIN        GPIO_PIN_0

/* Low-level SPI/GPIO bring-up for BlueNRG (defined in ble_spi_bsp.c) */
int32_t BLE_BSP_SPI1_Init(void);
int32_t BLE_BSP_SPI1_SendRecv(uint8_t *pTxData, uint8_t *pRxData, uint16_t len);
int32_t BLE_BSP_GetTick(void);   /* signature matches tHciIO.GetTick */

/* HCI transport layer operations */
int32_t HCI_TL_SPI_Init   (void *pConf);
int32_t HCI_TL_SPI_DeInit (void);
int32_t HCI_TL_SPI_Receive(uint8_t *buffer, uint16_t size);
int32_t HCI_TL_SPI_Send   (uint8_t *buffer, uint16_t size);
int32_t HCI_TL_SPI_Reset  (void);

void hci_tl_lowlevel_init(void);
void hci_tl_lowlevel_isr(void);

#ifdef __cplusplus
}
#endif
#endif /* __BLE_HCI_TL_INTERFACE_H */
