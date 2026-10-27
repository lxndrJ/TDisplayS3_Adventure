#ifndef GAMES_SAKURA_SAKURA_H
#define GAMES_SAKURA_SAKURA_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// SAKURA - Tamagotchi cherry tree in portrait mode (170x320).
// Grow a cherry tree from seed to a blossoming, cherry-bearing tree.
// The tree passes 5 growth stages over decades (fast-forwarded for
// testing: one in-game decade = ~8 seconds of real time).
//
// Stages:  SEED -> SPROUT -> SAPLING -> BLOSSOMING -> FRUITING (mature)
//
// Care actions (all mapped to the 2 buttons):
//   A short       = cycle action (Water / Fertilize / Prune / Harvest)
//   B short       = perform the selected action
//   A long        = back to the menu
//   Title/Game-Over: any press = start
//
// The tree has a health value (0..100). Neglect (no water / no fertilizer
// / overgrown) drains health; good care restores it. If health hits 0
// the tree withers -> game over. Harvesting cherries scores points.
class SakuraGame : public Game {
public:
  const char* name() const override { return "Sakura"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

  enum Phase : uint8_t { PH_TITLE, PH_PLAY, PH_DEAD };

  // Growth stages
  enum Stage : uint8_t {
    ST_SEED = 0,     // kernel in the soil
    ST_SPROUT,       // small green sprout
    ST_SAPLING,      // young tree with trunk + branches
    ST_BLOSSOMING,   // pink blossoms
    ST_FRUITING,     // mature tree with cherries
    ST_COUNT
  };

  // Care actions (cycled with A, performed with B)
  enum Action : uint8_t {
    ACT_WATER = 0,    // pour water -> restores moisture
    ACT_FERTILIZE,    // fertilize  -> restores nutrients
    ACT_PRUNE,        // prune leaves/branches -> restores vigor
    ACT_HARVEST,      // pick cherries -> score (only when fruiting)
    ACT_COUNT
  };

  // One in-game decade. For testing the decades pass quickly:
  // 8 seconds of real time = one decade.
  static constexpr uint32_t DECADE_MS = 8000;
  static constexpr uint8_t  DECADES_PER_STAGE = 2;  // 2 decades per stage

  // Decay rates per decade (how fast the stats drop if neglected)
  static constexpr int8_t DECAY_MOIST   = 12;  // moisture drain / decade
  static constexpr int8_t DECAY_NUTRI   = 8;   // nutrient drain / decade
  static constexpr int8_t DECAY_VIGOR   = 6;   // vigor drain / decade (overgrown)
  // Health drain when any stat is critically low
  static constexpr int8_t HEALTH_DRAIN  = 10;
  static constexpr int8_t HEALTH_RECOVER = 4;

  // Care action effects (instant)
  static constexpr int8_t WATER_AMOUNT     = 30;
  static constexpr int8_t FERTILIZE_AMOUNT = 25;
  static constexpr int8_t PRUNE_AMOUNT     = 20;
  static constexpr uint16_t CHERRY_SCORE  = 10;  // points per cherry
  static constexpr uint8_t  CHERRIES_MAX   = 12; // visible cherries on tree

  void startGame();
  void performAction();
  void advanceDecade();
  void updateHealth();
  bool  isMature() const { return stage_ >= ST_FRUITING; }
  uint8_t stageForDecade(uint16_t decade) const;

  void drawTitle(TFT_eSprite& fb);
  void drawScene(TFT_eSprite& fb);
  void drawTree(TFT_eSprite& fb);
  void drawHud(TFT_eSprite& fb);
  void drawActionMenu(TFT_eSprite& fb);
  void drawDead(TFT_eSprite& fb);

  Phase    phase_    = PH_TITLE;
  Stage    stage_    = ST_SEED;
  uint16_t decade_   = 0;        // in-game decades elapsed
  uint32_t decadeMs_ = 0;        // ms accumulated toward the next decade

  // Care stats (0..100)
  int8_t moisture_  = 60;
  int8_t nutrients_ = 60;
  int8_t vigor_     = 60;        // overgrown when low
  int8_t health_    = 100;

  // Score
  uint16_t score_    = 0;
  uint16_t cherries_ = 0;        // total harvested
  uint16_t best_     = 0;        // highscore (RAM only)

  // UI state
  Action   action_   = ACT_WATER;
  uint16_t actionFlashMs_ = 0;   // flash after performing an action
  uint16_t cycleFlashMs_  = 0;   // flash after cycling action
  uint32_t lastActionMs_  = 0;   // feedback timing

  // Visual animation
  uint16_t sway_     = 0;        // wind sway phase (ms)
  bool     rain_     = false;    // occasional rain event (auto-water)
  uint32_t rainUntil_ = 0;

  // Full-screen framebuffer (170x320x16 bit ~106 KB, in PSRAM on the S3)
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
