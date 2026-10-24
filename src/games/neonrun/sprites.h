#ifndef GAMES_NEONRUN_SPRITES_H
#define GAMES_NEONRUN_SPRITES_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "core/display.h"

// Zentrale Farbpalette (RGB565). Jedes Theme waehlt seine eigenen Farben.
struct NeonPalette {
  uint16_t bgTop;        // Himmel
  uint16_t bgMid;        // Strassenstreifen
  uint16_t bgBottom;     // Strassen-Basis
  uint16_t star;         // Sterne
  uint16_t horizon;      // Horizont-Linie
  uint16_t player;       // Spieler
  uint16_t playerGlow;   // Spieler-Glow
  uint16_t obstacle;     // Firewall-Balken / Drohne
  uint16_t obstacleDark; // Balken-Schatten
  uint16_t coin;         // Datenchip
  uint16_t coinDark;     // Chip-Kontur
  uint16_t text;
  uint16_t textDim;
};

// Austauschbare Grafik-Ebene. Spiele zeichnen ausschliesslich ueber dieses
// Interface. Das Standard-Theme zeichnet Vektorgrafik (Dreiecke/Rechtecke);
// ein Bitmap-Theme (z. B. PROGMEM-Sprite-Arrays) implementiert dieselben
// Methoden und laesst sich ohne Aenderung am Spiel einklinken.
class SpriteRenderer {
public:
  virtual ~SpriteRenderer() = default;
  virtual const NeonPalette& palette() const = 0;
  virtual void drawBackground(TFT_eSPI& tft, uint32_t scroll) = 0;
  virtual void drawPlayer(TFT_eSPI& tft, int16_t x, int16_t y, bool visible) = 0;
  virtual void drawBarObstacle(TFT_eSPI& tft, int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t gapX, int16_t gapW) = 0;
  virtual void drawDrone(TFT_eSPI& tft, int16_t x, int16_t y, uint32_t t) = 0;
  virtual void drawCoin(TFT_eSPI& tft, int16_t x, int16_t y, uint32_t t) = 0;
};

// Standard-Theme: Neon-Cyberpunk als Vektorgrafik.
class NeonVectorTheme : public SpriteRenderer {
public:
  const NeonPalette& palette() const override { return pal_; }
  void drawBackground(TFT_eSPI& tft, uint32_t scroll) override;
  void drawPlayer(TFT_eSPI& tft, int16_t x, int16_t y, bool visible) override;
  void drawBarObstacle(TFT_eSPI& tft, int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t gapX, int16_t gapW) override;
  void drawDrone(TFT_eSPI& tft, int16_t x, int16_t y, uint32_t t) override;
  void drawCoin(TFT_eSPI& tft, int16_t x, int16_t y, uint32_t t) override;

private:
  NeonPalette pal_ = {
    0x0841,  // bgTop        - Nachthimmel (dunkles Blau)
    0x1082,  // bgMid        - Strassenstreifen
    0x2104,  // bgBottom     - Strasse (tiefes Blau-Violett)
    0xC618,  // star         - Sterne (hellgrau)
    0xF81F,  // horizon      - Neon-Magenta
    0x07FF,  // player       - Neon-Cyan
    0x0780,  // playerGlow   - Cyan-Glow
    0xF81F,  // obstacle     - Neon-Magenta
    0x8010,  // obstacleDark - dunkles Magenta
    0xFFE0,  // coin         - Neon-Gelb
    0x8400,  // coinDark     - dunkles Gelb
    0xFFFF,  // text
    0x7BEF,  // textDim
  };
};

// Globaler Renderer. Hier kann zur Laufzeit ein anderes Theme eingeklinkt
// werden, z. B. in init(): gSprites = &meinBitmapTheme;
extern NeonVectorTheme neonVectorTheme;
extern SpriteRenderer* gSprites;

#endif
