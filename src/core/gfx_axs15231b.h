#ifndef CORE_GFX_AXS15231B_H
#define CORE_GFX_AXS15231B_H

// TFT_eSPI-compatible facade over Arduino_GFX for the Guition JC3248W535C_I_Y.
// TFT_eSPI cannot drive the AXS15231B over QSPI, so this board renders through
// Arduino_ESP32QSPI + Arduino_AXS15231B. The facade implements exactly the
// TFT_eSPI API surface the games use, so game code stays unchanged and both
// envs compile against a type named TFT_eSPI / TFT_eSprite.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "board_config.h"
#include "touch.h"

// TFT_eSPI color aliases (RGB565)
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xC618
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFD20
#define TFT_GREENYELLOW 0xAFE5
#define TFT_PINK        0xF81F
#define TFT_BROWN       0xBC40
#define TFT_GOLD        0xFEA0

// TFT_eSPI text datum constants
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8

// TFT_eSPI text API on top of an Arduino_GFX object: datum handling,
// numbered fonts (mapped to the classic font), textWidth, drawArc, drawNumber.
// .vlw smooth fonts are not supported by Arduino_GFX; loadFont() is a no-op
// and fontLoaded stays false, so games fall back to the built-in font.
class GfxTextCompat {
public:
  bool fontLoaded = false;

  void setTextDatum(uint8_t d) { datum_ = d; }

  void setTextFont(uint8_t f) {
    font_ = f;
    if (!g_) return;
    g_->setFont(nullptr);  // classic 5x7 font
    g_->setTextSize(f == 2 ? 2 : 1);
  }

  int16_t textWidth(const char* s) {
    if (!g_ || !s) return 0;
    int16_t x1, y1;
    uint16_t w, h;
    g_->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return (int16_t)w;
  }

  int16_t textWidth(const char* s, uint8_t font) {
    uint8_t old = font_;
    setTextFont(font);
    int16_t w = textWidth(s);
    setTextFont(old);
    return w;
  }

  void drawString(const char* s, int32_t x, int32_t y) {
    if (!g_ || !s) return;
    int16_t x1, y1;
    uint16_t w, h;
    g_->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    switch (datum_) {
      case TC_DATUM: x -= w / 2; break;
      case TR_DATUM: x -= w; break;
      case ML_DATUM: y -= h / 2; break;
      case MC_DATUM: x -= w / 2; y -= h / 2; break;
      case MR_DATUM: x -= w; y -= h / 2; break;
      case BL_DATUM: y -= h; break;
      case BC_DATUM: x -= w / 2; y -= h; break;
      case BR_DATUM: x -= w; y -= h; break;
      default: break;  // TL_DATUM
    }
    g_->setCursor(x, y);
    g_->print(s);
  }

  void drawString(const String& s, int32_t x, int32_t y) { drawString(s.c_str(), x, y); }

  void drawNumber(long n, int32_t x, int32_t y) {
    char b[16];
    snprintf(b, sizeof(b), "%ld", n);
    drawString(b, x, y);
  }

  void drawArc(int32_t x, int32_t y, int32_t r, int32_t thick,
               float start, float end, uint16_t fg, uint16_t bg) {
    (void)bg;
    if (!g_ || r <= 0 || thick <= 0) return;
    for (float a = start; a <= end; a += 0.5f) {
      float rad = a * 0.0174533f;
      for (int32_t k = 0; k < thick; k++) {
        int32_t rr = r - k;
        if (rr < 0) break;
        g_->drawPixel((int16_t)(x + rr * cosf(rad)), (int16_t)(y + rr * sinf(rad)), fg);
      }
    }
  }

  void drawArc(int32_t x, int32_t y, int32_t r, int32_t thick,
               float start, float end, uint16_t fg) {
    drawArc(x, y, r, thick, start, end, fg, fg);
  }

  void loadFont(const uint8_t* array) { (void)array; fontLoaded = false; }
  void unloadFont() { fontLoaded = false; }

protected:
  GfxTextCompat(Arduino_GFX* g) : g_(g) {}
  Arduino_GFX* g_;
  uint8_t datum_ = TL_DATUM;
  uint8_t font_ = 1;
};

// Display facade: AXS15231B over QSPI + backlight GPIO.
class TFT_eSPI : public Arduino_AXS15231B, public GfxTextCompat {
public:
  TFT_eSPI()
    : Arduino_AXS15231B(new Arduino_ESP32QSPI(board::PIN_LCD_CS, board::PIN_LCD_SCK,
                                              board::PIN_LCD_D0, board::PIN_LCD_D1,
                                              board::PIN_LCD_D2, board::PIN_LCD_D3),
                        GFX_NOT_DEFINED, 0, false, board::PANEL_W, board::PANEL_H),
      GfxTextCompat(this) {}

  void init() {
    begin();
    pinMode(board::PIN_LCD_BL, OUTPUT);
    digitalWrite(board::PIN_LCD_BL, HIGH);
  }

  void setRotation(uint8_t r) {
    Arduino_AXS15231B::setRotation(r);
    touchNotifyRotation(r);
  }
};

// Sprite facade: an Arduino_Canvas framebuffer flushed to the panel with
// pushSprite() (all call sites use 0,0).
class TFT_eSprite : public GfxTextCompat {
public:
  TFT_eSprite(TFT_eSPI* parent = nullptr) : GfxTextCompat(nullptr), parent_(parent) {}

  bool created() const { return c_ != nullptr; }

  void createSprite(int16_t w, int16_t h) {
    deleteSprite();
    c_ = new Arduino_Canvas(w, h, parent_, 0, 0, 0);
    c_->begin();
    g_ = c_;
  }

  void deleteSprite() {
    if (c_) {
      delete c_;
      c_ = nullptr;
    }
    g_ = nullptr;
  }

  void pushSprite(int32_t x, int32_t y) {
    (void)x;
    (void)y;
    if (c_) c_->flush();
  }

  void fillSprite(uint16_t color) { fillScreen(color); }

  void fillScreen(uint16_t c) { if (c_) c_->fillScreen(c); }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) { if (c_) c_->fillRect(x, y, w, h, c); }
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) { if (c_) c_->drawRect(x, y, w, h, c); }
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { if (c_) c_->fillCircle(x, y, r, c); }
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { if (c_) c_->drawCircle(x, y, r, c); }
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) { if (c_) c_->fillTriangle(x0, y0, x1, y1, x2, y2, c); }
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) { if (c_) c_->drawTriangle(x0, y0, x1, y1, x2, y2, c); }
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) { if (c_) c_->drawLine(x0, y0, x1, y1, c); }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) { if (c_) c_->drawFastHLine(x, y, w, c); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) { if (c_) c_->drawFastVLine(x, y, h, c); }
  void drawPixel(int16_t x, int16_t y, uint16_t c) { if (c_) c_->drawPixel(x, y, c); }
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) { if (c_) c_->drawRoundRect(x, y, w, h, r, c); }
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data) { if (c_) c_->pushImage(x, y, w, h, data); }
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) { if (c_) c_->pushImage(x, y, w, h, data); }
  void setTextColor(uint16_t c) { if (c_) c_->setTextColor(c); }
  void setTextColor(uint16_t fg, uint16_t bg) { if (c_) c_->setTextColor(fg, bg); }
  void setTextSize(uint8_t s) { if (c_) c_->setTextSize(s); }
  int16_t width() const { return c_ ? c_->width() : 0; }
  int16_t height() const { return c_ ? c_->height() : 0; }

private:
  Arduino_Canvas* c_ = nullptr;
  Arduino_G* parent_ = nullptr;
};

#endif
