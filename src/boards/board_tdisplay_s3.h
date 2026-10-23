#ifndef BOARD_TDISPLAY_S3_H
#define BOARD_TDISPLAY_S3_H

#include <Arduino.h>

// Hardware-Standard fuer das LilyGo T-Display-S3 (ESP32-S3).
// Nur diese Datei enthaelt GPIO-Nummern.
namespace board {

// Physische Pins
constexpr uint8_t PIN_BTN_A     = 0;   // BOOT-Taster (aktiv LOW)
constexpr uint8_t PIN_BTN_B     = 14;  // frei belegbarer Taster (aktiv LOW)
constexpr uint8_t PIN_POWER_ON  = 15;  // Power-Hold, muss HIGH bleiben
constexpr bool    HAS_POWER_HOLD = true;

// Taster-Timing
constexpr uint32_t LONG_A_MS   = 1200;
constexpr uint32_t LONG_B_MS   = 700;
constexpr uint32_t DEBOUNCE_MS = 30;

// Display-Geometrie in Sketch-Ausrichtung (Portrait, 170 breit x 320 hoch,
// wie im urspruenglichen Eldoria-Sketch mit setRotation(0) verwendet)
constexpr uint8_t  TFT_ROTATION = 0;
constexpr int16_t  SCR_W = 170;
constexpr int16_t  SCR_H = 320;

}  // namespace board

#endif
