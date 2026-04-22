/**
 ******************************************************************************
 * @file    uart_console.h
 * @brief   USART1 (PA9 TX-only) console used by the BLE counter demo.
 *          Lets you watch the 1..10 counter stream in a serial terminal
 *          connected to the Nucleo's onboard ST-Link Virtual COM Port.
 *
 * After Console_Init() succeeds, printf() routes to USART1 at 115200-8N1.
 ******************************************************************************
 */
#ifndef __UART_CONSOLE_H
#define __UART_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

/* Returns 0 on success, negative on HAL failure. */
int Console_Init(void);

#ifdef __cplusplus
}
#endif
#endif /* __UART_CONSOLE_H */
