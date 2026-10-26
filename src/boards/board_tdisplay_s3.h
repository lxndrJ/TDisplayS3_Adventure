#ifndef BOARD_TDISPLAY_S3_H
#define BOARD_TDISPLAY_S3_H

#include <Arduino.h>

// Hardware defaults for the LilyGo T-Display-S3 (ESP32-S3).
// Only this file contains GPIO numbers.
namespace board {

// Physische Pins
constexpr uint8_t PIN_BTN_A     = 0;   // BOOT button (active LOW)
constexpr uint8_t PIN_BTN_B     = 14;  // free button (active LOW)
constexpr uint8_t PIN_POWER_ON  = 15;  // Power-hold, must stay HIGH
constexpr bool    HAS_POWER_HOLD = true;

// Button timing
constexpr uint32_t LONG_A_MS   = 1200;
constexpr uint32_t LONG_B_MS   = 700;
constexpr uint32_t DEBOUNCE_MS = 30;

// Display geometry in sketch orientation (portrait, 170 wide x 320 high,
// as used by the original Eldoria sketch with setRotation(0))
constexpr uint8_t  TFT_ROTATION = 0;
constexpr int16_t  SCR_W = 170;
constexpr int16_t  SCR_H = 320;

}  // namespace board

#endif
