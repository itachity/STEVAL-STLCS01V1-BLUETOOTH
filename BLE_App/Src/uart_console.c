/**
 ******************************************************************************
 * @file    uart_console.c
 * @brief   Minimal USART1 console for the BLE counter demo.
 *
 * Pin map (SensorTile side):
 *   USART1_TX  PA9  (AF7)    <-- only pin used; we never RX
 *
 * Wire this pin to the Nucleo-F401RE's PA2 (morpho CN10 pin 35, or Arduino
 * header D1). PA2 is the Nucleo target's USART2_TX, which is routed through
 * solder bridge SB14 to the onboard ST-Link's VCP RX. Hold the Nucleo's
 * onboard F401 in reset (jumper NRST<->GND) so it doesn't fight us for PA2.
 *
 * On the PC side, open the ST-Link VCP COM port at 115200 8-N-1.
 ******************************************************************************
 */
#include "uart_console.h"

#include <stdio.h>

static UART_HandleTypeDef huart1;

int Console_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. Clocks ------------------------------------------------------------ */
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* 2. GPIO alternate function — PA9 as USART1_TX ----------------------- */
  GPIO_InitStruct.Pin       = GPIO_PIN_9;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* 3. USART1 peripheral ------------------------------------------------- */
  huart1.Instance                    = USART1;
  huart1.Init.BaudRate               = 115200;
  huart1.Init.WordLength             = UART_WORDLENGTH_8B;
  huart1.Init.StopBits               = UART_STOPBITS_1;
  huart1.Init.Parity                 = UART_PARITY_NONE;
  huart1.Init.Mode                   = UART_MODE_TX;
  huart1.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling           = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart1) != HAL_OK) {
    return -1;
  }

  /* Make stdout unbuffered so printf flushes immediately. */
  setvbuf(stdout, NULL, _IONBF, 0);

  return 0;
}

/* ============================================================= */
/*   newlib printf retargets — both kept for toolchain variance  */
/* ============================================================= */

int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart1, &c, 1, HAL_MAX_DELAY);
  return ch;
}

int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}
