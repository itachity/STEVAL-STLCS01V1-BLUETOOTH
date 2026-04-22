/**
 ******************************************************************************
 * @file    ble_app.h
 * @brief   Minimal BlueNRG-MS BLE application for SensorTile (STEVAL-STLKT01V1).
 *
 * Exposes the ST BLE Sensor "Environmental" service with a single temperature
 * slot. At 10 Hz the firmware writes a counter value 1..10 (repeating) into
 * that slot, which the STBLESensor mobile app renders on its Environmental
 * tab as cycling °C readings.
 *
 * Usage from Core/Src/main.c (inside USER CODE blocks so CubeMX re-generation
 * doesn't wipe it out):
 *
 *   #include "ble_app.h"
 *   ...
 *   BLE_App_Init();          // once, after HAL_Init + SystemClock_Config
 *   while (1) {
 *     BLE_App_Run();         // pump events + send counter every 100 ms
 *   }
 ******************************************************************************
 */
#ifndef __BLE_APP_H
#define __BLE_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Init: brings up SPI1, resets BlueNRG, configures GAP/GATT, adds the service,
 * starts advertising. Call once. Returns 0 on success, non-zero on failure. */
int BLE_App_Init(void);

/* Non-blocking: must be called in the main super-loop. Handles BLE events and
 * emits a counter value every 100 ms while a central is connected + subscribed. */
void BLE_App_Run(void);

/* Wire from stm32l4xx_it.c: EXTI9_5_IRQHandler() must call HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5)
 * (which is already done by CubeMX); this forwards the callback. */
void BLE_App_EXTI_Callback(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif
#endif /* __BLE_APP_H */
