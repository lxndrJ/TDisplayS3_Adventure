#include "sakura.h"

// ================================================================ Layout

static constexpr int16_t SW = 170;
static constexpr int16_t SH = 320;

// Ground line where the tree stands
static constexpr int16_t GROUND_Y = 252;
static constexpr int16_t TRUNK_X  = SW / 2;   // 85

// Sky gradient bands
static constexpr uint16_t COL_SKY_TOP = 0x0210;   // deep dusk blue
static constexpr uint16_t COL_SKY_MID = 0x4A52;   // soft purple-blue
static constexpr uint16_t COL_SKY_BOT = 0xB6DA;   // pale pink horizon
static constexpr uint16_t COL_GROUND  = 0x2A08;   // dark soil
static constexpr uint16_t COL_SOIL_HI = 0x5ACB;   // soil highlight

// Tree colors
static constexpr uint16_t COL_TRUNK   = 0x6B45;   // brown
static constexpr uint16_t COL_TRUNK_HI = 0x9CD3;  // bark highlight
static constexpr uint16_t COL_LEAF    = 0x4F46;   // green foliage
static constexpr uint16_t COL_LEAF_DK = 0x2E03;   // dark foliage
static constexpr uint16_t COL_BLOSSOM = 0xF59F;   // sakura pink
static constexpr uint16_t COL_BLOSSOM_HI = 0xFDBF; // light pink
static constexpr uint16_t COL_CHERRY  = 0xF800;   // red cherries
static constexpr uint16_t COL_CHERRY_HI = 0xFBEF; // cherry highlight
static constexpr uint16_t COL_SEED    = 0x8410;   // brown seed

// UI colors
static constexpr uint16_t COL_TEXT    = 0xFFFF;
static constexpr uint16_t COL_DIM     = 0x8410;
static constexpr uint16_t COL_WATER   = 0x29FF;   // cyan-blue
static constexpr uint16_t COL_FERT    = 0x8260;   // earthy orange
static constexpr uint16_t COL_PRUNE   = 0xC618;   // grey
static constexpr uint16_t COL_HARVEST = 0xF800;   // red
static constexpr uint16_t COL_OK      = 0x07E0;   // green (healthy)
static constexpr uint16_t COL_WARN    = 0xFFE0;   // yellow (low)
static constexpr uint16_t COL_BAD     = 0xF800;   // red (critical)

// ================================================================ Helpers

static uint16_t statColor(int8_t v) {
  if (v >= 50) return COL_OK;
  if (v >= 25) return COL_WARN;
  return COL_BAD;
}

static const char* stageName(SakuraGame::Stage s) {
  switch (s) {
    case SakuraGame::ST_SEED:      return "Seed";
    case SakuraGame::ST_SPROUT:    return "Sprout";
    case SakuraGame::ST_SAPLING:   return "Sapling";
    case SakuraGame::ST_BLOSSOMING: return "Blossom";
    case SakuraGame::ST_FRUITING:  return "Fruiting";
    default: return "?";
  }
}

static const char* actionName(SakuraGame::Action a) {
  switch (a) {
    case SakuraGame::ACT_WATER:     return "Water";
    case SakuraGame::ACT_FERTILIZE: return "Fertilize";
    case SakuraGame::ACT_PRUNE:     return "Prune";
    case SakuraGame::ACT_HARVEST:   return "Harvest";
    default: return "?";
  }
}

static uint16_t actionColor(SakuraGame::Action a) {
  switch (a) {
    case SakuraGame::ACT_WATER:     return COL_WATER;
    case SakuraGame::ACT_FERTILIZE: return COL_FERT;
    case SakuraGame::ACT_PRUNE:     return COL_PRUNE;
    case SakuraGame::ACT_HARVEST:   return COL_HARVEST;
    default: return COL_TEXT;
  }
}

static int8_t clampStat(int8_t v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return v;
}

// ================================================================ Lifecycle

void SakuraGame::init() {
  gfx.tft().setRotation(board::TFT_ROTATION);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  phase_ = PH_TITLE;
  wantsMenu = false;
}

void SakuraGame::quit() {
  fb_.deleteSprite();
}

void SakuraGame::startGame() {
  stage_       = ST_SEED;
  decade_      = 0;
  decadeMs_    = 0;
  moisture_    = 60;
  nutrients_   = 60;
  vigor_       = 60;
  health_      = 100;
  score_       = 0;
  cherries_    = 0;
  action_      = ACT_WATER;
  actionFlashMs_ = 0;
  cycleFlashMs_  = 0;
  lastActionMs_  = 0;
  sway_        = 0;
  rain_        = false;
  rainUntil_   = 0;
  phase_       = PH_PLAY;
}

// ================================================================ Growth

uint8_t SakuraGame::stageForDecade(uint16_t decade) const {
  uint8_t s = decade / DECADES_PER_STAGE;
  if (s >= ST_COUNT) s = ST_COUNT - 1;
  return s;
}

void SakuraGame::advanceDecade() {
  decade_++;
  // Occasional rain event (auto-water, ~1 in 6 decades)
  if (random(6) == 0) {
    rain_ = true;
    rainUntil_ = millis() + 2500;
    moisture_ = clampStat(moisture_ + 20);
  } else {
    rain_ = false;
  }

  // Natural decay of care stats
  moisture_  = clampStat(moisture_  - DECAY_MOIST);
  nutrients_ = clampStat(nutrients_ - DECAY_NUTRI);
  vigor_     = clampStat(vigor_     - DECAY_VIGOR);

  updateHealth();

  // Advance growth stage (tree grows when healthy enough)
  Stage newStage = (Stage)stageForDecade(decade_);
  if (newStage > stage_ && health_ >= 40) {
    stage_ = newStage;
  }

  // Mature tree produces cherries over time (up to CHERRIES_MAX visible)
  // Cherries are a harvestable resource tracked implicitly by score.
}

void SakuraGame::updateHealth() {
  // Health drains when any stat is critically low
  bool critical = (moisture_ < 20) || (nutrients_ < 20) || (vigor_ < 20);
  if (critical) {
    health_ -= HEALTH_DRAIN;
  } else {
    health_ += HEALTH_RECOVER;
  }
  if (health_ < 0) health_ = 0;
  if (health_ > 100) health_ = 100;
  if (health_ == 0 && phase_ == PH_PLAY) {
    phase_ = PH_DEAD;
    if (score_ > best_) best_ = score_;
  }
}

// ================================================================ Actions

void SakuraGame::performAction() {
  lastActionMs_ = millis();
  actionFlashMs_ = 400;
  switch (action_) {
    case ACT_WATER:
      moisture_ = clampStat(moisture_ + WATER_AMOUNT);
      break;
    case ACT_FERTILIZE:
      nutrients_ = clampStat(nutrients_ + FERTILIZE_AMOUNT);
      break;
    case ACT_PRUNE:
      vigor_ = clampStat(vigor_ + PRUNE_AMOUNT);
      break;
    case ACT_HARVEST:
      if (isMature()) {
        // Harvest: gain score, tree stays mature but loses some vigor
        // (pruning the fruit load). Cherries regrow over decades.
        uint8_t n = 1 + random(3);  // 1..3 cherries per harvest
        score_   += n * CHERRY_SCORE;
        cherries_ += n;
        vigor_    = clampStat(vigor_ - 5);
      }
      break;
    default: break;
  }
  updateHealth();
}

// ================================================================ Input

void SakuraGame::update(const InputState& in, uint32_t dtMs) {
  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B])
        startGame();
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; return; }

      // A short = cycle action
      if (in.pressed[(uint8_t)Key::A]) {
        action_ = (Action)((action_ + 1) % ACT_COUNT);
        cycleFlashMs_ = 260;
      }
      if (cycleFlashMs_ && dtMs)
        cycleFlashMs_ = (cycleFlashMs_ > dtMs) ? cycleFlashMs_ - dtMs : 0;

      // B short = perform action
      if (in.pressed[(uint8_t)Key::B]) performAction();

      // Timed flash decay
      if (actionFlashMs_ && dtMs)
        actionFlashMs_ = (actionFlashMs_ > dtMs) ? actionFlashMs_ - dtMs : 0;

      // Rain event timer
      if (rain_ && millis() > rainUntil_) rain_ = false;

      // Wind sway animation
      sway_ += dtMs;

      // Advance decades (fast for testing: DECADE_MS real ms = 1 decade)
      decadeMs_ += dtMs;
      while (decadeMs_ >= DECADE_MS) {
        decadeMs_ -= DECADE_MS;
        advanceDecade();
        if (phase_ == PH_DEAD) break;
      }
      break;
    }

    case PH_DEAD:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B])
        startGame();  // retry
      break;
  }
}

// ================================================================ Drawing

void SakuraGame::draw() {
  if (!fb_.created()) {
    TFT_eSPI& tft = gfx.tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Framebuffer alloc failed", 6, 8);
    return;
  }
  if (phase_ == PH_TITLE) {
    drawTitle(fb_);
  } else {
    drawScene(fb_);
    drawTree(fb_);
    drawHud(fb_);
    drawActionMenu(fb_);
    if (phase_ == PH_DEAD) drawDead(fb_);
  }
  fb_.pushSprite(0, 0);
}

void SakuraGame::drawTitle(TFT_eSprite& fb) {
  // Sky gradient
  for (int16_t y = 0; y < SH; y++) {
    uint16_t c;
    if (y < SH / 3)      c = COL_SKY_TOP;
    else if (y < 2*SH/3) c = COL_SKY_MID;
    else                 c = COL_SKY_BOT;
    fb.drawFastHLine(0, y, SW, c);
  }
  // ground
  fb.fillRect(0, GROUND_Y, SW, SH - GROUND_Y, COL_GROUND);

  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(COL_BLOSSOM, COL_SKY_TOP);
  fb.drawString("SAKURA", SW / 2, 40);
  fb.setTextFont(2);
  fb.setTextColor(COL_TEXT, COL_SKY_TOP);
  fb.drawString("Grow a cherry tree", SW / 2, 78);
  fb.setTextColor(COL_DIM, COL_SKY_TOP);
  fb.drawString("Seed -> Tree -> Blossom -> Fruit", SW / 2, 100);

  // Demo tree (mature, with cherries)
  // trunk
  fb.fillRect(TRUNK_X - 5, GROUND_Y - 50, 10, 50, COL_TRUNK);
  fb.fillRect(TRUNK_X - 4, GROUND_Y - 50, 3, 50, COL_TRUNK_HI);
  // branches
  fb.drawLine(TRUNK_X, GROUND_Y - 40, TRUNK_X - 22, GROUND_Y - 62, COL_TRUNK);
  fb.drawLine(TRUNK_X, GROUND_Y - 44, TRUNK_X + 24, GROUND_Y - 66, COL_TRUNK);
  // foliage cloud
  fb.fillCircle(TRUNK_X, GROUND_Y - 78, 34, COL_LEAF);
  fb.fillCircle(TRUNK_X - 24, GROUND_Y - 66, 20, COL_LEAF_DK);
  fb.fillCircle(TRUNK_X + 26, GROUND_Y - 70, 22, COL_LEAF_DK);
  // blossoms
  for (int i = 0; i < 8; i++) {
    int16_t bx = TRUNK_X - 28 + random(56);
    int16_t by = GROUND_Y - 92 + random(30);
    fb.fillCircle(bx, by, 4, COL_BLOSSOM);
    fb.fillCircle(bx - 1, by - 1, 1, COL_BLOSSOM_HI);
  }
  // cherries
  for (int i = 0; i < 5; i++) {
    int16_t cx = TRUNK_X - 24 + random(48);
    int16_t cy = GROUND_Y - 80 + random(28);
    fb.fillCircle(cx, cy, 3, COL_CHERRY);
    fb.fillCircle(cx - 1, cy - 1, 1, COL_CHERRY_HI);
  }

  fb.setTextFont(2);
  fb.setTextColor(COL_WATER, COL_GROUND);
  fb.drawString("A: Action  B: Do", SW / 2, 272);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_GROUND);
  fb.drawString("any key: Plant seed", SW / 2, 300);
}

void SakuraGame::drawScene(TFT_eSprite& fb) {
  // Sky gradient
  for (int16_t y = 0; y < GROUND_Y; y++) {
    uint16_t c;
    if (y < GROUND_Y / 3)      c = COL_SKY_TOP;
    else if (y < 2*GROUND_Y/3) c = COL_SKY_MID;
    else                       c = COL_SKY_BOT;
    fb.drawFastHLine(0, y, SW, c);
  }
  // Ground
  fb.fillRect(0, GROUND_Y, SW, SH - GROUND_Y, COL_GROUND);
  fb.drawFastHLine(0, GROUND_Y, SW, COL_SOIL_HI);

  // Rain effect
  if (rain_) {
    for (int i = 0; i < 12; i++) {
      int16_t rx = random(SW);
      int16_t ry = random(GROUND_Y);
      fb.drawPixel(rx, ry, COL_WATER);
      fb.drawPixel(rx, ry + 1, COL_WATER);
    }
  }

  // Sun/moon
  uint16_t sunCol = (stage_ >= ST_BLOSSOMING) ? 0xFFE0 : 0xC618;
  fb.fillCircle(SW - 24, 28, 10, sunCol);
  fb.drawCircle(SW - 24, 28, 12, sunCol);
}

void SakuraGame::drawTree(TFT_eSprite& fb) {
  // Wind sway: small horizontal offset proportional to stage size
  float swayOff = sinf(sway_ / 400.0f) * (stage_ >= ST_SAPLING ? 2.0f : 0.5f);
  int16_t sx = (int16_t)swayOff;

  switch (stage_) {
    case ST_SEED: {
      // Seed: small brown oval in the soil with a tiny crack
      int16_t cx = TRUNK_X + sx, cy = GROUND_Y + 4;
      fb.fillCircle(cx, cy, 5, COL_SEED);
      fb.fillCircle(cx - 1, cy - 1, 2, COL_TRUNK_HI);
      fb.drawFastHLine(cx - 3, cy - 2, 6, 0x0000);  // soil crack
      break;
    }
    case ST_SPROUT: {
      // Sprout: thin stem + two small leaves
      int16_t cx = TRUNK_X + sx;
      fb.drawLine(cx, GROUND_Y, cx, GROUND_Y - 18, 0x03E0);  // green stem
      fb.fillTriangle(cx, GROUND_Y - 12, cx - 10, GROUND_Y - 20, cx - 2, GROUND_Y - 6, COL_LEAF);
      fb.fillTriangle(cx, GROUND_Y - 16, cx + 10, GROUND_Y - 24, cx + 2, GROUND_Y - 8, COL_LEAF);
      fb.fillCircle(cx - 5, GROUND_Y - 15, 2, COL_LEAF);
      fb.fillCircle(cx + 5, GROUND_Y - 18, 2, COL_LEAF);
      break;
    }
    case ST_SAPLING: {
      // Sapling: trunk + branches + green foliage (no blossoms yet)
      int16_t cx = TRUNK_X + sx;
      fb.fillRect(cx - 3, GROUND_Y - 28, 6, 28, COL_TRUNK);
      fb.fillRect(cx - 2, GROUND_Y - 28, 2, 28, COL_TRUNK_HI);
      fb.drawLine(cx, GROUND_Y - 22, cx - 14, GROUND_Y - 36, COL_TRUNK);
      fb.drawLine(cx, GROUND_Y - 24, cx + 16, GROUND_Y - 38, COL_TRUNK);
      fb.fillCircle(cx, GROUND_Y - 42, 18, COL_LEAF);
      fb.fillCircle(cx - 14, GROUND_Y - 36, 11, COL_LEAF_DK);
      fb.fillCircle(cx + 15, GROUND_Y - 38, 12, COL_LEAF_DK);
      // a couple of early buds
      fb.fillCircle(cx - 8, GROUND_Y - 46, 2, COL_BLOSSOM);
      fb.fillCircle(cx + 10, GROUND_Y - 42, 2, COL_BLOSSOM);
      break;
    }
    case ST_BLOSSOMING:
    case ST_FRUITING: {
      // Mature tree: full trunk, branches, foliage + blossoms (+ cherries)
      int16_t cx = TRUNK_X + sx;
      bool fruiting = (stage_ == ST_FRUITING);
      // trunk (thicker for mature tree)
      int16_t tw = fruiting ? 8 : 6;
      fb.fillRect(cx - tw/2, GROUND_Y - (fruiting ? 56 : 48), tw, fruiting ? 56 : 48, COL_TRUNK);
      fb.fillRect(cx - tw/2 + 1, GROUND_Y - (fruiting ? 56 : 48), 2, fruiting ? 56 : 48, COL_TRUNK_HI);
      // branches
      int16_t bh = fruiting ? 48 : 40;
      fb.drawLine(cx, GROUND_Y - bh + 6, cx - 20, GROUND_Y - bh - 14, COL_TRUNK);
      fb.drawLine(cx, GROUND_Y - bh + 4, cx + 22, GROUND_Y - bh - 16, COL_TRUNK);
      fb.drawLine(cx, GROUND_Y - bh, cx - 6, GROUND_Y - bh - 22, COL_TRUNK);
      fb.drawLine(cx, GROUND_Y - bh, cx + 8, GROUND_Y - bh - 20, COL_TRUNK);
      // foliage cloud
      int16_t fy = GROUND_Y - bh - 26;
      fb.fillCircle(cx, fy, fruiting ? 34 : 28, COL_LEAF);
      fb.fillCircle(cx - 24, fy + 8, fruiting ? 20 : 16, COL_LEAF_DK);
      fb.fillCircle(cx + 26, fy + 6, fruiting ? 22 : 17, COL_LEAF_DK);
      fb.fillCircle(cx - 10, fy - 10, fruiting ? 18 : 14, COL_LEAF);
      fb.fillCircle(cx + 12, fy - 8, fruiting ? 19 : 15, COL_LEAF);
      // blossoms (more when blossoming, fewer when fruiting)
      uint8_t nBlossom = fruiting ? 6 : 14;
      for (uint8_t i = 0; i < nBlossom; i++) {
        int16_t bx = cx - (fruiting ? 30 : 26) + random(fruiting ? 60 : 52);
        int16_t by = fy - 14 + random(fruiting ? 28 : 24);
        fb.fillCircle(bx, by, 4, COL_BLOSSOM);
        fb.fillCircle(bx - 1, by - 1, 1, COL_BLOSSOM_HI);
      }
      // cherries (only when fruiting)
      if (fruiting) {
        uint8_t nCherry = 4 + random(5);  // 4..8 visible cherries
        for (uint8_t i = 0; i < nCherry; i++) {
          int16_t px = cx - 26 + random(52);
          int16_t py = fy - 6 + random(24);
          // stem
          fb.drawLine(px, py - 4, px, py - 1, 0x03E0);
          fb.fillCircle(px, py, 3, COL_CHERRY);
          fb.fillCircle(px - 1, py - 1, 1, COL_CHERRY_HI);
        }
      }
      // Fallen petals on the ground (more when blossoming)
      uint8_t nPetals = fruiting ? 3 : 8;
      for (uint8_t i = 0; i < nPetals; i++) {
        int16_t px = cx - 40 + random(80);
        int16_t py = GROUND_Y + 4 + random(16);
        fb.fillCircle(px, py, 1, COL_BLOSSOM);
      }
      break;
    }
    default: break;
  }

  // Withered overlay when health is critical
  if (phase_ == PH_PLAY && health_ < 25) {
    // brownish tint over the canopy area
    for (int16_t y = GROUND_Y - 90; y < GROUND_Y; y += 2)
      fb.drawFastHLine(TRUNK_X - 40, y, 80, 0x6280);
  }
}

void SakuraGame::drawHud(TFT_eSprite& fb) {
  // Top bar: stage + decade + score
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(1);

  char buf[32];
  fb.setTextColor(COL_TEXT, COL_SKY_TOP);
  snprintf(buf, sizeof(buf), "%s  D:%u", stageName(stage_), decade_);
  fb.drawString(buf, 4, 4);

  fb.setTextColor(COL_BLOSSOM, COL_SKY_TOP);
  snprintf(buf, sizeof(buf), "Score:%u", score_);
  fb.drawString(buf, SW - 52, 4);

  // Health bar
  int16_t hx = 4, hy = 14, hw = SW - 8, hh = 6;
  fb.drawRect(hx, hy, hw, hh, COL_DIM);
  uint16_t hcol = (health_ > 60) ? COL_OK : (health_ > 30) ? COL_WARN : COL_BAD;
  fb.fillRect(hx + 1, hy + 1, (hw - 2) * health_ / 100, hh - 2, hcol);
  fb.setTextColor(COL_DIM, COL_SKY_TOP);
  fb.drawString("HP", hx + hw + 1 - 14, hy - 1);

  // Care stat bars (3 mini bars below health)
  struct StatBar { int8_t val; uint16_t col; const char* lbl; };
  StatBar bars[3] = {
    { moisture_,  COL_WATER, "W" },
    { nutrients_, COL_FERT,  "N" },
    { vigor_,     COL_PRUNE, "V" },
  };
  int16_t by = 24;
  for (int i = 0; i < 3; i++) {
    int16_t bx = 4 + i * 56;
    fb.setTextColor(bars[i].col, COL_SKY_TOP);
    fb.drawString(bars[i].lbl, bx, by);
    fb.drawRect(bx + 8, by, 44, 5, COL_DIM);
    fb.fillRect(bx + 9, by + 1, 42 * bars[i].val / 100, 3, statColor(bars[i].val));
  }
}

void SakuraGame::drawActionMenu(TFT_eSprite& fb) {
  // Bottom action bar: 4 actions, selected one highlighted
  int16_t y = SH - 24;
  int16_t bw = 40, gap = 2;
  int16_t x0 = (SW - (ACT_COUNT * bw + (ACT_COUNT - 1) * gap)) / 2;

  for (uint8_t i = 0; i < ACT_COUNT; i++) {
    int16_t x = x0 + i * (bw + gap);
    bool sel = ((Action)i == action_);
    uint16_t col = actionColor((Action)i);
    // Button box
    fb.fillRect(x, y, bw, 18, sel ? col : 0x0000);
    fb.drawRect(x, y, bw, 18, col);
    // Label
    fb.setTextDatum(MC_DATUM);
    fb.setTextFont(1);
    fb.setTextColor(sel ? 0x0000 : col, sel ? col : 0x0000);
    // Abbreviated label
    const char* lbl = "";
    switch ((Action)i) {
      case ACT_WATER:     lbl = "Wtr"; break;
      case ACT_FERTILIZE: lbl = "Frt"; break;
      case ACT_PRUNE:     lbl = "Prn"; break;
      case ACT_HARVEST:   lbl = "Hrv"; break;
      default: break;
    }
    fb.drawString(lbl, x + bw / 2, y + 9);
  }
  fb.setTextDatum(TL_DATUM);

  // Hint line above the buttons
  fb.setTextFont(1);
  fb.setTextColor(actionFlashMs_ ? actionColor(action_) : COL_DIM, COL_GROUND);
  const char* hint = actionFlashMs_ ? "Done!" : "A:sel B:do";
  fb.setTextDatum(BC_DATUM);
  fb.drawString(hint, SW / 2, y - 4);
  fb.setTextDatum(TL_DATUM);

  // Cycle flash: highlight the selected action briefly
  if (cycleFlashMs_) {
    int16_t x = x0 + action_ * (bw + gap);
    fb.drawRect(x - 1, y - 1, bw + 2, 20, 0xFFFF);
  }

  // Harvest feedback: show cherries picked
  if (action_ == ACT_HARVEST && actionFlashMs_ && isMature()) {
    fb.setTextFont(1);
    fb.setTextColor(COL_CHERRY, COL_GROUND);
    fb.setTextDatum(BC_DATUM);
    char buf[16];
    snprintf(buf, sizeof(buf), "+%u cherries", cherries_);
    fb.drawString(buf, SW / 2, GROUND_Y + 14);
    fb.setTextDatum(TL_DATUM);
  }
}

void SakuraGame::drawDead(TFT_eSprite& fb) {
  // Dark overlay
  fb.fillRect(0, 70, SW, 120, 0x0000);
  fb.drawRect(4, 74, SW - 8, 112, COL_BAD);

  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(COL_BAD, 0x0000);
  fb.drawString("WITHERED", SW / 2, 84);

  fb.setTextFont(2);
  fb.setTextColor(COL_TEXT, 0x0000);
  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %u", score_);
  fb.drawString(buf, SW / 2, 112);
  snprintf(buf, sizeof(buf), "Decades: %u", decade_);
  fb.drawString(buf, SW / 2, 130);
  if (score_ >= best_ && score_ > 0) {
    fb.setTextColor(COL_WARN, 0x0000);
    fb.drawString("NEW BEST!", SW / 2, 148);
  }

  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, 0x0000);
  fb.drawString("any key: Plant again", SW / 2, 170);
  fb.drawString("A long: Menu", SW / 2, 182);
}
