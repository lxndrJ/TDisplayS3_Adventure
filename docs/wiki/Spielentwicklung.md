---
layout: page
title: "Game Development"
description: "How to write your own game for the TDisplayS3 Adventure engine: Game interface, input system, display, framebuffer, fonts, timing, random, memory, best practices."
---

# 🎮 Game Development

A detailed guide for developing your own games on the TDisplayS3 Adventure engine.

## Architecture Overview

```
┌─────────────────┐
│   Your game     │  <- implements the Game interface
│   (src/games/)  │
└────────┬────────┘
         │ uses
┌────────▼────────┐
│   Engine Core   │  <- Game, Engine, Display, Input
│   (src/core/)   │
└────────┬────────┘
         │ configured via
┌────────▼────────┐
│  Board Config   │  <- pins, display geometry, timing
│  (src/boards/)  │
└─────────────────┘
```

## The Game Interface

Every game implements these 4 methods (`src/core/game.h`):

```cpp
class Game {
public:
  virtual const char* name() const = 0;           // Game name for the menu
  virtual void init() = 0;                         // Once at start
  virtual void update(const InputState& in, uint32_t dtMs) = 0;  // Logic
  virtual void draw() = 0;                         // Render to gfx.tft()
  virtual void quit() {}                           // Optional: cleanup

  bool wantsMenu = false;  // true = back to the launcher
};
```

### Lifecycle of a Game

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  init()  │ ──► │ update() │ ──► │  draw()  │ ──► │  loop    │
│  once    │     │  ~60 FPS │     │  ~60 FPS │     │  repeat  │
└──────────┘     └──────────┘     └──────────┘     └────┬─────┘
                                                          │
                                                wantsMenu = true?
                                                          │
                                                     ┌────┴────┐
                                                     ▼         │
                                                   quit()   continue
```

## Input System

### InputState Structure (`src/core/input.h`)

```cpp
struct InputState {
  bool     down[2];        // true while the button is held (Key::A=0, Key::B=1)
  bool     pressed[2];     // true only in the frame of the release (short)
  bool     longPressed[2]; // true once after a long hold
  uint32_t holdMs[2];      // hold time in ms (0 = not pressed)
};
```

### Access in Your Game

```cpp
void MyGame::update(const InputState& in, uint32_t dtMs) {
  // Short press (once on release)
  if (in.pressed[0]) { /* A short */ }
  if (in.pressed[1]) { /* B short */ }

  // Long press (once after LONG_X_MS)
  if (in.longPressed[0]) { /* A long (~1200ms) */ }
  if (in.longPressed[1]) { /* B long (~700ms) */ }

  // Continuously while held
  if (in.down[0]) { /* A held */ }

  // Hold time in milliseconds
  uint32_t holdDuration = in.holdMs[1];
}
```

### Button Mapping (T-Display-S3)

| Logical | Physical | Pin | Active |
|---------|----------|-----|--------|
| Key::A  | BOOT     | GPIO0  | LOW |
| Key::B  | free     | GPIO14 | LOW |

## Display System

### Access via `gfx`

```cpp
#include "core/display.h"

// In draw():
auto& tft = gfx.tft();

// Display size
constexpr int16_t W = gfx.width();   // 170 (portrait)
constexpr int16_t H = gfx.height();  // 320 (portrait)
```

### Framebuffer for Flicker-Free Animation (recommended)

For animated games (like Neon Run) a sprite framebuffer is recommended:

```cpp
#include <TFT_eSPI.h>

// Declare as a member (lands in PSRAM on the S3)
TFT_eSprite fb_{&gfx.tft()};

void MyGame::init() {
  // Create the framebuffer (full screen size)
  fb_.setColorDepth(16);  // RGB565
  fb_.createSprite(gfx.width(), gfx.height());
}

void MyGame::draw() {
  // Draw everything into the framebuffer
  fb_.fillScreen(TFT_BLACK);
  fb_.fillCircle(85, 160, 20, TFT_RED);

  // Push to the display once (flicker-free!)
  fb_.pushSprite(0, 0);
}

void MyGame::quit() {
  fb_.deleteSprite();  // free memory
}
```

### Important TFT_eSPI Commands

| Command | Description | Example |
|---------|-------------|---------|
| `fillScreen(color)` | Clear screen | `tft.fillScreen(TFT_BLACK)` |
| `drawString(text, x, y, font)` | Draw text | `tft.drawString("HP: 3", 5, 5, 2)` |
| `setTextColor(fg, bg)` | Set text color | `tft.setTextColor(TFT_WHITE, TFT_BLACK)` |
| `setTextDatum(datum)` | Text alignment | `tft.setTextDatum(TC_DATUM)` // top-center |
| `fillRect(x,y,w,h,color)` | Filled rectangle | `tft.fillRect(10, 10, 50, 20, TFT_RED)` |
| `drawRect(x,y,w,h,color)` | Rectangle outline | `tft.drawRect(10, 10, 50, 20, TFT_WHITE)` |
| `fillCircle(cx,cy,r,color)` | Filled circle | `tft.fillCircle(85, 160, 20, TFT_BLUE)` |
| `drawCircle(cx,cy,r,color)` | Circle outline | `tft.drawCircle(85, 160, 20, TFT_WHITE)` |
| `drawLine(x1,y1,x2,y2,color)` | Draw line | `tft.drawLine(0, 0, 169, 319, TFT_GREEN)` |
| `fillTriangle(x1,y1,x2,y2,x3,y3,color)` | Filled triangle | `tft.fillTriangle(85,140, 65,180, 105,180, TFT_YELLOW)` |
| `drawPixel(x, y, color)` | Single pixel | `tft.drawPixel(85, 160, TFT_WHITE)` |
| `setRotation(r)` | Display rotation | `tft.setRotation(1)` // 0-3 |
| `pushImage(x, y, w, h, data)` | Load bitmap | `tft.pushImage(0, 0, 32, 32, mySprite)` |

### Fonts (1-8 available)

```cpp
tft.drawString("Small",  10, 10, 1);  // Font 1: small (8px)
tft.drawString("Normal", 10, 30, 2);  // Font 2: normal (16px)
tft.drawString("Large",  10, 50, 4);  // Font 4: large (26px)
tft.drawString("Huge",   10, 80, 7);  // Font 7: huge (48px)
tft.drawString("Number", 10, 140, 8); // Font 8: digits only (75px)
```

## Time & Frame Rate

### Delta-Time (`dtMs`)

The engine calls `update()` with the time delta since the last frame:

```cpp
void MyGame::update(const InputState& in, uint32_t dtMs) {
  // dtMs = milliseconds since the last frame (~16ms at 60 FPS)

  // Frame-rate-independent movement:
  float speed = 100.0f;  // pixels per second
  playerX += speed * (dtMs / 1000.0f);
}
```

### Custom Timing

```cpp
// In the class:
uint32_t lastActionMs = 0;
uint32_t actionCooldown = 500;  // 500ms cooldown

void MyGame::update(const InputState& in, uint32_t dtMs) {
  uint32_t now = millis();

  if (in.pressed[1] && now - lastActionMs > actionCooldown) {
    // do action
    lastActionMs = now;
  }
}
```

## Random Number Generator

The engine initializes `randomSeed()` automatically:

```cpp
// Dice roll W6 (1-6)
int die = random(1, 7);

// Random choice
if (random(100) < 25) {  // 25% chance
  // critical hit!
}

// Random position
int x = random(10, 160);
```

## Memory Management (ESP32-S3)

| Resource | Size | Note |
|----------|------|------|
| Internal RAM | ~512 KB | Variables, stack |
| PSRAM | 8 MB | Large framebuffers, bitmaps |
| Flash | 16 MB | Program, PROGMEM data |

### PROGMEM for Constant Data

```cpp
// Store bitmap in flash (saves RAM)
const uint16_t mySprite[] PROGMEM = {
  0xF800, 0xF800, 0x07E0, 0x07E0,  // ...
};

// Load from flash
tft.pushImage(x, y, 16, 16, mySprite);
```

### PSRAM for Large Objects

```cpp
// Framebuffers automatically land in PSRAM (with TFT_eSprite)
TFT_eSprite fb_{&gfx.tft()};
fb_.createSprite(170, 320);  // ~106 KB in PSRAM
```

## Sprite Themes (Swappable Graphics)

Neon Run demonstrates the sprite theme system (`src/games/neonrun/sprites.h`):

```cpp
// Implement your own theme:
class MyBitmapTheme : public SpriteRenderer {
public:
  const NeonPalette& palette() const override { return myPalette; }
  void drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) override {
    // Draw your own bitmap instead of vector graphics
    fb.pushImage(x, y, 16, 16, myPlayerSprite);
  }
  // ... more methods
};

// Activate in init():
void MyGame::init() {
  gSprites = &myBitmapTheme;  // instead of neonVectorTheme
}
```

## Best Practices

### 1. Do Not Block `draw()`

```cpp
// ❌ Bad: heavy computation in draw()
void draw() {
  for (int i = 0; i < 10000; i++) { /* ... */ }  // Blocks!
  // draw...
}

// ✅ Good: compute in update(), only draw in draw()
void update(const InputState& in, uint32_t dtMs) {
  // logic here
}
void draw() {
  // draw only
}
```

### 2. Framebuffer for Animations

```cpp
// ❌ Bad: draw directly to the display (flickers)
tft.fillScreen(TFT_BLACK);  // Flicker!
tft.fillCircle(x, y, r, TFT_RED);

// ✅ Good: framebuffer (flicker-free)
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, r, TFT_RED);
fb_.pushSprite(0, 0);  // push once
```

### 3. Constants for Magic Numbers

```cpp
// ❌ Bad
class MyGame : public Game {
  float x = 85;  // What does 85 mean?
};

// ✅ Good
class MyGame : public Game {
  static constexpr int16_t CENTER_X = 85;
  static constexpr int16_t PLAYER_Y = 262;
  float x = CENTER_X;
};
```

### 4. State Machine for Game Phases

```cpp
enum Phase : uint8_t { TITLE, PLAY, GAMEOVER, WIN };

Phase phase_ = TITLE;

void update(const InputState& in, uint32_t dtMs) {
  switch (phase_) {
    case TITLE:
      if (in.pressed[1]) phase_ = PLAY;
      break;
    case PLAY:
      // game logic
      if (gameOver) phase_ = GAMEOVER;
      break;
    case GAMEOVER:
      if (in.longPressed[1]) wantsMenu = true;
      break;
  }
}
```

## Debugging

### Serial Monitor

```bash
pio device monitor  # 115200 baud
```

```cpp
// In your code:
Serial.printf("Score: %d, HP: %d, Phase: %d\n", score, hp, phase);
Serial.println(F("Debug: player collided"));  // F() saves RAM
```

### Common Errors

| Problem | Cause | Solution |
|---------|-------|----------|
| Black screen | Power-hold GPIO15 not HIGH | Do not change Engine::begin() |
| Flickering graphics | Drawing directly to the display | Use a framebuffer (TFT_eSprite) |
| Buttons not responding | Wrong pin or active HIGH | Check board config (active LOW) |
| Build error TFT_eSPI | Wrong version | Pin to 2.5.43 (platformio.ini) |
| "Sketch too big" | Partition too small | Check partitions_16mb.csv |

## Further Links

- [TFT_eSPI Documentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [Project Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)

---

[← Wiki Home](Home.html) · [Docs Home](/TDisplayS3_Adventure/)
