#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <Arduino.h>
#include "input.h"

// Schnittstelle, die jedes Spiel implementiert.
// Der Engine-Launcher wechselt zwischen Spielen und dem Menue.
class Game {
public:
  virtual ~Game() = default;
  virtual const char* name() const = 0;
  virtual void init() = 0;                 // einmalig beim Start
  virtual void update(const InputState& in, uint32_t dtMs) = 0;
  virtual void draw() = 0;                 // zeichnet auf gfx.tft()
  virtual void quit() {}                   // optional: Aufraeumen vor dem Wechsel

  // true = Spiel laeuft noch; false = zurueck zum Launcher (Menue)
  bool wantsMenu = false;
};

#endif
