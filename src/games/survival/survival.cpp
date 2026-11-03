#include "survival.h"
#include "prompts.h"
#include "Roboto24.h"
#include "Roboto28.h"
#include "core/net.h"

// Truncate s to maxBytes without splitting a UTF-8 sequence: if the cut
// lands inside a multi-byte char (continuation byte 0x80-0xBF at the
// cut position), back up to the sequence start.
static size_t utf8SafeLen(const char* s, size_t maxBytes) {
  size_t n = 0;
  while (s[n] && n < maxBytes) n++;
  // If we stopped in the middle of a multi-byte char (s[n] is a
  // continuation byte), back up to the lead byte and drop the whole char.
  if (s[n] && n > 0 && ((uint8_t)s[n] & 0xC0) == 0x80) {
    size_t lead = n;
    while (lead > 0 && ((uint8_t)s[lead] & 0xC0) == 0x80) lead--;
    n = lead;
  }
  return n;
}

// UI strings, indexed by lang_ (0 = EN, 1 = DE).
struct UiText {
  const char* subtitle;
  const char* intro1, *intro2;
  const char* start;
  const char* langHint;
  const char* thinks, *waitWifi;
  const char* hintPage, *hintChoose;
  const char* hintSelect, *hintMenu;
  const char* died, *rescued, *survived;
  const char* scoreLabel, *again;
  const char* dayLabel, *ptsLabel, *ptsWord;
  const char* lastErrLabel, *connTitle;
};

static const UiText kUi[2] = {
  {   // English
    "Zombie Apocalypse",
    "The dead walk. You have 10 HP.",
    "Survive 30 days. Find a way out.",
    "Press A or B to start",
    "B long: Deutsch",
    "The storyteller thinks", "Waiting for WiFi",
    "B: next page", "B: choose",
    "A/B: select   B long: OK", "A long: menu",
    "YOU DIED", "RESCUED!", "Survived %u days",
    "Score: %d", "B long: again",
    "Day %u", "Pts %d", "pts",
    "Last error: ", "CONNECTION",
  },
  {   // German (UTF-8 umlauts)
    "Zombie-Apokalypse",
    "Die Toten laufen. Du hast 10 HP.",
    "Überlebe 30 Tage. Finde einen Ausweg.",
    "A oder B: Start",
    "B lang: English",
    "Der Erzähler denkt", "Warte auf WLAN",
    "B: weiter", "B: Auswahl",
    "A/B: wählen   B lang: OK", "A lang: Menü",
    "GESTORBEN", "GERETTET!", "%u Tage überlebt",
    "Punkte: %d", "B lang: Nochmal",
    "Tag %u", "Pkt %d", "Pkt",
    "Letzter Fehler: ", "VERBINDUNG",
  },
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

// Landscape layout constants (rotation 1, 320x170). Story text and
// choices use the compiled-in Roboto .vlw smooth fonts.
static constexpr int16_t K_STATUS_H   = 24;   // status row height
static constexpr int16_t K_TEXT_Y     = 30;   // first story line
static constexpr int16_t K_TEXT_LH    = 28;   // Roboto24 yAdvance ~29px
static constexpr int16_t K_OPT_Y      = 36;   // first choice row
static constexpr int16_t K_OPT_H      = 40;   // Roboto28 yAdvance ~33px + gap
static constexpr int16_t K_FOOTER_Y   = 160;  // bottom hint row (Font 1)

// Lenient line parser (see prompts.h for the expected format). Returns
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
      size_t n = utf8SafeLen(body, bodyLen);
      if (n > textCap - 1) n = textCap - 1;
      memcpy(text, body, n);
      text[n] = 0;
      found++;
    } else if (tag == 'C') {
      while (bodyLen > 0 && (*body == ' ' || *body == '\t')) { body++; bodyLen--; }
      size_t n = utf8SafeLen(body, bodyLen);
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
    snprintf(lastErr_, sizeof(lastErr_), "bad AI reply (%d fld), retry", fields);
    // No offline fallback: wait a moment and ask the LLM again.
    retryAt_ = millis() + 2000;
    phase_ = PH_LOADING;
    dirty_ = true;
    return;
  }
  retries_ = 0;
  storyOn_ = true;
  // Normalize ae/oe/ue/ss to real umlauts in-place (the Roboto .vlw font
  // renders UTF-8; models sometimes answer with ae/oe/ue despite the
  // prompt). In-place is safe: replacements are never longer than the
  // ASCII source.
  auto umlautify = [](char* str) {
    for (char* p = str; *p; ) {
      if      (p[0] == 'a' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\xA4'; p += 2; }
      else if (p[0] == 'o' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\xB6'; p += 2; }
      else if (p[0] == 'u' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\xBC'; p += 2; }
      else if (p[0] == 'A' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\x84'; p += 2; }
      else if (p[0] == 'O' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\x96'; p += 2; }
      else if (p[0] == 'U' && p[1] == 'e') { *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\x9C'; p += 2; }
      else p++;
    }
  };
  umlautify(text_);
  for (uint8_t i = 0; i < optCount_; i++) umlautify(opts_[i]);
  // ss -> ß at word end after a vowel (e.g. "stösst" written as "stoesst")
  for (char* p = text_; p[0] && p[1]; ) {
    if (p[0] == 's' && p[1] == 's' &&
        (p[2] == 0 || p[2] == ' ' || p[2] == '.' || p[2] == '!' || p[2] == ',' || p[2] == ':' || p[2] == ')') &&
        p > text_ && (p[-1] == 'a' || p[-1] == 'e' || p[-1] == 'i' || p[-1] == 'o' || p[-1] == 'u' ||
                      p[-1] == '\xC3')) {  // vowel or umlaut byte before
      *p = '\xC3'; memmove(p + 1, p + 2, strlen(p + 2) + 1); p[1] = '\x9F'; p += 2;
    } else p++;
  }
  page_ = 0;
  sel_ = 0;
  choosing_ = false;
  phase_ = PH_PLAY;
  dirty_ = true;
}

void SurvivalGame::choose(uint8_t idx) {
  if (idx >= optCount_) return;
  int8_t dHp = llHp_[idx];
  int16_t dScore = llScore_[idx];
  // Remember the beat for the LLM story memory (ring buffer).
  if (storyOn_ && histCount_ < HISTORY_N) {
    snprintf(history_[histCount_], sizeof(history_[0]),
             "Day %u: %.60s -> wählte: %.22s", day_, text_, opts_[idx]);
    size_t hl = strlen(history_[histCount_]);
    while (hl > 0 && ((uint8_t)history_[histCount_][hl] & 0xC0) == 0x80)
      history_[histCount_][--hl] = 0;  // drop a split UTF-8 tail
    histCount_++;
  } else if (storyOn_) {
    memmove(history_[0], history_[1], (HISTORY_N - 1) * sizeof(history_[0]));
    snprintf(history_[HISTORY_N - 1], sizeof(history_[0]),
             "Day %u: %.60s -> wählte: %.22s", day_, text_, opts_[idx]);
    size_t hl = strlen(history_[HISTORY_N - 1]);
    while (hl > 0 && ((uint8_t)history_[HISTORY_N - 1][hl] & 0xC0) == 0x80)
      history_[HISTORY_N - 1][--hl] = 0;
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
  retries_  = 0;
  retryAt_  = 0;
  storyOn_  = false;
  wifiAttempts_ = 0;
  lastWifiRetry_ = 0;
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
  // Keep WiFi associated between games (re-connecting costs seconds and
  // is what made the AI look "offline" after leaving the game).
  requested_ = false;
  retryAt_ = 0;
  wifiWaiting_ = false;
  if (gfx.tft().fontLoaded) gfx.tft().unloadFont();
}

void SurvivalGame::startGame() {
  phase_ = PH_LOADING;
  hp_     = MAX_HP;
  score_  = 0;
  day_    = 1;
  sel_    = 0;
  page_   = 0;
  choosing_ = false;
  dirty_  = true;
  retries_  = 0;
  retryAt_  = 0;
  storyOn_  = false;
  histCount_ = 0;
  for (auto& h : history_) h[0] = 0;
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
  // Wait for WiFi (retried) before sending the prompt; on any error the
  // game simply retries later - there is no offline fallback anymore.
  if (online_ && !net.connected()) {
    net.begin();  // (re)starts association
    NetClient::State st = net.poll();
    if (st == NetClient::State::WifiConnecting || st == NetClient::State::WifiFailed) {
      phase_ = PH_LOADING;
      dirty_ = true;
      strncpy(msg_, "Connecting to WiFi...", sizeof(msg_) - 1);
      msgUntil_ = millis() + 60000;
      wifiWaiting_ = true;
      wifiWaitStart_ = millis();
      lastWifiRetry_ = millis();
      return;
    }
    if (st == NetClient::State::WifiFailed) {
      strncpy(lastErr_, "wifi failed, retrying", sizeof(lastErr_) - 1);
    }
    if (online_ && !net.connected()) {
      retryAt_ = millis() + 1500;  // try again shortly
      phase_ = PH_LOADING;
      dirty_ = true;
      return;
    }
  }
  if (online_ && net.connected()) {
    const char* sys = (lang_ == 1) ? kSystemPromptDe : kSystemPromptEn;
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
    const PromptUser& pu = (lang_ == 1) ? kPromptUserDe : kPromptUserEn;
    append(pu.header, day_, hp_, MAX_HP, score_);
    if (histCount_ == 0) {
      append("%s", pu.noHistory);
    } else {
      for (uint8_t i = 0; i < histCount_; i++)
        append("%s\n", history_[i]);
    }
    append("%s", pu.tail);
    net.reset();
    if (net.request(sys, user)) {
      requested_ = true;
      return;
    }
    // request() refused: retry shortly, never offline.
    strncpy(lastErr_, net.error(), sizeof(lastErr_) - 1);
    retryAt_ = millis() + 1500;
    phase_ = PH_LOADING;
    dirty_ = true;
    return;
  }
  // No config / no WiFi at all: keep the loading screen and retry.
  retryAt_ = millis() + 2000;
  phase_ = PH_LOADING;
  dirty_ = true;
}

// Pixel-accurate word wrap, UTF-8 aware: a line is broken at the last
// space whose byte range still fits maxW. Expects the matching font
// (PROGMEM Roboto .vlw via loadStoryFont/loadBigFont). Multi-byte UTF-8 sequences (umlauts)
// are never split.
int SurvivalGame::wrapText(const char* t, bool big, int16_t maxW,
                           char out[][LINE_LEN + 1], int maxL) {
  TFT_eSPI& tft = gfx.tft();
  int n = 0;
  const char* p = t;
  while (*p && n < maxL) {
    const char* start = p;
    int i = 0, lastSpace = -1, w = 0;
    while (p[i] && i < LINE_LEN) {
      // measure one UTF-8 codepoint (1-4 bytes), never split it
      int clen = 1;
      uint8_t c0 = (uint8_t)p[i];
      if (c0 >= 0xF0) clen = 4;
      else if (c0 >= 0xE0) clen = 3;
      else if (c0 >= 0xC0) clen = 2;
      if (i + clen > LINE_LEN) break;
      char tmp[5] = {0, 0, 0, 0, 0};
      memcpy(tmp, p + i, clen);
      int cw = tft.textWidth(tmp);
      if (w + cw > maxW) break;
      w += cw;
      if (p[i] == ' ') lastSpace = i;
      i += clen;
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
    // No offline fallback: retry the request after a short pause.
    if (phase_ == PH_LOADING) {
      retryAt_ = millis() + 2000;
      dirty_ = true;
    }
  }
  netState_ = ns;

  switch (phase_) {
    case PH_TITLE:
      // Live status refresh on the title screen (every ~500 ms) and
      // keep WiFi associating: begin() restarts after WifiFailed/timeout.
      if (millis() - titleRefresh_ > 500) {
        titleRefresh_ = millis();
        dirty_ = true;
      }
      if (online_ && !net.connected() && millis() - lastWifiRetry_ > 2000) {
        lastWifiRetry_ = millis();
        net.begin();  // (re)start association from the title screen too
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
      // Retry a refused/failed request after a short pause (no offline mode).
      if (retryAt_ && millis() >= retryAt_) {
        retryAt_ = 0;
        startRequest();
        break;
      }
      // While waiting for WiFi, retry startRequest() once connected or
      // retry the association and keep waiting (no offline mode).
      if (wifiWaiting_) {
        if (net.connected()) {
          wifiWaiting_ = false;
          wifiAttempts_ = 0;
          startRequest();
        } else if (millis() - wifiWaitStart_ > 30000) {
          // 30 s without WiFi: keep waiting and retry association again.
          wifiAttempts_ = 0;
          strncpy(lastErr_, "wifi timeout, retrying", sizeof(lastErr_) - 1);
          wifiWaitStart_ = millis();
          lastWifiRetry_ = millis();
          dirty_ = true;
        } else {
          // Re-try the association every 5 s while waiting (handles
          // WifiFailed / dropped connection instead of hanging forever).
          if (millis() - lastWifiRetry_ > 5000) {
            lastWifiRetry_ = millis();
            if (wifiAttempts_ < 4) {
              wifiAttempts_++;
              net.begin();  // restarts WiFi association
            }
          }
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
      // Touch boards: tap directly on a choice row (full width, K_OPT_H tall)
      // confirms that choice immediately.
      if constexpr (board::HAS_TOUCH) {
        if (in.touchTap && choosing_ && optCount_ > 0) {
          int16_t row = (in.touchY - K_OPT_Y) / K_OPT_H;
          if (in.touchX >= 0 && in.touchX < SCR_W &&
              row >= 0 && row < optCount_) {
            choose((uint8_t)row);
            break;  // consumed - do not also fire the Key::B mapping
          }
        }
      }
      loadStoryFont();
      int lines = wrapText(text_,
                           false, SCR_W - 8, wrapBuf_, MAX_WRAP_LINES);
      unloadFontIfLoaded();
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
      if constexpr (board::HAS_TOUCH) {
        if (in.touchTap) { init(); break; }  // tap = restart
      }
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

void SurvivalGame::loadStoryFont() {
  TFT_eSPI& tft = gfx.tft();
  if constexpr (board::HAS_TOUCH) {
    // QSPI boards have no .vlw support in the display facade: use a bigger
    // built-in font so the text stays readable/tappable on the touch panel.
    tft.setTextFont(2);                // 16px classic
  } else {
    tft.loadFont(Roboto24_vlw);        // PROGMEM array from Roboto24.h
    tft.setTextFont(1);                // font 1 = use loaded smooth font
    tft.setTextSize(1);
  }
}

void SurvivalGame::loadBigFont() {
  TFT_eSPI& tft = gfx.tft();
  if constexpr (board::HAS_TOUCH) {
    tft.setTextFont(2);
    tft.setTextSize(3);                // 24px classic; choice rows are 40px tall
  } else {
    tft.loadFont(Roboto28_vlw);        // PROGMEM array from Roboto28.h
    tft.setTextFont(1);
    tft.setTextSize(1);
  }
}

void SurvivalGame::unloadFontIfLoaded() {
  if (gfx.tft().fontLoaded) gfx.tft().unloadFont();
}

int16_t SurvivalGame::drawText(const char* s, int16_t x, int16_t y,
                               bool big, uint16_t color) {
  if (!s || !s[0]) return x;
  TFT_eSPI& tft = gfx.tft();
  // Caller must have selected the font already (loadStoryFont/loadBigFont
  // or built-in fallback) and unloads it afterwards; drawText only
  // measures + draws, so multi-line draws keep the smooth font.
  (void)big;
  tft.setTextColor(color, K_COL_BG);  // bg needed for anti-aliasing
  tft.setTextDatum(TL_DATUM);
  int16_t w = tft.textWidth(s);
  if (x < 0) x = (SCR_W - w) / 2;          // negative x = center
  if (x < 0) x = 0;
  tft.drawString(s, x, y);
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
  if (net.connected())  { connColor = 0x07E0; connLabel = "AI"; }
  else                 { connColor = TFT_YELLOW; connLabel = "..."; }
  tft.setTextColor(connColor);
  tft.drawString(connLabel, 296, 4);
  tft.drawFastHLine(0, K_STATUS_H - 2, SCR_W, TFT_DARKGREY);
}

void SurvivalGame::drawTitle() {
  TFT_eSPI& tft = gfx.tft();
  const UiText& ui = kUi[lang_];
  tft.fillScreen(K_COL_BG);
  tft.setTextDatum(TL_DATUM);

  loadBigFont();
  drawText("SURVIVAL", -1, 12, true, K_COL_BLOOD);
  unloadFontIfLoaded();

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
      switch (netState_) {
        case NetClient::State::WifiConnecting: wifiState = "connecting..."; wifiColor = TFT_YELLOW; break;
        case NetClient::State::WifiFailed:     wifiState = "FAILED";        wifiColor = TFT_RED; break;
        case NetClient::State::NoConfig:       wifiState = "no secrets.h";  wifiColor = TFT_ORANGE; break;
        default:                               wifiState = "not connected"; wifiColor = TFT_RED; break;
      }
    }
    char line[48];
    snprintf(line, sizeof(line), "WiFi: %s (%d dBm)", wifiState, net.rssi());
    tft.setTextColor(wifiColor);
    tft.drawString(line, 20, y);

    tft.setTextColor(net.connected() ? 0x07E0 : TFT_DARKGREY);
    tft.drawString(net.connected() ? "API:  ready (Mistral)" : "API:  waiting for WiFi", 20, y + 10);
    // Show the last network error (diagnosis).
    if (lastErr_[0]) {
      tft.setTextFont(1);
      tft.setTextColor(TFT_ORANGE);
      char errLine[64];
      snprintf(errLine, sizeof(errLine), "last: %s", lastErr_);
      tft.drawString(errLine, 20, y + 22);
      tft.setTextFont(2);
    }
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
  loadBigFont();
  drawText(day, -1, 44, true, K_COL_TEXT);
  unloadFontIfLoaded();

  tft.setTextFont(2);
  tft.setTextColor(K_COL_TEXT);
  uint8_t dots = (millis() / 400) % 4;
  String s;
  if (net.connected()) {
    s = ui.thinks;
  } else {
    s = ui.waitWifi;
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
    // Story text: Roboto24 smooth font, 28px lines.
    loadStoryFont();
    int lines = wrapText(text_, false, SCR_W - 8, wrapBuf_, MAX_WRAP_LINES);
    int pages = (lines + TXT_LINES - 1) / TXT_LINES;
    if (pages < 1) pages = 1;
    if (page_ >= (uint8_t)pages) page_ = pages - 1;
    int start = page_ * TXT_LINES;

    for (uint8_t i = 0; i < TXT_LINES; i++) {
      int li = start + i;
      if (li < lines)
        drawText(wrapBuf_[li], -1, K_TEXT_Y + i * K_TEXT_LH, false, K_COL_TEXT);
    }
    unloadFontIfLoaded();

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
    // Choice screen: one option per row, Roboto28 smooth font (Font 4
    // centered; falls back to Roboto24 if a label is too wide.
    loadBigFont();
    for (uint8_t i = 0; i < optCount_; i++) {
      if (tft.textWidth(opts_[i]) > SCR_W - 40) {
        unloadFontIfLoaded();
        loadStoryFont();
      }
      bool sel = (i == sel_);
      int16_t y = K_OPT_Y + i * K_OPT_H;
      int16_t tx = drawText(opts_[i], -1, y, true, sel ? K_COL_SELECT : K_COL_DIM);
      if (tft.fontLoaded) loadBigFont();  // restore big font for next row
      if (sel) {
        tft.setTextFont(2);
        tft.setTextColor(K_COL_SELECT);
        tft.drawString(">", tx - 16, y + 6);
      }
    }
    unloadFontIfLoaded();
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
  loadBigFont();
  drawText(head, -1, 40, true, won ? K_COL_SAFE : K_COL_BLOOD);
  unloadFontIfLoaded();

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
