#ifndef CORE_TOUCH_H
#define CORE_TOUCH_H

#include <Arduino.h>
#include <Wire.h>
#include "board_config.h"

// Poller for the capacitive touch integrated in the AXS15231B (Guition
// JC3248W535C_I_Y). The touch controller is an I2C slave (0x3B) whose INT/RST
// pins are not wired on this board, so it is polled: write the 11-byte read
// command, then read 8 bytes (event flag, point count, x hi/lo, y hi/lo).
class TouchPanel {
public:
  void init() {
    if constexpr (board::HAS_TOUCH) {
      Wire.begin(board::PIN_TOUCH_SDA, board::PIN_TOUCH_SCL, board::TOUCH_I2C_FREQ);
    }
  }

  void setRotation(uint8_t r) { rot_ = r & 3; }

  // Fills down and (only when down) x/y, mapped from the native panel
  // orientation to the current display rotation. When there is no touch,
  // x/y are left unchanged so the caller can keep the last known position
  // (a tap event fires on release, when the finger is already up).
  // rawX/rawY/ev (optional) expose the unmapped panel coordinates + event
  // flag for diagnostics.
  bool poll(bool& down, int16_t& x, int16_t& y,
            int16_t* rawX = nullptr, int16_t* rawY = nullptr, uint8_t* ev = nullptr) {
    down = false;
    if constexpr (!board::HAS_TOUCH) return false;
    static const uint8_t kCmd[11] = {0xB5, 0xAB, 0xA5, 0x5A, 0, 0, 0, 0x08, 0, 0, 0};
    Wire.beginTransmission(board::TOUCH_I2C_ADDR);
    Wire.write(kCmd, sizeof(kCmd));
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(board::TOUCH_I2C_ADDR, (uint8_t)8) != 8) return false;
    uint8_t d[8];
    for (uint8_t i = 0; i < 8; i++) d[i] = Wire.read();
    // Vendor layout (esp_lcd_axs15231b.c): d[1] = point count,
    // (d[2] >> 6) & 3 = event flag (0 = down, 1 = up, 2 = contact).
    // Treat event == 1 (release) as "not down" even if num is still set,
    // otherwise the panel reads as permanently touched -> phantom long-press.
    uint8_t num   = d[1];
    uint8_t event = (d[2] >> 6) & 0x03;
    if (num == 0 || event == 1) return false;
    int16_t rx = (int16_t)(((d[2] & 0x0F) << 8) | d[3]);
    int16_t ry = (int16_t)(((d[4] & 0x0F) << 8) | d[5]);
    if (rawX) *rawX = rx;
    if (rawY) *rawY = ry;
    if (ev)   *ev   = event;
    switch (rot_) {
      case 1:  x = ry;                       y = board::PANEL_W - 1 - rx; break;
      case 2:  x = board::PANEL_W - 1 - rx; y = board::PANEL_H - 1 - ry; break;
      case 3:  x = board::PANEL_H - 1 - ry; y = rx;                       break;
      default: x = rx;                       y = ry;                       break;
    }
    down = true;
    return true;
  }

private:
  uint8_t rot_ = 0;
};

extern TouchPanel touchPanel;

// Called by the display backend when the rotation changes, so touch
// coordinates stay in sync with the sketch orientation.
void touchNotifyRotation(uint8_t r);

#endif
