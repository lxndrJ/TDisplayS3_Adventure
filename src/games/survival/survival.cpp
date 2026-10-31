#include "survival.h"
#include "core/net.h"

// System prompt: strict output format the device can parse line by line.
//   S:<situation text, max ~220 chars, no line breaks>
//   1:<choice label, max 22 chars = max 3 short words>
//   2:<choice label>
//   3:<choice label>
//   R1:H<hp delta> P<score delta>
//   R2:H<hp delta> P<score delta>
//   R3:H<hp delta> P<score delta>
// The user prompt carries the story so far, so the model must continue
// the SAME arc (same place, same stakes) instead of inventing a new one.
// Font 2 on the device is ASCII-only, so German must avoid umlauts.
static const char* kSystemPromptEn =
  "You are the storyteller of a grim zombie survival game on a tiny "
  "handheld screen. The user message contains the story so far. You "
  "MUST continue that exact story: same location, same characters, "
  "open threads. Never restart or jump to a new place. Reply in "
  "EXACTLY this format, nothing else:\n"
  "S:<situation, max 220 chars, 2-4 sentences, no newlines>\n"
  "1:<choice, max 22 chars, at most 3 short words>\n"
  "2:<choice, max 22 chars, at most 3 short words>\n"
  "3:<choice, max 22 chars, at most 3 short words>\n"
  "R1:H<signed hp delta> P<signed score delta>\n"
  "R2:H<signed hp delta> P<signed score delta>\n"
  "R3:H<signed hp delta> P<signed score delta>\n"
  "HP is 0..10, score is survival points. Choices are shown very big "
  "on a 320px screen: every choice at most 3 words and under 22 "
  "characters. Grim, tense, cinematic. Choices must have real "
  "consequences (hp/score deltas).";

static const char* kSystemPromptDe =
  "Du bist der Erzaehler eines grimmigen Zombie-Survival-Spiels auf "
  "einem winzigen Handheld-Bildschirm. Die User-Nachricht enthaelt die "
  "bisherige Geschichte. Du MUSST genau diese Geschichte fortsetzen: "
  "gleicher Ort, gleiche Figuren, offene Handlungsstraenge. Niemals "
  "neu anfangen oder an einen anderen Ort springen. Antworte GENAU in "
  "diesem Format, nichts anderes:\n"
  "S:<Situation, max 220 Zeichen, 2-4 Saetze, keine Zeilenumbrueche>\n"
  "1:<Wahl, max 22 Zeichen, hoechstens 3 kurze Woerter>\n"
  "2:<Wahl, max 22 Zeichen, hoechstens 3 kurze Woerter>\n"
  "3:<Wahl, max 22 Zeichen, hoechstens 3 kurze Woerter>\n"
  "R1:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "R2:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "R3:H<signierte HP-Aenderung> P<signierte Punkte-Aenderung>\n"
  "HP ist 0..10, Punkte sind Ueberlebenspunkte. Schreibe DEUTSCH, "
  "aber NUR ASCII: keine Umlaute, schreibe ae/oe/ue/ss. Die "
  "Wahlmoeglichkeiten erscheinen sehr gross auf einem 320px-Schirm: "
  "jede Wahl hoechstens 3 Woerter und unter 22 Zeichen. Grimmig, "
  "spannend, filmreif. Wahlen brauchen echte Konsequenzen "
  "(HP-/Punkte-Aenderungen).";

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

// Built-in offline story (used when WiFi/API is unavailable). The turns
// form a branching chain (next[choice]) instead of a loose rotation, so
// offline play tells one connected story: mall -> gate -> night ->
// police car -> gate horde -> rescue, with branches rejoining.
struct OfflineTurn {
  const char* text;
  const char* opt[3];
  int8_t  hp[3];
  int16_t score[3];
  uint8_t next[3];  // index of the follow-up turn per choice (255 = end)
};

static const OfflineTurn kOfflineEn[] = {
  // 0: opening - the dead mall
  {"Rain hammers the dead mall. You wake between the tables. A lone "
   "zombie drifts toward you.",
   {"Loot fridge", "Sneak out", "Climb up"},
   {0, 0, -1}, {30, 10, 10}, {1, 1, 1}},
  // 1: gate with the bitten man
  {"At the gate a bitten man begs for help. Behind him three zombies "
   "turn.",
   {"Give medkit", "Leave him", "Shoot them"},
   {1, -2, -1}, {40, 20, 10}, {2, 4, 4}},
  // 2: night generator (branch from helping)
  {"Night falls. Your flashlight dies. Somewhere a generator hums.",
   {"Follow hum", "Hide", "Rush exit"},
   {2, 0, -2}, {10, 10, 0}, {3, 3, 4}},
  // 3: the police car
  {"A police car waits. Keys inside, half a tank of gas.",
   {"Drive away", "Loot trunk", "Walk on"},
   {2, 0, 0}, {10, 20, -10}, {4, 4, 4}},
  // 4: horde at the gate
  {"A horde moans beyond the half-open gate. Something is coming.",
   {"Sprint out", "Climb fence", "Wait"},
   {-2, -1, 0}, {10, 0, 10}, {5, 5, 4}},
  // 5: dawn rescue
  {"Dawn. A far helicopter. A flare gun lies at your feet.",
   {"Fire flare", "Save it", "Wave arms"},
   {0, 0, 1}, {30, 10, 20}, {5, 5, 5}},
};

static const OfflineTurn kOfflineDe[] = {
  // 0: Eroeffnung - das tote Einkaufszentrum
  {"Regen trommelt auf das tote Einkaufszentrum. Du erwachst zwischen "
   "den Tischen. Ein einzelner Zombie treibt auf dich zu.",
   {"Kuehlschrank", "Schleichen", "Hochklettern"},
   {0, 0, -1}, {30, 10, 10}, {1, 1, 1}},
  // 1: Tor mit dem Gebissenen
  {"Am Tor fleht ein Gebissener um Hilfe. Hinter ihm drehen sich drei "
   "Zombies um.",
   {"Verband geben", "Ihn lassen", "Schiessen"},
   {1, -2, -1}, {40, 20, 10}, {2, 4, 4}},
  // 2: Nacht, Generator (Zweig fürs Helfen)
  {"Nacht bricht herein. Deine Lampe stirbt. Irgendwo brummt ein "
   "Generator.",
   {"Dem Brummen", "Verstecken", "Rausrennen"},
   {2, 0, -2}, {10, 10, 0}, {3, 3, 4}},
  // 3: der Streifenwagen
  {"Ein Streifenwagen steht bereit. Schluessel steckt, halber Tank.",
   {"Wegfahren", "Kofferraum", "Zu Fuss"},
   {2, 0, 0}, {10, 20, -10}, {4, 4, 4}},
  // 4: Horde am Tor
  {"Eine Horde stoesst hinter dem halboffenen Tor. Etwas naehert sich.",
   {"Rausrennen", "Zaun hoch", "Warten"},
   {-2, -1, 0}, {10, 0, 10}, {5, 5, 4}},
  // 5: Rettung im Morgengrauen
  {"Morgengrauen. Ein ferner Helikopter. Eine Signalpistole liegt vor "
   "dir.",
   {"Signal geben", "Sparen", "Winken"},
   {0, 0, 1}, {30, 10, 20}, {5, 5, 5}},
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

// Landscape layout constants (rotation 1, 320x170). All text is drawn
// with the TFT's native font renderer (integer setTextSize) - no manual
// pixel scaling, so glyphs stay crisp. Story and choices use Font 4
// (26px, the largest GLCD font with a full alphabet). Status row and
// footer use Font 2 / Font 1.
static constexpr int16_t K_STATUS_H   = 24;   // status row height
static constexpr int16_t K_TEXT_Y     = 30;   // first story line
static constexpr int16_t K_TEXT_LH    = 28;   // Font 4 (26px) + 2px gap
static constexpr int16_t K_OPT_Y      = 36;   // first choice row
static constexpr int16_t K_OPT_H      = 40;   // Font 4 (26px) + gap
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
  retries_ = 0;
  storyOn_ = false;
  turnOffline_ = false;
  wifiAttempts_ = 0;
  histCount_ = 0;
  for (auto& h : history_) h[0] = 0;
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
  offlineIdx_ = 0;
  retries_  = 0;
  storyOn_  = false;
  turnOffline_ = false;
  wifiAttempts_ = 0;
  histCount_ = 0;
  for (auto& h : history_) h[0] = 0;
  strncpy(msg_, "Starting...", sizeof(msg_) - 1);
  msgUntil_ = millis() + 2000;
  startRequest();
}

void SurvivalGame::startRequest() {
  requested_ = false;
  wifiWaiting_ = false;
  turnOffline_ = !online_;
  // Wait for WiFi (up to ~30 s, retried) before falling back to offline,
  // so a slow first association doesn't silently disable the AI.
  if (online_ && !net.connected()) {
    net.begin();  // (re)starts association, no-op while connected
    NetClient::State st = net.poll();
    if (st == NetClient::State::WifiConnecting || st == NetClient::State::WifiFailed) {
      if (st == NetClient::State::WifiFailed && wifiAttempts_ < 2) {
        wifiAttempts_++;  // one re-association attempt, then keep waiting
      }
      phase_ = PH_LOADING;
      dirty_ = true;
      strncpy(msg_, "Connecting to WiFi...", sizeof(msg_) - 1);
      msgUntil_ = millis() + 60000;  // effectively permanent
      wifiWaiting_ = true;
      wifiWaitStart_ = millis();
      return;
    }
    if (st == NetClient::State::WifiFailed) {
      strncpy(lastErr_, "wifi failed", sizeof(lastErr_) - 1);
      // keep online_ true: the loading screen retries the association
    }
  }
  if (online_ && net.connected()) {
    const char* sys = (lang_ == 1) ? kSystemPromptDe : kSystemPromptEn;
    // The prompt carries the story so far so the model continues the
    // same arc. history_ entries: "Day 3: <situation> -> chose: <option>"
    char user[640];
    size_t used = 0;
    auto append = [&](const char* fmt, ...) {
      if (used >= sizeof(user) - 1) return;
      va_list ap;
      va_start(ap, fmt);
      int n = vsnprintf(user + used, sizeof(user) - used, fmt, ap);
      va_end(ap);
      if (n > 0) used += (size_t)n;
      if (used > sizeof(user) - 1) used = sizeof(user) - 1;
    };
    if (lang_ == 1) {
      append("Zombie-Survival, Tag %u. HP %d/%d, Punkte %d. "
             "Bisherige Geschichte (fahre GENAU hier fort):\n",
             day_, hp_, MAX_HP, score_);
    } else {
      append("Zombie survival, day %u. HP %d/%d, score %d. "
             "Story so far (continue EXACTLY from here):\n",
             day_, hp_, MAX_HP, score_);
    }
    if (histCount_ == 0) {
      if (lang_ == 1)
        append("(Noch keine Geschichte. Beginne mit einer starken "
               "Eroeffnung: der Spieler wacht in einer verlassenen Stadt "
               "auf.)\n");
      else
        append("(No story yet. Start with a strong opening: the player "
               "wakes up in an abandoned city.)\n");
    } else {
      for (uint8_t i = 0; i < histCount_; i++)
        append("%s\n", history_[i]);
    }
    if (lang_ == 1) {
      append("Schreibe die naechste Situation (max 220 Zeichen, nur "
             "ASCII, keine Umlaute) mit 3 Wahlmoeglichkeiten (max 22 "
             "Zeichen, hoechstens 3 Woerter) und HP-/Punkte-Aenderungen "
             "pro Wahl.");
    } else {
      append("Write the next situation (max 220 chars) with 3 choices "
             "(max 22 chars, at most 3 words each) and per-choice "
             "HP/score deltas.");
    }
    net.reset();
    if (net.request(sys, user)) {
      requested_ = true;
      return;
    }
    // request() refused: remember why, this turn goes offline. The next
    // turn tries the LLM again - a refused request is not permanent.
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    turnOffline_ = true;
  }
  if (turnOffline_) offlineTurn();
}

void SurvivalGame::offlineTurn() {
  offlineTurnAt(offlineIdx_);
}

// Shows offline turn idx and remembers where each choice leads, so the
// offline story branches instead of rotating through unrelated scenes.
void SurvivalGame::offlineTurnAt(uint8_t idx) {
  const OfflineTurn* table = (lang_ == 1) ? kOfflineDe : kOfflineEn;
  constexpr uint8_t count = sizeof(kOfflineEn) / sizeof(kOfflineEn[0]);
  if (idx >= count) idx = 0;
  const OfflineTurn& t = table[idx];
  offlineIdx_ = idx;
  strncpy(text_, t.text, MAX_TEXT - 1);
  text_[MAX_TEXT - 1] = 0;
  optCount_ = 3;
  for (uint8_t i = 0; i < 3; i++) {
    strncpy(opts_[i], t.opt[i], OPT_LEN);
    opts_[i][OPT_LEN] = 0;
    offHp_[i] = t.hp[i];
    offScore_[i] = t.score[i];
    offNext_[i] = t.next[i];
  }
  page_ = 0;
  sel_ = 0;
  choosing_ = false;
  phase_ = PH_PLAY;
  dirty_ = true;
  if (turnOffline_) {
    strncpy(msg_, lastErr_[0] ? lastErr_ : kUi[lang_].offScenario,
            sizeof(msg_) - 1);
    msgUntil_ = millis() + 1500;
  }
}

// Lenient line parser: skips markdown fences and preamble, accepts
// "S:", "S.", "1:", "1)", "1.", "R1:H.. P.." in any order. Returns the
// number of parsed fields so the caller can decide on a retry.
static int parseTurn(const char* raw, char* text, size_t textCap,
                     char opts[][SurvivalGame::OPT_LEN + 1], uint8_t& optCount,
                     int8_t* hp, int16_t* score) {
  if (!raw) return 0;
  int found = 0;
  text[0] = 0;
  optCount = 0;
  const char* p = raw;
  while (*p) {
    // skip markdown code-fence lines (``` or ```lang) and blank lines;
    // the fenced CONTENT stays and is parsed normally
    while (*p == '\n' || *p == '\r' || *p == ' ') p++;
    if (p[0] == '`' && p[1] == '`' && p[2] == '`') {
      while (*p && *p != '\n') p++;  // drop the fence line only
      continue;
    }
    const char* eol = p;
    while (*eol && *eol != '\n') eol++;
    size_t len = eol - p;
    if (len > 0 && p[len - 1] == '\r') len--;
    // strip leading whitespace of the line
    while (len > 0 && (*p == ' ' || *p == '\t')) { p++; len--; }

    char tag = 0;
    uint8_t idx = 0;
    const char* body = nullptr;
    size_t bodyLen = 0;
    if (len >= 2 && p[0] == 'S' && (p[1] == ':' || p[1] == '.' || p[1] == ' ')) {
      tag = 'S'; body = p + 2; bodyLen = len - 2;
    } else if (len >= 3 && p[0] >= '1' && p[0] <= '3' &&
               (p[1] == ':' || p[1] == ')' || p[1] == '.' || p[1] == ' ')) {
      tag = 'C'; idx = p[0] - '1'; body = p + 2; bodyLen = len - 2;
    } else if (len >= 3 && (p[0] == 'R' || p[0] == 'r') && p[1] >= '1' && p[1] <= '3' &&
               (p[2] == ':' || p[2] == ' ' || p[2] == ')')) {
      tag = 'R'; idx = p[1] - '1'; body = p + 3; bodyLen = len - 3;
    }
    if (tag == 'S') {
      while (bodyLen > 0 && (*body == ' ' || *body == '\t')) { body++; bodyLen--; }
      size_t n = bodyLen;
      if (n > textCap - 1) n = textCap - 1;
      memcpy(text, body, n);
      text[n] = 0;
      found++;
    } else if (tag == 'C') {
      while (bodyLen > 0 && (*body == ' ' || *body == '\t')) { body++; bodyLen--; }
      size_t n = bodyLen;
      if (n > SurvivalGame::OPT_LEN) n = SurvivalGame::OPT_LEN;
      memcpy(opts[idx], body, n);
      opts[idx][n] = 0;
      if (idx + 1 > optCount) optCount = idx + 1;
      found++;
    } else if (tag == 'R') {
      const char* q = body;
      while (*q == ' ' || *q == ':') q++;
      if (*q == 'H' || *q == 'h') {
        hp[idx] = (int8_t)atoi(q + 1);
        const char* sp = strchr(q, 'P');
        if (!sp) sp = strchr(q, 'p');
        if (sp) score[idx] = (int16_t)atoi(sp + 1);
        found++;
      }
    }
    p = (*eol == '\n') ? eol + 1 : eol;
  }
  // sanitize control chars
  for (size_t i = 0; text[i]; i++)
    if ((uint8_t)text[i] < 32) text[i] = ' ';
  for (uint8_t i = 0; i < optCount; i++)
    for (size_t j = 0; opts[i][j]; j++)
      if ((uint8_t)opts[i][j] < 32) opts[i][j] = ' ';
  return found;
}

void SurvivalGame::applyTurn(const char* raw) {
  if (!raw) return;
  for (uint8_t i = 0; i < 3; i++) { opts_[i][0] = 0; llHp_[i] = 0; llScore_[i] = 0; }
  uint8_t optCount = 0;
  int fields = parseTurn(raw, text_, MAX_TEXT, opts_, optCount,
                         llHp_, llScore_);
  optCount_ = optCount;

  // A usable turn needs situation text and at least one choice.
  if (text_[0] == 0 || optCount_ == 0) {
    retries_++;
    if (retries_ <= 2 && online_ && net.connected()) {
      snprintf(lastErr_, sizeof(lastErr_), "bad AI reply (%d fld), retry",
               fields);
      phase_ = PH_LOADING;
      dirty_ = true;
      startRequest();
      return;
    }
    snprintf(lastErr_, sizeof(lastErr_), "bad AI reply (%d fld)", fields);
    // Only THIS turn goes offline; the LLM is retried next turn.
    turnOffline_ = true;
    offlineTurn();
    return;
  }
  retries_ = 0;
  storyOn_ = true;
  turnOffline_ = false;
  page_ = 0;
  sel_ = 0;
  choosing_ = false;
  phase_ = PH_PLAY;
  dirty_ = true;
}

void SurvivalGame::choose(uint8_t idx) {
  if (idx >= optCount_) return;
  int8_t dHp; int16_t dScore;
  if (!turnOffline_) {
    dHp = llHp_[idx]; dScore = llScore_[idx];
  } else {
    dHp = offHp_[idx]; dScore = offScore_[idx];
    // Follow the branch this choice leads to next turn.
    uint8_t nx = offNext_[idx];
    if (nx != 0xFF) offlineIdx_ = nx;
  }
  // Remember the beat for the LLM story memory (ring buffer).
  if (storyOn_ && histCount_ < HISTORY_N) {
    snprintf(history_[histCount_], sizeof(history_[0]),
             "Day %u: %.60s -> chose: %.22s", day_, text_, opts_[idx]);
    histCount_++;
  } else if (storyOn_) {
    memmove(history_[0], history_[1], (HISTORY_N - 1) * sizeof(history_[0]));
    snprintf(history_[HISTORY_N - 1], sizeof(history_[0]),
             "Day %u: %.60s -> chose: %.22s", day_, text_, opts_[idx]);
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

// Pixel-accurate word wrap: a line is broken at the last space that
// still fits maxW at the given font/size (measured with textWidth).
int SurvivalGame::wrapText(const char* t, uint8_t font, uint8_t size,
                           int16_t maxW, char out[][LINE_LEN + 1], int maxL) const {
  TFT_eSPI& tft = gfx.tft();
  tft.setTextFont(font);
  tft.setTextSize(size);
  int n = 0;
  const char* p = t;
  while (*p && n < maxL) {
    const char* start = p;
    int i = 0, lastSpace = -1, w = 0;
    while (p[i] && i < LINE_LEN) {
      char tmp[2] = {p[i], 0};
      int cw = tft.textWidth(tmp, font);
      if (w + cw > maxW) break;
      w += cw;
      if (p[i] == ' ') lastSpace = i;
      i++;
    }
    int cut;
    if (p[i] && lastSpace > 0) {
      cut = lastSpace;              // break at the last fitting space
      p = start + lastSpace + 1;
    } else {
      cut = i;                      // hard break (long word / full line)
      p = start + i;
    }
    if (cut > LINE_LEN) cut = LINE_LEN;
    memcpy(out[n], start, cut);
    out[n][cut] = 0;
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
    // Only this turn falls back to offline; the LLM is retried next turn.
    if (phase_ == PH_LOADING) {
      turnOffline_ = true;
      offlineTurn();
    }
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
          wifiWaiting_ = false;
          wifiAttempts_ = 0;
          startRequest();
        } else if (millis() - wifiWaitStart_ > 30000) {
          // 30 s without WiFi: play this turn offline, but keep online_
          // true so the next turn retries the LLM.
          wifiWaiting_ = false;
          wifiAttempts_ = 0;
          strncpy(lastErr_, "wifi timeout", sizeof(lastErr_) - 1);
          turnOffline_ = true;
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
      int lines = wrapText(text_, 4, 1, SCR_W - 8, wrapBuf_, MAX_WRAP_LINES);
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

int16_t SurvivalGame::drawText(const char* s, int16_t x, int16_t y,
                               uint8_t font, uint8_t size, uint16_t color) {
  if (!s || !s[0]) return x;
  TFT_eSPI& tft = gfx.tft();
  tft.setTextFont(font);
  tft.setTextSize(size);
  tft.setTextColor(color, K_COL_BG);  // opaque bg = clean glyphs on the field
  tft.setTextDatum(TL_DATUM);
  int16_t w = tft.textWidth(s, font);
  if (x < 0) x = (SCR_W - w) / 2;          // negative x = center
  if (x < 0) x = 0;
  tft.drawString(s, x, y, font);
  tft.setTextSize(1);                       // restore default
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
  if (!turnOffline_ && net.connected())  { connColor = 0x07E0; connLabel = "AI"; }
  else if (!turnOffline_)                 { connColor = TFT_YELLOW; connLabel = "..."; }
  else                                    { connColor = TFT_ORANGE; connLabel = "OFF"; }
  tft.setTextColor(connColor);
  tft.drawString(connLabel, 296, 4);
  tft.drawFastHLine(0, K_STATUS_H - 2, SCR_W, TFT_DARKGREY);
}

void SurvivalGame::drawTitle() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  tft.setTextDatum(TL_DATUM);

  drawText("SURVIVAL", -1, 12, 4, 2, K_COL_BLOOD);

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
  drawText(day, -1, 44, 4, 1, K_COL_TEXT);

  tft.setTextFont(2);
  tft.setTextColor(K_COL_TEXT);
  uint8_t dots = (millis() / 400) % 4;
  String s;
  if (!turnOffline_ && net.connected()) {
    s = ui.thinks;
  } else if (!turnOffline_) {
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
    // Story text: native Font 4 at size 1 (26px lines, crisp).
    int lines = wrapText(text_, 4, 1, SCR_W - 8, wrapBuf_, MAX_WRAP_LINES);
    int pages = (lines + TXT_LINES - 1) / TXT_LINES;
    if (pages < 1) pages = 1;
    if (page_ >= (uint8_t)pages) page_ = pages - 1;
    int start = page_ * TXT_LINES;

    for (uint8_t i = 0; i < TXT_LINES; i++) {
      int li = start + i;
      if (li < lines)
        drawText(wrapBuf_[li], -1, K_TEXT_Y + i * K_TEXT_LH, 4, 1, K_COL_TEXT);
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
    // Choice screen: one option per row, native Font 4 (26px) centered;
    // falls back to Font 2 if the label would overflow the width.
    for (uint8_t i = 0; i < optCount_; i++) {
      bool sel = (i == sel_);
      int16_t y = K_OPT_Y + i * K_OPT_H;
      tft.setTextFont(4);
      bool big = (tft.textWidth(opts_[i], 4) <= SCR_W - 40);
      int16_t tx = drawText(opts_[i], -1, y, big ? 4 : 2, 1,
                            sel ? K_COL_SELECT : K_COL_DIM);
      if (sel) {
        tft.setTextFont(2);
        tft.setTextColor(K_COL_SELECT);
        tft.drawString(">", tx - 16, y + 6);
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
  drawText(head, -1, 40, 4, 2, won ? K_COL_SAFE : K_COL_BLOOD);

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
