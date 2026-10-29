#ifndef GAMES_SURVIVAL_SURVIVAL_H
#define GAMES_SURVIVAL_SURVIVAL_H

#include <Arduino.h>
#include "core/game.h"
#include "core/display.h"
#include "core/net.h"

// SURVIVAL - zombie survival with an LLM storyteller (Mistral AI),
// rendered in landscape (320x170, rotation 1) so Font 2 stays readable.
// The board shows only: situation text, 3 choices, HP bar, day and score.
// The model generates situation + choices + outcome; the device parses
// a strict line format and applies HP/score locally.
//
// Without WiFi/API key (include/secrets.h missing) the game falls back
// to built-in offline scenarios, so it always runs. The title screen
// shows the live WiFi/API status (RSSI, connecting, ready, failed).
//
// Controls:
//   A short       = previous text page / previous choice
//   B short       = next text page / next choice
//   B long        = show choices / confirm choice / restart
//   A long        = back to the launcher menu
// Situation text and choices are drawn 2x big - text pages (page_)
// and a dedicated choice screen (choosing_). The title screen toggles
// the language with B long (0 = English, 1 = German, ASCII only).
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
  // Chars per wrapped line. Text is drawn 1.5x big (Font 2 into a
  // line sprite, block-scaled to 24px height, 320px = 213 sprite px),
  // and Font 2 averages ~7px per char -> 26 chars fill the width.
  static constexpr uint8_t TXT_W       = 26;
  static constexpr uint8_t TXT_LINES   = 5;   // text lines per page (24px each)
  static constexpr uint8_t MAX_WRAP_LINES = 16; // wrap buffer capacity (400/26)
  static constexpr uint8_t OPT_LEN     = 30;  // max chars per choice label
  static constexpr size_t  MAX_TEXT    = 400; // situation text cap

  // Landscape geometry (rotation 1). gfx.width()/height() are static
  // portrait values, so the game uses its own constants.
  static constexpr int16_t SCR_W = 320;
  static constexpr int16_t SCR_H = 170;

  void startGame();
  void startRequest();
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
  // Draws s scaled 1.5x: Font 2 rendered into a 213x16 line sprite,
  // pushed block-scaled (24px high). x < 0 centers the text
  // horizontally; returns the x where the block was drawn.
  int16_t drawBigText(const char* s, int16_t x, int16_t y, uint16_t color);

  Phase    phase_   = PH_TITLE;
  int      hp_      = MAX_HP;
  int16_t  score_   = 0;
  uint16_t day_     = 1;
  uint8_t  sel_     = 0;
  uint8_t  page_    = 0;
  bool     dirty_   = true;
  bool     online_  = false;   // LLM reachable, offline fallback otherwise
  bool     requested_ = false;  // an API request is in flight
  NetClient::State netState_ = NetClient::State::Idle;  // cached for draw
  uint8_t  offlineIdx_ = 0;     // rotating offline scenario index
  uint32_t titleRefresh_ = 0;   // title screen redraw timer (live status)
  uint32_t wifiWaitStart_ = 0;  // when the WiFi wait began (loading phase)
  bool     wifiWaiting_  = false; // loading phase is waiting for WiFi
  bool     choosing_     = false; // PH_PLAY: choice screen (vs. text pages)
  uint8_t  lang_         = 1;     // 0 = English, 1 = German (default), toggle: B long on title

  char text_[MAX_TEXT] = "";
  char opts_[MAX_CHOICES][OPT_LEN + 1] = {{0},{0},{0}};
  uint8_t optCount_ = 0;
  // per-choice outcomes, filled by applyTurn() (LLM) or offlineTurn()
  int8_t  llHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t llScore_[MAX_CHOICES] = {0, 0, 0};
  int8_t  offHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t offScore_[MAX_CHOICES] = {0, 0, 0};

  char msg_[48]      = "";
  uint32_t msgUntil_ = 0;
  char lastErr_[64]  = "";
  char wrapBuf_[MAX_WRAP_LINES][TXT_W + 1];
};

#endif
