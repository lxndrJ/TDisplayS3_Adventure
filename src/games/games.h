#ifndef GAMES_GAMES_H
#define GAMES_GAMES_H

#include "core/game.h"

// Zentrale Registry aller Spiele.
// Neues Spiel: Header hier einbinden, Instanz anlegen,
// in games[] eintragen. Fertig.
#include "eldoria/eldoria.h"
#include "neonrun/neonrun.h"
#include "neonshard/neonshard.h"
#include "chipslide/chipslide.h"

inline Game* makeGame(uint8_t index) {
  static EldoriaGame    eldoria;
  static NeonRunGame    neonRun;
  static NeonShardGame  neonShard;
  static ChipSlideGame  chipSlide;
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &neonShard;
    case 3:  return &chipSlide;
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 4;

#endif
