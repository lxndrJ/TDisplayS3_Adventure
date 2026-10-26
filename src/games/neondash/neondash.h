#ifndef GAMES_NEONDASH_NEONDASH_H
#define GAMES_NEONDASH_NEONDASH_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// NEON DASH - cyberpunk jump'n'run in portrait mode (170x320).
// A runner races through the neon city: firewall pillars and floating
// drones approach from the right, one button press = jump. The pace rises
// continuously; the distance (meters) is the score.
//
// Steuerung:
//   A or B short = jump (the only gameplay button)
//   B long        = back to the menu
//   Title/Game-Over: any press = (re)start
class NeonDashGame : public Game {
public:
  const char* name() const override { return "Neon Dash"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;

  enum Phase : uint8_t { PH_TITLE, PH_RUN, PH_GAMEOVER };

  struct Obstacle {
    float   x = 0;        // linke Kante
    int16_t y = 0;        // top edge (set every frame)
    uint8_t kind = 0;     // 0 = firewall pillar, 1 = floating drone
    uint8_t h = 0;        // Hoehe in px
    bool    alive = false;
  };
  struct Chip {
    float   x = 0, y = 0;  // Mitte
    bool    alive = false;
  };

  static constexpr uint8_t  MAX_OBS   = 8;
  static constexpr uint8_t  MAX_CHIPS = 4;

  static constexpr int16_t GROUND_Y   = 272;  // top edge of the ground line
  static constexpr int16_t PLAYER_X   = 36;   // fixed player lane
  static constexpr int16_t PLAYER_W   = 16;
  static constexpr int16_t PLAYER_H   = 16;

  static constexpr float GRAVITY      = 1500.0f;  // px/s^2
  static constexpr float JUMP_V       = -460.0f;  // px/s
  static constexpr float SPEED_START  = 150.0f;   // px/s
  static constexpr float SPEED_RAMP   = 2.6f;     // px/s per second
  static constexpr float SPEED_MAX    = 470.0f;   // px/s
  static constexpr uint16_t SCORE_CHIP = 25;      // Bonus pro Datenchip

  void startRun();
  void endRun();
  void spawnObstacle();
  bool overlaps(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                int16_t bx, int16_t by, int16_t bw, int16_t bh) const;
  void drawWorld(TFT_eSprite& fb);
  void drawHud(TFT_eSprite& fb);
  void drawOverlay(TFT_eSprite& fb, const char* title, const char* sub);

  Phase    phase_     = PH_TITLE;
  // player physics
  float    py_        = 0;      // top edge of the player box
  float    vy_        = 0;      // vertikale Geschwindigkeit
  bool     onGround_  = true;
  // Welt
  float    speed_     = SPEED_START;
  float    distance_  = 0;      // Akkumulierte Strecke in px
  uint32_t runMs_     = 0;
  uint32_t scroll_    = 0;      // Boden-Markierungen (Parallax)
  uint16_t score_     = 0;      // meters + chip bonus
  uint16_t chipBonus_ = 0;      // collected chip bonuses (survives frames)
  uint16_t best_      = 0;      // highscore (RAM only)
  float    spawnIn_   = 0;      // px until the next obstacle
  Obstacle obs_[MAX_OBS];
  Chip     chips_[MAX_CHIPS];

  // Full-screen framebuffer (170x320x16 bit ~106 KB, in PSRAM on the S3)
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
