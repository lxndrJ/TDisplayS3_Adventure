#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H

#include "board_config.h"

// Display backend per board. Games draw through this and know neither
// drivers nor pins. TFT_eSPI cannot drive QSPI panels, so the JC3248W535
// uses a TFT_eSPI-compatible facade over Arduino_GFX (same type names).
#if defined(BOARD_JC3248W535)
  #include "gfx_axs15231b.h"
#else
  #include <TFT_eSPI.h>
#endif
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
