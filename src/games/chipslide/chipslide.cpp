#include "chipslide.h"

// ---------------------------------------------------------------- Layout

// Portrait 170x320: board on top, HUD (arrow + blueprint miniature) below.
static constexpr int16_t SW = 170;
static constexpr int16_t SH = 320;

static constexpr int16_t GRID_X  = 8;    // board top-left
static constexpr int16_t GRID_Y  = 26;
static constexpr int16_t CELL    = 38;   // tile edge length
static constexpr int16_t GAP     = 1;    // gap between tiles
static constexpr int16_t STEP    = CELL + GAP;
static constexpr int16_t GRID_W  = 4 * STEP - GAP;  // 155

static constexpr int16_t HUD_Y   = 196;  // HUD-Beginn
static constexpr int16_t MINI_X  = 122;  // blueprint miniature (right)
static constexpr int16_t MINI_C  = 9;    // miniature tile
static constexpr int16_t MINI_S  = MINI_C + 1;

// Neon palette (RGB565), matching the engine menu and NeonShard
static const uint16_t COL_BG     = 0x0008;  // near black-blue
static const uint16_t COL_GRID   = 0x39E7;  // trace cyan
static const uint16_t COL_TILE   = 0x10C3;  // chip package
static const uint16_t COL_TILE2  = 0x0861;  // package shadow
static const uint16_t COL_ACCENT = 0x07FF;  // neon cyan
static const uint16_t COL_WARM   = 0xFFE0;  // neon yellow (arrow)
static const uint16_t COL_OK     = 0x07E0;  // neon green (solved)
static const uint16_t COL_DIM    = 0x630C;  // dim cyan (text)

// Symbol IDs: 0..14 components, 15 = gap (EMPTY)
enum : uint8_t {
  SYM_RESISTOR, SYM_DIODE, SYM_LED, SYM_TRANSISTOR,
  SYM_IC, SYM_CAPACITOR, SYM_BATTERY, SYM_INDUCTOR,
  SYM_CRYSTAL, SYM_SWITCH, SYM_RELAY, SYM_FUSE,
  SYM_ANTENNA, SYM_SPEAKER, SYM_GND,
  SYM_COUNT  // = 15 = EMPTY
};

static const char* const kSymNames[SYM_COUNT] = {
  "R", "D", "LED", "TR", "IC", "C", "BAT", "L",
  "Q", "SW", "RLY", "F", "ANT", "SPK", "GND"
};

static int16_t cellX(uint8_t i) { return GRID_X + (i % 4) * STEP; }
static int16_t cellY(uint8_t i) { return GRID_Y + (i / 4) * STEP; }
static int16_t cellCx(uint8_t i) { return cellX(i) + CELL / 2; }
static int16_t cellCy(uint8_t i) { return cellY(i) + CELL / 2; }

// ---------------------------------------------------------------- Spielstart

void ChipSlideGame::init() {
  // Portrait stays set (board default rotation 0)
  gfx.tft().setRotation(board::TFT_ROTATION);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  phase_       = PH_TITLE;
  animating_   = false;
  wantsMenu    = false;
  lastMoveMs_  = 0;
  repeatFired_ = false;
  dirFlashMs_  = 0;
  winT_        = 0;
}

void ChipSlideGame::quit() {
  fb_.deleteSprite();
}

void ChipSlideGame::startGame() {
  // solved base layout, then shuffle
  for (uint8_t i = 0; i < CELLS; i++) board_[i] = i;
  empty_ = EMPTY;
  shuffle();
  moves_       = 0;
  dir_         = DIR_RIGHT;
  animating_   = false;
  lastMoveMs_  = 0;
  repeatFired_ = false;
  dirFlashMs_  = 0;
  phase_       = PH_PLAY;
  countSolved();
  winT_        = 0;
}

void ChipSlideGame::shuffle() {
  // Random gap moves - guarantees a solvable layout and prevents
  // the start layout from being already solved.
  // Here dir_ is the gap's movement direction (opposite of the tile).
  for (uint16_t i = 0; i < SHUFFLE_MOVES; i++) {
    uint8_t choices[DIR_COUNT];
    uint8_t n = 0;
    for (uint8_t d = 0; d < DIR_COUNT; d++) {
      if (d == DIR_RIGHT && (empty_ % 4) == 3) continue;
      if (d == DIR_LEFT  && (empty_ % 4) == 0) continue;
      if (d == DIR_DOWN  && empty_ >= 12)        continue;
      if (d == DIR_UP    && empty_ < 4)          continue;
      choices[n++] = d;
    }
    if (n == 0) continue;
    uint8_t d = choices[random(n)];
    uint8_t to = empty_;
    switch (d) {
      case DIR_RIGHT: to = empty_ + 1; break;
      case DIR_LEFT:  to = empty_ - 1; break;
      case DIR_DOWN:  to = empty_ + 4; break;
      case DIR_UP:    to = empty_ - 4; break;
    }
    board_[empty_] = board_[to];
    board_[to]     = EMPTY;
    empty_         = to;
  }
  // The gap never lands in the goal corner (otherwise almost solved)
  if (empty_ == EMPTY) {
    uint8_t to = EMPTY - 1;
    board_[EMPTY] = board_[to];
    board_[to]    = EMPTY;
    empty_        = to;
  }
}

// ---------------------------------------------------------------- Logik
// dir_ describes the direction the tile is pushed: the tile must sit
// on the opposite side of the gap and glides into it.

bool ChipSlideGame::canMove(uint8_t dir) const {
  switch (dir) {
    case DIR_RIGHT: return (empty_ % 4) != 0;  // tile left of the gap
    case DIR_LEFT:  return (empty_ % 4) != 3;  // tile right of the gap
    case DIR_DOWN:  return empty_ >= 4;        // tile above the gap
    case DIR_UP:    return empty_ < 12;        // tile below the gap
    default:        return false;
  }
}

bool ChipSlideGame::tryMove() {
  if (!canMove(dir_) || animating_) return false;
  uint8_t from = empty_;
  switch (dir_) {
    case DIR_RIGHT: from = empty_ - 1; break;
    case DIR_LEFT:  from = empty_ + 1; break;
    case DIR_DOWN:  from = empty_ - 4; break;
    case DIR_UP:    from = empty_ + 4; break;
    default:        return false;
  }
  animFrom_  = from;
  animTo_    = empty_;
  animSym_   = board_[from];
  animMs_    = 0;
  animating_ = true;
  moves_++;
  lastMoveMs_  = millis();
  repeatFired_ = false;
  return true;
}

void ChipSlideGame::finishMove() {
  // After the move the tile sits at the target cell (animTo_), the gap
  // moves into its origin cell (animFrom_).
  board_[animFrom_] = EMPTY;
  board_[animTo_]   = animSym_;
  empty_     = animFrom_;
  animating_ = false;
  countSolved();
  if (solved_ == SYM_COUNT && empty_ == EMPTY) {
    phase_ = PH_WIN;
    winT_  = millis();
  }
}

void ChipSlideGame::countSolved() {
  // Only the 15 components count; the gap is at its goal only when it
  // sits in the bottom-right corner (checked separately).
  uint8_t n = 0;
  for (uint8_t i = 0; i < SYM_COUNT; i++) if (board_[i] == i) n++;
  solved_ = n;
}

// ---------------------------------------------------------------- Eingabe

void ChipSlideGame::update(const InputState& in, uint32_t dtMs) {
  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::B])
        startGame();
      break;

    case PH_PLAY: {
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; return; }

      // A short: cycle direction (arrow flashes as feedback)
      if (in.pressed[(uint8_t)Key::A]) {
        dir_ = (uint8_t)((dir_ + 1) % DIR_COUNT);
        dirFlashMs_ = 260;
      }
      if (dirFlashMs_ && dtMs)
        dirFlashMs_ = (dirFlashMs_ > dtMs) ? dirFlashMs_ - dtMs : 0;

      // Slide animation running: finish after ANIM_MS
      if (animating_) {
        animMs_ = (uint16_t)min((uint32_t)animMs_ + dtMs, (uint32_t)ANIM_MS);
        if (animMs_ >= ANIM_MS) finishMove();
        return;  // no further moves during the animation
      }

      // B: slide tile - short press = one move, holding = after a short
      // pause automatically slide on (smooth flow of play). After
      // holding, don't react to the release again.
      bool wantMove = in.pressed[(uint8_t)Key::B] && !repeatFired_;
      if (in.down[(uint8_t)Key::B] && !repeatFired_) {
        if (in.holdMs[(uint8_t)Key::B] >= REPEAT_FIRST_MS) {
          wantMove     = true;
          repeatFired_ = true;
        }
      }
      if (in.down[(uint8_t)Key::B] && repeatFired_) {
        if (millis() - lastMoveMs_ >= REPEAT_MS) wantMove = true;
      }
      if (!in.down[(uint8_t)Key::B]) repeatFired_ = false;
      if (wantMove) tryMove();
      break;
    }

    case PH_WIN:
      if (in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B])
        wantsMenu = true;
      else if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B])
        startGame();  // again from the start
      break;
  }
}

// ---------------------------------------------------------------- Zeichnen

void ChipSlideGame::draw() {
  if (!fb_.created()) {
    TFT_eSPI& tft = gfx.tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Framebuffer alloc failed", 6, 8);
    return;
  }
  fb_.fillScreen(COL_BG);
  if (phase_ == PH_TITLE) {
    drawTitle(fb_);
  } else {
    drawGrid(fb_);
    drawHud(fb_);
    if (phase_ == PH_WIN) drawWin(fb_);
  }
  fb_.pushSprite(0, 0);
}

void ChipSlideGame::drawTitle(TFT_eSprite& fb) {
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(COL_ACCENT, COL_BG);
  fb.drawString("CHIP SLIDE", SW / 2, 40);
  fb.setTextFont(2);
  fb.setTextColor(0xFFFF, COL_BG);
  fb.drawString("Sort the components", SW / 2, 80);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("Goal: the blueprint", SW / 2, 102);

  // blueprint miniature as preview
  for (uint8_t i = 0; i < CELLS; i++) {
    int16_t x = SW / 2 - 2 * MINI_S + (i % 4) * MINI_S;
    int16_t y = 128 + (i / 4) * MINI_S;
    uint16_t c = (i == EMPTY) ? COL_BG : COL_TILE;
    fb.fillRect(x, y, MINI_C, MINI_C, c);
    fb.drawRect(x, y, MINI_C, MINI_C, COL_GRID);
  }

  fb.setTextFont(2);
  fb.setTextColor(COL_WARM, COL_BG);
  fb.drawString("A: Direction", SW / 2, 186);
  fb.drawString("B: Slide tile", SW / 2, 206);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("any key: Start", SW / 2, 288);
}

void ChipSlideGame::drawGrid(TFT_eSprite& fb) {
  // heading
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(2);
  fb.setTextColor(COL_ACCENT, COL_BG);
  fb.drawString("CHIP SLIDE", 8, 6);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("M:", SW - 62, 6);
  fb.setTextColor(0xFFFF, COL_BG);
  fb.drawNumber(moves_, SW - 34, 6);

  // During the animation the gap already sits at its target
  uint8_t hole = animating_ ? animTo_ : empty_;
  fb.fillRect(cellX(hole), cellY(hole), CELL, CELL, 0x0000);
  fb.drawRect(cellX(hole), cellY(hole), CELL, CELL, 0x2104);

  for (uint8_t i = 0; i < CELLS; i++) {
    if (i == empty_ || i == hole) continue;
    if (animating_ && (i == animFrom_ || i == animTo_)) continue;
    bool locked = (board_[i] == i);              // correctly placed
    uint16_t col = locked ? COL_OK : COL_TILE;
    int16_t x = cellX(i), y = cellY(i);
    fb.fillRect(x, y, CELL, CELL, COL_TILE2);
    fb.fillRect(x, y, CELL - 2, CELL - 2, col);
    fb.drawRect(x, y, CELL, CELL, locked ? COL_OK : COL_GRID);
    drawSymbol(fb, board_[i], cellCx(i), cellCy(i),
               locked ? 0x0000 : 0xFFFF, col);
  }

  // Sliding tile: interpolated position between animFrom_ and animTo_
  if (animating_) {
    float t = animMs_ / (float)ANIM_MS;
    // ease-out: fast start, soft settle
    t = 1.0f - (1.0f - t) * (1.0f - t);
    int16_t x = (int16_t)(cellX(animFrom_) + (cellX(animTo_) - cellX(animFrom_)) * t);
    int16_t y = (int16_t)(cellY(animFrom_) + (cellY(animTo_) - cellY(animFrom_)) * t);
    fb.fillRect(x, y, CELL, CELL, COL_TILE2);
    fb.fillRect(x, y, CELL - 2, CELL - 2, COL_ACCENT);
    fb.drawRect(x, y, CELL, CELL, COL_WARM);
    drawSymbol(fb, animSym_, x + CELL / 2, y + CELL / 2, 0x0000, COL_ACCENT);
  }
}

void ChipSlideGame::drawHud(TFT_eSprite& fb) {
  // Trennlinie
  fb.drawFastHLine(0, HUD_Y - 4, SW, 0x1082);

  // Direction arrow (left) - flashes briefly after A press as feedback
  int16_t ax = 30, ay = HUD_Y + 34;
  uint16_t pcol = (dirFlashMs_ && (millis() / 60) % 2 == 0) ? 0xFFFF : COL_WARM;
  fb.drawCircle(ax, ay, 21, COL_GRID);
  switch (dir_) {
    case DIR_RIGHT:
      fb.fillTriangle(ax + 8, ay - 8, ax + 8, ay + 8, ax + 16, ay, pcol);
      break;
    case DIR_DOWN:
      fb.fillTriangle(ax - 8, ay + 8, ax + 8, ay + 8, ax, ay + 16, pcol);
      break;
    case DIR_LEFT:
      fb.fillTriangle(ax - 8, ay - 8, ax - 8, ay + 8, ax - 16, ay, pcol);
      break;
    case DIR_UP:
      fb.fillTriangle(ax - 8, ay - 8, ax + 8, ay - 8, ax, ay - 16, pcol);
      break;
  }
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("Direction", ax, ay + 26);

  // center: moves + progress
  fb.setTextFont(2);
  fb.setTextColor(COL_WARM, COL_BG);
  fb.drawNumber(moves_, 85, HUD_Y + 6);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("Moves", 85, HUD_Y + 24);
  fb.setTextColor(0x7BEF, COL_BG);
  char pr[12];
  snprintf(pr, sizeof(pr), "%u/15", solved_);
  fb.drawString(pr, 85, HUD_Y + 40);

  // Blueprint miniature (right): green = component at goal position
  int16_t mx = MINI_X, my = HUD_Y + 8;
  fb.setTextDatum(TL_DATUM);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("Blueprint", mx - 6, HUD_Y - 2);
  for (uint8_t i = 0; i < CELLS; i++) {
    int16_t x = mx + (i % 4) * MINI_S;
    int16_t y = my + (i / 4) * MINI_S;
    uint16_t c;
    if (i == EMPTY) c = (empty_ == EMPTY) ? COL_OK : COL_BG;
    else            c = (board_[i] == i) ? COL_OK : COL_TILE;
    fb.fillRect(x, y, MINI_C, MINI_C, c);
    fb.drawRect(x, y, MINI_C, MINI_C, (c == COL_OK) ? COL_OK : 0x2104);
  }

  // hint line
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("B: slide  A long: Menu", 6, SH - 11);
}

void ChipSlideGame::drawWin(TFT_eSprite& fb) {
  // band over the lower third
  uint32_t sinceWin = millis() - winT_;
  fb.fillRect(0, HUD_Y - 4, SW, SH - HUD_Y + 4, COL_BG);
  fb.drawFastHLine(0, HUD_Y - 4, SW, COL_WARM);

  // Pulsing frame around the finished board
  bool pulse = (sinceWin / 120) % 2 == 0;
  fb.drawRect(GRID_X - 3, GRID_Y - 3, GRID_W + 6, GRID_W + 6,
              pulse ? COL_WARM : COL_OK);

  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(pulse ? COL_WARM : COL_ACCENT, COL_BG);
  fb.drawString("SOLVED!", SW / 2, HUD_Y + 12);
  fb.setTextFont(2);
  fb.setTextColor(0xFFFF, COL_BG);
  fb.drawString("Circuit live", SW / 2, HUD_Y + 44);
  char s[16];
  snprintf(s, sizeof(s), "%u moves", moves_);
  fb.drawString(s, SW / 2, HUD_Y + 64);
  fb.setTextFont(1);
  fb.setTextColor(COL_DIM, COL_BG);
  fb.drawString("any key: again | long: Menu", SW / 2, SH - 11);
}

// ---------------------------------------------------------------- Symbole
// Each component is drawn as a simplified schematic symbol into the tile.
// cx/cy = tile center, col = symbol color, bg = tile inner color (label).

void ChipSlideGame::drawSymbol(TFT_eSprite& fb, uint8_t id, int16_t cx,
                                int16_t cy, uint16_t col, uint16_t bg) const {
  const int16_t L = 13;  // halbe Symbol-Ausdehnung (Zuleitungen)
  switch (id) {
    case SYM_RESISTOR: {
      // zigzag between two leads
      fb.drawFastHLine(cx - L, cy, L - 8, col);
      fb.drawFastHLine(cx + 8, cy, L - 8, col);
      fb.drawLine(cx - 8, cy, cx - 5, cy - 6, col);
      fb.drawLine(cx - 5, cy - 6, cx - 1, cy + 6, col);
      fb.drawLine(cx - 1, cy + 6, cx + 3, cy - 6, col);
      fb.drawLine(cx + 3, cy - 6, cx + 6, cy + 6, col);
      fb.drawLine(cx + 6, cy + 6, cx + 8, cy, col);
      break;
    }
    case SYM_DIODE: {
      // triangle + barrier line
      fb.fillTriangle(cx - 6, cy - 8, cx - 6, cy + 8, cx + 7, cy, col);
      fb.drawFastVLine(cx + 9, cy - 8, 16, col);
      fb.drawFastHLine(cx - L, cy, 7, col);
      fb.drawFastHLine(cx + 11, cy, L - 11, col);
      break;
    }
    case SYM_LED: {
      // diode + two emission arrows
      fb.fillTriangle(cx - 5, cy - 7, cx - 5, cy + 7, cx + 6, cy, col);
      fb.drawFastVLine(cx + 8, cy - 7, 14, col);
      fb.drawLine(cx + 3, cy - 7, cx + 10, cy - 14, col);
      fb.drawLine(cx + 3, cy + 7, cx + 10, cy + 14, col);
      fb.fillTriangle(cx + 12, cy - 15, cx + 8, cy - 13, cx + 12, cy - 11, col);
      fb.fillTriangle(cx + 12, cy + 15, cx + 8, cy + 11, cx + 12, cy + 13, col);
      fb.drawFastHLine(cx - L, cy, 8, col);
      fb.drawFastHLine(cx + 10, cy, L - 10, col);
      break;
    }
    case SYM_TRANSISTOR: {
      // circle + base bar + collector/emitter
      fb.drawCircle(cx, cy, 12, col);
      fb.drawFastVLine(cx - 4, cy - 7, 14, col);
      fb.drawLine(cx - 4, cy - 6, cx + 7, cy - 11, col);
      fb.drawLine(cx - 4, cy + 6, cx + 7, cy + 11, col);
      fb.drawFastHLine(cx - L, cy, L - 4, col);
      break;
    }
    case SYM_IC: {
      // rectangle with 3 pins per side + pin-1 marker
      fb.fillRect(cx - 8, cy - 10, 16, 20, col);
      fb.fillRect(cx - 1, cy - 12, 3, 3, col);
      for (int i = 0; i < 3; i++) {
        int16_t py = cy - 6 + i * 6;
        fb.drawFastHLine(cx - L, py, L - 8, col);
        fb.drawFastHLine(cx + 8, py, L - 8, col);
      }
      break;
    }
    case SYM_CAPACITOR: {
      // two parallel plates
      fb.drawFastVLine(cx - 2, cy - 9, 18, col);
      fb.drawFastVLine(cx + 2, cy - 9, 18, col);
      fb.drawFastHLine(cx - L, cy, L - 2, col);
      fb.drawFastHLine(cx + 2, cy, L - 2, col);
      break;
    }
    case SYM_BATTERY: {
      // long + short plate, double set
      fb.drawFastVLine(cx - 6, cy - 10, 20, col);
      fb.drawFastVLine(cx - 2, cy - 6, 12, col);
      fb.drawFastVLine(cx + 2, cy - 10, 20, col);
      fb.drawFastVLine(cx + 6, cy - 6, 12, col);
      fb.drawFastHLine(cx - L, cy, L - 6, col);
      fb.drawFastHLine(cx + 6, cy, L - 6, col);
      break;
    }
    case SYM_INDUCTOR: {
      // three half arcs over the lead (0 deg = 6 o'clock, 180 = 12 o'clock)
      fb.drawFastHLine(cx - L, cy, L - 11, col);
      fb.drawFastHLine(cx + 11, cy, L - 11, col);
      for (int i = 0; i < 3; i++) {
        int16_t bx = cx - 9 + i * 6;
        fb.drawArc(bx + 3, cy, 5, 4, 90, 270, col, bg);
      }
      break;
    }
    case SYM_CRYSTAL: {
      // rectangle between two plates (crystal)
      fb.fillRect(cx - 4, cy - 6, 8, 12, col);
      fb.drawFastVLine(cx - 8, cy - 9, 18, col);
      fb.drawFastVLine(cx + 8, cy - 9, 18, col);
      fb.drawFastHLine(cx - L, cy, L - 8, col);
      fb.drawFastHLine(cx + 8, cy, L - 8, col);
      break;
    }
    case SYM_SWITCH: {
      // open switch
      fb.drawCircle(cx - 8, cy + 6, 2, col);
      fb.drawCircle(cx + 8, cy + 6, 2, col);
      fb.drawLine(cx - 8, cy + 6, cx + 6, cy - 8, col);
      fb.drawFastHLine(cx - L, cy + 6, L - 8, col);
      fb.drawFastHLine(cx + 8, cy + 6, L - 8, col);
      break;
    }
    case SYM_RELAY: {
      // coil + switching contact
      fb.drawRect(cx - 11, cy - 10, 12, 20, col);
      for (int i = 0; i < 3; i++)
        fb.drawFastHLine(cx - 11, cy - 6 + i * 5, 12, col);
      fb.drawLine(cx + 1, cy + 8, cx + 10, cy - 2, col);
      fb.drawCircle(cx + 10, cy + 8, 2, col);
      fb.drawFastHLine(cx + 1, cy + 8, 5, col);
      fb.drawFastHLine(cx + 12, cy + 8, L - 12, col);
      break;
    }
    case SYM_FUSE: {
      // rectangle with a continuous line (fuse)
      fb.drawRect(cx - 8, cy - 5, 16, 10, col);
      fb.drawFastHLine(cx - L, cy, 2 * L + 1, col);
      break;
    }
    case SYM_ANTENNA: {
      // rod + two wave arcs to the right
      fb.drawFastVLine(cx, cy - 12, 20, col);
      fb.fillCircle(cx, cy - 12, 2, col);
      fb.drawFastHLine(cx - 5, cy + 8, 10, col);
      fb.drawArc(cx, cy - 4, 8, 7, 200, 340, col, bg);
      fb.drawArc(cx, cy - 4, 12, 11, 200, 340, col, bg);
      break;
    }
    case SYM_SPEAKER: {
      // speaker box + diaphragm + sound waves
      fb.fillRect(cx - 10, cy - 6, 6, 12, col);
      fb.fillTriangle(cx - 4, cy - 6, cx - 4, cy + 6, cx + 4, cy + 10, col);
      fb.fillTriangle(cx - 4, cy - 6, cx - 4, cy + 6, cx + 4, cy - 10, col);
      fb.drawArc(cx + 7, cy, 7, 6, 180, 360, col, bg);
      fb.drawArc(cx + 7, cy, 12, 11, 180, 360, col, bg);
      break;
    }
    case SYM_GND: {
      // ground symbol: three decreasing strokes
      fb.drawFastVLine(cx, cy - 12, 10, col);
      fb.drawFastHLine(cx - 3, cy - 2, 6, col);
      fb.drawFastHLine(cx - 7, cy + 3, 14, col);
      fb.drawFastHLine(cx - 11, cy + 8, 22, col);
      break;
    }
    default:
      fb.drawRect(cx - 6, cy - 6, 12, 12, col);
      break;
  }

  // short label under the symbol (font 1, 8px)
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(1);
  fb.setTextColor(col, bg);
  fb.drawString(kSymNames[id], cx, cy + 14);
}
