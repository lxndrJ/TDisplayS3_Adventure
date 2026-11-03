#ifndef CORE_GFX_AXS15231B_H
#define CORE_GFX_AXS15231B_H

// TFT_eSPI-compatible facade over Arduino_GFX for the Guition JC3248W535C_I_Y.
// TFT_eSPI cannot drive the AXS15231B over QSPI, so this board renders through
// Arduino_ESP32QSPI + Arduino_AXS15231B. The facade implements exactly the
// TFT_eSPI API surface the games use, so game code stays unchanged and both
// envs compile against a type named TFT_eSPI / TFT_eSprite.
//
// Important: the AXS15231B in QSPI mode cannot address arbitrary Y per draw,
// so ALL rendering goes through a full-screen Arduino_Canvas frame buffer;
// Display::present() flushes it to the panel once per frame (the only
// reliable render path on this chip - same approach as the vendor demo).

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "board_config.h"
#include "touch.h"
#include "fonts/FreeSans9pt7b.h"  // vendored Adafruit GFXfont - smoother than the 5x7 classic font

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
    g_->setFont(&FreeSans9pt7b);  // smooth GFXfont instead of pixelated 5x7 classic
    g_->setTextSize(f == 2 ? 2 : 1);  // font 2 = ~2x size (titles, choices)
  }

  // FreeSans9pt7b covers ASCII only (0x20-0x7E): fold German UTF-8 umlauts
  // to ae/oe/ue/ss so German game text stays readable on the QSPI board.
  static void foldUmlauts(const char* s, char* out, size_t cap) {
    size_t o = 0;
    for (size_t i = 0; s[i] && o + 3 < cap; ) {
      uint8_t c = (uint8_t)s[i];
      if (c == 0xC3 && s[i + 1]) {
        uint8_t c2 = (uint8_t)s[i + 1];
        const char* rep = nullptr;
        switch (c2) {
          case 0xA4: rep = "ae"; break;
          case 0xB6: rep = "oe"; break;
          case 0xBC: rep = "ue"; break;
          case 0x9F: rep = "ss"; break;
          case 0x84: rep = "Ae"; break;
          case 0x96: rep = "Oe"; break;
          case 0x9C: rep = "Ue"; break;
          default: break;
        }
        if (rep) {
          while (*rep && o + 1 < cap) out[o++] = *rep++;
          i += 2;
          continue;
        }
      }
      out[o++] = (char)c;
      i++;
    }
    out[o] = 0;
  }

  int16_t textWidth(const char* s) {
    if (!g_ || !s) return 0;
    char buf[160];
    foldUmlauts(s, buf, sizeof(buf));
    int16_t x1, y1;
    uint16_t w, h;
    g_->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
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
    char buf[160];
    foldUmlauts(s, buf, sizeof(buf));
    s = buf;
    int16_t x1, y1;
    uint16_t w, h;
    g_->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    // GFXfonts draw relative to the baseline (y1 is negative); the classic
    // font has x1=y1=0, so the same math covers both.
    int32_t cx = x - x1, cy = y - y1;  // TL_DATUM
    switch (datum_) {
      case TC_DATUM: cx = x - w / 2 - x1; break;
      case TR_DATUM: cx = x - w - x1; break;
      case ML_DATUM: cy = y - h / 2 - y1; break;
      case MC_DATUM: cx = x - w / 2 - x1; cy = y - h / 2 - y1; break;
      case MR_DATUM: cx = x - w - x1; cy = y - h / 2 - y1; break;
      case BL_DATUM: cy = y - h - y1; break;
      case BC_DATUM: cx = x - w / 2 - x1; cy = y - h - y1; break;
      case BR_DATUM: cx = x - w - x1; cy = y - h - y1; break;
      default: break;  // TL_DATUM
    }
    g_->setCursor(cx, cy);
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

// Shared forwarding of the Arduino_GFX drawing API onto a target canvas.
// Used by both the display facade (target = frame canvas) and sprites.
class GfxDrawForward {
public:
  void setTarget(Arduino_GFX* g) { t_ = g; }

  void fillScreen(uint16_t c) { if (t_) t_->fillScreen(c); }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) { if (t_) t_->fillRect(x, y, w, h, c); }
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) { if (t_) t_->drawRect(x, y, w, h, c); }
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { if (t_) t_->fillCircle(x, y, r, c); }
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { if (t_) t_->drawCircle(x, y, r, c); }
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) { if (t_) t_->fillTriangle(x0, y0, x1, y1, x2, y2, c); }
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) { if (t_) t_->drawTriangle(x0, y0, x1, y1, x2, y2, c); }
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) { if (t_) t_->drawLine(x0, y0, x1, y1, c); }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) { if (t_) t_->drawFastHLine(x, y, w, c); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) { if (t_) t_->drawFastVLine(x, y, h, c); }
  void drawPixel(int16_t x, int16_t y, uint16_t c) { if (t_) t_->drawPixel(x, y, c); }
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) { if (t_) t_->drawRoundRect(x, y, w, h, r, c); }
  void setTextColor(uint16_t c) { if (t_) t_->setTextColor(c); }
  void setTextColor(uint16_t fg, uint16_t bg) { if (t_) t_->setTextColor(fg, bg); }
  void setTextSize(uint8_t s) { if (t_) t_->setTextSize(s); }
  int16_t width() const { return t_ ? t_->width() : 0; }
  int16_t height() const { return t_ ? t_->height() : 0; }

  // Arduino_GFX 1.6.0 has no pushImage on Arduino_Canvas - use the
  // draw16bitRGBBitmap primitive (RGB565, same layout as TFT_eSPI pushImage).
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data) {
    if (t_) t_->draw16bitRGBBitmap((int16_t)x, (int16_t)y, data, (int16_t)w, (int16_t)h);
  }
  void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* data) {
    if (t_) t_->draw16bitRGBBitmap((int16_t)x, (int16_t)y, const_cast<uint16_t*>(data), (int16_t)w, (int16_t)h);
  }
  // Nearest-neighbour scaled blit (for fullscreen art on the bigger panel).
  void pushImageScaled(int32_t x, int32_t y, int32_t w, int32_t h,
                       const uint16_t* data, int32_t srcW, int32_t srcH) {
    if (!t_ || !data || w <= 0 || h <= 0 || srcW <= 0 || srcH <= 0) return;
    for (int32_t dy = 0; dy < h; dy++) {
      const uint16_t* row = data + (int32_t)((int64_t)dy * srcH / h) * srcW;
      for (int32_t dx = 0; dx < w; dx++)
        t_->drawPixel((int16_t)(x + dx), (int16_t)(y + dy),
                      row[(int32_t)((int64_t)dx * srcW / w)]);
    }
  }

protected:
  Arduino_GFX* t_ = nullptr;
};

// Display facade: panel behind a full-screen frame canvas (see note above).
class TFT_eSPI : public GfxTextCompat, public GfxDrawForward {
public:
  TFT_eSPI() : GfxTextCompat(nullptr) {
    panel_ = new Arduino_AXS15231B(
        new Arduino_ESP32QSPI(board::PIN_LCD_CS, board::PIN_LCD_SCK,
                              board::PIN_LCD_D0, board::PIN_LCD_D1,
                              board::PIN_LCD_D2, board::PIN_LCD_D3),
        GFX_NOT_DEFINED, 0, false, board::PANEL_W, board::PANEL_H);
    canvas_ = new Arduino_Canvas(board::PANEL_W, board::PANEL_H, panel_, 0, 0, 0);
    g_ = canvas_;
    setTarget(canvas_);
  }

  void init() {
    bool ok = canvas_->begin();  // begins the QSPI bus + panel, allocs framebuffer (PSRAM)
    if (Serial) Serial.printf("[jc3248w535] canvas begin=%d psram=%d\n", ok, psramFound());
    pinMode(board::PIN_LCD_BL, OUTPUT);
    digitalWrite(board::PIN_LCD_BL, HIGH);  // backlight on
    // boot flash: proves panel + backlight + flush path end to end
    fillScreen(TFT_RED);
    present();
    delay(300);
    fillScreen(TFT_BLACK);
    present();
  }

  void setRotation(uint8_t r) {
    canvas_->setRotation(r);
    touchNotifyRotation(r);
  }

  // Push the frame canvas to the panel - called once per frame by the engine.
  void present() { if (canvas_) canvas_->flush(); }

  Arduino_Canvas* frameCanvas() { return canvas_; }

private:
  Arduino_AXS15231B* panel_ = nullptr;
  Arduino_Canvas* canvas_ = nullptr;
};

// Sprite facade: an Arduino_Canvas whose flush() blits into the display's
// frame canvas (pushSprite call sites all use 0,0).
class TFT_eSprite : public GfxTextCompat, public GfxDrawForward {
public:
  TFT_eSprite(TFT_eSPI* parent = nullptr) : GfxTextCompat(nullptr), parent_(parent) {}

  bool created() const { return c_ != nullptr; }

  void createSprite(int16_t w, int16_t h) {
    deleteSprite();
    c_ = new Arduino_Canvas(w, h, parent_ ? parent_->frameCanvas() : nullptr, 0, 0, 0);
    c_->begin();
    g_ = c_;
    setTarget(c_);
  }

  void deleteSprite() {
    if (c_) {
      delete c_;
      c_ = nullptr;
    }
    g_ = nullptr;
    setTarget(nullptr);
  }

  void pushSprite(int32_t x, int32_t y) {
    (void)x;
    (void)y;
    if (c_) c_->flush();  // blits the sprite into the frame canvas
  }

  void fillSprite(uint16_t color) { fillScreen(color); }

private:
  Arduino_Canvas* c_ = nullptr;
  TFT_eSPI* parent_ = nullptr;
};

#endif
