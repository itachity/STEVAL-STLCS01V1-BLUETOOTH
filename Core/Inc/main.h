/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "bluenrg_conf.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* Define the MOTENV1 MAC address, otherwise it will create a MAC related to STM32 UID */
//#define STATIC_BLE_MAC 0xFF, 0xEE, 0xDD, 0xAA, 0xAA, 0xAA

/* Debug Defines (USB serial debug disabled for first bring-up) */
//#define ENABLE_USB_DEBUG
//#define ENABLE_USB_DEBUG_CONNECTION
//#define ENABLE_USB_DEBUG_NOTIFY_TRAMISSION

/* Package Version */
#define STLBLE_VERSION_MAJOR '2'
#define STLBLE_VERSION_MINOR '5'
#define STLBLE_VERSION_PATCH '0'

/* Define the BLE name; MUST be 7 chars long */
#define NAME_STLBLE 'S','T','L','B',STLBLE_VERSION_MAJOR,STLBLE_VERSION_MINOR,STLBLE_VERSION_PATCH

/* Package Name */
#define STLBLE_PACKAGENAME "STLBLE1"

/* USB-debug printf is a no-op until USB serial path is wired through CubeMX's CDC stack */
#define STLBLE_PRINTF(...) ((void)0)

/* STM32 Unique ID */
#define STM32_UUID    ((uint32_t *)0x1FFF7590)
/* STM32 MCU_ID */
#define STM32_MCU_ID  ((uint32_t *)0xE0042000)

/* Float-to-int helpers used by sensor send paths */
#define MCR_BLUEMS_F2I_1D(in, out_int, out_dec) {out_int = (int32_t)in; out_dec= (int32_t)((in-out_int)*10);};
#define MCR_BLUEMS_F2I_2D(in, out_int, out_dec) {out_int = (int32_t)in; out_dec= (int32_t)((in-out_int)*100);};

#define BLUEMSYS_CHECK_JUMP_TO_BOOTLOADER ((uint32_t)0x12345678)

/* 10 kHz timer ticks: 5000 -> 2 Hz environmental sample, 500 -> 20 Hz acc/gyro */
#define uhCCR1_Val  5000
#define uhCCR4_Val  500

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOG
#define BLE_RST_Pin GPIO_PIN_0
#define BLE_RST_GPIO_Port GPIOH
#define CS_AG_Pin GPIO_PIN_12
#define CS_AG_GPIO_Port GPIOB
#define CS_P_Pin GPIO_PIN_3
#define CS_P_GPIO_Port GPIOA
#define BLE_CS_Pin GPIO_PIN_2
#define BLE_CS_GPIO_Port GPIOB
#define CS_M_Pin GPIO_PIN_1
#define CS_M_GPIO_Port GPIOB
#define BLE_IRQ_Pin GPIO_PIN_5
#define BLE_IRQ_GPIO_Port GPIOC
#define BLE_IRQ_EXTI_IRQn EXTI9_5_IRQn
#define CS_A_Pin GPIO_PIN_4
#define CS_A_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */
extern uint8_t  BufferToWrite[256];
extern int32_t  BytesToWrite;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
