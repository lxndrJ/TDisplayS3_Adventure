#ifndef GAMES_CHIPSLIDE_CHIPSLIDE_H
#define GAMES_CHIPSLIDE_CHIPSLIDE_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// CHIP SLIDE - semiconductor sliding puzzle (15-puzzle) in portrait mode.
// 15 components (resistor, diode, LED, transistor, IC, battery, ...) are
// shuffled; the goal is the blueprint (miniature in the HUD). All symbols
// are drawn procedurally - no bitmap usage.
//
// Two-button control, no selection cursor:
//   A short       = change slide direction (arrow in the HUD)
//   B short/hold  = slide the tile in arrow direction; the tile glides
//                   animated into the gap, holding slides automatically
//                   onward (smooth flow of play)
//   A long        = back to the menu
// Correctly placed tiles glow green; complete circuit -> solved.
// An accelerometer (tilt) control can later be added as an additional
// direction input - the game interface stays the same.
class ChipSlideGame : public Game {
public:
  const char* name() const override { return "Chip Slide"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

  enum Phase : uint8_t { PH_TITLE, PH_PLAY, PH_WIN };
  enum Dir : uint8_t { DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_UP, DIR_COUNT };

  static constexpr uint8_t N     = 4;         // 4x4 Zellen
  static constexpr uint8_t CELLS = N * N;     // 16 = 15 Steine + Luecke
  static constexpr uint8_t EMPTY = CELLS - 1; // symbol ID of the gap

  static constexpr uint16_t ANIM_MS         = 140;  // slide animation per move
  static constexpr uint16_t REPEAT_FIRST_MS = 300;  // hold: first repeat
  static constexpr uint16_t REPEAT_MS       = 170;  // hold: subsequent moves
  static constexpr uint16_t SHUFFLE_MOVES   = 250;  // random moves while shuffling

  void startGame();
  void shuffle();
  bool canMove(uint8_t dir) const;
  bool tryMove();
  void finishMove();
  void countSolved();

  void drawTitle(TFT_eSprite& fb);
  void drawGrid(TFT_eSprite& fb);
  void drawHud(TFT_eSprite& fb);
  void drawWin(TFT_eSprite& fb);
  void drawSymbol(TFT_eSprite& fb, uint8_t id, int16_t cx, int16_t cy,
                  uint16_t col, uint16_t bg) const;

  Phase    phase_     = PH_TITLE;
  uint8_t  board_[CELLS];
  uint8_t  empty_     = EMPTY;
  uint8_t  dir_       = DIR_RIGHT;
  uint16_t moves_     = 0;
  uint8_t  solved_    = 0;   // korrekt platzierte Steine (0..15)
  // slide animation of the moved tile
  bool     animating_ = false;
  uint8_t  animFrom_  = 0;
  uint8_t  animTo_    = 0;
  uint8_t  animSym_   = 0;
  uint16_t animMs_    = 0;
  // auto-repeat while B is held
  uint32_t lastMoveMs_  = 0;
  bool     repeatFired_ = false;
  uint16_t dirFlashMs_  = 0;   // direction arrow flashes briefly after A press
  uint32_t winT_        = 0;

  // Full-screen framebuffer (170x320x16 bit ~106 KB, in PSRAM on the S3).
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
