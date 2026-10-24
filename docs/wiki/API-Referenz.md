# 📚 API-Referenz

Vollständige Referenz aller Engine-Klassen und -Funktionen.

## Game-Interface (`src/core/game.h`)

```cpp
class Game {
public:
  virtual ~Game() = default;

  // Name des Spiels (für Menü/Launcher)
  virtual const char* name() const = 0;

  // Einmalig beim Spielstart aufgerufen
  virtual void init() = 0;

  // Wird jeden Frame (~60 FPS) aufgerufen
  // @param in    Aktueller Input-Status
  // @param dtMs  Zeit seit letztem Frame in Millisekunden
  virtual void update(const InputState& in, uint32_t dtMs) = 0;

  // Zeichnet das Spiel auf gfx.tft()
  virtual void draw() = 0;

  // Optional: Aufräumen vor Spielwechsel
  virtual void quit() {}

  // Auf true setzen, um zum Launcher zurückzukehren
  bool wantsMenu = false;
};
```

## Engine (`src/core/engine.h`)

```cpp
class Engine {
public:
  // Startet das angegebene Spiel
  void begin(Game* game);

  // Hauptschleife – in Arduino loop() aufrufen
  void loop();
};

extern Engine engine;  // Globale Instanz
```

**Verwendung in `main.cpp`:**
```cpp
#include "core/engine.h"
#include "games/games.h"

void setup() {
  engine.begin(makeGame(BOOT_GAME));
}

void loop() {
  engine.loop();
}
```

## Input (`src/core/input.h`)

### Key-Enum

```cpp
enum class Key : uint8_t { A, B, KeyCount };
// Key::A = 0, Key::B = 1
```

### InputState

```cpp
struct InputState {
  bool     down[2];         // true während Tastendruck
  bool     pressed[2];      // true nur im Frame des Loslassens (kurz)
  bool     longPressed[2];  // true einmalig nach langer Haltezeit
  uint32_t holdMs[2];       // Haltezeit in ms (0 = nicht gedrückt)
};
```

### InputManager

```cpp
class InputManager {
public:
  void init();                      // Einmalig aufrufen
  void update(InputState& s);       // Jeden Frame aufrufen
};

extern InputManager input;  // Globale Instanz
```

### ButtonTracker (`src/core/buttons.h`)

```cpp
enum class BtnEvent : uint8_t {
  None,   // Kein Ereignis
  Short,  // Kurzer Tastendruck (Loslassen vor longMs)
  Long    // Langer Tastendruck (longMs erreicht)
};

class ButtonTracker {
public:
  ButtonTracker(uint8_t pin, uint32_t longMs, uint32_t debounceMs = 30);

  BtnEvent update();        // Jeden Frame aufrufen
  bool     isDown() const;  // true während gedrückt
  uint32_t heldMs() const;  // Haltezeit in ms
};
```

## Display (`src/core/display.h`)

```cpp
class Display {
public:
  void init();                    // Display initialisieren
  TFT_eSPI& tft();                // Zugriff auf TFT_eSPI-Objekt

  static constexpr int16_t width();   // Display-Breite (170)
  static constexpr int16_t height();  // Display-Höhe (320)
};

extern Display gfx;  // Globale Instanz
```

**Beispiel:**
```cpp
#include "core/display.h"

auto& tft = gfx.tft();
tft.fillScreen(TFT_BLACK);
tft.drawString("Hallo", 10, 10, 2);
```

## Board-Konfiguration (`src/boards/board_*.h`)

Alle Boards definieren diese Konstanten im `board`-Namespace:

```cpp
namespace board {
  // Taster-Pins
  constexpr uint8_t PIN_BTN_A;       // Taster A (z.B. BOOT)
  constexpr uint8_t PIN_BTN_B;       // Taster B
  constexpr uint8_t PIN_POWER_ON;    // Power-Hold (muss HIGH)
  constexpr bool    HAS_POWER_HOLD;  // true wenn Power-Hold existiert

  // Timing
  constexpr uint32_t LONG_A_MS;      // ms bis Long für A
  constexpr uint32_t LONG_B_MS;      // ms bis Long für B
  constexpr uint32_t DEBOUNCE_MS;    // Entprellzeit

  // Display
  constexpr uint8_t  TFT_ROTATION;   // 0-3
  constexpr int16_t  SCR_W;          // Breite in Sketch-Ausrichtung
  constexpr int16_t  SCR_H;          // Höhe in Sketch-Ausrichtung
}
```

## Game-Registry (`src/games/games.h`)

```cpp
// Spiel erstellen
Game* makeGame(uint8_t index);

// Anzahl registrierter Spiele
constexpr uint8_t GAME_COUNT;

// Index des Spiels, das beim Booten startet
constexpr uint8_t BOOT_GAME;
```

## SpriteRenderer (`src/games/neonrun/sprites.h`)

Für austauschbare Grafik-Themes:

```cpp
struct NeonPalette {
  uint16_t bgTop, bgMid, bgBottom;   // Hintergrund
  uint16_t star, horizon;            // Dekoration
  uint16_t player, playerGlow;       // Spieler
  uint16_t obstacle, obstacleDark;   // Hindernisse
  uint16_t coin, coinDark;           // Sammelobjekte
  uint16_t text, textDim;            // Text
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

extern SpriteRenderer* gSprites;           // Aktives Theme
extern NeonVectorTheme neonVectorTheme;    // Standard-Theme
```

## TFT_eSPI Referenz (Auszug)

### Farben (RGB565)

| Konstante | Wert | Farbe |
|-----------|------|-------|
| `TFT_BLACK` | 0x0000 | Schwarz |
| `TFT_WHITE` | 0xFFFF | Weiß |
| `TFT_RED` | 0xF800 | Rot |
| `TFT_GREEN` | 0x07E0 | Grün |
| `TFT_BLUE` | 0x001F | Blau |
| `TFT_YELLOW` | 0xFFE0 | Gelb |
| `TFT_MAGENTA` | 0xF81F | Magenta |
| `TFT_CYAN` | 0x07FF | Cyan |
| `TFT_ORANGE` | 0xFD20 | Orange |
| `TFT_GRAY` / `TFT_DARKGRAY` | 0x7BEF / 0x39E7 | Grau |
| `TFT_LIGHTGRAY` | 0xC618 | Hellgrau |

### Text-Datums (Ausrichtung)

```cpp
tft.setTextDatum(TL_DATUM);  // Top-Left (Standard)
tft.setTextDatum(TC_DATUM);  // Top-Center
tft.setTextDatum(TR_DATUM);  // Top-Right
tft.setTextDatum(ML_DATUM);  // Middle-Left
tft.setTextDatum(MC_DATUM);  // Middle-Center
tft.setTextDatum(MR_DATUM);  // Middle-Right
tft.setTextDatum(BL_DATUM);  // Bottom-Left
tft.setTextDatum(BC_DATUM);  // Bottom-Center
tft.setTextDatum(BR_DATUM);  // Bottom-Right
```

### Wichtige Methoden

```cpp
TFT_eSPI tft;

// Initialisierung
tft.init();
tft.setRotation(0);  // 0-3

// Bildschirm
tft.fillScreen(color);

// Text
tft.setTextColor(fg, bg);
tft.setTextDatum(datum);
tft.drawString(str, x, y, font);  // font: 1-8
tft.drawNumber(num, x, y, font);
tft.drawFloat(num, decimals, x, y, font);

// Formen
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

// Messung
tft.textWidth(str, font);
tft.fontHeight(font);
```

### TFT_eSprite (Framebuffer)

```cpp
TFT_eSprite spr{&tft};

spr.setColorDepth(16);           // 8 oder 16 Bit
spr.createSprite(w, h);          // Speicher allozieren
spr.deleteSprite();              // Freigeben

// Gleiche Zeichen-Methoden wie TFT_eSPI
spr.fillSprite(color);
spr.drawString(...);
// ...

spr.pushSprite(x, y);            // Auf Display ausgeben
spr.pushSprite(x, y, transparent);
```

## Arduino/ESP32 Funktionen

### Zeit

```cpp
millis();           // ms seit Boot (uint32_t, läuft nach ~49 Tagen über)
micros();           // µs seit Boot
delay(ms);          // Blockiert (in Game-Loop vermeiden!)
delayMicroseconds(us);
```

### Zufall

```cpp
randomSeed(seed);   // Wird von Engine gesetzt (esp_random())
random(max);        // 0 bis max-1
random(min, max);   // min bis max-1
```

### Speicher

```cpp
ESP.getFreeHeap();      // Freier Heap (interner RAM)
ESP.getFreePsram();     // Freier PSRAM
ESP.getHeapSize();      // Gesamt-Heap
ESP.getPsramSize();     // Gesamt-PSRAM
```

### GPIO

```cpp
pinMode(pin, OUTPUT);
pinMode(pin, INPUT);
pinMode(pin, INPUT_PULLUP);
digitalWrite(pin, HIGH/LOW);
digitalRead(pin);
```

## Konstanten-Referenz

### Display (T-Display-S3)

| Konstante | Wert |
|-----------|------|
| `gfx.width()` | 170 (Portrait) |
| `gfx.height()` | 320 (Portrait) |
| `board::TFT_ROTATION` | 0 (Portrait) |

### Timing

| Konstante | Wert | Bedeutung |
|-----------|------|-----------|
| `board::LONG_A_MS` | 1200 ms | Long-Press Taster A |
| `board::LONG_B_MS` | 700 ms | Long-Press Taster B |
| `board::DEBOUNCE_MS` | 30 ms | Entprellzeit |
| `Engine::FRAME_MS` | 16 ms | ~60 FPS Ziel-Framerate |

### Pins (T-Display-S3)

| Konstante | GPIO | Funktion |
|-----------|------|----------|
| `board::PIN_BTN_A` | 0 | BOOT-Taster |
| `board::PIN_BTN_B` | 14 | Zweiter Taster |
| `board::PIN_POWER_ON` | 15 | Power-Hold (HIGH) |

## Weiterführende Links

- [TFT_eSPI volle Dokumentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32 Arduino Core API](https://github.com/espressif/arduino-esp32)
- [Projekt-Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)
