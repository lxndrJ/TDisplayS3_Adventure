---
layout: default
title: "Quickstart: Turn Game Ideas into Reality"
description: "3 steps to your first game on the TDisplayS3 Adventure engine: create the game class, register it in the game registry, build and flash with PlatformIO."
---

# 🚀 Quickstart: Turn Game Ideas into Reality

This guide shows how to quickly implement a new game idea on the T-Display-S3.

## Prerequisites

- [PlatformIO](https://platformio.org/) installed (VS Code extension or CLI)
- LilyGo T-Display-S3 connected
- Project cloned: `git clone https://github.com/lxndrJ/TDisplayS3_Adventure.git`

## Quickstart in 3 Steps

### Step 1: Create a new game

```bash
# Create the folder
mkdir -p src/games/mygame
```

**`src/games/mygame/mygame.h`** – minimal template:

```cpp
#ifndef MYGAME_H
#define MYGAME_H
#include "core/game.h"

class MyGameGame : public Game {
public:
  const char* name() const override { return "My Game"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
private:
  // Your game state here
  int score = 0;
};
#endif
```

**`src/games/mygame/mygame.cpp`** – implementation:

```cpp
#include "mygame.h"
#include "core/display.h"

void MyGameGame::init() {
  gfx.tft().fillScreen(TFT_BLACK);
  score = 0;
}

void MyGameGame::update(const InputState& in, uint32_t dtMs) {
  // Controls: in.pressed[0], in.pressed[1], in.longPressed[0], in.longPressed[1]
  if (in.pressed[(uint8_t)Key::A]) score += 10;
  if (in.longPressed[(uint8_t)Key::B]) wantsMenu = true;  // back to the menu
}

void MyGameGame::draw() {
  auto& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Score: " + String(score), 10, 10, 4);
}
```

### Step 2: Register in the game registry

Edit **`src/games/games.h`**:

```cpp
#include "mygame/mygame.h"  // add

inline Game* makeGame(uint8_t index) {
  static EldoriaGame   eldoria;
  static NeonRunGame   neonRun;
  static MyGameGame    myGame;  // add
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &myGame;    // add
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 3;  // 2 -> 3
```

### Step 3: Build & flash

```bash
pio run -t upload    # builds and flashes
```

## Controls (T-Display-S3)

| Action | Button |
|--------|--------|
| Next option / right | **B (GPIO14) short** |
| Previous option / left | **A (GPIO0) short** |
| Select / confirm | **B (GPIO14) long (~0.7 s)** |
| Inventory / special | **A (GPIO0) long (~1.2 s)** |

> ⚠️ **For flashing:** hold BOOT → press RST → release → start upload

## Engine Features (ready to use)

| Feature | Access |
|---------|--------|
| **Button input** | `InputState` with `pressed[]`, `longPressed[]`, `down[]`, `holdMs[]` |
| **Draw to display** | `gfx.tft()` – direct TFT_eSPI access |
| **Print text** | `tft.drawString(text, x, y, font)` |
| **Draw shapes** | `tft.fillRect()`, `tft.drawCircle()`, etc. |
| **Colors** | `TFT_BLACK`, `TFT_WHITE`, `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`, `TFT_YELLOW` (RGB565) |
| **Back to menu** | set `wantsMenu = true` |
| **Game name** | override `name()` |

## Templates to Copy From

| Genre | Template | File |
|-------|----------|------|
| **Text adventure** | Scenes, inventory, combat dice | `src/games/eldoria/eldoria.cpp` |
| **Action/runner** | Game loop, collision, sprites | `src/games/neonrun/neonrun.cpp` |
| **Sprite graphics** | Swappable themes | `src/games/neonrun/sprites.h` |

## Fast Test Workflow

To iterate quickly without going through the menu every time, boot directly
into your game via the engine:

```cpp
// src/core/engine.h (or main.cpp): start a specific game instead of the menu
engine.begin(makeGame(2));   // your game's index
```

1. `pio run -t upload`
2. Test → change code → repeat

## Display Specifications

| Property | Value |
|----------|-------|
| Resolution | 320 × 170 pixels |
| Color depth | 16-bit RGB565 |
| Rotation | 1 (landscape) |
| Display driver | ST7789 (8-bit parallel) |

## Useful TFT_eSPI Commands

```cpp
auto& tft = gfx.tft();

// Clear the screen
tft.fillScreen(TFT_BLACK);

// Text
tft.setTextColor(TFT_WHITE, TFT_BLACK);  // foreground, background
tft.drawString("Hello", 10, 10, 2);      // font 1-8

// Shapes
tft.fillRect(x, y, w, h, TFT_RED);
tft.drawRect(x, y, w, h, TFT_WHITE);
tft.fillCircle(cx, cy, r, TFT_GREEN);
tft.drawLine(x1, y1, x2, y2, TFT_BLUE);

// Pixel
tft.drawPixel(x, y, color);

// Rotation
tft.setRotation(0);  // 0=portrait, 1=landscape, 2=portrait-180, 3=landscape-180
```

## Further Links

- [TFT_eSPI documentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32-S3 Arduino Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO documentation](https://docs.platformio.org/)
- [LilyGo T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)

## Help

- **Questions?** → [GitHub Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Report a bug** → [GitHub Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **Project repository** → [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)
