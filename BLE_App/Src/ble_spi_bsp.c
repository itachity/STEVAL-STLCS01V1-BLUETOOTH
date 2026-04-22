/**
 ******************************************************************************
 * @file    ble_spi_bsp.c
 * @brief   Minimal SPI1 bring-up for BlueNRG-MS on the SensorTile.
 *
 * Pin map (see hci_tl_interface.h):
 *   SPI1_SCK   PA5  (AF5)
 *   SPI1_MISO  PA6  (AF5)
 *   SPI1_MOSI  PA7  (AF5)
 *
 * These values are lifted straight from STMicro's STSW-STLKT01_V2.5.0
 * SensorTile_bus.c (MX_SPI1_Init) so the BlueNRG chip sees exactly the
 * same bus timings it does under the reference BLE_SampleApp.
 ******************************************************************************
 */
#include "hci_tl_interface.h"

static SPI_HandleTypeDef hble_spi1;

int32_t BLE_BSP_SPI1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. Clocks ------------------------------------------------------------ */
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* 2. GPIO alternate-function pins -------------------------------------- */
  GPIO_InitStruct.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* 3. SPI1 peripheral ---------------------------------------------------- */
  hble_spi1.Instance               = SPI1;
  hble_spi1.Init.Mode              = SPI_MODE_MASTER;
  hble_spi1.Init.Direction         = SPI_DIRECTION_2LINES;
  hble_spi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  hble_spi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hble_spi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hble_spi1.Init.NSS               = SPI_NSS_SOFT;
  hble_spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hble_spi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hble_spi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  hble_spi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hble_spi1.Init.CRCPolynomial     = 7;
  hble_spi1.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
  hble_spi1.Init.NSSPMode          = SPI_NSS_PULSE_ENABLE;

  if (HAL_SPI_Init(&hble_spi1) != HAL_OK) {
    return -1;
  }
  return 0;
}

int32_t BLE_BSP_SPI1_SendRecv(uint8_t *pTxData, uint8_t *pRxData, uint16_t len)
{
  if (HAL_SPI_TransmitReceive(&hble_spi1, pTxData, pRxData, len, HAL_MAX_DELAY) != HAL_OK) {
    return -1;
  }
  return (int32_t)len;
}

int32_t BLE_BSP_GetTick(void)
{
  return (int32_t)HAL_GetTick();
}
