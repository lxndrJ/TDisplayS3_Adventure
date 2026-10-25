#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <Arduino.h>
#include "display.h"
#include "input.h"
#include "game.h"
#include "games/games.h"

// Launcher: zeigt das Spielmenue und startet das gewaehlte Spiel. Ein Spiel
// setzt wantsMenu, um dorthin zurueckzukehren (B lang ist die Konvention).
// Spiele duerfen in init() eine eigene Rotation setzen (z. B. Landscape);
// die Engine stellt vor jedem Start und im Menue die Portrait-Rotation
// des Boards wieder her.
class Engine {
public:
  void begin(Game* boot) {
    // Power-Hold VOR dem Display-Init: ohne HIGH auf GPIO15 bleibt das
    // Panel beim T-Display-S3 stromlos (schwarzer Screen, Backlight aus).
    boardPowerOn();
    gfx.init();
    input.init();
    randomSeed(esp_random());
    active_ = boot;
    if (active_) active_->init();
    else         menuDirty_ = true;
  }

  void loop() {
    uint32_t now = millis();
    uint32_t dt = now - lastMs_;
    lastMs_ = now;

    InputState in;
    input.update(in);

    if (active_) {
      active_->update(in, dt);
      active_->draw();
      if (active_->wantsMenu) {
        active_->quit();
        active_ = nullptr;
        menuDirty_ = true;
      }
    } else {
      menuUpdate(in);
      menuDraw();
    }

    // Frame-Takt (~60 FPS): ohne Begrenzung rendert die Engine so schnell
    // wie der Bus es hergibt - Animationen flackern, dt wird unsauber.
    uint32_t used = millis() - now;
    if (used < FRAME_MS) delay(FRAME_MS - used);
  }

private:
  static constexpr uint32_t FRAME_MS = 16;  // ~60 FPS

  static void boardPowerOn() {
    if constexpr (board::HAS_POWER_HOLD) {
      pinMode(board::PIN_POWER_ON, OUTPUT);
      digitalWrite(board::PIN_POWER_ON, HIGH);
      delay(10);  // Panel-Strom stabilisieren lassen
    }
  }

  void menuUpdate(const InputState& in) {
    if (in.pressed[(uint8_t)Key::A]) {
      menuSel_ = (menuSel_ + GAME_COUNT - 1) % GAME_COUNT;
      menuDirty_ = true;
    }
    if (in.pressed[(uint8_t)Key::B]) {
      menuSel_ = (menuSel_ + 1) % GAME_COUNT;
      menuDirty_ = true;
    }
    if (in.longPressed[(uint8_t)Key::B]) {
      active_ = makeGame(menuSel_);
      if (active_) active_->init();
    }
  }

  void menuDraw() {
    if (!menuDirty_) return;
    menuDirty_ = false;
    TFT_eSPI& tft = gfx.tft();
    tft.setRotation(board::TFT_ROTATION);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(0x07FF);  // Neon-Cyan
    tft.drawString("T-Display S3", 6, 8);
    tft.setTextColor(0x7BEF);
    tft.drawString("Spiele", 6, 26);
    tft.drawFastHLine(0, 44, gfx.width(), 0x8010);
    for (uint8_t i = 0; i < GAME_COUNT; i++) {
      bool sel = (i == menuSel_);
      tft.setTextFont(2);
      tft.setTextColor(sel ? 0xFFE0 : 0x7BEF);  // Gelb = ausgewaehlt
      tft.drawString(sel ? "> " : "  ", 12, 56 + i * 24);
      tft.drawString(makeGame(i)->name(), 34, 56 + i * 24);
    }
    tft.setTextFont(1);
    tft.setTextColor(0x7BEF);
    tft.drawString("A/B: waehlen | B lang: Start", 6, gfx.height() - 12);
  }

  Game*    active_  = nullptr;
  uint8_t  menuSel_  = 0;
  bool     menuDirty_ = false;
  uint32_t lastMs_   = 0;
};

extern Engine engine;

#endif
