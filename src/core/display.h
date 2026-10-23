#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H

#include <TFT_eSPI.h>
#include "board_config.h"

// Duennes Wrapper-Objekt um TFT_eSPI. Spiele zeichnen hierrueber und
// kennen weder Treiber noch Pins. Bei Bedarf (z. B. LovyanGFX) laesst
// sich die Implementierung hinter dieser Schnittstelle austauschen.
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
