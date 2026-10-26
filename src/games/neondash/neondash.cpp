#include "neondash.h"

// ---------------------------------------------------------------- Palette
// Gleiche Neon-Familie wie NeonRun (eigenstaendig, damit das Spiel auch
// ohne das SpriteRenderer-Interface des Runners laeuft).
static const uint16_t COL_BG_TOP  = 0x0841;  // Nachthimmel
static const uint16_t COL_BG_MID  = 0x1082;  // Strassenstreifen
static const uint16_t COL_BG_BOT  = 0x2104;  // Strasse
static const uint16_t COL_STAR    = 0xC618;  // Sterne
static const uint16_t COL_HORIZON = 0xF81F;  // Neon-Magenta
static const uint16_t COL_PLAYER  = 0x07FF;  // Neon-Cyan
static const uint16_t COL_GLOW    = 0x0780;  // Cyan-Glow
static const uint16_t COL_OBS     = 0xF81F;  // Firewall-Magenta
static const uint16_t COL_OBS_DK  = 0x8010;  // dunkles Magenta
static const uint16_t COL_DRONE   = 0xFD20;  // Neon-Orange
static const uint16_t COL_DRONE_D = 0xA000;  // dunkles Orange
static const uint16_t COL_CHIP    = 0xFFE0;  // Neon-Gelb
static const uint16_t COL_CHIP_D  = 0x8400;  // dunkles Gelb
static const uint16_t COL_TEXT    = 0xFFFF;
static const uint16_t COL_DIM     = 0x7BEF;

static constexpr int16_t SW = 170;
static constexpr int16_t SH = 320;

// ---------------------------------------------------------------- Spielstart

void NeonDashGame::init() {
  gfx.tft().setRotation(board::TFT_ROTATION);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  phase_    = PH_TITLE;
  py_       = GROUND_Y - PLAYER_H;
  vy_       = 0;
  onGround_ = true;
  wantsMenu = false;
}

void NeonDashGame::startRun() {
  phase_     = PH_RUN;
  py_        = GROUND_Y - PLAYER_H;
  vy_        = 0;
  onGround_  = true;
  speed_     = SPEED_START;
  distance_  = 0;
  runMs_     = 0;
  scroll_    = 0;
  score_     = 0;
  chipBonus_ = 0;
  spawnIn_   = 260;  // erstes Hindernis etwas spaeter
  for (auto& o : obs_)   o.alive = false;
  for (auto& c : chips_) c.alive = false;
}

void NeonDashGame::endRun() {
  phase_ = PH_GAMEOVER;
  if (score_ > best_) best_ = score_;
}

bool NeonDashGame::overlaps(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                            int16_t bx, int16_t by, int16_t bw, int16_t bh) const {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void NeonDashGame::spawnObstacle() {
  // Passender freier Slot finden
  Obstacle* slot = nullptr;
  for (auto& o : obs_) if (!o.alive) { slot = &o; break; }
  if (!slot) return;

  // Drohnen schweben hoch, Staemme stehen auf dem Boden. Mit zunehmender
  // Geschwindigkeit mehr Drohnen.
  uint8_t laneRoll = random(0, 100);
  uint8_t maxDronePct = (uint8_t)min(20 + runMs_ / 4000, 55u);
  bool drone = laneRoll < maxDronePct;

  if (drone) {
    slot->kind = 1;
    slot->h    = 12;
    slot->x    = SW + 20;
    // y wird in update() als Schwebe-Hoehe gesetzt
    slot->y    = GROUND_Y - 24;
  } else {
    slot->kind = 0;
    // Staemme: 1-3 Segmente hoch (Sprung schafft max ~70 px)
    slot->h    = 16 + (uint8_t)random(0, 3) * 10;  // 16/26/36
    slot->x    = SW + 20;
    slot->y    = GROUND_Y - slot->h;
  }
  slot->alive  = true;

  // Datenchip: Bonus in Sprunghoehe ueber Boden-Staemmen
  if (!drone && slot->h >= 26) {
    for (auto& c : chips_) {
      if (c.alive) continue;
      c.x     = slot->x + 8;
      c.y     = (float)(GROUND_Y - slot->h - 22);
      c.alive = true;
      break;
    }
  }
}

// ---------------------------------------------------------------- Eingabe

void NeonDashGame::update(const InputState& in, uint32_t dtMs) {
  float dt = dtMs / 1000.0f;
  if (dt > 0.05f) dt = 0.05f;  // dt clampen (erster Frame, Haenger)

  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::B])
        startRun();
      break;

    case PH_RUN: {
      if (in.longPressed[(uint8_t)Key::B]) { wantsMenu = true; return; }

      // Einziger Spiel-Button: Sprung (nur am Boden moeglich)
      if ((in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B]) &&
          onGround_) {
        vy_       = JUMP_V;
        onGround_ = false;
      }

      runMs_ += dtMs;

      // Tempo steigt kontinuierlich bis zum Cap
      speed_ = min(SPEED_MAX, SPEED_START + SPEED_RAMP * (runMs_ / 1000.0f));

      // Sprung-Physik
      vy_ += GRAVITY * dt;
      py_ += vy_ * dt;
      if (py_ >= GROUND_Y - PLAYER_H) {
        py_       = GROUND_Y - PLAYER_H;
        vy_       = 0;
        onGround_ = true;
      }

      float step = speed_ * dt;
      distance_ += step;
      scroll_   = (uint32_t)(scroll_ + step);
      score_    = (uint16_t)(distance_ / 10) + chipBonus_;  // 1 m = 10 px

      // Hindernisse bewegen sich nach links
      int16_t pTop = (int16_t)py_;
      for (auto& o : obs_) {
        if (!o.alive) continue;
        o.x -= step;
        int16_t ox = (int16_t)o.x;
        int16_t oy;
        if (o.kind == 0) {
          oy = GROUND_Y - o.h;  // Firewall-Staemme am Boden
        } else {
          // Drohne: schwebt tief genug, dass der Spieler kollidiert,
          // aber der Sprung (max ~70 px) locker drueber kommt
          oy = GROUND_Y - 24 + (int16_t)(sinf((ox + runMs_) * 0.006f) * 8);
        }
        o.y = oy;
        if (ox < -24) { o.alive = false; continue; }
        // Kollision mit der Spieler-Box
        if (overlaps(PLAYER_X, pTop, PLAYER_W, PLAYER_H,
                     ox, oy, 12, o.h)) {
          endRun();
          return;
        }
      }

      // Datenchips: einsammeln (auch in der Luft), bringen Bonus
      for (auto& c : chips_) {
        if (!c.alive) continue;
        c.x -= step;
        if (c.x < -10) { c.alive = false; continue; }
        if (overlaps(PLAYER_X, pTop, PLAYER_W, PLAYER_H,
                     (int16_t)c.x - 5, (int16_t)c.y - 5, 10, 10)) {
          c.alive = false;
          chipBonus_ += SCORE_CHIP;
        }
      }

      // Naechstes Hindernis spawnen: Abstand schrumpft mit dem Tempo,
      // damit die Reaktionszeit konstant bleibt (Speed-Up = dichter).
      spawnIn_ -= step;
      if (spawnIn_ <= 0) {
        spawnObstacle();
        // Abstand in Pixeln; je schneller, desto enger - aber nie so
        // eng, dass zwei Hindernisse nicht mehr nacheinander zu schaffen
        // sind (min. ~1.3x Sprungreichweite).
        float gap = 230.0f + (float)random(0, 160);
        gap = max(200.0f, gap - (speed_ - SPEED_START) * 0.35f);
        spawnIn_ = gap;
      }
      break;
    }

    case PH_GAMEOVER:
      if (in.longPressed[(uint8_t)Key::B]) { wantsMenu = true; return; }
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B])
        startRun();
      break;
  }
}

// ---------------------------------------------------------------- Zeichnen

void NeonDashGame::draw() {
  if (!fb_.created()) {
    TFT_eSPI& tft = gfx.tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Framebuffer alloc fehlgeschlagen", 6, 8);
    return;
  }
  if (phase_ == PH_TITLE || phase_ == PH_GAMEOVER) {
    drawWorld(fb_);
    drawOverlay(fb_, phase_ == PH_TITLE ? "NEON DASH" : "CRASH!",
                phase_ == PH_TITLE ? "A/B: Sprung - Hindernisse meiden"
                                   : "A/B: nochmal | B lang: Menue");
  } else {
    drawWorld(fb_);
  }
  drawHud(fb_);
  fb_.pushSprite(0, 0);
}

void NeonDashGame::drawWorld(TFT_eSprite& fb) {
  const int16_t roadY = GROUND_Y + 8;

  fb.fillSprite(COL_BG_BOT);
  fb.fillRect(0, 0, SW, roadY, COL_BG_TOP);

  // Sterne mit Parallax (halbe Weltgeschwindigkeit)
  int32_t sm = (int32_t)(scroll_ / 6) % (roadY - 24);
  for (uint8_t i = 0; i < 20; i++) {
    int16_t sx = (int16_t)((i * 67 + 13) % SW);
    int16_t sy = 12 + (int16_t)((i * 131 + 29 + sm) % (roadY - 24));
    if ((i & 3) == 0) fb.fillRect(sx, sy, 2, 2, COL_STAR);
    else             fb.fillRect(sx, sy, 1, 1, COL_STAR);
  }

  // Horizont-Glow
  fb.drawFastHLine(0, roadY - 1, SW, COL_OBS_DK);
  fb.drawFastHLine(0, roadY, SW, COL_HORIZON);

  // Bewegte Boden-Markierungen
  int32_t m = (int32_t)(scroll_ / 24) % 24;
  for (int16_t y = roadY + 4 + (int16_t)m; y < SH; y += 24)
    fb.fillRect(0, y, SW, 10, COL_BG_MID);
  fb.drawFastHLine(0, GROUND_Y, SW, COL_GLOW);

  // Hindernisse
  for (auto& o : obs_) {
    if (!o.alive) continue;
    int16_t ox = (int16_t)o.x;
    int16_t oy = o.y;
    if (o.kind == 0) {
      // Firewall-Staemme: Warnstreifen-Saeule
      fb.fillRect(ox, oy, 12, o.h, COL_OBS_DK);
      for (int16_t sy = oy; sy < oy + o.h; sy += 8)
        fb.fillRect(ox, sy, 12, 4, COL_OBS);
      fb.drawRect(ox, oy, 12, o.h, COL_HORIZON);
    } else {
      // Schwebende Drohne
      fb.drawFastVLine(ox + 6, oy, 4, COL_DRONE);
      fb.fillCircle(ox + 6, oy, 1, COL_CHIP);
      fb.fillRect(ox, oy + 3, 12, 8, COL_DRONE_D);
      fb.drawRect(ox, oy + 3, 12, 8, COL_DRONE);
      bool eye = (millis() / 250) & 1;
      fb.fillRect(ox + 2, oy + 5, 3, 3, eye ? COL_CHIP : COL_DRONE);
      fb.fillRect(ox + 7, oy + 5, 3, 3, eye ? COL_CHIP : COL_DRONE);
    }
  }

  // Datenchips (pulsierend)
  int16_t r = 4 + (int16_t)((millis() / 180) & 1);
  for (auto& c : chips_) {
    if (!c.alive) continue;
    int16_t cx = (int16_t)c.x, cy = (int16_t)c.y;
    fb.fillTriangle(cx, cy - r, cx - r, cy, cx + r, cy, COL_CHIP);
    fb.fillTriangle(cx, cy + r, cx - r, cy, cx + r, cy, COL_CHIP);
    fb.drawLine(cx, cy - r, cx + r, cy, COL_CHIP_D);
    fb.drawLine(cx, cy - r, cx - r, cy, COL_CHIP_D);
    fb.fillCircle(cx, cy, 1, COL_CHIP_D);
  }

  // Spieler: Dreieck-Runner mit Glow + Flamme beim Aufwaertsflug
  int16_t px = PLAYER_X, pTop = (int16_t)py_;
  fb.fillTriangle(px + 1, pTop + 11, px + 8, pTop - 2, px + 15, pTop + 11, COL_GLOW);
  fb.fillTriangle(px + 3, pTop + 9, px + 8, pTop + 2, px + 13, pTop + 9, COL_PLAYER);
  fb.fillCircle(px + 8, pTop + 6, 1, TFT_WHITE);
  if (vy_ < -50)  // Schub nach oben
    fb.fillTriangle(px + 6, pTop + 11, px + 10, pTop + 11, px + 8,
                    pTop + 15 + (int16_t)(-vy_ / 60), COL_DRONE);
}

void NeonDashGame::drawHud(TFT_eSprite& fb) {
  fb.setTextFont(1);
  fb.setTextDatum(TL_DATUM);

  // Meter (Score) rechts oben
  char b[24];
  snprintf(b, sizeof(b), "%um", score_);
  fb.setTextColor(COL_CHIP, COL_BG_TOP);
  fb.drawString(b, 128, 4);

  // Highscore darunter
  snprintf(b, sizeof(b), "HI %um", best_);
  fb.setTextColor(COL_DIM, COL_BG_TOP);
  fb.drawString(b, 118, 16);

  // Speed-Bar: aktuelles Tempo als Balken (links)
  fb.setTextColor(COL_DIM, COL_BG_TOP);
  fb.drawString("SPD", 4, 4);
  int16_t bw = (int16_t)((speed_ / SPEED_MAX) * 56);
  fb.fillRect(26, 5, 56, 7, COL_BG_MID);
  fb.fillRect(26, 5, bw, 7, COL_PLAYER);
  fb.drawRect(26, 5, 56, 7, COL_DIM);

  fb.drawFastHLine(0, 26, SW, COL_OBS_DK);

  // Steuerungshinweis unten
  if (phase_ == PH_RUN) {
    fb.setTextColor(COL_DIM, COL_BG_BOT);
    fb.drawString("A/B: Sprung", 4, SH - 10);
    fb.drawString("B lang: Menue", SW - 62, SH - 10);
  }
}

void NeonDashGame::drawOverlay(TFT_eSprite& fb, const char* title, const char* sub) {
  fb.fillRect(0, 92, SW, 104, COL_BG_TOP);
  fb.drawFastHLine(0, 92, SW, COL_HORIZON);
  fb.drawFastHLine(0, 195, SW, COL_HORIZON);
  fb.setTextFont(4);
  fb.setTextColor(COL_PLAYER, COL_BG_TOP);
  fb.setTextDatum(MC_DATUM);
  fb.drawString(title, SW / 2, 110);
  fb.setTextFont(1);
  fb.setTextColor(COL_TEXT, COL_BG_TOP);
  fb.drawString(sub, SW / 2, 146);
  char b[32];
  snprintf(b, sizeof(b), "Rekord: %um", best_);
  fb.setTextColor(COL_CHIP, COL_BG_TOP);
  fb.drawString(b, SW / 2, 164);
  if (phase_ == PH_GAMEOVER) {
    snprintf(b, sizeof(b), "Gefahren: %um", score_);
    fb.setTextColor(COL_TEXT, COL_BG_TOP);
    fb.drawString(b, SW / 2, 178);
  }
}
