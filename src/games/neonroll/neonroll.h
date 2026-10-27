#ifndef GAMES_NEONROLL_NEONROLL_H
#define GAMES_NEONROLL_NEONROLL_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// NEON ROLL - neon marble puzzle in portrait mode (170x320).
// The player does not have full control: one button gives the glowing
// ball a short impulse ("kick") in the current direction, the other
// rotates the direction by 90 degrees. The ball rolls with momentum,
// friction and wall bounce through procedurally generated neon mazes
// until it lands in the goal socket. 8 levels, each with its own
// graphic theme (different neon color palettes and maze styles).
//
// Two-button control, intuitive - no selection cursor:
//   A short       = kick (impulse in the current direction)
//   B short       = rotate direction 90 degrees clockwise
//   A long        = back to the menu
//   Title/Win: any press = start / next level
class NeonRollGame : public Game {
public:
  const char* name() const override { return "Neon Roll"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

  enum Phase : uint8_t { PH_TITLE, PH_PLAY, PH_WIN };
  enum Dir : uint8_t { DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_UP, DIR_COUNT };

  // ---- Board geometry (logical cells) ----
  static constexpr uint8_t COLS = 10;
  static constexpr uint8_t ROWS = 14;
  // Playfield in pixels (portrait): 10 cols x 14 rows
  static constexpr int16_t CELL   = 15;          // logical cell edge in px
  static constexpr int16_t FIELD_X = 5;           // playfield left
  static constexpr int16_t FIELD_Y = 30;          // playfield top (below HUD)
  static constexpr int16_t FIELD_W = COLS * CELL; // 150
  static constexpr int16_t FIELD_H = ROWS * CELL; // 210

  // ---- Ball physics ----
  static constexpr float BALL_R       = 5.0f;   // ball radius px
  static constexpr float KICK_V       = 210.0f; // impulse speed px/s
  static constexpr float FRICTION     = 0.35f;  // 1/s velocity damping
  static constexpr float MIN_V        = 6.0f;   // below this the ball rests
  static constexpr float REST_EPS     = 0.4f;   // position epsilon for "resting"
  static constexpr float GOAL_R       = 6.0f;   // goal socket radius
  static constexpr uint16_t KICK_COOLDOWN_MS = 180; // min time between kicks

  static constexpr uint8_t LEVEL_COUNT = 8;

  // Cell types in the maze grid
  enum Cell : uint8_t {
    C_EMPTY = 0,
    C_WALL  = 1,
    C_GOAL  = 2,
    C_START = 3,   // spawn marker (converted to EMPTY after spawn)
    C_BUMPER = 4,  // bounces the ball with extra energy
  };

  // Per-level theme (different graphics)
  struct Theme {
    uint16_t bg;        // playfield background
    uint16_t wall;      // wall color
    uint16_t wallHi;    // wall highlight
    uint16_t ball;      // ball core
    uint16_t ballGlow;  // ball glow / trail
    uint16_t goal;      // goal socket
    uint16_t bumper;    // bumper accent
    uint16_t grid;      // faint grid lines
    const char* name;   // level theme name (HUD)
  };

  void startLevel(uint8_t lvl);
  void genLevel(uint8_t lvl);
  void kick();
  void rotateDir();
  void stepPhysics(float dt);
  bool cellSolid(uint8_t cx, uint8_t cy) const;
  bool atGoal() const;
  const Theme& theme() const { return kThemes[level_ % LEVEL_COUNT]; }

  void drawTitle(TFT_eSprite& fb);
  void drawField(TFT_eSprite& fb);
  void drawBall(TFT_eSprite& fb);
  void drawHud(TFT_eSprite& fb);
  void drawWin(TFT_eSprite& fb);

  Phase    phase_   = PH_TITLE;
  uint8_t  level_   = 0;
  uint8_t  grid_[ROWS][COLS];

  // ball state (pixel space, center of the ball)
  float    bx_ = 0, by_ = 0;
  float    vx_ = 0, vy_ = 0;
  Dir      dir_ = DIR_RIGHT;
  bool     moving_ = false;

  uint16_t kicks_      = 0;   // kicks used this level
  uint16_t totalKicks_ = 0;   // across all levels
  uint16_t bestKicks_  = 0xFFFF; // fewest kicks to clear a level (RAM)
  uint32_t levelStartMs_ = 0;
  uint32_t lastKickMs_   = 0;
  uint32_t winT_         = 0;

  // trail (previous positions for the glow effect)
  static constexpr uint8_t TRAIL_LEN = 6;
  float trailX_[TRAIL_LEN];
  float trailY_[TRAIL_LEN];
  uint8_t trailN_ = 0;

  // Full-screen framebuffer (170x320x16 bit ~106 KB, in PSRAM on the S3)
  TFT_eSprite fb_{&gfx.tft()};

  // 8 neon themes
  static const Theme kThemes[LEVEL_COUNT];
};

#endif
