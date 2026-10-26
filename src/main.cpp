#include <Arduino.h>
#include "core/engine.h"
#include "games/games.h"

// Entry point: show the menu. The engine starts the selected game
// and returns there when wantsMenu is set.
void setup() {
  engine.begin(nullptr);
}

void loop() {
  engine.loop();
}
