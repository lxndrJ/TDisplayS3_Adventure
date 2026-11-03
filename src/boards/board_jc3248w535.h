#ifndef BOARD_JC3248W535_H
#define BOARD_JC3248W535_H

#include <Arduino.h>

// Hardware defaults for the Guition JC3248W535C_I_Y (ESP32-S3-WROOM-1 N16R8,
// 3.5" 320x480 IPS, AXS15231B display over QSPI + integrated capacitive touch).
// Only this file contains GPIO numbers.
namespace board {

constexpr const char* BOARD_NAME = "JC3248W535";

// Physische Pins
constexpr uint8_t PIN_BTN_A      = 0;     // BOOT button (active LOW) - the only button
constexpr uint8_t PIN_BTN_B      = 0xFF;  // none; touch tap/hold acts as Key::B
// (PIN_BTN_B must exist even when unused: with -std=gnu++11 the compiler
// does not discard if-constexpr branches, so both sides must compile)
constexpr bool    HAS_BTN_B      = false;
constexpr bool    HAS_POWER_HOLD = false;
constexpr uint8_t PIN_POWER_ON   = 0;     // unused

// Button timing
constexpr uint32_t LONG_A_MS   = 1200;
constexpr uint32_t LONG_B_MS   = 700;    // also the touch long-press threshold
constexpr uint32_t DEBOUNCE_MS = 30;

// Display geometry in sketch orientation (portrait = native panel orientation)
constexpr uint8_t  TFT_ROTATION = 0;
constexpr int16_t  SCR_W = 320;
constexpr int16_t  SCR_H = 480;
constexpr int16_t  PANEL_W = 320;  // native panel size (raw touch coordinates)
constexpr int16_t  PANEL_H = 480;

// Display: AXS15231B over QSPI (no RST pin wired on this board)
constexpr uint8_t PIN_LCD_CS  = 45;
constexpr uint8_t PIN_LCD_SCK = 47;
constexpr uint8_t PIN_LCD_D0  = 21;
constexpr uint8_t PIN_LCD_D1  = 48;
constexpr uint8_t PIN_LCD_D2  = 40;
constexpr uint8_t PIN_LCD_D3  = 39;
constexpr uint8_t PIN_LCD_BL  = 1;   // backlight enable, HIGH = on

// Touch: capacitive touch integrated in the AXS15231B, I2C slave 0x3B.
// INT/RST are not wired -> polled (see src/core/touch.h).
constexpr bool     HAS_TOUCH      = true;
constexpr uint8_t  PIN_TOUCH_SDA  = 4;
constexpr uint8_t  PIN_TOUCH_SCL  = 8;
constexpr uint8_t  TOUCH_I2C_ADDR = 0x3B;
constexpr uint32_t TOUCH_I2C_FREQ = 400000;

// Other on-board peripherals (free for future use)
constexpr uint8_t PIN_SD_CS     = 10;  // microSD over SPI
constexpr uint8_t PIN_SD_MOSI   = 11;
constexpr uint8_t PIN_SD_SCK    = 12;
constexpr uint8_t PIN_SD_MISO   = 13;
constexpr uint8_t PIN_SPK_DIN   = 41;  // speaker amplifier, I2S
constexpr uint8_t PIN_SPK_BCLK  = 42;
constexpr uint8_t PIN_SPK_LRCLK = 2;
constexpr uint8_t PIN_BAT_ADC   = 5;   // battery voltage divider (33k/100k)

}  // namespace board

#endif
