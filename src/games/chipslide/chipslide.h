#ifndef GAMES_CHIPSLIDE_CHIPSLIDE_H
#define GAMES_CHIPSLIDE_CHIPSLIDE_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// CHIP SLIDE - Halbleiter-Schiebepuzzle (15-Puzzle) im Portrait-Modus.
// 15 Bauteile (Widerstand, Diode, LED, Transistor, IC, Batterie, ...) werden
// gemischt; Ziel ist der Bauplan (Miniatur im HUD). Alle Symbole werden
// prozedural gezeichnet - kein Bitmap-Verbrauch.
//
// Steuerung mit zwei Tastern, ohne Auswahl-Cursor:
//   A kurz        = Schiebe-Richtung wechseln (Pfeil im HUD)
//   B kurz/halten = Stein in Pfeilrichtung schieben; der Stein gleitet
//                   animiert in die Luecke, Halten schiebt automatisch
//                   weiter (fluessiger Spielfluss)
//   A lang        = zurueck zum Menue
// Richtig platzierte Steine leuchten gruen; fertige Schaltung -> Sieg.
// Eine Beschleuniger-Steuerung (Tilt) laesst sich spaeter als zusaetzliche
// Richtungseingabe einhaengen - die Game-Schnittstelle bleibt dieselbe.
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
  static constexpr uint8_t EMPTY = CELLS - 1; // Symbol-ID der Luecke

  static constexpr uint16_t ANIM_MS         = 140;  // Gleit-Animation pro Zug
  static constexpr uint16_t REPEAT_FIRST_MS = 300;  // Halten: erste Wiederholung
  static constexpr uint16_t REPEAT_MS       = 170;  // Halten: Folge-Zuege
  static constexpr uint16_t SHUFFLE_MOVES   = 250;  // Zufallszuege beim Mischen

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
  // Gleit-Animation des gezogenen Steins
  bool     animating_ = false;
  uint8_t  animFrom_  = 0;
  uint8_t  animTo_    = 0;
  uint8_t  animSym_   = 0;
  uint16_t animMs_    = 0;
  // Auto-Repeat waehrend B gehalten wird
  uint32_t lastMoveMs_  = 0;
  bool     repeatFired_ = false;
  uint16_t dirFlashMs_  = 0;   // Richtungspfeil blinkt kurz nach A-Druck
  uint32_t winT_        = 0;

  // Vollbild-Framebuffer (170x320x16 Bit ~106 KB, auf dem S3 im PSRAM).
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
