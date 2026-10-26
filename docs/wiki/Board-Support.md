---
layout: default
title: "Board Support"
description: "How to add a new ESP32 board (with display + 2 buttons) to the TDisplayS3 Adventure engine: board file, board_config.h, TFT_eSPI setup, PlatformIO environment, pin reference and checklist."
---

# 🖥️ Board Support

Guide for adding new boards (ESP32-based with display + 2 buttons).

## Supported Boards

| Board | Status | Environment name |
|-------|--------|------------------|
| LilyGo T-Display-S3 | ✅ Standard | `t_display_s3` |

## Adding a New Board (4 Steps)

### Step 1: Create the Board File

Create **`src/boards/board_<name>.h`**:

```cpp
#ifndef BOARD_<NAME>_H
#define BOARD_<NAME>_H

#include <Arduino.h>

// All GPIO numbers and display parameters of the board.
// Games and the engine must NEVER use direct GPIO numbers,
// only these constants.
namespace board {

// ---- Buttons ----
constexpr uint8_t PIN_BTN_A      = 0;   // first button (e.g. BOOT)
constexpr uint8_t PIN_BTN_B      = 14;  // second button
constexpr bool    HAS_POWER_HOLD = true; // GPIO for display power
constexpr uint8_t PIN_POWER_ON   = 15;  // must stay HIGH

// ---- Button timing ----
constexpr uint32_t LONG_A_MS   = 1200;  // ms until Long for button A
constexpr uint32_t LONG_B_MS   = 700;   // ms until Long for button B
constexpr uint32_t DEBOUNCE_MS = 30;    // debounce time in ms

// ---- Display geometry ----
// IMPORTANT: SCR_W/SCR_H in the rotation that TFT_ROTATION produces!
constexpr uint8_t  TFT_ROTATION = 1;    // 0=portrait, 1=landscape, ...
constexpr int16_t  SCR_W = 320;         // width in sketch orientation
constexpr int16_t  SCR_H = 170;         // height in sketch orientation

}  // namespace board

#endif
```

### Step 2: Include in board_config.h

Edit **`src/core/board_config.h`**:

```cpp
#ifndef CORE_BOARD_CONFIG_H
#define CORE_BOARD_CONFIG_H

#include <Arduino.h>

#if defined(BOARD_TDISPLAY_S3)
  #include "boards/board_tdisplay_s3.h"
#elif defined(BOARD_MY_BOARD)        // <- NEW
  #include "boards/board_my_board.h" // <- NEW
#else
  #error "No board selected - set -D BOARD_<NAME> in platformio.ini"
#endif

#endif
```

### Step 3: TFT_eSPI Setup (if needed)

If the board has a different display driver or different pins,
create **`include/tft_setup_<name>.h`** (or adapt an existing one):

```cpp
#ifndef TFT_SETUP_<NAME>_H
#define TFT_SETUP_<NAME>_H

// Select the display driver (one of):
// #define ST7789_DRIVER   // 240x320, common on ESP32 boards
// #define ILI9341_DRIVER  // 240x320 SPI
// #define ST7735_DRIVER   // 128x160 small

#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pins (adapt to the board!)
#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  33
#define TFT_BL   32

// For SPI displays:
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1  // often not connected

// For 8-bit parallel (like the T-Display-S3):
// #define TFT_PARALLEL_8_BIT
// #define TFT_D0 39 ... TFT_D7 48
// #define TFT_WR 8
// #define TFT_RD 9

// SPI frequency (only for SPI displays)
#define SPI_FREQUENCY  40000000

#endif
```

### Step 4: Create a PlatformIO Environment

Extend **`platformio.ini`**:

```ini
[platformio]
default_envs = t_display_s3

; Existing environment...
[env:t_display_s3]
; ...

; ============================================
; New board: My ESP32 Board
; ============================================
[env:my_board]
platform = espressif32@^6.5.0
board = esp32dev                    ; or esp32-s3-devkitc-1, etc.
framework = arduino
board_build.mcu = esp32             ; or esp32s3
board_build.f_cpu = 240000000L
monitor_speed = 115200
build_flags =
  -std=gnu++17
  -D BOARD_MY_BOARD                 ; <- define the board name
  -D ARDUINO_USB_CDC_ON_BOOT=1
  -I include
; lib_deps =                        ; adapt if the display differs
;   bodmer/TFT_eSPI@2.5.43
```

## Build & Flash for a New Board

```bash
# Build for a specific board
pio run -e my_board

# Build & flash
pio run -e my_board -t upload

# Serial monitor
pio device monitor
```

## Board-Specific Details

### Power-Hold (Display Power)

Many ESP32 boards with a display need a "power-hold" pin that must stay
HIGH so the display gets power:

```cpp
// In board_<name>.h:
constexpr bool    HAS_POWER_HOLD = true;
constexpr uint8_t PIN_POWER_ON   = 15;  // GPIO number
```

The engine sets this pin in `Engine::begin()` **before** the display init:

```cpp
// Automatically in Engine::begin():
boardPowerOn();  // GPIO15 HIGH + 10ms wait
```

> ⚠️ **Important:** Do not change this order, or the display stays black!

### Buttons Active LOW vs. HIGH

Most boards have buttons active LOW (to GND). If active HIGH:

```cpp
// buttons.h must then be adapted:
// bool raw = digitalRead(btn_.pin) == HIGH;  // instead of LOW
```

All current boards are active LOW - please adapt `ButtonTracker` if a
board is active HIGH.

## Pin Reference for Common Boards

### LilyGo T-Display-S3 (Reference)

| Function | GPIO | Note |
|----------|------|------|
| TFT 8-bit parallel | D0-D7: 39,40,41,42,45,46,47,48 | |
| TFT CS | 6 | |
| TFT DC | 7 | |
| TFT WR | 8 | |
| TFT RD | 9 | |
| TFT RST | 5 | |
| TFT Backlight | 38 | |
| Button A (BOOT) | 0 | active LOW |
| Button B | 14 | active LOW |
| Power-hold | 15 | must be HIGH |
| Battery ADC | 4 | unused |

### Typical ESP32 DevKit (SPI Display)

| Function | GPIO | Note |
|----------|------|------|
| TFT MOSI | 23 | VSPI default |
| TFT SCLK | 18 | VSPI default |
| TFT CS | 5 | |
| TFT DC | 27 | |
| TFT RST | 33 | |
| TFT BL | 32 | or 4 |
| Button A | 0 | BOOT (usually present) |
| Button B | 35 | or another free pin |

## Checklist: Test a New Board

- [ ] `pio run -e <board>` builds without errors
- [ ] Display shows something after upload (not black/white)
- [ ] Both buttons respond (short + long)
- [ ] Backlight is on
- [ ] `pio device monitor` shows serial output
- [ ] Game switching works (wantsMenu)
- [ ] Deep sleep / restart works (if implemented)

## Troubleshooting Board Support

| Problem | Solution |
|---------|----------|
| Black screen, backlight on | Check pin configuration in `tft_setup.h`; check driver (ST7789 vs ILI9341) |
| Black screen, backlight off | Check `HAS_POWER_HOLD` and `PIN_POWER_ON` |
| White/odd colors | Adjust `TFT_INVERSION_ON` or `TFT_RGB_ORDER` |
| Button A does not work | GPIO0 is BOOT - mind the flash BOOT+RST sequence |
| Build error "board not defined" | Check `-D BOARD_<NAME>` in platformio.ini |
| Display flickers | Reduce SPI frequency or use 8-bit parallel |

## Further Links

- [TFT_eSPI Supported Displays](https://github.com/Bodmer/TFT_eSPI#supported-displays)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [LilyGo T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- [Project Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)

---

[← Wiki Home](Home.html) · [Docs Home](/)
