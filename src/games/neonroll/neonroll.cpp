#include "neonroll.h"

// ================================================================ Layout

static constexpr int16_t SW = 170;
static constexpr int16_t SH = 320;

// HUD occupies the top ~26 px, playfield below.
static constexpr int16_t HUD_H = 26;

// Neon marble physics tuning
static constexpr float BOUNCE      = 0.62f;  // wall restitution
static constexpr float BUMPER_BOOST = 1.25f; // bumper energy gain
static constexpr float V_MAX       = 320.0f; // speed cap px/s
static constexpr float CAPTURE_V   = 110.0f; // max speed to fall into goal

// ================================================================ Themes
// 8 levels, each with its own neon palette + wall rendering style.
// wallStyle: 0 = solid block, 1 = block with rivet highlight, 2 = rounded
const NeonRollGame::Theme NeonRollGame::kThemes[LEVEL_COUNT] = {
  // 0: Neon Grid (cyan)
  { 0x0008, 0x10C3, 0x39E7, 0x07FF, 0x04F0, 0xFFE0, 0xF800, 0x0210, "Neon Grid" },
  // 1: Magenta Maze
  { 0x0801, 0x3016, 0xF81F, 0xF81F, 0xD81F, 0x07E0, 0x07FF, 0x2104, "Magenta Maze" },
  // 2: Toxic Green
  { 0x0004, 0x0861, 0x07E0, 0x07E0, 0x03E0, 0xFFE0, 0xF81F, 0x0188, "Toxic Green" },
  // 3: Solar Flare (orange/yellow)
  { 0x2000, 0x6280, 0xFD20, 0xFFE0, 0xFC00, 0x07FF, 0xF81F, 0x4208, "Solar Flare" },
  // 4: Ice Cave (white/blue)
  { 0x0006, 0x2104, 0x9DD7, 0xFFFF, 0x867F, 0x07FF, 0xFFE0, 0x0410, "Ice Cave" },
  // 5: Purple Haze
  { 0x0802, 0x2811, 0xB81F, 0xC618, 0x8410, 0x07FF, 0x07E0, 0x2104, "Purple Haze" },
  // 6: Laser Red
  { 0x0800, 0x6000, 0xF800, 0xF800, 0xC000, 0x07FF, 0xFFE0, 0x2100, "Laser Red" },
  // 7: Gold Rush
  { 0x1800, 0x6320, 0xFE60, 0xFFE0, 0xFD20, 0x07E0, 0xF800, 0x3200, "Gold Rush" },
};

// wallStyle per level (visual variety in wall rendering)
static constexpr uint8_t kWallStyle[NeonRollGame::LEVEL_COUNT] = {
  0, 1, 0, 2, 1, 0, 2, 1
};

// ================================================================ Helpers

static inline int16_t cellLeft(uint8_t c) { return NeonRollGame::FIELD_X + c * NeonRollGame::CELL; }
static inline int16_t cellTop(uint8_t r)  { return NeonRollGame::FIELD_Y + r * NeonRollGame::CELL; }
static inline int16_t cellCx(uint8_t c)   { return cellLeft(c) + NeonRollGame::CELL / 2; }
static inline int16_t cellCy(uint8_t r)   { return cellTop(r) + NeonRollGame::CELL / 2; }

// ================================================================ Lifecycle

void NeonRollGame::init() {
  gfx.tft().setRotation(board::TFT_ROTATION);
  if (!fb_.created()) fb_.createSprite(SW, SH);
  phase_   = PH_TITLE;
  level_   = 0;
  wantsMenu = false;
}

void NeonRollGame::quit() {
  fb_.deleteSprite();
}

void NeonRollGame::startLevel(uint8_t lvl) {
  level_ = lvl % LEVEL_COUNT;
  genLevel(level_);
  // spawn the ball at the start cell center
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      if (grid_[r][c] == C_START) {
        bx_ = (float)cellCx(c);
        by_ = (float)cellCy(r);
        grid_[r][c] = C_EMPTY;
      }
  vx_ = vy_ = 0;
  dir_     = DIR_RIGHT;
  moving_  = false;
  kicks_   = 0;
  trailN_  = 0;
  levelStartMs_ = millis();
  lastKickMs_   = 0;
  winT_         = 0;
  phase_        = PH_PLAY;
}

// ================================================================ Level gen
// Randomized maze with guaranteed solvable path (BFS flood fill).
// Density and bumper count scale with the level.

static uint8_t densityFor(uint8_t lvl) {
  // 6% .. 20% interior walls
  return 6 + lvl * 2;
}

static uint8_t bumpersFor(uint8_t lvl) {
  return 1 + lvl / 3;  // 1..3 bumpers
}

void NeonRollGame::genLevel(uint8_t lvl) {
  uint8_t density = densityFor(lvl);
  uint8_t nbump   = bumpersFor(lvl);

  for (uint8_t attempt = 0; attempt < 60; attempt++) {
    // clear grid: border walls, interior empty
    for (uint8_t r = 0; r < ROWS; r++)
      for (uint8_t c = 0; c < COLS; c++)
        grid_[r][c] = (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1)
                      ? C_WALL : C_EMPTY;

    // scatter interior walls
    for (uint8_t r = 1; r < ROWS - 1; r++)
      for (uint8_t c = 1; c < COLS - 1; c++)
        if (random(100) < density) grid_[r][c] = C_WALL;

    // place bumpers on empty interior cells
    for (uint8_t i = 0; i < nbump; i++) {
      uint8_t c = random(1, COLS - 1);
      uint8_t r = random(1, ROWS - 1);
      if (grid_[r][c] == C_EMPTY) grid_[r][c] = C_BUMPER;
    }

    // pick start and goal far apart
    uint8_t sc, sr, gc, gr;
    do { sc = random(1, COLS - 1); sr = random(1, ROWS - 1); } while (grid_[sr][sc] != C_EMPTY);
    do {
      gc = random(1, COLS - 1); gr = random(1, ROWS - 1);
    } while (grid_[gr][gc] != C_EMPTY ||
             (abs((int)gc - (int)sc) + abs((int)gr - (int)sr)) < 6);

    grid_[sr][sc] = C_START;
    grid_[gr][gc] = C_GOAL;

    // BFS flood fill from start - is the goal reachable?
    // (4-neighbour walk over non-wall cells)
    static uint8_t visited[ROWS][COLS];
    for (uint8_t r = 0; r < ROWS; r++)
      for (uint8_t c = 0; c < COLS; c++) visited[r][c] = 0;

    // simple queue
    static uint8_t qc[ROWS * COLS], qr[ROWS * COLS];
    uint16_t head = 0, tail = 0;
    qc[tail] = sc; qr[tail] = sr; tail++; visited[sr][sc] = 1;
    bool reachable = false;
    while (head < tail) {
      uint8_t c = qc[head], r = qr[head]; head++;
      if (c == gc && r == gr) { reachable = true; break; }
      static const int8_t dc[4] = {1, -1, 0, 0};
      static const int8_t dr[4] = {0, 0, 1, -1};
      for (uint8_t k = 0; k < 4; k++) {
        int8_t nc = c + dc[k], nr = r + dr[k];
        if (nc < 0 || nr < 0 || nc >= COLS || nr >= ROWS) continue;
        if (visited[nr][nc]) continue;
        if (grid_[nr][nc] == C_WALL) continue;
        visited[nr][nc] = 1;
        qc[tail] = nc; qr[tail] = nr; tail++;
      }
    }
    if (reachable) return;  // good level
  }
  // fallback: guaranteed-open level (should never happen)
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      grid_[r][c] = (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1)
                    ? C_WALL : C_EMPTY;
  grid_[1][1] = C_START;
  grid_[ROWS - 2][COLS - 2] = C_GOAL;
}

// ================================================================ Collision

bool NeonRollGame::cellSolid(uint8_t cx, uint8_t cy) const {
  if (cx >= COLS || cy >= ROWS) return true;  // outside = solid
  return grid_[cy][cx] == C_WALL;
}

// Exact circle-vs-cell-rect test. Checks every wall cell the circle's
// bounding box overlaps - smooth cornering (no false collision at L-junctions).
static bool circleHitsWall(const uint8_t grid[NeonRollGame::ROWS][NeonRollGame::COLS],
                           float x, float y, float r) {
  int c0 = (int)((x - r - NeonRollGame::FIELD_X) / NeonRollGame::CELL);
  int c1 = (int)((x + r - NeonRollGame::FIELD_X) / NeonRollGame::CELL);
  int r0 = (int)((y - r - NeonRollGame::FIELD_Y) / NeonRollGame::CELL);
  int r1 = (int)((y + r - NeonRollGame::FIELD_Y) / NeonRollGame::CELL);
  if (c0 < 0) c0 = 0; if (r0 < 0) r0 = 0;
  if (c1 >= NeonRollGame::COLS) c1 = NeonRollGame::COLS - 1;
  if (r1 >= NeonRollGame::ROWS) r1 = NeonRollGame::ROWS - 1;
  for (int rr = r0; rr <= r1; rr++)
    for (int cc = c0; cc <= c1; cc++) {
      if (grid[rr][cc] != NeonRollGame::C_WALL && grid[rr][cc] != NeonRollGame::C_BUMPER)
        continue;
      float cl = NeonRollGame::FIELD_X + cc * NeonRollGame::CELL;
      float ct = NeonRollGame::FIELD_Y + rr * NeonRollGame::CELL;
      float cr = cl + NeonRollGame::CELL;
      float cb = ct + NeonRollGame::CELL;
      float nx = (x < cl) ? cl : (x > cr ? cr : x);
      float ny = (y < ct) ? ct : (y > cb ? cb : y);
      float dx = x - nx, dy = y - ny;
      if (dx * dx + dy * dy < r * r) return true;
    }
  return false;
}

bool NeonRollGame::atGoal() const {
  // find goal cell
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      if (grid_[r][c] == C_GOAL) {
        float dx = bx_ - cellCx(c);
        float dy = by_ - cellCy(r);
        return (dx * dx + dy * dy) < (GOAL_R * GOAL_R);
      }
  return false;
}

// ================================================================ Physics

void NeonRollGame::kick() {
  uint32_t now = millis();
  if (now - lastKickMs_ < KICK_COOLDOWN_MS) return;
  lastKickMs_ = now;
  switch (dir_) {
    case DIR_RIGHT: vx_ += KICK_V; break;
    case DIR_LEFT:  vx_ -= KICK_V; break;
    case DIR_DOWN:  vy_ += KICK_V; break;
    case DIR_UP:    vy_ -= KICK_V; break;
    default: break;
  }
  // cap speed
  float sp = sqrtf(vx_ * vx_ + vy_ * vy_);
  if (sp > V_MAX) { vx_ *= V_MAX / sp; vy_ *= V_MAX / sp; }
  kicks_++;
  totalKicks_++;
  moving_ = true;
}

void NeonRollGame::rotateDir() {
  dir_ = (Dir)((dir_ + 1) % DIR_COUNT);
}

void NeonRollGame::stepPhysics(float dt) {
  if (dt <= 0) return;
  if (dt > 0.05f) dt = 0.05f;  // clamp huge frame gaps

  float speed = sqrtf(vx_ * vx_ + vy_ * vy_);

  // record trail position when moving
  if (speed > MIN_V) {
    // shift trail
    for (uint8_t i = TRAIL_LEN - 1; i > 0; i--) {
      trailX_[i] = trailX_[i - 1];
      trailY_[i] = trailY_[i - 1];
    }
    trailX_[0] = bx_;
    trailY_[0] = by_;
    if (trailN_ < TRAIL_LEN) trailN_++;
  } else if (trailN_ > 0) {
    // fade out trail when resting
    trailN_ = (trailN_ > 0) ? trailN_ - 1 : 0;
  }

  // friction (exponential decay)
  float damp = 1.0f - FRICTION * dt;
  if (damp < 0) damp = 0;
  vx_ *= damp;
  vy_ *= damp;

  speed = sqrtf(vx_ * vx_ + vy_ * vy_);
  if (speed < MIN_V) {
    // resting: snap tiny velocities to zero
    if (speed < MIN_V * 0.5f) { vx_ = 0; vy_ = 0; }
    moving_ = (speed >= MIN_V);
    if (!moving_) {
      // check goal capture while resting inside the goal
      if (atGoal()) { phase_ = PH_WIN; winT_ = millis(); return; }
      return;
    }
  }

  // substep movement (1 px per substep) for stable collision
  float dist = speed * dt;
  int steps = (int)(dist / 1.0f) + 1;
  if (steps > 64) steps = 64;
  float sdt = dt / steps;

  for (int s = 0; s < steps; s++) {
    // --- X axis ---
    if (vx_ != 0) {
      float newBx = bx_ + vx_ * sdt;
      if (circleHitsWall(grid_, newBx, by_, BALL_R)) {
        // clamp against the wall face
        if (vx_ > 0) {
          int col = (int)((newBx + BALL_R - FIELD_X) / CELL);
          float wallLeft = FIELD_X + col * CELL;
          bx_ = wallLeft - BALL_R - 0.3f;
        } else {
          int col = (int)((newBx - BALL_R - FIELD_X) / CELL);
          float wallRight = FIELD_X + (col + 1) * CELL;
          bx_ = wallRight + BALL_R + 0.3f;
        }
        // bumper boost if the wall cell we hit is a bumper
        // (bumper cells are solid for collision but give extra bounce)
        vx_ = -vx_ * BOUNCE;
      } else {
        bx_ = newBx;
      }
    }
    // --- Y axis ---
    if (vy_ != 0) {
      float newBy = by_ + vy_ * sdt;
      if (circleHitsWall(grid_, bx_, newBy, BALL_R)) {
        if (vy_ > 0) {
          int row = (int)((newBy + BALL_R - FIELD_Y) / CELL);
          float wallTop = FIELD_Y + row * CELL;
          by_ = wallTop - BALL_R - 0.3f;
        } else {
          int row = (int)((newBy - BALL_R - FIELD_Y) / CELL);
          float wallBot = FIELD_Y + (row + 1) * CELL;
          by_ = wallBot + BALL_R + 0.3f;
        }
        vy_ = -vy_ * BOUNCE;
      } else {
        by_ = newBy;
      }
    }
  }

  // goal capture: ball slow enough and inside the goal socket
  float spd = sqrtf(vx_ * vx_ + vy_ * vy_);
  if (spd < CAPTURE_V && atGoal()) {
    // center the ball in the goal and stop
    for (uint8_t r = 0; r < ROWS; r++)
      for (uint8_t c = 0; c < COLS; c++)
        if (grid_[r][c] == C_GOAL) {
          bx_ = (float)cellCx(c);
          by_ = (float)cellCy(r);
        }
    vx_ = vy_ = 0;
    moving_ = false;
    phase_  = PH_WIN;
    winT_   = millis();
    if (kicks_ < bestKicks_) bestKicks_ = kicks_;
  }
}

// ================================================================ Input

void NeonRollGame::update(const InputState& in, uint32_t dtMs) {
  float dt = dtMs / 1000.0f;

  switch (phase_) {
    case PH_TITLE:
      if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B] ||
          in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B])
        startLevel(0);
      break;

    case PH_PLAY:
      if (in.longPressed[(uint8_t)Key::A]) { wantsMenu = true; return; }

      // A short = kick (impulse). Hold = auto-repeat after cooldown.
      if (in.pressed[(uint8_t)Key::A]) kick();
      if (in.down[(uint8_t)Key::A] &&
          millis() - lastKickMs_ >= KICK_COOLDOWN_MS)
        kick();

      // B short = rotate direction 90 deg clockwise
      if (in.pressed[(uint8_t)Key::B]) rotateDir();

      stepPhysics(dt);
      break;

    case PH_WIN:
      if (in.longPressed[(uint8_t)Key::A] || in.longPressed[(uint8_t)Key::B]) {
        wantsMenu = true;
      } else if (in.pressed[(uint8_t)Key::A] || in.pressed[(uint8_t)Key::B]) {
        if (level_ + 1 < LEVEL_COUNT)
          startLevel(level_ + 1);
        else
          phase_ = PH_TITLE;  // all levels done -> back to title
      }
      break;
  }
}

// ================================================================ Drawing

void NeonRollGame::draw() {
  if (!fb_.created()) {
    TFT_eSPI& tft = gfx.tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Framebuffer alloc failed", 6, 8);
    return;
  }
  const Theme& th = theme();
  fb_.fillScreen(th.bg);

  if (phase_ == PH_TITLE) {
    drawTitle(fb_);
  } else {
    drawField(fb_);
    drawBall(fb_);
    drawHud(fb_);
    if (phase_ == PH_WIN) drawWin(fb_);
  }
  fb_.pushSprite(0, 0);
}

void NeonRollGame::drawTitle(TFT_eSprite& fb) {
  const Theme& th = theme();
  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(th.ball, th.bg);
  fb.drawString("NEON ROLL", SW / 2, 44);
  fb.setTextFont(2);
  fb.setTextColor(0xFFFF, th.bg);
  fb.drawString("Guide the glow ball", SW / 2, 84);
  fb.drawString("into the goal socket", SW / 2, 104);

  // animated demo ball
  static uint32_t t = 0;
  t += 16;
  float dx = cosf(t / 300.0f) * 40.0f;
  float dy = sinf(t / 400.0f) * 24.0f;
  float cx = SW / 2 + dx, cy = 170 + dy;
  for (uint8_t i = 4; i > 0; i--)
    fb.fillCircle((int16_t)cx, (int16_t)cy, i + 2, th.ballGlow);
  fb.fillCircle((int16_t)cx, (int16_t)cy, 5, th.ball);
  fb.fillCircle((int16_t)cx - 1, (int16_t)cy - 1, 2, 0xFFFF);

  // goal demo
  fb.drawCircle(SW / 2 + 52, 170, 8, th.goal);
  fb.drawCircle(SW / 2 + 52, 170, 5, th.goal);

  fb.setTextFont(2);
  fb.setTextColor(th.goal, th.bg);
  fb.drawString("A: Kick", SW / 2, 224);
  fb.drawString("B: Rotate", SW / 2, 244);
  fb.setTextFont(1);
  fb.setTextColor(th.grid, th.bg);
  fb.drawString("any key: Start", SW / 2, 292);
}

void NeonRollGame::drawField(TFT_eSprite& fb) {
  const Theme& th = theme();
  uint8_t wstyle = kWallStyle[level_ % LEVEL_COUNT];

  // faint grid
  for (uint8_t c = 0; c <= COLS; c++) {
    int16_t x = FIELD_X + c * CELL;
    fb.drawFastVLine(x, FIELD_Y, FIELD_H, th.grid);
  }
  for (uint8_t r = 0; r <= ROWS; r++) {
    int16_t y = FIELD_Y + r * CELL;
    fb.drawFastHLine(FIELD_X, y, FIELD_W, th.grid);
  }

  // cells
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      uint8_t cell = grid_[r][c];
      int16_t x = cellLeft(c), y = cellTop(r);
      if (cell == C_WALL) {
        fb.fillRect(x + 1, y + 1, CELL - 2, CELL - 2, th.wall);
        if (wstyle == 1) {
          // rivet highlight
          fb.drawRect(x + 2, y + 2, CELL - 4, CELL - 4, th.wallHi);
          fb.fillRect(x + 4, y + 4, 2, 2, th.wallHi);
          fb.fillRect(x + CELL - 6, y + CELL - 6, 2, 2, th.wallHi);
        } else if (wstyle == 2) {
          // rounded block
          fb.drawRoundRect(x + 1, y + 1, CELL - 2, CELL - 2, 3, th.wallHi);
          fb.fillRect(x + 3, y + 3, CELL - 6, 2, th.wallHi);
        } else {
          // solid block with top-left highlight
          fb.drawFastHLine(x + 1, y + 1, CELL - 2, th.wallHi);
          fb.drawFastVLine(x + 1, y + 1, CELL - 2, th.wallHi);
        }
      } else if (cell == C_GOAL) {
        // goal socket: glowing concentric rings
        int16_t cx = cellCx(c), cy = cellCy(r);
        fb.drawCircle(cx, cy, 7, th.goal);
        fb.drawCircle(cx, cy, 4, th.goal);
        fb.fillCircle(cx, cy, 2, th.goal);
        // pulsing outer ring
        uint32_t t = millis();
        if ((t / 200) % 2 == 0)
          fb.drawCircle(cx, cy, 9, th.goal);
      } else if (cell == C_BUMPER) {
        // bumper: diamond with accent
        int16_t cx = cellCx(c), cy = cellCy(r);
        fb.fillTriangle(cx, cy - 6, cx - 6, cy, cx, cy + 6, th.bumper);
        fb.fillTriangle(cx, cy - 6, cx + 6, cy, cx, cy + 6, th.bumper);
        fb.fillCircle(cx, cy, 2, 0xFFFF);
      }
    }
  }

  // playfield border
  fb.drawRect(FIELD_X - 1, FIELD_Y - 1, FIELD_W + 2, FIELD_H + 2, th.wallHi);
}

void NeonRollGame::drawBall(TFT_eSprite& fb) {
  const Theme& th = theme();
  int16_t bx = (int16_t)bx_;
  int16_t by = (int16_t)by_;

  // trail (fading glow behind the ball)
  for (uint8_t i = trailN_; i > 0; i--) {
    uint8_t idx = i - 1;
    int16_t tx = (int16_t)trailX_[idx];
    int16_t ty = (int16_t)trailY_[idx];
    uint8_t r = (uint8_t)(2 + idx / 2);
    fb.fillCircle(tx, ty, r, th.ballGlow);
  }

  // glow layers
  fb.fillCircle(bx, by, 8, th.ballGlow);
  fb.fillCircle(bx, by, 6, th.ballGlow);
  fb.fillCircle(bx, by, (int16_t)BALL_R, th.ball);
  // specular highlight
  fb.fillCircle(bx - 1, by - 1, 2, 0xFFFF);

  // direction ghost arrow (where the next kick goes)
  static uint16_t flash = 0;
  flash = (flash > 16) ? flash - 16 : 240;  // pulse
  int16_t ax = bx, ay = by;
  switch (dir_) {
    case DIR_RIGHT: ax += 16; break;
    case DIR_LEFT:  ax -= 16; break;
    case DIR_DOWN:  ay += 16; break;
    case DIR_UP:    ay -= 16; break;
    default: break;
  }
  // clamp arrow inside playfield
  if (ax < FIELD_X + 4) ax = FIELD_X + 4;
  if (ax > FIELD_X + FIELD_W - 4) ax = FIELD_X + FIELD_W - 4;
  if (ay < FIELD_Y + 4) ay = FIELD_Y + 4;
  if (ay > FIELD_Y + FIELD_H - 4) ay = FIELD_Y + FIELD_H - 4;
  fb.drawCircle(ax, ay, 3, th.goal);
  fb.fillCircle(ax, ay, 1, th.goal);
}

void NeonRollGame::drawHud(TFT_eSprite& fb) {
  const Theme& th = theme();
  fb.setTextDatum(TL_DATUM);
  fb.setTextFont(1);

  // level + theme name
  fb.setTextColor(th.ball, th.bg);
  char buf[24];
  snprintf(buf, sizeof(buf), "LV %u %s", level_ + 1, th.name);
  fb.drawString(buf, 4, 4);

  // kicks
  fb.setTextColor(0xFFFF, th.bg);
  snprintf(buf, sizeof(buf), "K:%u", kicks_);
  fb.drawString(buf, SW - 28, 4);

  // direction arrow in HUD (top-right area)
  int16_t cx = SW - 52, cy = 11;
  fb.drawRect(cx - 8, cy - 6, 16, 12, th.grid);
  int16_t dx = 0, dy = 0;
  switch (dir_) {
    case DIR_RIGHT: dx = 5; break;
    case DIR_LEFT:  dx = -5; break;
    case DIR_DOWN:  dy = 4; break;
    case DIR_UP:    dy = -4; break;
    default: break;
  }
  fb.fillTriangle(cx + dx, cy + dy,
                  cx + dx - 3, cy + dy - 3,
                  cx + dx - 3, cy + dy + 3, th.goal);

  // controls hint at the bottom
  fb.setTextColor(th.grid, th.bg);
  fb.setTextDatum(BC_DATUM);
  fb.drawString("A:Kick B:Rot", SW / 2, SH - 2);
}

void NeonRollGame::drawWin(TFT_eSprite& fb) {
  const Theme& th = theme();
  // dim overlay
  fb.fillRect(0, 90, SW, 110, 0x0000);
  fb.drawRect(4, 94, SW - 8, 102, th.goal);

  fb.setTextDatum(TC_DATUM);
  fb.setTextFont(4);
  fb.setTextColor(th.goal, 0x0000);
  fb.drawString("CLEAR!", SW / 2, 104);

  fb.setTextFont(2);
  fb.setTextColor(0xFFFF, 0x0000);
  char buf[32];
  snprintf(buf, sizeof(buf), "%u kicks", kicks_);
  fb.drawString(buf, SW / 2, 132);

  if (kicks_ == bestKicks_) {
    fb.setTextColor(th.ball, 0x0000);
    fb.drawString("NEW BEST!", SW / 2, 152);
  }

  fb.setTextFont(1);
  fb.setTextColor(th.grid, 0x0000);
  if (level_ + 1 < LEVEL_COUNT)
    fb.drawString("any key: Next level", SW / 2, 176);
  else
    fb.drawString("any key: Title", SW / 2, 176);
  fb.drawString("long: Menu", SW / 2, 188);
}
