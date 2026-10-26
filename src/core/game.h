#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <Arduino.h>
#include "input.h"

// Interface that every game implements.
// The engine launcher switches between games and the menu.
class Game {
public:
  virtual ~Game() = default;
  virtual const char* name() const = 0;
  virtual void init() = 0;                 // once at startup
  virtual void update(const InputState& in, uint32_t dtMs) = 0;
  virtual void draw() = 0;                 // draws onto gfx.tft()
  virtual void quit() {}                   // optional: cleanup before switching

  // true = game still running; false = back to the launcher (menu)
  bool wantsMenu = false;
};

#endif
