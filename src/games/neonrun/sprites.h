#ifndef GAMES_NEONRUN_SPRITES_H
#define GAMES_NEONRUN_SPRITES_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "core/display.h"

// Central color palette (RGB565). Each theme picks its own colors.
struct NeonPalette {
  uint16_t bgTop;        // sky
  uint16_t bgMid;        // road stripes
  uint16_t bgBottom;     // road base
  uint16_t star;         // stars
  uint16_t horizon;      // horizon line
  uint16_t player;       // player
  uint16_t playerGlow;   // player glow
  uint16_t obstacle;     // firewall bar / drone
  uint16_t obstacleDark; // bar shadow
  uint16_t coin;         // data chip
  uint16_t coinDark;     // chip outline
  uint16_t text;
  uint16_t textDim;
};

// Swappable graphics layer. Games draw exclusively through this
// interface - into the framebuffer (TFT_eSprite), not directly onto
// the panel: only one pushSprite() per frame prevents flicker.
// The standard theme draws vector graphics (triangles/rectangles);
// a bitmap theme (e.g. PROGMEM sprite arrays) implements the same
// methods and plugs into the game without changes.
class SpriteRenderer {
public:
  virtual ~SpriteRenderer() = default;
  virtual const NeonPalette& palette() const = 0;
  virtual void drawBackground(TFT_eSprite& fb, uint32_t scroll) = 0;
  virtual void drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) = 0;
  virtual void drawBarObstacle(TFT_eSprite& fb, int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t gapX, int16_t gapW) = 0;
  virtual void drawDrone(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) = 0;
  virtual void drawCoin(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) = 0;
};

// Standard theme: neon cyberpunk as vector graphics.
class NeonVectorTheme : public SpriteRenderer {
public:
  const NeonPalette& palette() const override { return pal_; }
  void drawBackground(TFT_eSprite& fb, uint32_t scroll) override;
  void drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) override;
  void drawBarObstacle(TFT_eSprite& fb, int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t gapX, int16_t gapW) override;
  void drawDrone(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) override;
  void drawCoin(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) override;

private:
  NeonPalette pal_ = {
    0x0841,  // bgTop        - night sky (dark blue)
    0x1082,  // bgMid        - road stripes
    0x2104,  // bgBottom     - road (deep blue-violet)
    0xC618,  // star         - stars (light grey)
    0xF81F,  // horizon      - neon magenta
    0x07FF,  // player       - neon cyan
    0x0780,  // playerGlow   - cyan glow
    0xF81F,  // obstacle     - neon magenta
    0x8010,  // obstacleDark - dark magenta
    0xFFE0,  // coin         - neon yellow
    0x8400,  // coinDark     - dark yellow
    0xFFFF,  // text
    0x7BEF,  // textDim
  };
};

// Global renderer. A different theme can be plugged in at runtime,
// e.g. in init(): gSprites = &myBitmapTheme;
extern NeonVectorTheme neonVectorTheme;
extern SpriteRenderer* gSprites;

#endif
