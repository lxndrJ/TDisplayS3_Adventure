#ifndef GAMES_NEONDASH_NEONDASH_H
#define GAMES_NEONDASH_NEONDASH_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// NEON DASH - Cyberpunk-Jump'n'Run im Portrait-Modus (170x320).
// Ein Runner rast durch die Neonstadt: Firewall-Staemme und schwebende
// Drohnen kommen von rechts, ein Tastendruck = Sprung. Das Tempo steigt
// kontinuierlich, die Distanz (Meter) ist der Score.
//
// Steuerung:
//   A oder B kurz = springen (einziger Spiel-Button)
//   B lang        = zurueck zum Menue
//   Titel/Game-Over: beliebiger Druck = (neu) starten
class NeonDashGame : public Game {
public:
  const char* name() const override { return "Neon Dash"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;

  enum Phase : uint8_t { PH_TITLE, PH_RUN, PH_GAMEOVER };

  struct Obstacle {
    float   x = 0;        // linke Kante
    int16_t y = 0;        // Oberkante (wird pro Frame gesetzt)
    uint8_t kind = 0;     // 0 = Firewall-Staemme, 1 = schwebende Drohne
    uint8_t h = 0;        // Hoehe in px
    bool    alive = false;
  };
  struct Chip {
    float   x = 0, y = 0;  // Mitte
    bool    alive = false;
  };

  static constexpr uint8_t  MAX_OBS   = 8;
  static constexpr uint8_t  MAX_CHIPS = 4;

  static constexpr int16_t GROUND_Y   = 272;  // Oberkante Bodenlinie
  static constexpr int16_t PLAYER_X   = 36;   // feste Spieler-Spur
  static constexpr int16_t PLAYER_W   = 16;
  static constexpr int16_t PLAYER_H   = 16;

  static constexpr float GRAVITY      = 1500.0f;  // px/s^2
  static constexpr float JUMP_V       = -460.0f;  // px/s
  static constexpr float SPEED_START  = 150.0f;   // px/s
  static constexpr float SPEED_RAMP   = 2.6f;     // px/s pro Sekunde
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
  // Spieler-Physik
  float    py_        = 0;      // Oberkante Spieler-Box
  float    vy_        = 0;      // vertikale Geschwindigkeit
  bool     onGround_  = true;
  // Welt
  float    speed_     = SPEED_START;
  float    distance_  = 0;      // Akkumulierte Strecke in px
  uint32_t runMs_     = 0;
  uint32_t scroll_    = 0;      // Boden-Markierungen (Parallax)
  uint16_t score_     = 0;      // Meter + Chip-Bonus
  uint16_t chipBonus_ = 0;      // gesammelte Chip-Boni (ueberlebt Frames)
  uint16_t best_      = 0;      // Highscore (nur RAM)
  float    spawnIn_   = 0;      // px bis zum naechsten Hindernis
  Obstacle obs_[MAX_OBS];
  Chip     chips_[MAX_CHIPS];

  // Vollbild-Framebuffer (170x320x16 Bit ~106 KB, auf dem S3 im PSRAM)
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
