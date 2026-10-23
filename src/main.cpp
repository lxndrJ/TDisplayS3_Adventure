#include <Arduino.h>
#include "core/engine.h"
#include "games/games.h"

// Einstiegspunkt: Engine initialisieren und das Boot-Spiel starten.
// Bei wantsMenu landet man wieder hier - spaeter durch ein Spielmenue
// mit Auswahl ersetzen (gleiche Game-Schnittstelle).
void setup() {
  engine.begin(makeGame(BOOT_GAME));
}

void loop() {
  engine.loop();
}
