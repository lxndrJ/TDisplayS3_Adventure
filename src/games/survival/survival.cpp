#include "survival.h"
#include "core/net.h"

// System prompt: strict output format the device can parse line by line.
//   S:<situation text, max ~180 chars, no line breaks>
//   1:<choice label>
//   2:<choice label>
//   3:<choice label>
//   R1:H<hp delta> P<score delta>
//   R2:H<hp delta> P<score delta>
//   R3:H<hp delta> P<score delta>
static const char* kSystemPrompt =
  "You are the storyteller of a grim zombie survival game on a tiny "
  "handheld screen. The player is shown ONLY your situation text and 3 "
  "choices, so the story must be self-contained each turn, but it must "
  "also continue logically from the history given in the user message. "
  "Reply in EXACTLY this format, nothing else:\n"
  "S:<one situation, 120-180 chars, no newlines, continue the story>\n"
  "1:<choice, max 3 words>\n"
  "2:<choice, max 3 words>\n"
  "3:<choice, max 3 words>\n"
  "R1:H<signed hp delta> P<signed score delta>\n"
  "R2:H<signed hp delta> P<signed score delta>\n"
  "R3:H<signed hp delta> P<signed score delta>\n"
  "HP is 0..10, score is survival points. Consequences must fit the "
  "chosen option. Be dark, be brief. Never break the format.";

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

// Landscape layout constants (rotation 1, 320x170)
static constexpr int16_t K_STATUS_H   = 26;   // HP bar row height
static constexpr int16_t K_TEXT_Y     = 32;
static constexpr int16_t K_LINE_H     = 17;   // Font 2 line height
static constexpr int16_t K_OPT_Y      = 108;  // first option row
static constexpr int16_t K_OPT_H      = 19;
static constexpr int16_t K_FOOTER_Y   = 158;

void SurvivalGame::init() {
  gfx.tft().setRotation(1);  // landscape
  phase_  = PH_TITLE;
  hp_     = MAX_HP;
  score_  = 0;
  day_    = 1;
  sel_    = 0;
  page_   = 0;
  dirty_  = true;
  wantsMenu = false;
  requested_ = false;
  errCount_ = 0;
  parseFails_ = 0;
  retryPending_ = false;
  histCount_ = 0;
  for (auto& h : hist_) h[0] = 0;
  msg_[0] = 0;
  lastErr_[0] = 0;
  text_[0] = 0;
  optCount_ = 0;
  for (auto& o : opts_) o[0] = 0;
  titleRefresh_ = 0;  // force first draw

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
  errCount_ = 0;
  parseFails_ = 0;
  retryPending_ = false;
  histCount_ = 0;
  for (auto& h : hist_) h[0] = 0;
  text_[0] = 0;
  optCount_ = 0;
  for (auto& o : opts_) o[0] = 0;
  strncpy(msg_, "Starting...", sizeof(msg_) - 1);
  msgUntil_ = millis() + 2000;
  startRequest();
}

void SurvivalGame::startRequest() {
  requested_ = false;
  wifiWaiting_ = false;
  // Wait for WiFi (up to ~10 s) before falling back to offline, so a
  // slow association doesn't silently disable the AI storyteller.
  if (online_ && !net.connected()) {
    net.begin();  // no-op when already started
    if (net.poll() == NetClient::State::WifiConnecting) {
      phase_ = PH_LOADING;
      dirty_ = true;
      strncpy(msg_, "Connecting to WiFi...", sizeof(msg_) - 1);
      msgUntil_ = millis() + 60000;  // effectively permanent
      wifiWaiting_ = true;
      wifiWaitStart_ = millis();
      return;
    }
  }
  if (online_ && net.connected()) {
    // Build the user prompt: current state + short history of the last
    // rounds, so the LLM continues the story instead of starting fresh.
    char user[640];
    int off = snprintf(user, sizeof(user),
             "Zombie survival, day %u. HP %d/%d, score %d.\n",
             day_, hp_, MAX_HP, score_);
    if (histCount_ > 0) {
      off += snprintf(user + off, sizeof(user) - off, "History:\n");
      for (uint8_t i = 0; i < histCount_ && off < (int)sizeof(user) - HIST_LEN - 8; i++) {
        off += snprintf(user + off, sizeof(user) - off, "- %s\n", hist_[i]);
      }
    }
    snprintf(user + off, sizeof(user) - off,
             "Write the next situation with 3 choices and per-choice "
             "HP/score deltas. Continue the story.");
    net.reset();
    if (net.request(kSystemPrompt, user)) {
      requested_ = true;
      return;
    }
    // request() refused: remember why, degrade to offline.
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    online_ = false;
  }
  offlineTurn();
}

void SurvivalGame::addHistory(const char* chosenLabel, int8_t dHp, int16_t dScore) {
  if (histCount_ < HISTORY) {
    snprintf(hist_[histCount_], HIST_LEN, "Day %u: chose '%s' (%+d HP, %+d pts)",
             day_, chosenLabel ? chosenLabel : "?", dHp, dScore);
    histCount_++;
  } else {
    // shift left, drop the oldest entry
    for (uint8_t i = 0; i < HISTORY - 1; i++)
      strncpy(hist_[i], hist_[i + 1], HIST_LEN);
    snprintf(hist_[HISTORY - 1], HIST_LEN, "Day %u: chose '%s' (%+d HP, %+d pts)",
             day_, chosenLabel ? chosenLabel : "?", dHp, dScore);
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
  if (!online_) {
    strncpy(msg_, lastErr_[0] ? lastErr_ : "Offline scenario",
            sizeof(msg_) - 1);
    msgUntil_ = millis() + 1500;
  }
}

void SurvivalGame::applyTurn(const char* raw) {
  if (!raw) return;
  text_[0] = 0;
  optCount_ = 0;
  for (uint8_t i = 0; i < 3; i++) { opts_[i][0] = 0; llHp_[i] = 0; llScore_[i] = 0; }

  const char* p = raw;
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
    } else if (len >= 3 && p[0] == 'R' && p[1] >= '1' && p[1] <= '3'
               && (p[2] == ':' || p[2] == ' ')) {
      uint8_t idx = p[1] - '1';
      const char* q = p + 2;
      while (*q == ' ' || *q == ':') q++;
      if (*q == 'H') {
        llHp_[idx] = (int8_t)atoi(q + 1);
        const char* sp = strchr(q, 'P');
        if (sp) llScore_[idx] = (int16_t)atoi(sp + 1);
      }
    }
    p = (*eol == '\n') ? eol + 1 : eol;
  }

  // Fallback: if the model produced no usable text, retry once, then
  // degrade to offline — a single bad reply must not end the AI run.
  if (text_[0] == 0 || optCount_ == 0) {
    parseFails_++;
    if (online_ && parseFails_ <= 1) {
      strncpy(lastErr_, "bad AI reply, retry", sizeof(lastErr_) - 1);
      retryPending_ = true;
      retryAt_ = millis() + 800;
      phase_ = PH_LOADING;
      dirty_ = true;
      return;
    }
    if (!lastErr_[0]) strncpy(lastErr_, "bad AI reply", sizeof(lastErr_) - 1);
    online_ = false;
    offlineTurn();
    return;
  }
  parseFails_ = 0;
  page_ = 0;
  sel_ = 0;
  phase_ = PH_PLAY;
  dirty_ = true;
}

void SurvivalGame::choose(uint8_t idx) {
  if (idx >= optCount_) return;
  int8_t dHp; int16_t dScore;
  const char* label;
  if (online_) {
    dHp = llHp_[idx]; dScore = llScore_[idx]; label = opts_[idx];
  } else {
    dHp = offHp_[idx]; dScore = offScore_[idx]; label = opts_[idx];
  }
  applyOutcome(dHp, dScore, label);
}

void SurvivalGame::applyOutcome(int8_t dHp, int16_t dScore, const char* chosenLabel) {
  addHistory(chosenLabel, dHp, dScore);
  hp_ += dHp;
  if (hp_ > MAX_HP) hp_ = MAX_HP;
  score_ += dScore;
  if (score_ < 0) score_ = 0;
  day_++;

  char b[48];
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
  phase_ = PH_LOADING;
  dirty_ = true;
  startRequest();
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
  if (requested_ && ns == NetClient::State::Done) {
    requested_ = false;
    errCount_ = 0;
    applyTurn(net.response());
  } else if (requested_ && ns == NetClient::State::Error) {
    requested_ = false;
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    // Retry transient API errors (429/5xx/timeout) up to 2 times before
    // giving up on the AI for this run — one failed call must not turn
    // the whole game offline.
    errCount_++;
    if (online_ && errCount_ <= 2) {
      retryPending_ = true;
      retryAt_ = millis() + 1200 * errCount_;  // simple backoff
      dirty_ = true;
    } else {
      online_ = false;
      if (phase_ == PH_LOADING) offlineTurn();
    }
  }
  netState_ = ns;

  // Fire a pending retry (API error or unparseable reply).
  if (retryPending_ && millis() >= retryAt_) {
    retryPending_ = false;
    startRequest();
  }

  switch (phase_) {
    case PH_TITLE:
      // Live status refresh on the title screen (every ~500 ms)
      if (millis() - titleRefresh_ > 500) {
        titleRefresh_ = millis();
        dirty_ = true;
      }
      if (in.pressed[(uint8_t)Key::B] || in.pressed[(uint8_t)Key::A]) {
        startGame();
      }
      break;

    case PH_LOADING:
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; break; }
      // While waiting for WiFi, retry startRequest() once connected or
      // give up to offline mode after a timeout.
      if (wifiWaiting_) {
        if (net.connected()) {
          startRequest();
        } else if (millis() - wifiWaitStart_ > 10000) {
          wifiWaiting_ = false;
          strncpy(lastErr_, "wifi timeout", sizeof(lastErr_) - 1);
          online_ = false;
          offlineTurn();
        } else {
          dirty_ = true;  // keep the "Connecting..." animation fresh
        }
      }
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; break; }
      int lines = wrapText(text_, wrapBuf_, 8);
      int pages = (lines + TXT_LINES - 1) / TXT_LINES;
      if (pages < 1) pages = 1;
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
  tft.drawString("HP", 4, 4);
  for (uint8_t i = 0; i < MAX_HP; i++)
    tft.fillRect(20 + i * 12, 4, 9, 8, i < hp_ ? TFT_RED : TFT_DARKGREY);
  char b[24];
  snprintf(b, sizeof(b), "Day %u", day_);
  tft.drawString(b, 150, 4);
  snprintf(b, sizeof(b), "Pts %d", score_);
  tft.drawString(b, 220, 4);
  // connection indicator
  uint16_t connColor;
  const char* connLabel;
  if (online_ && net.connected())      { connColor = 0x07E0; connLabel = "AI"; }
  else if (online_)                    { connColor = TFT_YELLOW; connLabel = "..."; }
  else                                 { connColor = TFT_ORANGE; connLabel = "OFF"; }
  tft.setTextColor(connColor);
  tft.drawString(connLabel, 286, 4);
  tft.drawFastHLine(0, K_STATUS_H - 2, SCR_W, TFT_DARKGREY);
}

void SurvivalGame::drawTitle() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(2);
  tft.setTextColor(TFT_RED);
  tft.drawString("SURVIVAL", 100, 14);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString("Zombie Apocalypse - Day 1", 82, 36);

  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("The dead walk. You have 10 HP.", 20, 58);
  tft.drawString("Survive 30 days. Find a way out.", 20, 72);

  // Live connection status block
  tft.drawFastHLine(0, 92, SCR_W, TFT_DARKGREY);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("CONNECTION", 20, 100);

  tft.setTextColor(TFT_LIGHTGREY);
  uint8_t y = 114;
  if (!net.configured()) {
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("No secrets.h - offline mode", 20, y);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("Copy include/secrets.h.example to", 20, y + 12);
    tft.drawString("include/secrets.h and flash again", 20, y + 24);
  } else {
    const char* wifiState;
    uint16_t wifiColor;
    if (net.connected()) {
      wifiState = "connected"; wifiColor = 0x07E0;
    } else {
      wifiState = (netState_ == NetClient::State::WifiConnecting) ? "connecting..." : "not connected";
      wifiColor = (netState_ == NetClient::State::WifiConnecting) ? TFT_YELLOW : TFT_RED;
    }
    char line[48];
    snprintf(line, sizeof(line), "WiFi: %s (%d dBm)", wifiState, net.rssi());
    tft.setTextColor(wifiColor);
    tft.drawString(line, 20, y);

    tft.setTextColor(net.connected() ? 0x07E0 : TFT_DARKGREY);
    tft.drawString(net.connected() ? "API:  ready (Mistral)" : "API:  waiting for WiFi", 20, y + 12);
  }

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press A or B to start", 20, 148);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 250, 148);
}

void SurvivalGame::drawLoading() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Day " + String(day_), 8, 40);

  tft.setTextFont(1);
  tft.setTextColor(TFT_LIGHTGREY);
  uint8_t dots = (millis() / 400) % 4;
  String s;
  if (online_ && net.connected()) {
    s = "The storyteller thinks";
  } else if (online_) {
    s = "Waiting for WiFi";
  } else {
    s = "Loading scenario";
  }
  for (uint8_t i = 0; i < dots; i++) s += '.';
  tft.drawString(s, 8, 80);

  if (lastErr_[0]) {
    tft.setTextColor(TFT_ORANGE);
    tft.drawString(String("Last error: ") + lastErr_, 8, 100);
  }

  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 8, K_FOOTER_Y);
}

void SurvivalGame::drawPlay() {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();

  int lines = wrapText(text_, wrapBuf_, 8);
  int pages = (lines + TXT_LINES - 1) / TXT_LINES;
  if (pages < 1) pages = 1;
  if (page_ >= (uint8_t)pages) page_ = pages - 1;
  int start = page_ * TXT_LINES;

  // Situation text, Font 2 (double size, readable in landscape)
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  for (uint8_t i = 0; i < TXT_LINES; i++) {
    int li = start + i;
    tft.drawString(li < lines ? wrapBuf_[li] : "", 8, K_TEXT_Y + i * K_LINE_H);
  }
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page_ + 1, pages);
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(pg, SCR_W - 24, K_TEXT_Y);

  // Outcome flash from the previous choice
  if (msg_[0] && millis() < msgUntil_) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(msg_, 8, 96);
  }

  tft.drawFastHLine(0, K_OPT_Y - 4, SCR_W, TFT_DARKGREY);

  if (page_ < pages - 1) {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("B: next page", 8, K_OPT_Y);
  } else {
    for (uint8_t i = 0; i < optCount_; i++) {
      bool sel = (i == sel_);
      tft.setTextColor(sel ? TFT_YELLOW : TFT_LIGHTGREY);
      tft.drawString(sel ? "> " : "  ", 8, K_OPT_Y + i * K_OPT_H);
      tft.drawString(opts_[i], 26, K_OPT_Y + i * K_OPT_H);
    }
  }

  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A/B: select | B long: OK | A long: menu", 8, K_FOOTER_Y);
}

void SurvivalGame::drawEnd(bool won) {
  TFT_eSPI& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  drawStatus();
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(2);
  tft.setTextColor(won ? TFT_GREEN : TFT_RED);
  tft.drawString(won ? "RESCUED!" : "YOU DIED", 90, 40);

  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  char b[40];
  snprintf(b, sizeof(b), "Survived %u days", day_ - 1);
  tft.drawString(b, 90, 72);
  snprintf(b, sizeof(b), "Score: %d", score_);
  tft.drawString(b, 90, 88);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("B long: again", 90, 112);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("A long: menu", 90, K_FOOTER_Y);
}
