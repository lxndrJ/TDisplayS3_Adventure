---
layout: page
title: "API Reference"
description: "Complete reference of all TDisplayS3 Adventure engine classes and functions: Game interface, Engine, Input, Display, Board config, Game registry, SpriteRenderer and TFT_eSPI."
---

# 📖 API Reference

Complete reference of all engine classes and functions.

## Game Interface (`src/core/game.h`)

```cpp
class Game {
public:
  virtual ~Game() = default;

  // Name of the game (for the menu/launcher)
  virtual const char* name() const = 0;

  // Called once when the game starts
  virtual void init() = 0;

  // Called every frame (~60 FPS)
  // @param in    Current input state
  // @param dtMs  Time since the last frame in milliseconds
  virtual void update(const InputState& in, uint32_t dtMs) = 0;

  // Draws the game to gfx.tft()
  virtual void draw() = 0;

  // Optional: cleanup before switching games
  virtual void quit() {}

  // Set to true to return to the launcher
  bool wantsMenu = false;
};
```

## Engine (`src/core/engine.h`)

```cpp
class Engine {
public:
  // Starts the given game (nullptr = show the menu)
  void begin(Game* game);

  // Main loop - call from Arduino loop()
  void loop();
};

extern Engine engine;  // Global instance
```

**Usage in `main.cpp`:**
```cpp
#include "core/engine.h"
#include "games/games.h"

void setup() {
  engine.begin(nullptr);   // nullptr = launcher menu
  // engine.begin(makeGame(2));  // boot directly into a game (for testing)
}

void loop() {
  engine.loop();
}
```

The engine runs a launcher menu when no game is active:
**A/B short** = select, **B long** = start the selected game.

## Input (`src/core/input.h`)

### Key Enum

```cpp
enum class Key : uint8_t { A, B, KeyCount };
// Key::A = 0, Key::B = 1
```

### InputState

```cpp
struct InputState {
  bool     down[2];         // true while the button is held
  bool     pressed[2];      // true only in the frame of the release (short)
  bool     longPressed[2];  // true once after a long hold
  uint32_t holdMs[2];       // hold time in ms (0 = not pressed)
};
```

### InputManager

```cpp
class InputManager {
public:
  void init();                      // call once
  void update(InputState& s);       // call every frame
};

extern InputManager input;  // Global instance
```

### ButtonTracker (`src/core/buttons.h`)

```cpp
enum class BtnEvent : uint8_t {
  None,   // No event
  Short,  // Short press (released before longMs)
  Long    // Long press (longMs reached)
};

class ButtonTracker {
public:
  ButtonTracker(uint8_t pin, uint32_t longMs, uint32_t debounceMs = 30);

  BtnEvent update();        // call every frame
  bool     isDown() const;  // true while held
  uint32_t heldMs() const;  // hold time in ms
};
```

## Display (`src/core/display.h`)

```cpp
class Display {
public:
  void init();                    // initialize the display
  TFT_eSPI& tft();                // access the TFT_eSPI object

  static constexpr int16_t width();   // display width (170)
  static constexpr int16_t height();  // display height (320)
};

extern Display gfx;  // Global instance
```

**Example:**
```cpp
#include "core/display.h"

auto& tft = gfx.tft();
tft.fillScreen(TFT_BLACK);
tft.drawString("Hello", 10, 10, 2);
```

## Board Configuration (`src/boards/board_*.h`)

All boards define these constants in the `board` namespace:

```cpp
namespace board {
  // Button pins
  constexpr uint8_t PIN_BTN_A;       // Button A (e.g. BOOT)
  constexpr uint8_t PIN_BTN_B;       // Button B
  constexpr uint8_t PIN_POWER_ON;    // Power-hold (must be HIGH)
  constexpr bool    HAS_POWER_HOLD;  // true if a power-hold pin exists

  // Timing
  constexpr uint32_t LONG_A_MS;      // ms until Long for A
  constexpr uint32_t LONG_B_MS;      // ms until Long for B
  constexpr uint32_t DEBOUNCE_MS;    // debounce time

  // Display
  constexpr uint8_t  TFT_ROTATION;   // 0-3
  constexpr int16_t  SCR_W;          // width in sketch orientation
  constexpr int16_t  SCR_H;          // height in sketch orientation
}
```

## Game Registry (`src/games/games.h`)

```cpp
// Create a game
Game* makeGame(uint8_t index);

// Number of registered games
constexpr uint8_t GAME_COUNT;
```

There is no `BOOT_GAME` constant. To boot directly into a game
(skipping the menu), call `engine.begin(makeGame(N))` in `main.cpp`.

## SpriteRenderer (`src/games/neonrun/sprites.h`)

For swappable graphics themes:

```cpp
struct NeonPalette {
  uint16_t bgTop, bgMid, bgBottom;   // background
  uint16_t star, horizon;            // decoration
  uint16_t player, playerGlow;       // player
  uint16_t obstacle, obstacleDark;   // obstacles
  uint16_t coin, coinDark;           // collectibles
  uint16_t text, textDim;            // text
};

class SpriteRenderer {
public:
  virtual const NeonPalette& palette() const = 0;
  virtual void drawBackground(TFT_eSprite& fb, uint32_t scroll) = 0;
  virtual void drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) = 0;
  virtual void drawBarObstacle(TFT_eSprite& fb, int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               int16_t gapX, int16_t gapW) = 0;
  virtual void drawDrone(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) = 0;
  virtual void drawCoin(TFT_eSprite& fb, int16_t x, int16_t y, uint32_t t) = 0;
};

extern SpriteRenderer* gSprites;           // active theme
extern NeonVectorTheme neonVectorTheme;    // default theme
```

## TFT_eSPI Reference (Excerpt)

### Colors (RGB565)

| Constant | Value | Color |
|----------|-------|-------|
| `TFT_BLACK` | 0x0000 | black |
| `TFT_WHITE` | 0xFFFF | white |
| `TFT_RED` | 0xF800 | red |
| `TFT_GREEN` | 0x07E0 | green |
| `TFT_BLUE` | 0x001F | blue |
| `TFT_YELLOW` | 0xFFE0 | yellow |
| `TFT_MAGENTA` | 0xF81F | magenta |
| `TFT_CYAN` | 0x07FF | cyan |
| `TFT_ORANGE` | 0xFD20 | orange |
| `TFT_GRAY` / `TFT_DARKGRAY` | 0x7BEF / 0x39E7 | grey |
| `TFT_LIGHTGRAY` | 0xC618 | light grey |

### Text Datums (Alignment)

```cpp
tft.setTextDatum(TL_DATUM);  // Top-Left (default)
tft.setTextDatum(TC_DATUM);  // Top-Center
tft.setTextDatum(TR_DATUM);  // Top-Right
tft.setTextDatum(ML_DATUM);  // Middle-Left
tft.setTextDatum(MC_DATUM);  // Middle-Center
tft.setTextDatum(MR_DATUM);  // Middle-Right
tft.setTextDatum(BL_DATUM);  // Bottom-Left
tft.setTextDatum(BC_DATUM);  // Bottom-Center
tft.setTextDatum(BR_DATUM);  // Bottom-Right
```

### Important Methods

```cpp
TFT_eSPI tft;

// Init
tft.init();
tft.setRotation(0);  // 0-3

// Screen
tft.fillScreen(color);

// Text
tft.setTextColor(fg, bg);
tft.setTextDatum(datum);
tft.drawString(str, x, y, font);  // font: 1-8
tft.drawNumber(num, x, y, font);
tft.drawFloat(num, decimals, x, y, font);

// Shapes
tft.drawPixel(x, y, color);
tft.drawLine(x1, y1, x2, y2, color);
tft.drawRect(x, y, w, h, color);
tft.fillRect(x, y, w, h, color);
tft.drawRoundRect(x, y, w, h, r, color);
tft.fillRoundRect(x, y, w, h, r, color);
tft.drawCircle(cx, cy, r, color);
tft.fillCircle(cx, cy, r, color);
tft.drawTriangle(x1,y1, x2,y2, x3,y3, color);
tft.fillTriangle(x1,y1, x2,y2, x3,y3, color);

// Bitmaps
tft.pushImage(x, y, w, h, data);           // RGB565
tft.pushImage(x, y, w, h, data, transparent);
tft.drawBitmap(x, y, data, w, h, fg, bg);  // 1-bit

// Measurement
tft.textWidth(str, font);
tft.fontHeight(font);
```

### TFT_eSprite (Framebuffer)

```cpp
TFT_eSprite spr{&tft};

spr.setColorDepth(16);           // 8 or 16 bit
spr.createSprite(w, h);          // allocate memory
spr.deleteSprite();              // free

// Same drawing methods as TFT_eSPI
spr.fillSprite(color);
spr.drawString(...);
// ...

spr.pushSprite(x, y);            // output to display
spr.pushSprite(x, y, transparent);
```

## Arduino/ESP32 Functions

### Time

```cpp
millis();           // ms since boot (uint32_t, wraps after ~49 days)
micros();           // µs since boot
delay(ms);          // blocks (avoid in the game loop!)
delayMicroseconds(us);
```

### Random

```cpp
randomSeed(seed);   // set by the engine (esp_random())
random(max);        // 0 to max-1
random(min, max);   // min to max-1
```

### Memory

```cpp
ESP.getFreeHeap();      // free heap (internal RAM)
ESP.getFreePsram();     // free PSRAM
ESP.getHeapSize();      // total heap
ESP.getPsramSize();     // total PSRAM
```

### GPIO

```cpp
pinMode(pin, OUTPUT);
pinMode(pin, INPUT);
pinMode(pin, INPUT_PULLUP);
digitalWrite(pin, HIGH/LOW);
digitalRead(pin);
```

## Constants Reference

### Display (T-Display-S3)

| Constant | Value |
|----------|-------|
| `gfx.width()` | 170 (portrait) |
| `gfx.height()` | 320 (portrait) |
| `board::TFT_ROTATION` | 0 (portrait) |

### Timing

| Constant | Value | Meaning |
|----------|-------|---------|
| `board::LONG_A_MS` | 1200 ms | long press for button A |
| `board::LONG_B_MS` | 700 ms | long press for button B |
| `board::DEBOUNCE_MS` | 30 ms | debounce time |
| `Engine::FRAME_MS` | 16 ms | ~60 FPS target frame rate |

### Pins (T-Display-S3)

| Constant | GPIO | Function |
|----------|------|----------|
| `board::PIN_BTN_A` | 0 | BOOT button |
| `board::PIN_BTN_B` | 14 | second button |
| `board::PIN_POWER_ON` | 15 | power-hold (HIGH) |

## Further Links

- [TFT_eSPI full documentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32 Arduino Core API](https://github.com/espressif/arduino-esp32)
- [Project Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)

---

[← Wiki Home](Home.html) · [Docs Home](/TDisplayS3_Adventure/)
