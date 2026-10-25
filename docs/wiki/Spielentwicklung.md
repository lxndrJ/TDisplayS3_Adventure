# 🎮 Spielentwicklung

Detaillierte Anleitung für die Entwicklung eigener Spiele auf der TDisplayS3 Adventure Engine.

## Architektur-Übersicht

```
┌─────────────────┐
│   Dein Spiel    │  ← implementiert Game-Interface
│  (src/games/)   │
└────────┬────────┘
         │ nutzt
┌────────▼────────┐
│   Engine Core   │  ← Game, Engine, Display, Input
│   (src/core/)   │
└────────┬────────┘
         │ konfiguriert über
┌────────▼────────┐
│  Board Config   │  ← Pins, Display-Geometrie, Timing
│  (src/boards/)  │
└─────────────────┘
```

## Das Game-Interface

Jedes Spiel implementiert diese 4 Methoden (`src/core/game.h`):

```cpp
class Game {
public:
  virtual const char* name() const = 0;           // Spielname für Menü
  virtual void init() = 0;                         // Einmalig beim Start
  virtual void update(const InputState& in, uint32_t dtMs) = 0;  // Logik
  virtual void draw() = 0;                         // Rendern auf gfx.tft()
  virtual void quit() {}                           // Optional: Aufräumen

  bool wantsMenu = false;  // true = zurück zum Launcher
};
```

### Lebenszyklus eines Spiels

```
┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐
│  init() │ ──► │ update()│ ──► │  draw() │ ──► │  loop   │
│ einmalig│     │  ~60 FPS│     │  ~60 FPS│     │ wiederholen│
└─────────┘     └─────────┘     └─────────┘     └─────────┘
                                              │
                                    wantsMenu = true?
                                              │
                                         ┌────┴────┐
                                         ▼         │
                                       quit()   weiter
```

## Input-System

### InputState-Struktur (`src/core/input.h`)

```cpp
struct InputState {
  bool     down[2];        // true während Tastendruck (Key::A=0, Key::B=1)
  bool     pressed[2];     // true nur im Frame des Loslassens (kurz)
  bool     longPressed[2]; // true einmalig nach langer Haltezeit
  uint32_t holdMs[2];      // Haltezeit in ms (0 = nicht gedrückt)
};
```

### Zugriff in deinem Spiel

```cpp
void MeinSpiel::update(const InputState& in, uint32_t dtMs) {
  // Kurzer Druck (einmalig beim Loslassen)
  if (in.pressed[0]) { /* A kurz */ }
  if (in.pressed[1]) { /* B kurz */ }

  // Langer Druck (einmalig nach LONG_X_MS)
  if (in.longPressed[0]) { /* A lang (~1200ms) */ }
  if (in.longPressed[1]) { /* B lang (~700ms) */ }

  // Kontinuierlich während Druck
  if (in.down[0]) { /* A gehalten */ }

  // Haltezeit in Millisekunden
  uint32_t haltedauer = in.holdMs[1];
}
```

### Taster-Zuordnung (T-Display-S3)

| Logisch | Physisch | Pin | Aktiv |
|---------|----------|-----|-------|
| Key::A  | BOOT     | GPIO0  | LOW |
| Key::B  | Frei     | GPIO14 | LOW |

## Display-System

### Zugriff über `gfx`

```cpp
#include "core/display.h"

// Im draw():
auto& tft = gfx.tft();

// Display-Größe
constexpr int16_t W = gfx.width();   // 170 (Portrait)
constexpr int16_t H = gfx.height();  // 320 (Portrait)
```

### Framebuffer für flackerfreie Animation (empfohlen)

Für animierte Spiele (wie NeonRun) empfiehlt sich ein Sprite-Framebuffer:

```cpp
#include <TFT_eSPI.h>

// Als Member deklarieren (im PSRAM auf dem S3)
TFT_eSprite fb_{&gfx.tft()};

void MeinSpiel::init() {
  // Framebuffer erstellen (volle Bildschirmgröße)
  fb_.setColorDepth(16);  // RGB565
  fb_.createSprite(gfx.width(), gfx.height());
}

void MeinSpiel::draw() {
  // Alles in den Framebuffer zeichnen
  fb_.fillScreen(TFT_BLACK);
  fb_.fillCircle(85, 160, 20, TFT_RED);

  // Einmalig auf das Display pushen (flackerfrei!)
  fb_.pushSprite(0, 0);
}

void MeinSpiel::quit() {
  fb_.deleteSprite();  // Speicher freigeben
}
```

### Wichtige TFT_eSPI-Befehle

| Befehl | Beschreibung | Beispiel |
|--------|-------------|----------|
| `fillScreen(color)` | Bildschirm löschen | `tft.fillScreen(TFT_BLACK)` |
| `drawString(text, x, y, font)` | Text ausgeben | `tft.drawString("HP: 3", 5, 5, 2)` |
| `setTextColor(fg, bg)` | Textfarbe setzen | `tft.setTextColor(TFT_WHITE, TFT_BLACK)` |
| `setTextDatum(datum)` | Textausrichtung | `tft.setTextDatum(TC_DATUM)` // top-center |
| `fillRect(x,y,w,h,color)` | Rechteck füllen | `tft.fillRect(10, 10, 50, 20, TFT_RED)` |
| `drawRect(x,y,w,h,color)` | Rechteck-Rahmen | `tft.drawRect(10, 10, 50, 20, TFT_WHITE)` |
| `fillCircle(cx,cy,r,color)` | Kreis füllen | `tft.fillCircle(85, 160, 20, TFT_BLUE)` |
| `drawCircle(cx,cy,r,color)` | Kreis-Rahmen | `tft.drawCircle(85, 160, 20, TFT_WHITE)` |
| `drawLine(x1,y1,x2,y2,color)` | Linie zeichnen | `tft.drawLine(0, 0, 169, 319, TFT_GREEN)` |
| `fillTriangle(x1,y1,x2,y2,x3,y3,color)` | Dreieck füllen | `tft.fillTriangle(85,140, 65,180, 105,180, TFT_YELLOW)` |
| `drawPixel(x, y, color)` | Einzelpixel | `tft.drawPixel(85, 160, TFT_WHITE)` |
| `setRotation(r)` | Display-Rotation | `tft.setRotation(1)` // 0-3 |
| `pushImage(x, y, w, h, data)` | Bitmap laden | `tft.pushImage(0, 0, 32, 32, meinSprite)` |

### Fonts (1-8 verfügbar)

```cpp
tft.drawString("Klein",  10, 10, 1);  // Font 1: klein (8px)
tft.drawString("Normal", 10, 30, 2);  // Font 2: normal (16px)
tft.drawString("Groß",   10, 50, 4);  // Font 4: groß (26px)
tft.drawString("Riesig", 10, 80, 7);  // Font 7: riesig (48px)
tft.drawString("Zahl",   10, 140, 8); // Font 8: nur Ziffern (75px)
```

## Zeit & Framerate

### Delta-Time (`dtMs`)

Die Engine ruft `update()` mit dem Zeitdelta seit dem letzten Frame auf:

```cpp
void MeinSpiel::update(const InputState& in, uint32_t dtMs) {
  // dtMs = Millisekunden seit letztem Frame (~16ms bei 60 FPS)

  // Frame-unabhängige Bewegung:
  float geschwindigkeit = 100.0f;  // Pixel pro Sekunde
  playerX += geschwindigkeit * (dtMs / 1000.0f);
}
```

### Eigenes Timing

```cpp
// In der Klasse:
uint32_t letzteAktionMs = 0;
uint32_t aktionCooldown = 500;  // 500ms Cooldown

void MeinSpiel::update(const InputState& in, uint32_t dtMs) {
  uint32_t jetzt = millis();

  if (in.pressed[1] && jetzt - letzteAktionMs > aktionCooldown) {
    // Aktion ausführen
    letzteAktionMs = jetzt;
  }
}
```

## Zufallsgenerator

Die Engine initialisiert `randomSeed()` automatisch:

```cpp
// Würfelwurf W6 (1-6)
int wuerfel = random(1, 7);

// Zufällige Auswahl
if (random(100) < 25) {  // 25% Chance
  // kritischer Treffer!
}

// Zufällige Position
int x = random(10, 160);
```

## Speicher-Management (ESP32-S3)

| Ressource | Größe | Hinweis |
|-----------|-------|---------|
| Interner RAM | ~512 KB | Variablen, Stack |
| PSRAM | 8 MB | Große Framebuffer, Bitmaps |
| Flash | 16 MB | Programm, PROGMEM-Daten |

### PROGMEM für konstante Daten

```cpp
// Bitmap im Flash speichern (spart RAM)
const uint16_t meinSprite[] PROGMEM = {
  0xF800, 0xF800, 0x07E0, 0x07E0,  // ...
};

// Aus Flash laden
tft.pushImage(x, y, 16, 16, meinSprite);
```

### PSRAM für große Objekte

```cpp
// Framebuffer liegen automatisch im PSRAM (bei TFT_eSprite)
TFT_eSprite fb_{&gfx.tft()};
fb_.createSprite(170, 320);  // ~106 KB im PSRAM
```

## Sprite-Themes (austauschbare Grafik)

NeonRun demonstriert das Sprite-Theme-System (`src/games/neonrun/sprites.h`):

```cpp
// Eigenes Theme implementieren:
class MeinBitmapTheme : public SpriteRenderer {
public:
  const NeonPalette& palette() const override { return meinePalette; }
  void drawPlayer(TFT_eSprite& fb, int16_t x, int16_t y, bool visible) override {
    // Eigenes Bitmap zeichnen statt Vektorgrafik
    fb.pushImage(x, y, 16, 16, meinPlayerSprite);
  }
  // ... weitere Methoden
};

// In init() aktivieren:
void MeinSpiel::init() {
  gSprites = &meinBitmapTheme;  // statt neonVectorTheme
}
```

## Best Practices

### 1. `draw()` nicht blockieren

```cpp
// ❌ Schlecht: Lange Berechnung in draw()
void draw() {
  for (int i = 0; i < 10000; i++) { /* ... */ }  // Blockiert!
  // zeichnen...
}

// ✅ Gut: Berechnung in update(), nur Zeichnen in draw()
void update(const InputState& in, uint32_t dtMs) {
  // Logik hier
}
void draw() {
  // Nur zeichnen
}
```

### 2. Framebuffer für Animationen

```cpp
// ❌ Schlecht: Direkt auf Display (flackert)
tft.fillScreen(TFT_BLACK);  // Flackern!
tft.fillCircle(x, y, r, TFT_RED);

// ✅ Gut: Framebuffer (flackerfrei)
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, r, TFT_RED);
fb_.pushSprite(0, 0);  // Einmalig pushen
```

### 3. Konstanten für Magische Zahlen

```cpp
// ❌ Schlecht
class MeinSpiel : public Game {
  float x = 85;  // Was bedeutet 85?
};

// ✅ Gut
class MeinSpiel : public Game {
  static constexpr int16_t MITTE_X = 85;
  static constexpr int16_t SPIELER_Y = 262;
  float x = MITTE_X;
};
```

### 4. Zustandsmaschine für Spielphasen

```cpp
enum Phase : uint8_t { TITEL, SPIEL, GAMEOVER, SIEG };

Phase phase_ = TITEL;

void update(const InputState& in, uint32_t dtMs) {
  switch (phase_) {
    case TITEL:
      if (in.pressed[1]) phase_ = SPIEL;
      break;
    case SPIEL:
      // Spiellogik
      if (gameOver) phase_ = GAMEOVER;
      break;
    case GAMEOVER:
      if (in.longPressed[1]) wantsMenu = true;
      break;
  }
}
```

## Debugging

### Serieller Monitor

```bash
pio device monitor  # 115200 Baud
```

```cpp
// In deinem Code:
Serial.printf("Score: %d, HP: %d, Phase: %d\n", score, hp, phase);
Serial.println(F("Debug: Spieler kollidiert"));  // F() spart RAM
```

### Häufige Fehler

| Problem | Ursache | Lösung |
|---------|---------|--------|
| Schwarzer Screen | Power-Hold GPIO15 nicht HIGH | Engine::begin() nicht ändern |
| Flackernde Grafik | Direkt auf Display gezeichnet | Framebuffer (TFT_eSprite) nutzen |
| Taster reagieren nicht | Falscher Pin oder aktiv HIGH | Board-Config prüfen (aktiv LOW) |
| Build-Fehler TFT_eSPI | Falsche Version | Auf 2.5.43 pinnen (platformio.ini) |
| "Sketch too big" | Partition zu klein | partitions_16mb.csv prüfen |

## Weiterführende Links

- [TFT_eSPI Dokumentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [Projekt-Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)
