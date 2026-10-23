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
  }

private:
  Game*    active_ = nullptr;
  uint32_t lastMs_ = 0;
};

extern Engine engine;

#endif
