# GitHub Copilot Instructions

This is an **ESP32-S3 game framework** for the LilyGo T-Display-S3. When suggesting or generating code, follow these rules strictly.

## Project Context

- **Hardware:** LilyGo T-Display-S3 (ESP32-S3, 320×170 ST7789 display, 2 buttons, 16MB Flash, 8MB PSRAM)
- **Framework:** PlatformIO + Arduino + TFT_eSPI 2.5.43 (pinned – do NOT suggest updates)
- **Language:** C++17 (`-std=gnu++17`)
- **Architecture:** Modular game engine – games never touch GPIOs or display drivers directly

## Critical Rules (NEVER violate)

### 1. No direct GPIO access in games
```cpp
// ❌ WRONG
pinMode(0, INPUT_PULLUP);
digitalRead(14);
digitalWrite(15, HIGH);

// ✅ CORRECT
#include "core/board_config.h"
// Use board::PIN_BTN_A, board::PIN_BTN_B, board::PIN_POWER_ON
// Or better: use the InputState from update()
```

### 2. Display access only through `gfx`
```cpp
// ❌ WRONG
TFT_eSPI tft;  // Never create your own instance!
tft.init();

// ✅ CORRECT
#include "core/display.h"
auto& tft = gfx.tft();
tft.fillScreen(TFT_BLACK);
```

### 3. Input only through `InputState`
```cpp
// ❌ WRONG
void update(const InputState& in, uint32_t dtMs) {
  if (digitalRead(0) == LOW) { /* ... */ }  // NEVER!
}

// ✅ CORRECT
void update(const InputState& in, uint32_t dtMs) {
  if (in.pressed[0])       { /* A short press */ }
  if (in.pressed[1])       { /* B short press */ }
  if (in.longPressed[0])   { /* A long press (~1200ms) */ }
  if (in.longPressed[1])   { /* B long press (~700ms) */ }
  if (in.down[1])          { /* B held down */ }
  uint32_t hold = in.holdMs[0];  // Hold duration in ms
}
```

### 4. Use framebuffer for animations
```cpp
// ❌ WRONG – causes flickering
tft.fillScreen(TFT_BLACK);
tft.fillCircle(x, y, r, TFT_RED);

// ✅ CORRECT – flicker-free
TFT_eSprite fb_{&gfx.tft()};  // Member variable
// init(): fb_.createSprite(gfx.width(), gfx.height());
// draw():
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, r, TFT_RED);
fb_.pushSprite(0, 0);  // Single push per frame!
// quit(): fb_.deleteSprite();
```

### 5. Never use `delay()` in game loop
```cpp
// ❌ WRONG – blocks the engine
void update(const InputState& in, uint32_t dtMs) {
  delay(500);  // Blocks everything!
}

// ✅ CORRECT – use delta time
uint32_t lastAction_ = 0;
void update(const InputState& in, uint32_t dtMs) {
  if (millis() - lastAction_ > 500) {
    // do something
    lastAction_ = millis();
  }
}
```

## Creating a New Game

When asked to create a new game, follow this exact structure:

### File: `src/games/<name>/<name>.h`
```cpp
#ifndef GAMES_<NAME>_<NAME>_H
#define GAMES_<NAME>_<NAME>_H

#include "core/game.h"
#include "core/display.h"

class <Name>Game : public Game {
public:
  const char* name() const override { return "<Display Name>"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;  // Optional: cleanup (deleteSprite, etc.)

private:
  // Game state
  TFT_eSprite fb_{&gfx.tft()};  // Framebuffer for flicker-free rendering
};

#endif
```

### File: `src/games/<name>/<name>.cpp`
```cpp
#include "<name>.h"

void <Name>Game::init() {
  fb_.setColorDepth(16);
  fb_.createSprite(gfx.width(), gfx.height());
  fb_.fillScreen(TFT_BLACK);
}

void <Name>Game::update(const InputState& in, uint32_t dtMs) {
  // Handle input
  if (in.pressed[1]) {
    // B short: action
  }
  if (in.longPressed[1]) {
    wantsMenu = true;  // Return to launcher
  }
}

void <Name>Game::draw() {
  fb_.fillScreen(TFT_BLACK);
  // Draw everything to framebuffer...
  fb_.drawString("Score: 42", 10, 10, 2);
  fb_.pushSprite(0, 0);  // Single push!
}

void <Name>Game::quit() {
  fb_.deleteSprite();
}
```

### Register in `src/games/games.h`
```cpp
#include "<name>/<name>.h"  // Add include

inline Game* makeGame(uint8_t index) {
  static EldoriaGame  eldoria;
  static NeonRunGame  neonRun;
  static <Name>Game   <name>;  // Add instance
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &<name>;  // Add case
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 3;  // Increment!
```

## Key Constants (T-Display-S3)

| Constant | Value | Description |
|----------|-------|-------------|
| `gfx.width()` | 170 | Display width (portrait) |
| `gfx.height()` | 320 | Display height (portrait) |
| `board::PIN_BTN_A` | 0 | BOOT button (active LOW) |
| `board::PIN_BTN_B` | 14 | Second button (active LOW) |
| `board::PIN_POWER_ON` | 15 | Power-hold (must stay HIGH) |
| `board::LONG_A_MS` | 1200 | Long-press threshold A |
| `board::LONG_B_MS` | 700 | Long-press threshold B |

## Colors (RGB565)

`TFT_BLACK`, `TFT_WHITE`, `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`, `TFT_YELLOW`, `TFT_MAGENTA`, `TFT_CYAN`, `TFT_ORANGE`

## Build Commands

```bash
pio run              # Build
pio run -t upload    # Build + flash (BOOT+RST sequence!)
pio device monitor   # Serial monitor (115200 baud)
```

## What NOT to do

- ❌ Update TFT_eSPI (pinned to 2.5.43, see TFT_eSPI#3329)
- ❌ Change `Engine::begin()` order (power-hold must be before display init)
- ❌ Use `String` in performance-critical paths (heap fragmentation)
- ❌ Large stack variables (ESP32 has limited stack)
- ❌ `using namespace std;`
- ❌ Tabs for indentation (use 2 spaces)

## Documentation

- Wiki: `docs/wiki/` (German)
- Agent guidelines: `AGENTS.md`
- Contributing: `CONTRIBUTING.md`
- API reference: `docs/wiki/API-Referenz.md`
