#ifndef GAMES_GAMES_H
#define GAMES_GAMES_H

#include "core/game.h"

// Zentrale Registry aller Spiele.
// Neues Spiel: Header hier einbinden, Instanz anlegen,
// in games[] eintragen. Fertig.
#include "eldoria/eldoria.h"

inline Game* makeGame(uint8_t index) {
  static EldoriaGame eldoria;
  switch (index) {
    case 0:  return &eldoria;
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 1;

// Index des Spiels, das beim Booten startet (0 = erstes in der Liste)
inline constexpr uint8_t BOOT_GAME = 0;

#endif
