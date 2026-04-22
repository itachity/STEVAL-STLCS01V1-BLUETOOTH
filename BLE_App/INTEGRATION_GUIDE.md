# BLE Counter → STBLESensor — Integration Guide

Port of the STSW-STLKT01_V2.5.0 `BLE_SampleApp` into your CubeMX-generated
`bluetooth.ioc` project so the SensorTile advertises as `STLBLE1` and streams
a repeating **1 → 10 counter at 10 Hz** to the STBLESensor mobile app.

Target board: **STEVAL-STLKT01V1** (SensorTile core on STM32L476JGY6PTR),
typically sitting in a cradle like **STEVAL-STLCS01V1** for flashing/power.

---

## 1. What's already been created for you

Inside `BLUETOOTH/` (your project root) there are now two new directories:

```
Middlewares/ST/BlueNRG-MS/      ← wholesale copy of ST's BlueNRG-MS stack
    hci/                        ← HCI layer (controller + transport)
    includes/                   ← all public headers
    utils/                      ← ble_list.c helper

BLE_App/
    Inc/
        ble_app.h               ← public API of the BLE app (Init + Run)
        bluenrg_conf.h          ← middleware config
        ble_list_utils.h        ← HAL pull-in header required by ble_list.c
        hci_tl_interface.h      ← SPI/GPIO pin defs for BlueNRG
        uart_console.h          ← USART1 console header (printf → terminal)
        uuid_ble_service.h      ← ST BLE Sensor UUIDs + pack macros
    Src/
        ble_app.c               ← stack bring-up + 10 Hz counter loop
        ble_spi_bsp.c           ← minimal SPI1 init on PA5/PA6/PA7
        hci_tl_interface.c      ← HCI transport (adapted from reference)
        uart_console.c          ← USART1 TX-only init + printf retarget
    main_snippets.c             ← snippets to paste into main.c / it.c
    INTEGRATION_GUIDE.md        ← this file
```

These files never need to be re-generated — they are hand-written and safe to
keep across CubeMX re-generation cycles.

---

## 2. Open your project in STM32CubeIDE

1. Launch STM32CubeIDE.
2. `File → Open Projects from File System…` → browse to the `BLUETOOTH/`
   folder and import it.
3. Double-click `bluetooth.ioc` to open the CubeMX perspective.

---

## 3. Changes you must make in `bluetooth.ioc`

Right now your `.ioc` only configures I2C3 and leaves the MCU on the 4 MHz
MSI default. BLE needs SPI1, three GPIOs, a fast clock, and the EXTI9_5
interrupt enabled.

### 3.1 Clock tree — 80 MHz SYSCLK

`Clock Configuration` tab:

| Parameter                | Value                              |
|--------------------------|------------------------------------|
| MSI                      | **ON**                             |
| MSI RANGE                | **RCC_MSIRANGE_11** (48 MHz)       |
| LSE                      | **ON** (external 32.768 kHz)       |
| PLL source               | **MSI**                            |
| PLL M                    | **6**                              |
| PLL N                    | **40**                             |
| PLL R                    | **4**   → SYSCLK 80 MHz            |
| PLL Q                    | **4**                              |
| PLL P                    | **7**                              |
| SYSCLK source            | **PLLCLK**                         |
| AHB prescaler            | **/1**  → HCLK 80 MHz              |
| APB1/APB2 prescaler      | **/1**                             |

Tick `RCC → Low-Speed Clock (LSE)` in the Pinout panel to enable LSE.

### 3.2 SPI1 — BlueNRG bus

Pinout panel:

1. In the left pane, expand `Connectivity → SPI1`.
2. Set **Mode = Full-Duplex Master**, NSS = **Disable**.
3. Parameters:
   - Frame Format: **Motorola**
   - Data Size: **8 Bits**
   - First Bit: **MSB First**
   - Prescaler for Baud Rate: **16** (→ 5 MHz with an 80 MHz APB2)
   - Clock Polarity: **Low**
   - Clock Phase: **1 Edge**
   - CRC Calculation: **Disabled**
   - NSSP Mode: **Enabled**
4. Confirm the pins CubeMX auto-picks are **PA5 / PA6 / PA7** (alternate
   function AF5). If it picks different pins, drag them manually on the
   chip view to match PA5=SCK, PA6=MISO, PA7=MOSI — the SensorTile is hard-
   wired to those.

### 3.3 Three GPIOs for BlueNRG control

Click each pin on the chip view and set:

| Pin  | Label           | Mode                      | Pull      | Speed      | Output level |
|------|-----------------|---------------------------|-----------|------------|--------------|
| PB2  | `BLE_CS`        | GPIO_Output (push-pull)   | No pull   | Low        | **High**     |
| PH0  | `BLE_RST`       | GPIO_Output (push-pull)   | No pull   | Low        | **High**     |
| PC5  | `BLE_IRQ`       | **GPIO_EXTI5** (rising)   | No pull   | —          | —            |

The labels are optional but make debugging easier.

### 3.3b USART1 — serial console on PA9

Pinout panel:

1. Expand `Connectivity → USART1`.
2. Set **Mode = Asynchronous**, Hardware Flow Control = **Disable**.
3. Parameters:
   - Baud Rate: **115200**
   - Word Length: **8 Bits (including Parity)**
   - Parity: **None**
   - Stop Bits: **1**
   - Direction: **TX Only** (we never RX from the VCP)
4. Confirm the TX pin is **PA9** (alternate function AF7). PA10 can be left
   alone — we don't use it.

There is no NVIC line to enable here — `Console_Init()` uses blocking
`HAL_UART_Transmit`, not interrupt-driven IO.

### 3.4 NVIC — enable EXTI9_5

`System Core → NVIC` panel, tick **EXTI line[9:5] interrupts**.

### 3.5 Power — enable VddIO2

`System Core → PWR` → tick **Enable VDDIO2 supply** (PH0 lives there).
If you can't find that option, leave it — the runtime code also calls
`HAL_PWREx_EnableVddIO2()`, so either path is fine.

### 3.6 Regenerate

`Project → Generate Code` (or Ctrl+S inside the .ioc view). CubeMX will
re-write `Core/Src/main.c` and `Core/Src/stm32l4xx_it.c` — your USER CODE
blocks survive.

---

## 4. Add the new files to the build

1. Right-click the project → `Properties → C/C++ General → Paths and
   Symbols → Source Location → Link Folder…` — add:
   - `BLE_App/Src`
   - `Middlewares/ST/BlueNRG-MS/hci`
   - `Middlewares/ST/BlueNRG-MS/hci/controller`
   - `Middlewares/ST/BlueNRG-MS/hci/hci_tl_patterns/Basic`
   - `Middlewares/ST/BlueNRG-MS/utils`

2. On the `Includes` tab of the same dialog, add (one per line):
   - `../BLE_App/Inc`
   - `../Middlewares/ST/BlueNRG-MS/includes`
   - `../Middlewares/ST/BlueNRG-MS/hci/hci_tl_patterns/Basic`

3. Apply & Close → project rebuild should now find every header.

---

## 5. Paste the user-code snippets

Open the file `BLE_App/main_snippets.c` next to you — it holds three tiny
blocks that go into matching `/* USER CODE BEGIN … */` regions:

- `Core/Src/main.c`
  - `USER CODE BEGIN Includes` → `#include "ble_app.h"`
  - `USER CODE BEGIN 2` → `BLE_App_Init();` (after all `MX_*_Init()` calls)
  - `USER CODE BEGIN WHILE` → `BLE_App_Run();` inside the `while (1)`

- `Core/Src/stm32l4xx_it.c`
  - `USER CODE BEGIN EV` → define `HAL_GPIO_EXTI_Callback` and forward the
    pin to `BLE_App_EXTI_Callback`.

Do **not** edit outside USER CODE blocks — CubeMX will overwrite them.

---

## 6. Build + flash

1. Hammer icon → Build. Expect ~zero warnings. If you hit a missing-header
   error, double-check the include paths in §4.
2. Plug the SensorTile via the STEVAL-STLCS01V1 cradle into your ST-LINK.
3. Debug icon (bug) → Flash + run, then press Resume once.

If nothing happens, verify that your ST-LINK sees the target:
`STM32_Programmer_CLI -c port=SWD -q`.

---

## 6b. Wire the serial terminal via the Nucleo's ST-Link VCP

Goal: use the Nucleo-F401RE sitting next to your SensorTile as a
USB-to-serial bridge so `printf("%u\r\n", counter)` shows up in a PC
terminal over the Nucleo's normal USB cable.

### The path the bytes take

```
SensorTile PA9 (USART1_TX) ─── wire ───▶ Nucleo PA2 (target USART2_TX pin)
                                                │
                                              SB14 (already soldered shut)
                                                │
                                                ▼
                                       ST-Link MCU UART RX
                                                │
                                                ▼
                                         USB VCP  ───▶  your PC
```

We use the Nucleo's onboard ST-Link purely as a USB-UART bridge. The
Nucleo's own F401 MCU has to be kept quiet so it doesn't drive PA2
against us.

### One-time Nucleo prep

1. **Leave both CN2 jumpers in place** (or remove them — either works, the
   VCP wiring to PA2/PA3 goes through SB13/SB14, not CN2).
2. **Hold the Nucleo's F401 in reset.** Easiest way: drop a jumper wire
   from a GND pin (e.g. CN6 pin 6) to the **NRST** pin on CN7 pin 14.
   With the F401 parked, PA2 is high-Z and won't fight our signal.
3. Plug the Nucleo into your PC with its normal USB cable. Windows will
   enumerate "STMicroelectronics STLink Virtual COM Port"; macOS/Linux
   will surface `/dev/tty.usbmodemXXXX` or `/dev/ttyACM0`.

### The one new wire between the boards

You already have SWD (SWCLK, SWDIO, NRST-target, GND, +3V3) running from
the Nucleo's CN4 header to your SensorTile cradle. Add **one** wire:

| From (SensorTile)          | To (Nucleo-F401RE)                         |
|----------------------------|--------------------------------------------|
| **PA9** (USART1_TX)        | **PA2** — CN10 pin 35 on the morpho side, <br>or D1 on the Arduino header |
| GND                        | any GND (already wired for SWD)            |

If you're not sure which pin on the SensorTile cradle is PA9, check the
STEVAL-STLCS01V1 schematic — it's labelled `TX` / `UART1_TX` on the
breakout depending on revision.

### Open the terminal

- Baud: **115200**, 8-N-1, no flow control.
- PuTTY / TeraTerm on Windows; `screen /dev/tty.usbmodemXXXX 115200` on
  macOS; `minicom -D /dev/ttyACM0 -b 115200` on Linux; or
  STM32CubeIDE → `Window → Show View → Terminal` → "Serial Terminal".

On boot you should see:

```
[BLE_App] boot
1
2
3
...
10
1
2
...
```

one line every 100 ms, regardless of whether STBLESensor is connected.

---

## 7. Verify on STBLESensor

1. Install **ST BLE Sensor** (Google Play / App Store) — also called
   STBLESensor.
2. Phone Bluetooth ON → Location permissions granted.
3. Open the app → it scans automatically.
4. You should see a peripheral named **STLBLE1**. Tap to connect.
5. The app will land on the **Environmental** tab. You'll see the
   "Temperature" reading tick: `1.0 °C → 2.0 °C → … → 10.0 °C → 1.0 °C …`
   cycling ~10 times per second.

The counter state machine is reset every time the central subscribes, so
re-connecting always starts at 1.

### What you'll see
- Because we abuse the environmental *temperature* slot to carry the
  counter, the app shows it as °C. That's cosmetic — the byte stream on
  air is simply the integer value multiplied by 10 (little-endian int16).
- If you prefer the raw counter (1..10 instead of 1.0..10.0), drop the
  `* 10` in `BLE_SendCounter()` inside `BLE_App/Src/ble_app.c`.

---

## 8. Troubleshooting

**Device never appears in STBLESensor.**
- Confirm MCU is running (LED pulse code isn't included here — add a
  GPIO_Output on `PG12` / whichever LED your cradle has and toggle it in
  `BLE_App_Run` to sanity-check).
- Open a serial console: the reference project streams HCI errors over
  USB CDC. You can re-enable that by pulling in `usbd_cdc_interface.c`
  from the reference under `Projects/SensorTile/Applications/BLE_SampleApp/Src/`.

**Device appears but named `BlueNRG`, not `STLBLE1`.**
- `aci_gatt_update_char_value()` on the device-name characteristic
  returned an error. Put a breakpoint on the `return -4;` branch in
  `BLE_Stack_Init()` and inspect the status code. Most common cause is
  skipping the `hci_reset()` + delay before issuing GATT commands.

**Connects, but Environmental tab is empty.**
- The CCCD subscribe event never arrived. Check that
  `EVT_BLUE_GATT_ATTRIBUTE_MODIFIED` is reaching `BLE_HandleEvent()`.
  Almost always a misconfigured EXTI: the BlueNRG IRQ on PC5 must be
  **rising-edge** and `HAL_GPIO_EXTI_Callback` must call
  `BLE_App_EXTI_Callback`.

**Serial terminal is silent.**
- Confirm the Nucleo enumerated as an ST-Link VCP on your PC and you
  opened the right COM port at 115200-8N1.
- Double-check that you grounded the Nucleo's NRST (pin on CN7.14 of the
  F401RE). If the onboard F401 is running, PA2 will be driven by both
  MCUs and the ST-Link sees garbage.
- Verify SensorTile PA9 really is on the pin you think it is — cradle
  silkscreen labelling varies by revision.

**Serial prints, but STBLESensor is empty (or vice-versa).**
- The two are independent paths. UART printing doesn't depend on BLE
  connectivity, and BLE notifications don't depend on UART. If only one
  works, trace that path in isolation rather than suspecting shared code.

**Build fails on `_write` multiply-defined.**
- Some CubeIDE templates also retarget `_write` in `syscalls.c`. Either
  delete Cube's copy or remove the one in `uart_console.c` — keep only
  one definition in the link.

**Build fails on `aci_gap_init_IDB05A1 undefined`.**
- You missed `Middlewares/ST/BlueNRG-MS/hci/controller/` in the source
  locations. That's where `bluenrg_gap_aci.c` lives.

---

## 9. Quicker fallback: use the reference project as-is

If you just want to *see* something on STBLESensor before investing in the
port above, open
`STSW-STLKT01_V2.5.0/Projects/SensorTile/Applications/BLE_SampleApp/STM32CubeIDE/`
as a separate CubeIDE project, build, flash — it will stream real temp /
humidity / pressure data out of the box. Then use the working board as a
known-good baseline while you debug the port.
