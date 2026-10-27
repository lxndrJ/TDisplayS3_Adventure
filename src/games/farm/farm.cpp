#include "farm.h"

// ================================================================ Layout

static constexpr int16_t SW = 320;
static constexpr int16_t SH = 170;

// Field area: 8 x 4 plots of 16 px = 128 x 64, centred-left.
static constexpr int16_t FIELD_X = 8;
static constexpr int16_t FIELD_Y = 40;
static constexpr int16_t TILE = 16;

// Side panel: field info + controls.
static constexpr int16_t PANEL_X = FIELD_X + FarmGame::COLS * TILE + 8;  // 144
static constexpr int16_t PANEL_W = SW - PANEL_X - 6;                     // 170

// HUD rows
static constexpr int16_t HUD_Y = 4;
static constexpr int16_t MSG_Y = SH - 14;

// Decor: farmhouse 32x32 top-right, sun top-left.
static constexpr int16_t HOUSE_X = SW - 36;
static constexpr int16_t HOUSE_Y = 2;

// Colors (RGB565)
static constexpr uint16_t COL_SKY_TOP = 0x7DEB;  // light blue
static constexpr uint16_t COL_SKY_BOT = 0xCFF7;  // pale horizon
static constexpr uint16_t COL_GRASS   = 0x5ACB;  // grass green
static constexpr uint16_t COL_GRASS_D = 0x3A03;  // dark grass
static constexpr uint16_t COL_PANEL   = 0x2104;  // dark panel bg
static constexpr uint16_t COL_PANEL_L = 0x4A6B;  // panel border
static constexpr uint16_t COL_TEXT    = 0xFFFF;
static constexpr uint16_t COL_DIM     = 0xAD55;
static constexpr uint16_t COL_GOLD    = 0xFEA0;
static constexpr uint16_t COL_OK      = 0x07E0;
static constexpr uint16_t COL_WARN    = 0xFFE0;
static constexpr uint16_t COL_BAD     = 0xF800;
static constexpr uint16_t COL_WATER   = 0x29FF;
static constexpr uint16_t COL_RAIN    = 0x39FF;

// ================================================================ Helpers

static const char* dirName(uint8_t d) {
  switch (d) {
    case 0: return "up";
    case 1: return "right";
    case 2: return "down";
    default: return "left";
  }
}

// ================================================================ Lifecycle

void FarmGame::init() {
  // Landscape: the farm uses the full panel width
  gfx.tft().setRotation(1);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  phase_ = PH_TITLE;
  wantsMenu = false;
}

void FarmGame::quit() {
  fb_.deleteSprite();
}

void FarmGame::startGame() {
  for (uint8_t i = 0; i < FIELD_COUNT; i++) fields_[i] = Field{};
  px_       = FIELD_X + 2 * TILE;   // start on plot (2,0)
  py_       = FIELD_Y;
  dir_      = 2;
  moveMs_   = 0;
  day_      = 0;
  dayMs_    = 0;
  gold_     = 0;
  rain_     = false;
  rainUntil_ = 0;
  msg_[0]   = 0;
  msgMs_    = 0;
  phase_    = PH_PLAY;
}

// ================================================================ Game logic

uint8_t FarmGame::fieldAt(int16_t x, int16_t y) const {
  int16_t fx = (x - FIELD_X) / TILE;
  int16_t fy = (y - FIELD_Y) / TILE;
  if (fx < 0 || fx >= COLS || fy < 0 || fy >= ROWS) return 0xFF;
  return (uint8_t)(fy * COLS + fx);
}

void FarmGame::doAction() {
  // Act on the plot the farmer is standing on
  uint8_t idx = fieldAt(px_, py_);
  if (idx >= FIELD_COUNT) { flash("No plot here"); return; }
  Field& f = fields_[idx];
  switch (f.state) {
    case FS_SOIL:
      f.state = FS_TILLED;
      flash("Dug the soil");
      break;
    case FS_TILLED:
      f.state   = FS_GROWING;
      f.stage   = 0;
      f.watered = false;
      f.dryDays = 0;
      flash("Planted a seed");
      break;
    case FS_GROWING:
      if (!f.watered) {
        f.watered = true;
        flash("Watered");
      } else {
        flash("Already watered");
      }
      break;
    case FS_READY:
      f.state = FS_TILLED;   // harvested: replant-ready soil
      gold_  += HARVEST_VALUE;
      if (gold_ > best_) best_ = gold_;
      flash("+5 gold (harvest!)");
      break;
    case FS_WITHERED:
      f.state = FS_SOIL;
      flash("Cleared the withered crop");
      break;
  }
}

void FarmGame::advanceDay() {
  day_++;
  // Occasional rain event (auto-waters everything, ~1 in 5 days)
  if (random(5) == 0) {
    rain_ = true;
    rainUntil_ = millis() + 1800;
    flash("Rain! All crops watered");
  } else {
    rain_ = false;
  }

  for (uint8_t i = 0; i < FIELD_COUNT; i++) {
    Field& f = fields_[i];
    if (f.state != FS_GROWING) continue;

    if (f.watered || rain_) {
      f.dryDays = 0;
      f.stage++;
      if (f.stage >= GROW_STAGES) f.state = FS_READY;
    } else {
      f.dryDays++;
      if (f.dryDays >= DRY_DAYS_TO_WITHER) f.state = FS_WITHERED;
    }
    // Water from yesterday does not carry over
    f.watered = false;
  }
}

void FarmGame::flash(const char* msg) {
  strncpy(msg_, msg, sizeof(msg_) - 1);
  msg_[sizeof(msg_) - 1] = 0;
  msgMs_ = 1600;
}

// ================================================================ Input

void FarmGame::update(const InputState& in, uint32_t dtMs) {
  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B])
        startGame();
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::B]) { wantsMenu = true; return; }

      // A short = turn clockwise
      if (in.pressed[(uint8_t)Key::A]) {
        dir_ = (dir_ + 1) & 3;
      }

      // Hold A = walk in facing direction (one 1-px step per MOVE_MS)
      if (in.down[(uint8_t)Key::A]) {
        moveMs_ += dtMs;
        while (moveMs_ >= MOVE_MS) {
          moveMs_ -= MOVE_MS;
          int16_t nx = px_, ny = py_;
          switch (dir_) {
            case 0: ny -= 1; break;
            case 1: nx += 1; break;
            case 2: ny += 1; break;
            default: nx -= 1; break;
          }
          // Clamp to the field area
          if (nx < FIELD_X) nx = FIELD_X;
          if (ny < FIELD_Y) ny = FIELD_Y;
          if (nx > FIELD_X + COLS * TILE - TILE)
            nx = FIELD_X + COLS * TILE - TILE;
          if (ny > FIELD_Y + ROWS * TILE - TILE)
            ny = FIELD_Y + ROWS * TILE - TILE;
          px_ = nx;
          py_ = ny;
        }
      } else {
        moveMs_ = 0;
      }

      // B short = act
      if (in.pressed[(uint8_t)Key::B]) doAction();

      // Message timeout
      if (msgMs_ && dtMs)
        msgMs_ = (msgMs_ > dtMs) ? msgMs_ - dtMs : 0;

      // Rain animation timer
      if (rain_ && millis() > rainUntil_) rain_ = false;

      // Day cycle (fast for testing: DAY_MS real ms = 1 in-game day)
      dayMs_ += dtMs;
      while (dayMs_ >= DAY_MS) {
        dayMs_ -= DAY_MS;
        advanceDay();
      }
      break;
    }
  }
}

// ================================================================ Drawing

void FarmGame::draw() {
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
    drawHud(fb_);
    drawPanel(fb_);
  }
  fb_.pushSprite(0, 0);
}

static void drawSprite(TFT_eSprite& fb, const SpriteArt* sp,
                       int16_t x, int16_t y, uint16_t scale = 1) {
  if (!sp) return;
  if (scale == 1) {
    fb.pushImage(x, y, sp->w, sp->h, (uint16_t*)sp->data);
  } else {
    // nearest-neighbour scale
    for (uint8_t sy = 0; sy < sp->h; sy++)
      for (uint8_t sx = 0; sx < sp->w; sx++) {
        uint16_t c = sp->data[sy * sp->w + sx];
        fb.fillRect(x + sx * scale, y + sy * scale, scale, scale, c);
      }
  }
}

void FarmGame::drawTitle(TFT_eSprite& fb) {
  // Sky gradient
  for (int16_t y = 0; y < SH; y++) {
    uint16_t c = (y < SH / 2)
        ? COL_SKY_TOP
        : (uint16_t)(((COL_SKY_BOT & 0xF800) * y + (COL_SKY_TOP & 0xF800) * (SH - y)) / SH & 0xF800)
          | (uint16_t)(((COL_SKY_BOT & 0x07E0) * y + (COL_SKY_TOP & 0x07E0) * (SH - y)) / SH & 0x07E0)
          | (uint16_t)(((COL_SKY_BOT & 0x001F) * y + (COL_SKY_TOP & 0x001F) * (SH - y)) / SH & 0x001F);
    fb.drawFastHLine(0, y, SW, c);
  }
  // Grass ground
  fb.fillRect(0, SH - 60, SW, 60, COL_GRASS);

  // Title
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(0x2945, COL_SKY_TOP);   // dark green
  fb.drawString("SPROUT", SW / 2, 30);
  fb.setTextFont(2);
  fb.setTextColor(0x0000, COL_SKY_TOP);
  fb.drawString("A tiny farm on a tiny display", SW / 2, 62);

  // Demo farm scene: 8x4 field with crops at various stages
  const int16_t dx = SW / 2 - 4 * TILE;
  const int16_t dy = 96;
  static const uint8_t demo[ROWS][COLS] = {
    { SP_TILLED, SP_SPROUT, SP_CROP1, SP_CROP2, SP_CROP3, SP_SOIL, SP_TILLED, SP_CROP2 },
    { SP_CROP3, SP_WITHERED, SP_TILLED, SP_SPROUT, SP_CROP1, SP_CROP3, SP_SOIL, SP_SPROUT },
    { SP_SOIL, SP_CROP1, SP_CROP3, SP_TILLED, SP_SPROUT, SP_WITHERED, SP_CROP2, SP_TILLED },
    { SP_CROP2, SP_SOIL, SP_SPROUT, SP_CROP3, SP_TILLED, SP_CROP1, SP_SOIL, SP_CROP3 },
  };
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      drawSprite(fb, kSprites[demo[r][c]], dx + c * TILE, dy + r * TILE);
  // Demo farmer
  drawSprite(fb, kSprites[SP_FARMER], dx + 3 * TILE, dy + 1 * TILE);

  // Instructions
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(2);
  fb.setTextColor(0x0000, COL_GRASS);
  fb.drawString("Hold A: walk   A: turn   B: dig/plant/water/harvest", SW / 2, SH - 40);
  fb.setTextColor(0xFFE0, COL_GRASS);
  fb.drawString("B long: Menu", SW / 2, SH - 22);
  fb.setTextColor(0x07E0, COL_GRASS);
  fb.drawString("Press any button to start", SW / 2, SH - 6);
}

void FarmGame::drawScene(TFT_eSprite& fb) {
  // Sky
  for (int16_t y = 0; y < FIELD_Y; y++) {
    uint16_t t = y * 255 / (FIELD_Y > 1 ? FIELD_Y - 1 : 1);
    uint16_t c = ((COL_SKY_TOP & 0xF800) * (255 - t) + (COL_SKY_BOT & 0xF800) * t) / 255 & 0xF800
               | ((COL_SKY_TOP & 0x07E0) * (255 - t) + (COL_SKY_BOT & 0x07E0) * t) / 255 & 0x07E0
               | ((COL_SKY_TOP & 0x001F) * (255 - t) + (COL_SKY_BOT & 0x001F) * t) / 255 & 0x001F;
    fb.drawFastHLine(0, y, SW, c);
  }
  // Sun
  fb.fillCircle(24, 18, 9, 0xFFE0);
  fb.fillCircle(24, 18, 6, 0xFFC0);

  // Grass below the field
  int16_t grassTop = FIELD_Y + ROWS * TILE;
  fb.fillRect(0, grassTop, SW, SH - grassTop, COL_GRASS);
  // grass tufts
  for (int16_t x = 4; x < SW; x += 17) {
    int16_t gy = grassTop + 4 + (x / 17) % 3 * 3;
    fb.drawLine(x, gy, x, gy - 3, COL_GRASS_D);
  }

  // Farmhouse (32x32, drawn at 1x)
  drawSprite(fb, kSprites[SP_HOUSE], HOUSE_X, HOUSE_Y);

  // Field plots
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      const Field& f = fields_[r * COLS + c];
      SpriteId sp = SP_SOIL;
      switch (f.state) {
        case FS_SOIL:     sp = SP_SOIL; break;
        case FS_TILLED:   sp = SP_TILLED; break;
        case FS_GROWING:  sp = (SpriteId)(SP_SPROUT + f.stage); break;
        case FS_READY:    sp = SP_CROP3; break;
        case FS_WITHERED: sp = SP_WITHERED; break;
      }
      int16_t x = FIELD_X + c * TILE;
      int16_t y = FIELD_Y + r * TILE;
      drawSprite(fb, kSprites[sp], x, y);
      // Wet soil highlight (watered today)
      if (f.state == FS_GROWING && f.watered)
        fb.drawFastHLine(x + 1, y + TILE - 2, TILE - 2, COL_WATER);
      // Ripe sparkle
      if (f.state == FS_READY && (millis() / 400) % 2)
        fb.drawPixel(x + 2, y + 2, 0xFFFF);
      // Withered marker
      if (f.state == FS_WITHERED)
        fb.drawPixel(x + TILE - 3, y + 2, COL_BAD);
    }
  }

  // Rain overlay
  if (rain_) {
    for (int16_t x = 0; x < SW; x += 12)
      fb.drawLine(x + (millis() / 30) % 12, FIELD_Y - 8,
                  x + (millis() / 30) % 12 - 2, FIELD_Y + ROWS * TILE + 8,
                  COL_RAIN);
  }

  // Farmer
  drawSprite(fb, kSprites[SP_FARMER], px_, py_);

  // Facing indicator (small arrow above the farmer)
  {
    int16_t ax = px_ + TILE / 2, ay = py_ - 4;
    switch (dir_) {
      case 0: fb.fillTriangle(ax, ay - 3, ax - 3, ay + 2, ax + 3, ay + 2, COL_TEXT); break;
      case 1: fb.fillTriangle(ax + 3, ay, ax - 2, ay - 3, ax - 2, ay + 3, COL_TEXT); break;
      case 2: fb.fillTriangle(ax, ay + 3, ax - 3, ay - 2, ax + 3, ay - 2, COL_TEXT); break;
      default: fb.fillTriangle(ax - 3, ay, ax + 2, ay - 3, ax + 2, ay + 3, COL_TEXT); break;
    }
  }

  // Field border
  fb.drawRect(FIELD_X - 1, FIELD_Y - 1, COLS * TILE + 2, ROWS * TILE + 2,
              COL_PANEL_L);

  // Message line
  if (msgMs_ > 0) {
    fb.setTextDatum(TL_DATUM);
    fb.setTextFont(1);
    fb.setTextColor(COL_WARN, COL_GRASS);
    fb.drawString(msg_, 4, MSG_Y);
  }
}

const char* FarmGame::fieldInfo(uint8_t idx, char* buf, size_t len) const {
  if (idx >= FIELD_COUNT) {
    snprintf(buf, len, "walk with A, act with B");
    return buf;
  }
  const Field& f = fields_[idx];
  switch (f.state) {
    case FS_SOIL:     snprintf(buf, len, "fallow soil - B: dig"); break;
    case FS_TILLED:   snprintf(buf, len, "tilled - B: plant seed"); break;
    case FS_GROWING: {
      const char* st = f.stage == 0 ? "sprout" : f.stage == 1 ? "growing" : "almost ripe";
      snprintf(buf, len, "%s (%d/%d)%s", st, f.stage + 1, GROW_STAGES,
               f.watered ? " [wet]" : " [dry]");
      break;
    }
    case FS_READY:    snprintf(buf, len, "RIPE! - B: harvest +%d", HARVEST_VALUE); break;
    case FS_WITHERED: snprintf(buf, len, "withered - B: clear"); break;
    default:          snprintf(buf, len, "?"); break;
  }
  return buf;
}

void FarmGame::drawHud(TFT_eSprite& fb) {
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(2);
  // Day counter + gold + watering can icon
  fb.setTextColor(COL_TEXT, COL_SKY_TOP);
  char b[24];
  snprintf(b, sizeof(b), "Day %u", (unsigned)day_);
  fb.drawString(b, 40, HUD_Y);
  snprintf(b, sizeof(b), "Gold %u", (unsigned)gold_);
  fb.setTextColor(COL_GOLD, COL_SKY_TOP);
  fb.drawString(b, 110, HUD_Y);
  drawSprite(fb, kSprites[SP_WATERCAN], 180, HUD_Y - 4);
  snprintf(b, sizeof(b), "Best %u", (unsigned)best_);
  fb.setTextColor(COL_DIM, COL_SKY_TOP);
  fb.drawString(b, 200, HUD_Y);
  // Rain indicator
  if (rain_) {
    fb.setTextColor(COL_WATER, COL_SKY_TOP);
    fb.drawString("RAIN!", 262, HUD_Y);
  }
}

void FarmGame::drawPanel(TFT_eSprite& fb) {
  // Side panel with plot info and controls
  fb.fillRect(PANEL_X, FIELD_Y, PANEL_W, ROWS * TILE, COL_PANEL);
  fb.drawRect(PANEL_X, FIELD_Y, PANEL_W, ROWS * TILE, COL_PANEL_L);

  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(1);
  fb.setTextColor(COL_TEXT, COL_PANEL);
  fb.drawString("SPROUT FARM", PANEL_X + 4, FIELD_Y + 4);
  fb.setTextColor(COL_DIM, COL_PANEL);
  fb.drawString("hold A: walk", PANEL_X + 4, FIELD_Y + 16);
  fb.drawString("A: turn", PANEL_X + 4, FIELD_Y + 26);
  fb.drawString("B: act on plot", PANEL_X + 4, FIELD_Y + 36);
  fb.drawString("B long: menu", PANEL_X + 4, FIELD_Y + 46);

  // Facing direction
  fb.setTextColor(COL_OK, COL_PANEL);
  char b[24];
  snprintf(b, sizeof(b), "facing: %s", dirName(dir_));
  fb.drawString(b, PANEL_X + 4, FIELD_Y + 58);

  // Plot under the farmer
  uint8_t idx = fieldAt(px_, py_);
  char info[40];
  fieldInfo(idx, info, sizeof(info));
  fb.setTextColor(COL_WARN, COL_PANEL);
  fb.drawString(info, PANEL_X + 4, FIELD_Y + 70);

  // Legend: growth stages
  fb.setTextColor(COL_DIM, COL_PANEL);
  fb.drawString("growth:", PANEL_X + 4, FIELD_Y + 84);
  for (uint8_t s = 0; s < GROW_STAGES + 1; s++) {
    drawSprite(fb, kSprites[SP_SPROUT + s], PANEL_X + 4 + s * 18, FIELD_Y + 94);
  }
  fb.drawString("water daily or", PANEL_X + 4, FIELD_Y + 112);
  fb.drawString("it withers!", PANEL_X + 4, FIELD_Y + 122);

  // Coin icon in the panel footer
  drawSprite(fb, kSprites[SP_COIN], PANEL_X + PANEL_W - 20, FIELD_Y + ROWS * TILE - 20);
}
