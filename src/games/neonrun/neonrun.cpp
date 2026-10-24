#include "neonrun.h"

// Out-of-Line-Definition (C++17: noetig, sobald die Adresse gebraucht wird)
constexpr int16_t NeonRunGame::LANE_X[3];

void NeonRunGame::init() {
  phase_      = PH_TITLE;
  lane_       = 1;
  playerX_    = LANE_X[1];
  hp_         = MAX_HP;
  score_      = 0;
  scroll_     = 0;
  worldSpeed_ = WORLD_SPEED;
  spawnTimer_ = 0;
  invulnUntil_ = 0;
  wantsMenu   = false;
  barActive_  = false;
  for (auto& e : ents_) e.alive = false;
}

void NeonRunGame::startRun() {
  phase_   = PH_RUN;
  lane_    = 1;
  playerX_ = LANE_X[1];
  hp_      = MAX_HP;
  score_   = 0;
  scroll_  = 0;
  worldSpeed_ = WORLD_SPEED;
  spawnTimer_ = 0;
  invulnUntil_ = 0;
  barActive_  = false;
  for (auto& e : ents_) e.alive = false;
}

void NeonRunGame::endRun(bool won) {
  phase_ = won ? PH_WIN : PH_GAMEOVER;
  if (score_ > best_) best_ = score_;
}

void NeonRunGame::spawnEntity() {
  // Spawn-Position: oberhalb des Bildschirms, Spur frei waehlen
  for (uint8_t tries = 0; tries < 4; tries++) {
    uint8_t lane = (uint8_t)random(0, 3);
    bool taken = false;
    for (auto& e : ents_)
      if (e.alive && e.lane == lane && e.y < 90) taken = true;
    if (taken) continue;

    Entity* slot = nullptr;
    for (auto& e : ents_) if (!e.alive) { slot = &e; break; }
    if (!slot) return;

    // Mit zunehmendem Score mehr Drohnen, zu Beginn fast nur Chips
    bool drone = random(0, 100) < (uint8_t)min(15 + score_ * 2, 45);
    slot->lane  = lane;
    slot->x     = LANE_X[lane] + 7;
    slot->y     = -14;
    slot->kind  = drone ? 1 : 0;
    slot->alive = true;
    return;
  }
}

void NeonRunGame::hurt() {
  if (millis() < invulnUntil_) return;
  invulnUntil_ = millis() + INVULN_MS;
  if (hp_ > 0) hp_--;
  if (hp_ == 0) endRun(false);
}

void NeonRunGame::update(const InputState& in, uint32_t dtMs) {
  uint32_t now = millis();
  float dt = dtMs / 1000.0f;
  if (dt > 0.05f) dt = 0.05f;  // dt clampen (erste Frame, Hänger)

  if (in.pressed[(uint8_t)Key::A]) {
    if (phase_ == PH_TITLE || phase_ == PH_GAMEOVER || phase_ == PH_WIN) startRun();
    else if (lane_ > 0) lane_--;
  }
  if (in.pressed[(uint8_t)Key::B]) {
    if (phase_ == PH_TITLE || phase_ == PH_GAMEOVER || phase_ == PH_WIN) startRun();
    else if (lane_ < 2) lane_++;
  }
  if (in.longPressed[(uint8_t)Key::A] && phase_ != PH_RUN) init();
  if (in.longPressed[(uint8_t)Key::B]) wantsMenu = true;

  if (phase_ != PH_RUN) return;

  // Spieler bewegt sich zur Ziel-Spur
  float target = LANE_X[lane_];
  float diff = target - playerX_;
  float step = PLAYER_SPEED * dt;
  if (fabsf(diff) <= step) playerX_ = target;
  else playerX_ += (diff > 0 ? step : -step);

  // Welt scrollt
  float speed = worldSpeed_ * (1.0f + score_ * 0.012f);
  scroll_ += (uint32_t)(speed * dt);

  // Firewall-Balken: laeuft periodisch von oben nach unten, Luecke in
  // einer festen Spur. Logik hier, Zeichnen in drawWorld().
  uint32_t cycle = scroll_ % BAR_CYCLE;
  if (cycle < (uint32_t)BAR_TRAVEL) {
    barActive_  = true;
    barY_       = BAR_TOP + (int16_t)cycle;
    barGapLane_ = (uint8_t)((scroll_ / BAR_CYCLE) % 3);
    int16_t py = PLAYER_Y;
    if (barY_ + 12 > py - 10 && barY_ < py + 14 && lane_ != barGapLane_)
      hurt();
  } else {
    barActive_ = false;
  }

  // Spawn-Timer (wird mit Score schneller)
  uint32_t interval = (uint32_t)(SPAWN_MS / (1.0f + score_ * 0.01f));
  spawnTimer_ += dtMs;
  if (spawnTimer_ >= interval) {
    spawnTimer_ = 0;
    spawnEntity();
  }

  // Entities bewegen sich nach unten
  for (auto& e : ents_) {
    if (!e.alive) continue;
    e.y += speed * dt;
    if (e.y > gfx.height() + 20) { e.alive = false; continue; }

    // Kollision: Entity in Spieler-Spur und auf Spieler-Höhe
    int16_t px = (int16_t)playerX_;
    int16_t py = PLAYER_Y;
    if (now >= invulnUntil_ &&
        e.lane == lane_ &&
        e.y + 10 > py - 12 && e.y < py + 12) {
      if (e.kind == 0) {
        score_++;
        if (score_ >= WIN_SCORE) { endRun(true); }
      } else {
        hurt();
      }
      e.alive = false;
    }
  }
}

void NeonRunGame::drawHud() {
  TFT_eSPI& tft = gfx.tft();
  const NeonPalette& pal = gSprites->palette();
  tft.setTextFont(1);
  tft.setTextDatum(TL_DATUM);

  // HP als Neon-Balken
  tft.setTextColor(pal.textDim);
  tft.drawString("HP", 4, 4);
  for (uint8_t i = 0; i < MAX_HP; i++)
    tft.fillRect(20 + i * 14, 4, 12, 8, i < hp_ ? pal.obstacle : pal.bgMid);

  // Score + Best rechts
  char b[24];
  snprintf(b, sizeof(b), "%u", score_);
  tft.setTextColor(pal.coin);
  tft.drawString(b, 128, 4);
  snprintf(b, sizeof(b), "HI %u", best_);
  tft.setTextColor(pal.textDim);
  tft.drawString(b, 92, 16);

  tft.drawFastHLine(0, 14, gfx.height(), pal.obstacleDark);
}

void NeonRunGame::drawWorld() {
  TFT_eSPI& tft = gfx.tft();
  gSprites->drawBackground(tft, scroll_);

  // Firewall-Balken (Position wird in update() berechnet)
  if (barActive_)
    gSprites->drawBarObstacle(tft, 20, barY_, gfx.width() - 40, 12,
                              LANE_X[barGapLane_] - 20, 34);

  for (auto& e : ents_) {
    if (!e.alive) continue;
    if (e.kind == 0) gSprites->drawCoin(tft, (int16_t)e.x, (int16_t)e.y, millis());
    else             gSprites->drawDrone(tft, (int16_t)e.x - 7, (int16_t)e.y, millis());
  }

  bool blink = millis() < invulnUntil_ && (millis() / 80) & 1;
  gSprites->drawPlayer(tft, (int16_t)playerX_, PLAYER_Y, !blink);
}

void NeonRunGame::drawOverlay(const char* title, const char* sub) {
  TFT_eSPI& tft = gfx.tft();
  const NeonPalette& pal = gSprites->palette();
  int16_t w = gfx.width();
  tft.fillRect(0, 96, w, 92, pal.bgTop);
  tft.drawFastHLine(0, 96, w, pal.horizon);
  tft.drawFastHLine(0, 187, w, pal.horizon);
  tft.setTextFont(2);
  tft.setTextColor(pal.player);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title, w / 2, 118);
  tft.setTextFont(1);
  tft.setTextColor(pal.text);
  tft.drawString(sub, w / 2, 146);
  char b[32];
  snprintf(b, sizeof(b), "Punkte: %u   Best: %u", score_, best_);
  tft.setTextColor(pal.coin);
  tft.drawString(b, w / 2, 166);
  tft.setTextColor(pal.textDim);
  tft.drawString("A/B: Spur | Taste: Start", w / 2, 296);
  tft.drawString("B lang: Menue", w / 2, 308);
}

void NeonRunGame::render() {
  TFT_eSPI& tft = gfx.tft();
  if (phase_ == PH_RUN) {
    drawWorld();
    drawHud();
  } else if (phase_ == PH_TITLE) {
    drawWorld();
    drawHud();
    drawOverlay("NEON RUN", "Sprint durch die Neonstadt");
  } else if (phase_ == PH_GAMEOVER) {
    drawWorld();
    drawHud();
    drawOverlay("SYSTEM DOWN", "Die Firewall hat dich erwischt");
  } else {
    drawWorld();
    drawHud();
    drawOverlay("RUN COMPLETE", "Alle 25 Chips gesammelt!");
  }
}

void NeonRunGame::draw() {
  render();
}
