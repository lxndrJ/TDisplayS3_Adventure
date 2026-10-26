#ifndef GAMES_NEONRUN_NEONRUN_H
#define GAMES_NEONRUN_NEONRUN_H

#include "core/game.h"
#include "core/display.h"
#include "sprites.h"

// Neon Run - cyberpunk skill game without text.
// You steer a runner across three lanes through the neon city:
// dodge firewall bars, collect data chips, avoid drones.
class NeonRunGame : public Game {
public:
  const char* name() const override { return "Neon Run"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;

  enum Phase : uint8_t { PH_TITLE, PH_RUN, PH_GAMEOVER, PH_WIN };

  struct Entity {
    float    x = 0, y = 0;    // screen position
    uint8_t  lane = 1;       // 0..2
    uint8_t  kind = 0;       // 0 = chip, 1 = drone
    bool     alive = false;
  };

  static constexpr int16_t LANE_X[3]    = {34, 85, 136};
  static constexpr int16_t PLAYER_Y      = 262;
  static constexpr float   PLAYER_SPEED  = 90.0f;   // px/s pro Spurwechsel
  static constexpr float   WORLD_SPEED   = 130.0f;  // px/s Scroll
  static constexpr uint16_t INVULN_MS    = 1400;    // blink frames after a hit
  static constexpr uint16_t MAX_HP       = 3;
  static constexpr uint16_t WIN_SCORE    = 25;      // collected chips to win
  static constexpr uint8_t  MAX_ENTITIES = 8;
  static constexpr uint32_t SPAWN_MS     = 620;     // Basis-Spawn-Intervall
  static constexpr uint32_t BAR_CYCLE    = 900;     // Abstand Firewall-Balken (Scroll-px)
  static constexpr int16_t  BAR_TOP      = -20;     // bar start Y
  static constexpr int16_t  BAR_TRAVEL   = 380;     // bar travel distance

  void startRun();
  void endRun(bool won);
  void spawnEntity();
  void hurt();
  // All draw* helpers render into the framebuffer; render() pushes once per frame.
  void drawHud(TFT_eSprite& fb);
  void drawWorld(TFT_eSprite& fb);
  void drawOverlay(TFT_eSprite& fb, const char* title, const char* sub);
  void render();

  Phase    phase_      = PH_TITLE;
  uint8_t  lane_       = 1;
  float    playerX_    = LANE_X[1];
  uint16_t hp_         = MAX_HP;
  uint16_t score_      = 0;
  uint32_t scroll_     = 0;      // akkumulierte Welt-Pixel
  float    worldSpeed_ = WORLD_SPEED;
  uint32_t spawnTimer_ = 0;
  uint32_t invulnUntil_ = 0;
  uint32_t best_       = 0;      // Highscore (nur RAM)
  int16_t  barY_       = 0;      // Firewall-Balken (nur in update berechnet)
  uint8_t  barGapLane_ = 0;
  bool     barActive_  = false;
  Entity   ents_[MAX_ENTITIES];

  // Full-screen framebuffer (170x320x16 bit ~106 KB, in PSRAM on the S3).
  // Only the single pushSprite() per frame makes the 60 FPS animation
  // flicker-free - drawing directly to the panel flickers on every clear.
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
