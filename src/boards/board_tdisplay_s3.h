#ifndef BOARD_TDISPLAY_S3_H
#define BOARD_TDISPLAY_S3_H

#include <Arduino.h>

// Hardware defaults for the LilyGo T-Display-S3 (ESP32-S3).
// Only this file contains GPIO numbers.
namespace board {

constexpr const char* BOARD_NAME = "T-Display S3";

// Physische Pins
constexpr uint8_t PIN_BTN_A     = 0;   // BOOT button (active LOW)
constexpr uint8_t PIN_BTN_B     = 14;  // free button (active LOW)
constexpr bool    HAS_BTN_B     = true;
constexpr uint8_t PIN_POWER_ON  = 15;  // Power-hold, must stay HIGH
constexpr bool    HAS_POWER_HOLD = true;

// Touch (none on this board - values only keep the core code generic)
constexpr bool     HAS_TOUCH      = false;
constexpr uint8_t  PIN_TOUCH_SDA  = 0;
constexpr uint8_t  PIN_TOUCH_SCL  = 0;
constexpr uint8_t  TOUCH_I2C_ADDR = 0x3B;
constexpr uint32_t TOUCH_I2C_FREQ = 400000;

// Button timing
constexpr uint32_t LONG_A_MS   = 1200;
constexpr uint32_t LONG_B_MS   = 700;
constexpr uint32_t DEBOUNCE_MS = 30;

// Display geometry in sketch orientation (portrait, 170 wide x 320 high,
// as used by the original Eldoria sketch with setRotation(0))
constexpr uint8_t  TFT_ROTATION = 0;
constexpr int16_t  SCR_W = 170;
constexpr int16_t  SCR_H = 320;
constexpr int16_t  SCR_LW = 320;  // landscape (rotation 1) sketch size
constexpr int16_t  SCR_LH = 170;
constexpr int16_t  PANEL_W = 170;  // native panel size (raw touch coordinates)
constexpr int16_t  PANEL_H = 320;

}  // namespace board

#endif
