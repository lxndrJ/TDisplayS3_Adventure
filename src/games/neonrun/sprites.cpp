#include "sprites.h"

NeonVectorTheme neonVectorTheme;
SpriteRenderer* gSprites = &neonVectorTheme;

void NeonVectorTheme::drawBackground(TFT_eSprite& fb, uint32_t scroll) {
  const int16_t W = gfx.width();
  const int16_t H = gfx.height();
  const int16_t roadY = H - 64;

  fb.fillSprite(pal_.bgBottom);
  fb.fillRect(0, 0, W, roadY, pal_.bgTop);

  // Stars with parallax (slower than the world)
  int32_t sm = (int32_t)(scroll / 3) % (roadY - 24);
  for (uint8_t i = 0; i < 24; i++) {
    int16_t sx = (int16_t)((i * 67 + 13) % W);
    int16_t sy = 12 + (int16_t)((i * 131 + 29 + sm) % (roadY - 24));
    if ((i & 3) == 0) fb.fillRect(sx, sy, 2, 2, pal_.star);
    else             fb.fillRect(sx, sy, 1, 1, pal_.star);
  }

  // horizon glow
  fb.drawFastHLine(0, roadY - 1, W, pal_.obstacleDark);
  fb.drawFastHLine(0, roadY, W, pal_.horizon);

  // moving road stripes
  int32_t m = (int32_t)(scroll / 24) % 24;
  for (int16_t y = roadY - 24 + (int16_t)m; y < H; y += 24)
    fb.fillRect(0, y, W, 12, pal_.bgMid);
}

void NeonVectorTheme::drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) {
  if (!visible) return;  // blinking while invulnerable
  fb.fillTriangle(x + 1, y + 11, x + 8, y - 2, x + 15, y + 11, pal_.playerGlow);
  fb.fillTriangle(x + 3, y + 9, x + 8, y + 2, x + 13, y + 9, pal_.player);
  fb.fillCircle(x + 8, y + 6, 1, TFT_WHITE);
  fb.fillTriangle(x + 6, y + 11, x + 10, y + 11, x + 8, y + 15, 0xFD20);
}

void NeonVectorTheme::drawBarObstacle(TFT_eSprite& fb, int16_t x, int16_t y, int16_t w, int16_t h,
                                      int16_t gapX, int16_t gapW) {
  // Warning-stripe bar left and right of the gap
  fb.fillRect(x, y, gapX, h, pal_.obstacleDark);
  fb.fillRect(x + gapX + gapW, y, w - gapX - gapW, h, pal_.obstacleDark);
  for (int16_t sx = x; sx < x + gapX; sx += 8)
    fb.fillRect(sx, y, 4, h, pal_.obstacle);
  for (int16_t sx = x + gapX + gapW; sx < x + w; sx += 8)
    fb.fillRect(sx, y, 4, h, pal_.obstacle);
}

void NeonVectorTheme::drawDrone(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) {
  fb.drawFastVLine(x + 7, y, 4, pal_.obstacle);
  fb.fillCircle(x + 7, y, 1, pal_.coin);
  fb.fillRect(x, y + 3, 14, 10, pal_.obstacleDark);
  fb.drawRect(x, y + 3, 14, 10, pal_.obstacle);
  bool eye = (t / 250) & 1;
  fb.fillRect(x + 3, y + 6, 3, 3, eye ? pal_.coin : pal_.obstacle);
  fb.fillRect(x + 8, y + 6, 3, 3, eye ? pal_.coin : pal_.obstacle);
}

void NeonVectorTheme::drawCoin(TFT_eSprite& fb, int16_t cx, int16_t cy, uint32_t t) {
  int16_t r = 4 + (int16_t)((t / 180) & 1);  // Puls
  fb.fillTriangle(cx, cy - r, cx - r, cy, cx + r, cy, pal_.coin);
  fb.fillTriangle(cx, cy + r, cx - r, cy, cx + r, cy, pal_.coin);
  fb.drawLine(cx, cy - r, cx + r, cy, pal_.coinDark);
  fb.drawLine(cx, cy - r, cx - r, cy, pal_.coinDark);
  fb.fillCircle(cx, cy, 1, pal_.coinDark);
}
