#include "survival.h"
#include "core/net.h"

// System prompt: strict output format the device can parse line by line.
//   S:<situation text, max ~130 chars, no line breaks>
//   1:<choice label, max 26 chars>
//   2:<choice label>
//   3:<choice label>
//   R1:H<hp delta> P<score delta>
//   R2:H<hp delta> P<score delta>
//   R3:H<hp delta> P<score delta>
// Font 2 on the device is ASCII-only, so German must avoid umlauts.
static const char* kSystemPromptEn =
  "You are the storyteller of a grim zombie survival game on a tiny "
  "handheld screen. Reply in EXACTLY this format, nothing else:\n"
  "S:<one short situation, max 130 chars, no newlines>\n"
  "1:<choice, max 26 chars>\n"
  "2:<choice, max 26 chars>\n"
  "3:<choice, max 26 chars>\n"
  "R1:H<signed hp delta> P<signed score delta>\n"
  "R2:H<signed hp delta> P<signed score delta>\n"
  "R3:H<signed hp delta> P<signed score delta>\n"
  "HP is 0..10, score is survival points. The choices are shown very "
  "big on a 320px screen: keep every choice under 26 characters. "
  "Be dark, be brief.";

static const char* kSystemPromptDe =
  "Du bist der Erzaehler eines grimmigen Zombie-Survival-Spiels auf "
  "einem winzigen Handheld-Bildschirm. Antworte GENAU in diesem "
  "Format, nichts anderes:\n"
  "S:<eine kurze Situation, max 130 Zeichen, keine Zeilenumbrueche>\n"
  "1:<Wahl, max 26 Zeichen>\n"
  "2:<Wahl, max 26 Zeichen>\n"
  "3:<Wahl, max 26 Zeichen>\n"
  "R1:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "R2:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "R3:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "HP ist 0..10, Punkte sind Ueberlebenspunkte. Schreibe DEUTSCH, aber "
  "NUR ASCII: keine Umlaute, schreibe ae/oe/ue/ss. Die Wahlmoeglichkeiten "
  "erscheinen sehr gross auf einem 320px-Schirm: jede Wahl unter 26 "
  "Zeichen. Grimmig, kurz.";

// UI strings, indexed by lang_ (0 = EN, 1 = DE).
struct UiText {
  const char* subtitle;
  const char* intro1, *intro2;
  const char* start;
  const char* langHint;
  const char* thinks, *waitWifi, *offline;
  const char* hintPage, *hintChoose;
  const char* hintSelect, *hintMenu;
  const char* died, *rescued, *survived;
  const char* scoreLabel, *again;
  const char* dayLabel, *ptsLabel, *ptsWord;
  const char* offScenario, *lastErrLabel, *connTitle;
};

static const UiText kUi[2] = {
  {   // English
    "Zombie Apocalypse",
    "The dead walk. You have 10 HP.",
    "Survive 30 days. Find a way out.",
    "Press A or B to start",
    "B long: Deutsch",
    "The storyteller thinks", "Waiting for WiFi", "Loading scenario",
    "B: next page", "B: choose",
    "A/B: select   B long: OK", "A long: menu",
    "YOU DIED", "RESCUED!", "Survived %u days",
    "Score: %d", "B long: again",
    "Day %u", "Pts %d", "pts",
    "Offline scenario", "Last error: ", "CONNECTION",
  },
  {   // German (ASCII only)
    "Zombie-Apokalypse",
    "Die Toten laufen. Du hast 10 HP.",
    "Ueberlebe 30 Tage. Find einen Ausweg.",
    "A oder B: Start",
    "B lang: English",
    "Der Erzaehler denkt", "Warte auf WLAN", "Lade Szenario",
    "B: weiter", "B: Auswahl",
    "A/B: waehlen   B lang: OK", "A lang: Menue",
    "GESTORBEN", "GERETTET!", "%u Tage ueberlebt",
    "Punkte: %d", "B lang: Nochmal",
    "Tag %u", "Pkt %d", "Pkt",
    "Offline-Szenario", "Letzter Fehler: ", "VERBINDUNG",
  },
};

// Built-in offline scenarios (used when WiFi/API is unavailable).
struct OfflineTurn {
  const char* text;
  const char* opt[3];
  int8_t  hp[3];
  int16_t score[3];
};

static const OfflineTurn kOfflineEn[] = {
  {"Rain hammers the dead mall. A lone zombie drifts between the tables.",
   {"Loot fridge", "Sneak past", "Climb up"},
   {0, 0, -1}, {30, 10, 10}},
  {"A bitten man begs at the gate. Behind him three zombies turn.",
   {"Give medkit", "Leave him", "Shoot"},
   {1, -2, -1}, {40, 20, 10}},
  {"Night. Your flashlight dies. A generator hums in the dark.",
   {"Follow hum", "Hide", "Rush exit"},
   {2, 0, -2}, {10, 10, 0}},
  {"A police car. Keys inside, half a tank of gas.",
   {"Drive it", "Loot it", "Walk on"},
   {2, 0, 0}, {10, 20, -10}},
  {"A horde moans beyond the half-open gate.",
   {"Sprint", "Climb", "Wait"},
   {-2, -1, 0}, {10, 0, 10}},
  {"Dawn. A far helicopter. A flare gun at your feet.",
   {"Fire flare", "Save it", "Signal"},
   {0, 0, 1}, {30, 10, 20}},
};

static const OfflineTurn kOfflineDe[] = {
  {"Regen trommelt auf das tote Einkaufszentrum. Ein einzelner Zombie "
   "treibt zwischen den Tischen.",
   {"Kuehlschrank", "Schleichen", "Klettern"},
   {0, 0, -1}, {30, 10, 10}},
  {"Ein Gebissener fleht am Tor. Hinter ihm drehen sich drei Zombies um.",
   {"Verband", "Ihn lassen", "Schiessen"},
   {1, -2, -1}, {40, 20, 10}},
  {"Nacht. Deine Lampe stirbt. Ein Generator brummt in der Dunkelheit.",
   {"Folgen", "Verstecken", "Rennen"},
   {2, 0, -2}, {10, 10, 0}},
  {"Ein Streifenwagen. Der Schluessel steckt, halber Tank.",
   {"Fahren", "Kofferraum", "Zu Fuss"},
   {2, 0, 0}, {10, 20, -10}},
  {"Eine Horde stoehnt hinter dem halboffenen Tor.",
   {"Sprint", "Klettern", "Warten"},
   {-2, -1, 0}, {10, 0, 10}},
  {"Morgengrauen. Ein ferner Helikopter. Eine Signalpistole liegt vor dir.",
   {"Signal", "Sparen", "Winken"},
   {0, 0, 1}, {30, 10, 20}},
};

// Palette: soft night tones instead of harsh white on black.
// All story text is a pale sickly green, unselected choices are dim,
// the selection is warm amber.
static constexpr uint16_t K_COL_BG     = 0x861;   // near-black green tint
static constexpr uint16_t K_COL_TEXT   = 0x964D;  // pale zombie green
static constexpr uint16_t K_COL_DIM    = 0x640B;  // dim green (unselected)
static constexpr uint16_t K_COL_SELECT = TFT_ORANGE;  // amber selection
static constexpr uint16_t K_COL_BLOOD  = 0xC943;  // muted blood red
static constexpr uint16_t K_COL_SAFE   = 0x5E4B;  // muted rescue green

// Landscape layout constants (rotation 1, 320x170).
// Story text and choices are drawn 1.5x: Font 2 (16px) into a line
// sprite, block-scaled by 3/2 -> 24px lines, 320px = 213 sprite px.
static constexpr int16_t K_SCALE_NUM   = 3;    // 1.5x = 3/2
static constexpr int16_t K_SCALE_DEN   = 2;
static constexpr int16_t K_SPR_W       = 213;  // 320 / 1.5
static constexpr int16_t K_SPR_H       = 16;   // Font 2 line height
static constexpr int16_t K_STATUS_H   = 24;   // status row height
static constexpr int16_t K_TEXT_Y     = 28;   // first big text line
static constexpr int16_t K_TEXT_LH     = 24;   // 16 * 1.5
static constexpr int16_t K_OPT_Y      = 34;   // first choice row
static constexpr int16_t K_OPT_H      = 24;
static constexpr int16_t K_FOOTER_Y   = 160;  // bottom hint row (Font 1)

void SurvivalGame::init() {
  gfx.tft().setRotation(1);  // landscape
  phase_  = PH_TITLE;
  hp_     = MAX_HP;
  score_  = 0;
  day_    = 1;
  sel_    = 0;
  page_   = 0;
  choosing_ = false;
  dirty_  = true;
  wantsMenu = false;
  requested_ = false;
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
  choosing_ = false;
  dirty_ = true;
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
    const char* sys = (lang_ == 1) ? kSystemPromptDe : kSystemPromptEn;
    char user[240];
    if (lang_ == 1) {
      snprintf(user, sizeof(user),
               "Zombie-Survival, Tag %u. HP %d/%d, Punkte %d. Der Spieler "
               "hat die letzte Situation ueberlebt. Schreibe die naechste "
               "kurze Situation (max 130 Zeichen, nur ASCII, keine "
               "Umlaute) mit 3 Wahlmoeglichkeiten (max 26 Zeichen) und "
               "HP-/Punkte-Aenderungen pro Wahl.",
               day_, hp_, MAX_HP, score_);
    } else {
      snprintf(user, sizeof(user),
               "Zombie survival, day %u. HP %d/%d, score %d. The player "
               "just survived the last situation. Write the next short "
               "situation (max 130 chars) with 3 choices (max 26 chars "
               "each) and per-choice HP/score deltas.",
               day_, hp_, MAX_HP, score_);
    }
    net.reset();
    if (net.request(sys, user)) {
      requested_ = true;
      return;
    }
    // request() refused: remember why, degrade to offline.
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    online_ = false;
  }
  offlineTurn();
}

void SurvivalGame::offlineTurn() {
  const OfflineTurn* table = (lang_ == 1) ? kOfflineDe : kOfflineEn;
  constexpr uint8_t count = sizeof(kOfflineEn) / sizeof(kOfflineEn[0]);
  const OfflineTurn& t = table[offlineIdx_ % count];
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
  choosing_ = false;
  phase_ = PH_PLAY;
  dirty_ = true;
  if (!online_) {
    strncpy(msg_, lastErr_[0] ? lastErr_ : kUi[lang_].offScenario,
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
    } else if (len >= 3 && p[0] == 'R' && p[1] >= '1' && p[1] <= '3') {
      uint8_t idx = p[1] - '1';
      // Tolerate both "R1:H..." and "R1 H...".
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

  // Fallback: if the model produced no usable text, use offline.
  if (text_[0] == 0 || optCount_ == 0) {
    if (!lastErr_[0]) strncpy(lastErr_, "bad AI reply", sizeof(lastErr_) - 1);
    online_ = false;
    offlineTurn();
    return;
  }
  page_ = 0;
  sel_ = 0;
  choosing_ = false;
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

  char b[48];
  snprintf(b, sizeof(b), "%+d HP  %+d %s", dHp, dScore, kUi[lang_].ptsWord);
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
    if (i == TXT_W && *p == ' ') {
      // Line fits exactly; the space starts the next line's first word.
      out[n][i] = 0;
      p++;
    } else if (*p && i == TXT_W && lastSpace > 0) {
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
    applyTurn(net.response());
  } else if (requested_ && ns == NetClient::State::Error) {
    requested_ = false;
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    online_ = false;
    if (phase_ == PH_LOADING) offlineTurn();
  }
  netState_ = ns;

  switch (phase_) {
    case PH_TITLE:
      // Live status refresh on the title screen (every ~500 ms)
      if (millis() - titleRefresh_ > 500) {
        titleRefresh_ = millis();
        dirty_ = true;
      }
      // B long toggles the language (English <-> German).
      if (in.longPressed[(uint8_t)Key::B]) {
        lang_ = (lang_ == 1) ? 0 : 1;
        dirty_ = true;
        break;
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
      } else if (millis() - titleRefresh_ > 250) {
        // Animate the "storyteller thinks..." dots while a request runs.
        titleRefresh_ = millis();
        dirty_ = true;
      }
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; break; }
      int lines = wrapText(text_, wrapBuf_, MAX_WRAP_LINES);
      int pages = (lines + TXT_LINES - 1) / TXT_LINES;
      if (pages < 1) pages = 1;
      if (page_ >= (uint8_t)pages) page_ = pages - 1;

      if (in.longPressed[(uint8_t)Key::B]) {
        if (!choosing_) { choosing_ = true; sel_ = 0; dirty_ = true; }
        else choose(sel_);
        break;
      }
      if (choosing_) {
        if (in.pressed[(uint8_t)Key::A] && optCount_ > 0) {
          sel_ = (sel_ + optCount_ - 1) % optCount_; dirty_ = true;
        }
        if (in.pressed[(uint8_t)Key::B] && optCount_ > 0) {
          sel_ = (sel_ + 1) % optCount_; dirty_ = true;
        }
      } else {
        if (in.pressed[(uint8_t)Key::A] && page_ > 0) { page_--; dirty_ = true; }
        if (in.pressed[(uint8_t)Key::B]) {
          if (page_ < pages - 1) { page_++; dirty_ = true; }
          else { choosing_ = true; sel_ = 0; dirty_ = true; }
        }
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

int16_t SurvivalGame::drawBigText(const char* s, int16_t x, int16_t y, uint16_t color) {
  if (!s || !s[0]) return x;
  TFT_eSPI& tft = gfx.tft();
  // Line sprite holding one Font 2 line; scaled up 3/2 on push.
  // Created on first use, kept for the whole game (~7 KB).
  static TFT_eSprite spr(&gfx.tft());
  if (!spr.created()) spr.createSprite(K_SPR_W, K_SPR_H);

  int16_t w = tft.textWidth(s, 2);         // width in Font 2 pixels
  if (w > K_SPR_W) w = K_SPR_W;
  const int16_t dstH = K_SPR_H * K_SCALE_NUM / K_SCALE_DEN;  // 24
  const int16_t dstW = w * K_SCALE_NUM / K_SCALE_DEN;
  if (x < 0) x = (SCR_W - dstW) / 2;       // negative x = center
  if (x < 0) x = 0;

  spr.fillSprite(K_COL_BG);  // same tint as fillScreen: seamless push
  spr.setTextFont(2);
  spr.setTextColor(color);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(s, 0, 0);

  static uint16_t row[SCR_W];
  for (int16_t dy = 0; dy < dstH; dy++) {
    const int16_t sy = dy * K_SCALE_DEN / K_SCALE_NUM;
    for (int16_t dx = 0; dx < dstW; dx++)
      row[dx] = spr.readPixel(dx * K_SCALE_DEN / K_SCALE_NUM, sy);
    tft.pushImage(x, y + dy, dstW, 1, row);
  }
  return x;
}

void SurvivalGame::drawStatus() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.setTextFont(2);
  tft.setTextColor(K_COL_TEXT);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("HP", 4, 4);
  for (uint8_t i = 0; i < MAX_HP; i++)
    tft.fillRect(30 + i * 13, 6, 11, 12, i < hp_ ? TFT_RED : TFT_DARKGREY);
  char b[24];
  snprintf(b, sizeof(b), ui.dayLabel, day_);
  tft.drawString(b, 150, 4);
  snprintf(b, sizeof(b), ui.ptsLabel, score_);
  tft.drawString(b, 224, 4);
  // connection indicator
  uint16_t connColor;
  const char* connLabel;
  if (online_ && net.connected())      { connColor = 0x07E0; connLabel = "AI"; }
  else if (online_)                    { connColor = TFT_YELLOW; connLabel = "..."; }
  else                                 { connColor = TFT_ORANGE; connLabel = "OFF"; }
  tft.setTextColor(connColor);
  tft.drawString(connLabel, 296, 4);
  tft.drawFastHLine(0, K_STATUS_H - 2, SCR_W, TFT_DARKGREY);
}

void SurvivalGame::drawTitle() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  tft.setTextDatum(TL_DATUM);

  drawBigText("SURVIVAL", -1, 12, K_COL_BLOOD);

  tft.setTextFont(2);
  tft.setTextColor(K_COL_DIM);
  tft.drawString(ui.subtitle, (SCR_W - tft.textWidth(ui.subtitle, 2)) / 2, 56);

  tft.setTextFont(1);
  tft.setTextColor(K_COL_TEXT);
  tft.drawString(ui.intro1, 20, 76);
  tft.drawString(ui.intro2, 20, 88);

  // Live connection status block
  tft.drawFastHLine(0, 100, SCR_W, TFT_DARKGREY);
  tft.setTextColor(TFT_CYAN);
  tft.drawString(ui.connTitle, 20, 106);

  tft.setTextColor(K_COL_TEXT);
  uint8_t y = 120;
  if (!net.configured()) {
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("No secrets.h - offline mode", 20, y);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("Copy include/secrets.h.example to", 20, y + 10);
    tft.drawString("include/secrets.h and flash again", 20, y + 20);
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
    tft.drawString(net.connected() ? "API:  ready (Mistral)" : "API:  waiting for WiFi", 20, y + 10);
  }

  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(ui.start, 20, 140);
  tft.setTextFont(1);
  tft.setTextColor(TFT_ORANGE);
  tft.drawString(ui.langHint, 20, 146);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(ui.hintMenu, 250, 146);
}

void SurvivalGame::drawLoading() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  drawStatus();
  tft.setTextDatum(TL_DATUM);

  char day[12];
  snprintf(day, sizeof(day), ui.dayLabel, day_);
  drawBigText(day, -1, 44, K_COL_TEXT);

  tft.setTextFont(2);
  tft.setTextColor(K_COL_TEXT);
  uint8_t dots = (millis() / 400) % 4;
  String s;
  if (online_ && net.connected()) {
    s = ui.thinks;
  } else if (online_) {
    s = ui.waitWifi;
  } else {
    s = ui.offline;
  }
  for (uint8_t i = 0; i < dots; i++) s += '.';
  tft.drawString(s, (SCR_W - tft.textWidth(s.c_str(), 2)) / 2, 104);

  if (lastErr_[0]) {
    tft.setTextFont(1);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString(String(ui.lastErrLabel) + lastErr_, 8, 126);
  }

  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(ui.hintMenu, 8, K_FOOTER_Y);
}

void SurvivalGame::drawPlay() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  drawStatus();
  tft.setTextDatum(TL_DATUM);

  if (!choosing_) {
    // Situation text, 2x big (32px lines), four lines fill the screen.
    int lines = wrapText(text_, wrapBuf_, MAX_WRAP_LINES);
    int pages = (lines + TXT_LINES - 1) / TXT_LINES;
    if (pages < 1) pages = 1;
    if (page_ >= (uint8_t)pages) page_ = pages - 1;
    int start = page_ * TXT_LINES;

    for (uint8_t i = 0; i < TXT_LINES; i++) {
      int li = start + i;
      if (li < lines)
        drawBigText(wrapBuf_[li], -1, K_TEXT_Y + i * K_TEXT_LH, K_COL_TEXT);
    }

    tft.setTextFont(1);
    // Footer: outcome flash or controls hint, plus page indicator
    if (msg_[0] && millis() < msgUntil_) {
      tft.setTextColor(TFT_YELLOW);
      tft.drawString(msg_, 4, K_FOOTER_Y);
    } else {
      tft.setTextColor(K_COL_DIM);
      tft.drawString(page_ < pages - 1 ? ui.hintPage : ui.hintChoose,
                     4, K_FOOTER_Y);
    }
    char pg[10];
    snprintf(pg, sizeof(pg), "%d/%d", page_ + 1, pages);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(pg, SCR_W - 26, K_FOOTER_Y);
  } else {
    // Choice screen: one option per 24px row, 1.5x big, centered.
    for (uint8_t i = 0; i < optCount_; i++) {
      bool sel = (i == sel_);
      int16_t y = K_OPT_Y + i * (K_OPT_H + 8);
      int16_t tx = drawBigText(opts_[i], -1, y, sel ? K_COL_SELECT : K_COL_DIM);
      if (sel) {
        tft.setTextFont(1);
        tft.setTextColor(K_COL_SELECT);
        tft.drawString(">", tx - 18, y + 8);
      }
    }
    tft.setTextFont(1);
    tft.setTextColor(K_COL_DIM);
    tft.drawString(ui.hintSelect, 4, K_FOOTER_Y);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(ui.hintMenu, SCR_W - 76, K_FOOTER_Y);
  }
}

void SurvivalGame::drawEnd(bool won) {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  drawStatus();
  tft.setTextDatum(TL_DATUM);

  const char* head = won ? ui.rescued : ui.died;
  drawBigText(head, -1, 40, won ? K_COL_SAFE : K_COL_BLOOD);

  tft.setTextFont(2);
  tft.setTextColor(K_COL_TEXT);
  char b[40];
  int16_t x;
  snprintf(b, sizeof(b), ui.survived, day_ - 1);
  x = (SCR_W - tft.textWidth(b, 2)) / 2;
  if (x < 0) x = 0;
  tft.drawString(b, x, 96);
  snprintf(b, sizeof(b), ui.scoreLabel, score_);
  x = (SCR_W - tft.textWidth(b, 2)) / 2;
  if (x < 0) x = 0;
  tft.drawString(b, x, 116);

  tft.setTextColor(TFT_YELLOW);
  x = (SCR_W - tft.textWidth(ui.again, 2)) / 2;
  if (x < 0) x = 0;
  tft.drawString(ui.again, x, 136);
  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(ui.hintMenu, 90, K_FOOTER_Y);
}
