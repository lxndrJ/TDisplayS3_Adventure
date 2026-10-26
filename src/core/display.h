#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H

#include <TFT_eSPI.h>
#include "board_config.h"

// Thin wrapper object around TFT_eSPI. Games draw through this and
// know neither drivers nor pins. If needed (e.g. LovyanGFX), the
// implementation can be swapped behind this interface.
class Display {
public:
  void init() {
    tft_.init();
    tft_.setRotation(board::TFT_ROTATION);
    tft_.fillScreen(TFT_BLACK);
  }

  TFT_eSPI& tft() { return tft_; }

  static constexpr int16_t width()  { return board::SCR_W; }
  static constexpr int16_t height() { return board::SCR_H; }

private:
  TFT_eSPI tft_;
};

extern Display gfx;

#endif
