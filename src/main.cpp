#include <Arduino.h>
#include "core/engine.h"
#include "games/games.h"

// Einstiegspunkt: Menue zeigen. Die Engine startet das gewaehlte Spiel
// und kehrt bei wantsMenu dorthin zurueck.
void setup() {
  engine.begin(nullptr);
}

void loop() {
  engine.loop();
}
