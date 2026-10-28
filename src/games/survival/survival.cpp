#include "survival.h"
#include "core/net.h"

// System prompt: strict output format the device can parse line by line.
//   S:<situation text, max ~200 chars, no line breaks>
//   1:<choice label>
//   2:<choice label>
//   3:<choice label>
//   H:<hp delta for choice 1> S:<score delta 1>
//   H:<hp delta for choice 2> S:<score delta 2>
//   H:<hp delta for choice 3> S:<score delta 3>
static const char* kSystemPrompt =
  "You are the storyteller of a grim zombie survival game on a tiny "
  "handheld screen (24 chars per line). Reply in EXACTLY this format, "
  "nothing else:\n"
  "S:<one short situation, max 180 chars, no newlines>\n"
  "1:<choice, max 20 chars>\n"
  "2:<choice, max 20 chars>\n"
  "3:<choice, max 20 chars>\n"
  "R1:H<signed hp delta> P<signed score delta>\n"
  "R2:H<signed hp delta> P<signed score delta>\n"
  "R3:H<signed hp delta> P<signed score delta>\n"
  "HP is 0..10, score is survival days x10 plus loot. Be dark, be brief.";

// Built-in offline scenarios (used when WiFi/API is unavailable).
struct OfflineTurn {
  const char* text;
  const char* opt[3];
  int8_t  hp[3];
  int16_t score[3];
};

static const OfflineTurn kOffline[] = {
  {"Rain drums on the tin roof of the abandoned mall. In the food court a lone zombie shuffles between overturned tables.",
   {"Search the freezers", "Sneak past", "Climb the shelves"},
   {0, 0, -1}, {30, 10, 10}},
  {"A bitten survivor begs at the mall entrance. Behind him, three zombies wake up and turn.",
   {"Give him a medkit", "Run, leave him", "Shoot the zombies"},
   {1, -2, -1}, {40, 20, 10}},
  {"Night falls. Your flashlight flickers over a dark corridor. Somewhere a generator hums.",
   {"Follow the hum", "Hide in a shop", "Rush the exit"},
   {20, 0, -2}, {10, 10, 0}},
  {"You find a police car. Keys are in the ignition. The tank is half full.",
   {"Hotwire and drive", "Loot the trunk", "Walk on foot"},
   {20, 0, 0}, {10, 20, -10}},
  {"A horde moans beyond the parking garage gate. The gate is half open.",
   {"Sprint through", "Climb the ramp", "Wait it out"},
   {-2, -1, 0}, {10, 0, 10}},
  {"Dawn. A helicopter passes far away. On the roof you spot a flare gun.",
   {"Fire the flare", "Save it", "Signal and wait"},
   {0, 0, 1}, {30, 10, 20}},
};
static constexpr uint8_t kOfflineCount = sizeof(kOffline) / sizeof(kOffline[0]);

void SurvivalGame::init() {
  phase_  = PH_TITLE;
  hp_     = MAX_HP;
  score_  = 0;
  day_    = 1;
  sel_    = 0;
  page_   = 0;
  dirty_  = true;
  wantsMenu = false;
  msg_[0] = 0;
  text_[0] = 0;
  optCount_ = 0;
  for (auto& o : opts_) o[0] = 0;

  net.begin();
  online_ = net.configured();
}

void SurvivalGame::quit() {
  net.reset();
}

void SurvivalGame::startGame() {
  phase_ = PH_LOADING;
  hp_ = MAX_HP;
  score_ = 0;
  day_ = 1;
  sel_ = 0;
  page_ = 0;
  dirty_ = true;
  text_[0] = 0;
  optCount_ = 0;
  for (auto& o : opts_) o[0] = 0;
  strncpy(msg_, online_ ? "Connecting..." : "Offline mode", sizeof(msg_) - 1);
  msgUntil_ = millis() + 2000;
  requestTurn();
}

void SurvivalGame::requestTurn() {
  if (online_) {
    char user[220];
    snprintf(user, sizeof(user),
             "Zombie survival, day %u. HP %d/%d, score %d. The player "
             "just survived the last situation. Write the next situation "
             "with 3 choices and per-choice HP/score deltas.",
             day_, hp_, MAX_HP, score_);
    net.reset();
    if (!net.request(kSystemPrompt, user)) {
      // WiFi dropped or busy: degrade to offline mode for this run.
      online_ = false;
      offlineTurn();
    }
  } else {
    offlineTurn();
  }
}

void SurvivalGame::offlineTurn() {
  const OfflineTurn& t = kOffline[offlineIdx_ % kOfflineCount];
  offlineIdx_++;
  strncpy(text_, t.text, MAX_TEXT - 1);
  text_[MAX_TEXT - 1] = 0;
  optCount_ = 3;
  for (uint8_t i = 0; i < 3; i++) {
    strncpy(opts_[i], t.opt[i], OPT_LEN);
    opts_[i][OPT_LEN] = 0;
    offHp_[i] = t.hp[i];
    offScore_[i] = t.score[i];
  }
  page_ = 0;
  sel_ = 0;
  phase_ = PH_PLAY;
  dirty_ = true;
  strncpy(msg_, "Offline scenario", sizeof(msg_) - 1);
  msgUntil_ = millis() + 1500;
}

void SurvivalGame::applyTurn(const char* raw) {
  if (!raw) return;
  // Parse the strict LLM format.
  const char* p = raw;
  text_[0] = 0;
  optCount_ = 0;
  for (uint8_t i = 0; i < 3; i++) { opts_[i][0] = 0; llHp_[i] = 0; llScore_[i] = 0; }

  while (*p) {
    const char* eol = p;
    while (*eol && *eol != '\n') eol++;
    size_t len = eol - p;

    if (len >= 2 && p[0] == 'S' && p[1] == ':') {
      size_t n = len - 2;
      if (n > MAX_TEXT - 1) n = MAX_TEXT - 1;
      memcpy(text_, p + 2, n);
      text_[n] = 0;
    } else if (len >= 3 && p[0] >= '1' && p[0] <= '3' && p[1] == ':') {
      uint8_t idx = p[0] - '1';
      size_t n = len - 2;
      if (n > OPT_LEN) n = OPT_LEN;
      memcpy(opts_[idx], p + 2, n);
      opts_[idx][n] = 0;
      if (idx + 1 > optCount_) optCount_ = idx + 1;
    } else if (len >= 3 && p[0] == 'R' && p[1] >= '1' && p[1] <= '3') {
      uint8_t idx = p[1] - '1';
      const char* q = p + 2;
      while (*q == ' ') q++;
      if (*q == 'H') {
        llHp_[idx] = (int8_t)atoi(q + 1);
        const char* sp = strchr(q, 'P');
        if (sp) llScore_[idx] = (int16_t)atoi(sp + 1);
      }
    }
    p = (*eol == '\n') ? eol + 1 : eol;
  }

  // Fallback: if the model produced no usable text, use offline.
  if (text_[0] == 0 || optCount_ == 0) {
    online_ = false;
    offlineTurn();
    return;
  }
  page_ = 0;
  sel_ = 0;
  phase_ = PH_PLAY;
  dirty_ = true;
}

void SurvivalGame::choose(uint8_t idx) {
  if (idx >= optCount_) return;
  int8_t dHp; int16_t dScore;
  if (online_) {
    dHp = llHp_[idx]; dScore = llScore_[idx];
  } else {
    dHp = offHp_[idx]; dScore = offScore_[idx];
  }
  applyOutcome(dHp, dScore);
}

void SurvivalGame::applyOutcome(int8_t dHp, int16_t dScore) {
  hp_ += dHp;
  if (hp_ > MAX_HP) hp_ = MAX_HP;
  score_ += dScore;
  if (score_ < 0) score_ = 0;
  day_++;

  char b[40];
  snprintf(b, sizeof(b), "%+d HP  %+d pts", dHp, dScore);
  strncpy(msg_, b, sizeof(msg_) - 1);
  msgUntil_ = millis() + 1500;

  if (hp_ <= 0) {
    phase_ = PH_DEAD;
    dirty_ = true;
    return;
  }
  if (day_ > 30) {
    phase_ = PH_WIN;
    dirty_ = true;
    return;
  }
  // Next turn
  phase_ = PH_LOADING;
  dirty_ = true;
  requestTurn();
}

int SurvivalGame::wrapText(const char* t, char out[][TXT_W + 1], int maxL) const {
  int n = 0;
  const char* p = t;
  while (*p && n < maxL) {
    const char* start = p;
    int i = 0, lastSpace = -1;
    while (*p && i < TXT_W) {
      if (*p == ' ') lastSpace = i;
      out[n][i++] = *p++;
    }
    if (*p && i == TXT_W && lastSpace > 0) {
      out[n][lastSpace] = 0;
      p = start + lastSpace + 1;
    } else {
      out[n][i] = 0;
    }
    n++;
  }
  return n;
}

void SurvivalGame::update(const InputState& in, uint32_t dtMs) {
  (void)dtMs;

  // Poll the network client every frame, whatever the phase.
  NetClient::State ns = net.poll();
  if (phase_ == PH_LOADING && ns == NetClient::State::Done) {
    applyTurn(net.response());
  } else if (phase_ == PH_LOADING && ns == NetClient::State::Error) {
    // API failed: degrade to offline for the rest of the run.
    online_ = false;
    strncpy(msg_, net.error(), sizeof(msg_) - 1);
    msgUntil_ = millis() + 1500;
    offlineTurn();
  }

  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::B] || in.pressed[(uint8_t)Key::A]) {
        startGame();
      }
      break;

    case PH_LOADING:
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; }
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; break; }
      int lines = wrapText(text_, wrapBuf_, 12);
      int pages = (lines + TXT_LINES - 1) / TXT_LINES;
      if (page_ >= (uint8_t)pages) page_ = pages - 1;

      if (in.pressed[(uint8_t)Key::A]) {
        if (page_ > 0) { page_--; dirty_ = true; }
        else if (optCount_ > 0) { sel_ = (sel_ + optCount_ - 1) % optCount_; dirty_ = true; }
      }
      if (in.pressed[(uint8_t)Key::B]) {
        if (page_ < pages - 1) { page_++; dirty_ = true; }
        else if (optCount_ > 0) { sel_ = (sel_ + 1) % optCount_; dirty_ = true; }
      }
      if (in.longPressed[(uint8_t)Key::B]) {
        if (page_ < pages - 1) { page_++; dirty_ = true; }
        else choose(sel_);
      }
      break;
    }

    case PH_DEAD:
    case PH_WIN:
      if (in.longPressed[(uint8_t)Key::B]) init();
      else if (in.longPressed[(uint8_t)Key::A]) wantsMenu = true;
      break;
  }

  if (msg_[0] && millis() > msgUntil_) { msg_[0] = 0; dirty_ = true; }
}

void SurvivalGame::draw() {
  if (!dirty_) return;
  dirty_ = false;
  switch (phase_) {
    case PH_TITLE:   drawTitle(); break;
    case PH_LOADING: drawLoading(); break;
    case PH_PLAY:    drawPlay(); break;
    case PH_DEAD:    drawEnd(false); break;
    case PH_WIN:     drawEnd(true); break;
  }
}

void SurvivalGame::drawStatus() {
  TFT_eSPI& tft = gfx.tft();
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("HP", 6, 2);
  for (uint8_t i = 0; i < MAX_HP; i++)
    tft.fillRect(20 + i * 14, 2, 11, 8, i < hp_ ? TFT_RED : TFT_DARKGREY);
  char b[24];
  snprintf(b, sizeof(b), "Day:%u", day_);
  tft.drawString(b, 6, 14);
  snprintf(b, sizeof(b), "Pts:%d", score_);
  tft.drawString(b, 70, 14);
  tft.setTextColor(online_ ? 0x07E0 : TFT_ORANGE);
  tft.drawString(online_ ? "AI" : "OFF", 128, 14);
  tft.drawFastHLine(0, 24, gfx.width(), TFT_DARKGREY);
}

void SurvivalGame::drawTitle() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_RED);
  tft.drawString("SURVIVAL", 30, 60);
  tft.setTextFont(1);
  tft.setTextColor(TFT_LIGHTGREY);
  const char* lines[] = {
    "The dead walk. You have",
    "10 HP and a backpack.",
    "Survive 30 days.",
    "",
    online_ ? "AI storyteller: online" : "AI storyteller: OFFLINE",
  };
  for (uint8_t i = 0; i < 5; i++)
    tft.drawString(lines[i], 20, 110 + i * 18);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press A or B to start", 20, 220);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 20, 300);
}

void SurvivalGame::drawLoading() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Day " + String(day_), 6, 60);
  tft.setTextFont(1);
  tft.setTextColor(TFT_LIGHTGREY);
  // animated dots
  uint8_t dots = (millis() / 400) % 4;
  String s = online_ ? "The storyteller thinks" : "Loading";
  for (uint8_t i = 0; i < dots; i++) s += '.';
  tft.drawString(s, 6, 100);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 6, 300);
}

void SurvivalGame::drawPlay() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();

  int lines = wrapText(text_, wrapBuf_, 12);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  if (page_ >= (uint8_t)pages) page_ = pages - 1;
  int start = page_ * TXT_LINES;

  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  for (uint8_t i = 0; i < TXT_LINES; i++) {
    int li = start + i;
    tft.drawString(li < lines ? wrapBuf_[li] : "", 6, 32 + i * 14);
  }
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page_ + 1, pages);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(pg, 150, 32);

  if (msg_[0] && millis() < msgUntil_) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(msg_, 6, 122);
  }

  tft.drawFastHLine(0, 136, gfx.width(), TFT_DARKGREY);

  if (page_ < pages - 1) {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("B: next page", 6, 148);
  } else {
    for (uint8_t i = 0; i < optCount_; i++) {
      bool sel = (i == sel_);
      tft.setTextColor(sel ? TFT_YELLOW : TFT_LIGHTGREY);
      tft.drawString(sel ? "> " : "  ", 6, 148 + i * 18);
      tft.drawString(opts_[i], 20, 148 + i * 18);
    }
  }

  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A/B: select | B long: OK", 6, 300);
}

void SurvivalGame::drawEnd(bool won) {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(won ? TFT_GREEN : TFT_RED);
  tft.drawString(won ? "RESCUED!" : "YOU DIED", 20, 70);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  char b[40];
  snprintf(b, sizeof(b), "Survived %u days", day_ - 1);
  tft.drawString(b, 20, 110);
  snprintf(b, sizeof(b), "Score: %d", score_);
  tft.drawString(b, 20, 128);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("B long: again", 20, 180);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 20, 300);
}

void SurvivalGame::render() {}
