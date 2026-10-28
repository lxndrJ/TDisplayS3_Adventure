#ifndef GAMES_SURVIVAL_SURVIVAL_H
#define GAMES_SURVIVAL_SURVIVAL_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"

// SURVIVAL - zombie survival with an LLM storyteller (Mistral AI).
// The board shows only: situation text, 3 choices, HP and score.
// The model generates situation + choices + outcome; the device parses
// a strict line format and applies HP/score locally.
//
// Without WiFi/API key (include/secrets.h missing) the game falls back
// to built-in offline scenarios, so it always runs.
//
// Controls:
//   A short       = previous choice / previous text page
//   B short       = next choice / next text page
//   B long        = confirm choice / start / restart
//   A long        = back to the launcher menu
class SurvivalGame : public Game {
public:
  const char* name() const override { return "Survival"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

  enum Phase : uint8_t {
    PH_TITLE, PH_LOADING, PH_PLAY, PH_DEAD, PH_WIN
  };

  static constexpr uint8_t MAX_HP      = 10;
  static constexpr uint8_t MAX_CHOICES = 3;
  static constexpr uint8_t TXT_W       = 24;   // chars per wrapped line
  static constexpr uint8_t TXT_LINES   = 6;    // text lines per page
  static constexpr uint8_t OPT_LEN     = 22;   // max chars per choice label
  static constexpr size_t  MAX_TEXT    = 400;  // situation text cap

  void startGame();
  void requestTurn();
  void applyTurn(const char* raw);
  void choose(uint8_t idx);
  void applyOutcome(int8_t dHp, int16_t dScore);
  void offlineTurn();

  int  wrapText(const char* t, char out[][TXT_W + 1], int maxL) const;

  void drawTitle();
  void drawLoading();
  void drawPlay();
  void drawEnd(bool won);
  void drawStatus();
  void render();

  Phase    phase_   = PH_TITLE;
  int      hp_      = MAX_HP;
  int16_t  score_   = 0;
  uint16_t day_     = 1;
  uint8_t  sel_     = 0;
  uint8_t  page_    = 0;
  bool     dirty_   = true;
  bool     online_  = false;   // LLM reachable, offline fallback otherwise
  uint8_t  offlineIdx_ = 0;    // rotating offline scenario index

  char text_[MAX_TEXT] = "";
  char opts_[MAX_CHOICES][OPT_LEN + 1] = {{0},{0},{0}};
  uint8_t optCount_ = 0;
  // per-choice outcomes, filled by applyTurn() (LLM) or offlineTurn()
  int8_t  llHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t llScore_[MAX_CHOICES] = {0, 0, 0};
  int8_t  offHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t offScore_[MAX_CHOICES] = {0, 0, 0};

  char msg_[40]      = "";
  uint32_t msgUntil_ = 0;
  char wrapBuf_[12][TXT_W + 1];
};

#endif
