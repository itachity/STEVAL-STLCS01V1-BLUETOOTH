# SensorTile BLE bring-up — flash & verify

This guide walks the five checkpoints requested for the STM32L476JGY6 SensorTile port of `BLE_SampleApp`.

## What was changed in `STM32L476JGY6`

- `Core/Inc/main.h` — added BLE app defines (NAME_STLBLE, version, F2I helpers, env-tick reload values, `bluenrg_conf.h` include, `BufferToWrite/BytesToWrite` externs). USB-debug printf is a no-op for first bring-up.
- `Core/Src/main.c` — replaced `SystemClock_Config` with the BLE-sample variant (MSI 48 MHz, PLL → 80 MHz SYSCLK, USB clock = MSI). In `main()` we skip `MX_I2C3_Init`, `MX_TIM1_Init`, `MX_TIM4_Init`, `MX_TIM5_Init` (commented out), then call `InitTargetPlatform`, `Init_BlueNRG_Stack`, `Init_BlueNRG_Custom_Services`, `InitTimers`. The main loop blinks LED1 when not connected, dispatches BLE events, restarts advertising, and pushes env data at ~2 Hz. Added `HAL_TIM_OC_DelayElapsedCallback`, `HAL_GPIO_EXTI_Callback`, `InitTimers`, `Init_BlueNRG_Stack`, `Init_BlueNRG_Custom_Services`, `SendEnvironmentalData`. `InitTimers` enables the TIM1 clock + `TIM1_CC_IRQn` itself because `MX_TIM1_Init` is no longer called.
- `Core/Src/stm32l4xx_it.c` — `TIM1_CC_IRQHandler` dispatches to the application-owned `TimCCHandle` (Output-Compare) instead of the unused `htim1`. `EXTI9_5_IRQHandler` already calls `HAL_GPIO_EXTI_IRQHandler(BLE_IRQ_Pin)`, which routes to our `HAL_GPIO_EXTI_Callback` → `hci_tl_lowlevel_isr()`.
- `Core/Inc/` — copied `TargetFeatures.h`, `console.h`, `hci_tl_interface.h`, `sensor_service.h`, `uuid_ble_service.h`, `bluenrg_conf.h`, `ble_list_utils.h`, `SensorTile_conf.h`, `SensorTile_errno.h` from STSW-STLKT01. USB CDC headers stubbed (CubeMX's USB stack is the live one).
- `Core/Src/` — copied `TargetPlatform.c`, `hci_tl_interface.c`, `sensor_service.c`. USB CDC sources stubbed (`#if 0`) so they don't fight the CubeMX stack.
- `Drivers/BSP/` — full SensorTile BSP plus hts221/lps22hb/lsm303agr/lsm6dsm components.
- `Middlewares/ST/BlueNRG-MS/` — full BlueNRG-MS middleware. `bluenrg_utils_small.c` is wrapped in `#if 0` to avoid duplicate `getBlueNRGVersion` symbol with the full `bluenrg_utils.c`.
- `.cproject` — added BSP/Middleware include paths to both Debug and Release tool-chain configs.

## Step 1 — Open & build in STM32CubeIDE

1. File → Import → General → Existing Projects into Workspace, point at `C:\Users\santo\bluetooth\STM32L476JGY6`.
2. Right-click project → Build Project.
3. If you see "header not found" for `hts221.h`/`lps22hb.h`/`bluenrg_*.h`, right-click project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths and confirm the eleven entries listed above are present (they are written in `.cproject` in both Debug and Release).

## Step 2 — Flash

1. Connect the SensorTile cradle (or your STLINK) and power the board.
2. Run → Debug As → STM32 Cortex-M C/C++ Application (or just Run, target reset+halt).
3. Resume execution.

## Step 3 — Verify the five checkpoints

| # | Check | Expected |
|---|---|---|
| 1 | SensorTile powers on | Cradle 3V3 LED on, MCU running (LED1 active in step below) |
| 2 | LED blinks | LED1 (PG12) toggles ~once per second when no BLE client is connected (1 s on, 50 ms off) |
| 3 | Phone sees SensorTile | In ST BlueMS app (Android/iOS) → scan; advertised name is **STLBLE250** (NAME_STLBLE expanded to 7 chars S-T-L-B-2-5-0) |
| 4 | Phone connects | Tap the entry. LED behavior changes (no longer blinking the way it does while advertising — the loop branch only blinks `if (!connected)`) |
| 5 | Environmental data appears | Open the Environmental tab; Pressure (LPS22HB) and the LPS22HB temperature update at 2 Hz. Humidity / HTS221-temp will only appear once I2C3 is configured (deferred — see below) |

## Notes / known limits in this build

- **I2C3 is intentionally not initialized** (per request). The HTS221 humidity + temperature sensor sits on I2C3, so its values stay at 0 until I2C3 is wired up. Pressure (LPS22HB on SPI2) and BLE both work without it.
- **USB CDC debug is disabled.** `STLBLE_PRINTF` is a no-op. To enable later, undefine the disable shim in `usbd_cdc_interface.h`, undisable `usbd_cdc_interface.c`, and replace the no-op `STLBLE_PRINTF` macro in `main.h` with a `printf`-style writer that pumps via `CDC_Fill_Buffer`.
- **TIM4 / TIM5** are configured by CubeMX but not started (their `MX_TIMx_Init` calls are commented out). They were used by the original sample for the USB-CDC pump — re-enable the same way you enable USB CDC.
- **BLE MAC** is derived from STM32 UID by default. Define `STATIC_BLE_MAC` in `main.h` if you want a fixed address.

## If something doesn't work

- **No advertising / can't see SensorTile in BlueMS** → confirm `BLE_RST` (PH0) goes high after `hci_init`. If the BlueNRG never comes out of reset, advertising never starts. Also check `getBlueNRGVersion` returns successfully.
- **LED never blinks** → the loop only blinks while `!connected`, so it will be steady once a client is connected. If it's solid before any connection, set a breakpoint in `Error_Handler` — most likely the clock config or HAL_Init failed.
- **Pressure stuck at 0** → likely `BSP_ENV_SENSOR_Init(LPS22HB_0, ...)` returned non-zero. Check that PB2 (BLE_CS) and PA3 (LPS22HB CS) are not being driven simultaneously, and that SPI2 MspInit is firing for `hbusspi2`.
- **MCU resets on the first env tick** → that means `TimCCHandle` Output-Compare started but TIM1 clock isn't enabled. Confirm `__HAL_RCC_TIM1_CLK_ENABLE()` and `HAL_NVIC_EnableIRQ(TIM1_CC_IRQn)` happen inside `InitTimers` (they do as of this commit).

## Reference

The unmodified `BLE_SampleApp` `main.c` is at `STM32L476JGY6/main_BLE_SampleApp_REF.c.txt` (renamed `.txt` so it doesn't get compiled).
