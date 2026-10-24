#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <Arduino.h>
#include "display.h"
#include "input.h"
#include "game.h"

// Mini-Launcher: startet das aktive Spiel, wechselt bei wantsMenu
// zurueck. Spaeter durch ein grafisches Spielmenue ersetzbar -
// die Engine-Schnittstelle bleibt gleich.
class Engine {
public:
  void begin(Game* game) {
    // Power-Hold VOR dem Display-Init: ohne HIGH auf GPIO15 bleibt das
    // Panel beim T-Display-S3 stromlos (schwarzer Screen, Backlight aus).
    boardPowerOn();
    gfx.init();
    input.init();
    randomSeed(esp_random());
    active_ = game;
    if (active_) active_->init();
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
      }
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

  Game*    active_ = nullptr;
  uint32_t lastMs_ = 0;
};

extern Engine engine;

#endif
