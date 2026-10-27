#ifndef GAMES_GAMES_H
#define GAMES_GAMES_H

#include "core/game.h"

// Central registry of all games.
// New game: include the header here, create the instance,
// add it to the switch below. Done.
#include "eldoria/eldoria.h"
#include "neonrun/neonrun.h"
#include "neonshard/neonshard.h"
#include "chipslide/chipslide.h"
#include "neondash/neondash.h"
#include "neonroll/neonroll.h"

inline Game* makeGame(uint8_t index) {
  static EldoriaGame    eldoria;
  static NeonRunGame    neonRun;
  static NeonShardGame  neonShard;
  static ChipSlideGame  chipSlide;
  static NeonDashGame   neonDash;
  static NeonRollGame   neonRoll;
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &neonShard;
    case 3:  return &chipSlide;
    case 4:  return &neonDash;
    case 5:  return &neonRoll;
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 6;

#endif
