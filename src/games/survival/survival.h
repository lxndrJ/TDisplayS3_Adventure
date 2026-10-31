#ifndef GAMES_SURVIVAL_SURVIVAL_H
#define GAMES_SURVIVAL_SURVIVAL_H

#include <Arduino.h>
#include <FS.h>
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
// All text uses the TFT's native font renderer (story/choices: Font 4,
// the largest GLCD font with a full alphabet; title/end: Font 4 size 2)
// with pixel-accurate word wrap.
// Text pages (page_) and a dedicated choice screen (choosing_).
// The title screen toggles the language with B long (0 = English,
// 1 = German, ASCII only).
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
  // Text wrapping is measured in pixels via TFT textWidth, so lines
  // fit the 320px screen exactly at any font size.
  static constexpr uint8_t LINE_LEN     = 96;  // max bytes per wrapped line
  static constexpr uint8_t TXT_LINES    = 5;   // story lines per page (Roboto20 ~24px)
  static constexpr uint8_t MAX_WRAP_LINES = 20; // wrap buffer capacity
  // Smooth fonts (TFT_eSPI .vlw) on the FAT "storage" partition,
  // uploaded with `pio run -t uploadfs`. Story: Roboto20, choices/title:
  // Roboto28. Both contain Basic Latin + Latin-1 Supplement (umlauts).
  static constexpr const char* FONT_STORY  = "Roboto20";
  static constexpr const char* FONT_BIG    = "Roboto28";
  bool     fontReady_ = false;   // .vlw files found on the filesystem
  bool     ffatMounted_ = false; // FFat mounted (kept mounted for font file handle)
  // Story and choices use native Font 4 (26px, full alphabet - the
  // biggest TFT_eSPI GLCD font with letters; fonts 6-8 are digits
  // only). 22 chars * ~13px = ~286px fits the 320px width at size 1,
  // so the LLM is asked for max 3 words / 22 chars per choice.
  static constexpr uint8_t OPT_LEN     = 22;  // max chars per choice label
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
  void offlineTurnAt(uint8_t idx);

  // Word-wrap t into pixel-fitting lines (font/size as drawn).
  // Word wrap in pixels for the currently loaded font (Roboto .vlw when
  // fontReady_, built-in fallback otherwise). UTF-8 safe (umlauts).
  int  wrapText(const char* t, bool big, int16_t maxW,
                char out[][LINE_LEN + 1], int maxL);

  // Load/unload the Roboto .vlw font for the current screen; no-op when
  // falling back to built-in fonts (fontReady_ == false).
  void loadStoryFont();
  void loadBigFont();
  void unloadFontIfLoaded();
  void drawTitle();
  void drawLoading();
  void drawPlay();
  void drawEnd(bool won);
  void drawStatus();
  // Draws s with the TFT's native renderer. big=true uses the Roboto28
  // .vlw smooth font (when fontReady_), otherwise built-in Font 4;
  // big=false uses Roboto20 .vlw or Font 2. x < 0 centers horizontally.
  int16_t drawText(const char* s, int16_t x, int16_t y, bool big,
                   uint16_t color);

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
  uint8_t  retries_      = 0;     // consecutive bad LLM replies this turn
  bool     storyOn_      = false; // a coherent story arc is in progress
  bool     turnOffline_  = false; // current turn uses offline content
  uint8_t  wifiAttempts_ = 0;     // WiFi association retries this wait

  char text_[MAX_TEXT] = "";
  char opts_[MAX_CHOICES][OPT_LEN + 1] = {{0},{0},{0}};
  uint8_t optCount_ = 0;
  // per-choice outcomes, filled by applyTurn() (LLM) or offlineTurn()
  int8_t  llHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t llScore_[MAX_CHOICES] = {0, 0, 0};
  int8_t  offHp_[MAX_CHOICES]    = {0, 0, 0};
  int16_t offScore_[MAX_CHOICES] = {0, 0, 0};
  uint8_t offNext_[MAX_CHOICES]  = {0, 0, 0}; // branch target per choice
  // Story memory: last situations + chosen options, sent back to the
  // LLM so each new turn continues the arc instead of restarting it.
  static constexpr uint8_t HISTORY_N = 4;
  char history_[HISTORY_N][96] = {{0}};
  uint8_t histCount_ = 0;

  char msg_[48]      = "";
  uint32_t msgUntil_ = 0;
  char lastErr_[64]  = "";
  char wrapBuf_[MAX_WRAP_LINES][LINE_LEN + 1];
};

#endif
