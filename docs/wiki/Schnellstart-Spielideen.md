# 🚀 Schnellstart: Neue Spielideen umsetzen

Diese Anleitung zeigt, wie du schnell eine neue Spielidee auf dem T-Display-S3 umsetzt.

## Voraussetzungen

- [PlatformIO](https://platformio.org/) installiert (VS Code Extension oder CLI)
- LilyGo T-Display-S3 angeschlossen
- Projekt geklont: `git clone https://github.com/lxndrJ/TDisplayS3_Adventure.git`

## Schnellstart in 3 Schritten

### Schritt 1: Neues Spiel anlegen

```bash
# Ordner erstellen
mkdir -p src/games/meinspiel
```

**`src/games/meinspiel/meinspiel.h`** – minimale Vorlage:

```cpp
#ifndef MEINSPIEL_H
#define MEINSPIEL_H
#include "core/game.h"

class MeinSpielGame : public Game {
public:
  const char* name() const override { return "Mein Spiel"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
private:
  // Dein Spielzustand hier
  int score = 0;
};
#endif
```

**`src/games/meinspiel/meinspiel.cpp`** – Implementierung:

```cpp
#include "meinspiel.h"
#include "core/display.h"

void MeinSpielGame::init() {
  gfx.tft().fillScreen(TFT_BLACK);
  score = 0;
}

void MeinSpielGame::update(const InputState& in, uint32_t dtMs) {
  // Steuerung: in.aShort, in.bShort, in.aLong, in.bLong
  if (in.aShort) score += 10;
  if (in.bLong) wantsMenu = true;  // zurück zum Menü
}

void MeinSpielGame::draw() {
  auto& tft = gfx.tft();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Score: " + String(score), 10, 10, 4);
}
```

### Schritt 2: In Registry eintragen

**`src/games/games.h`** bearbeiten:

```cpp
#include "meinspiel/meinspiel.h"  // hinzufügen

inline Game* makeGame(uint8_t index) {
  static EldoriaGame  eldoria;
  static NeonRunGame  neonRun;
  static MeinSpielGame meinSpiel;  // hinzufügen
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &meinSpiel;    // hinzufügen
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 3;  // 2 → 3
```

### Schritt 3: Build & Flash

```bash
pio run -t upload    # baut und flasht
```

## Steuerung (T-Display-S3)

| Aktion | Taster |
|--------|--------|
| Nächste Option / rechts | **B (GPIO14) kurz** |
| Vorherige Option / links | **A (GPIO0) kurz** |
| Auswählen / Bestätigen | **B (GPIO14) lang (~0,7 s)** |
| Inventar / Spezial | **A (GPIO0) lang (~1,2 s)** |

> ⚠️ **Zum Flashen:** BOOT halten → RST drücken → loslassen → Upload starten

## Engine-Features (bereits fertig)

| Feature | Zugriff |
|---------|---------|
| **Taster-Eingaben** | `InputState` mit `aShort`, `bShort`, `aLong`, `bLong` |
| **Display zeichnen** | `gfx.tft()` – direkter TFT_eSPI-Zugriff |
| **Text ausgeben** | `tft.drawString(text, x, y, font)` |
| **Formen zeichnen** | `tft.fillRect()`, `tft.drawCircle()`, etc. |
| **Farben** | `TFT_BLACK`, `TFT_WHITE`, `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`, `TFT_YELLOW` (RGB565) |
| **Zurück zum Menü** | `wantsMenu = true` setzen |
| **Spiel-Name** | `name()` überschreiben |

## Vorlagen zum Kopieren

| Genre | Vorlage | Datei |
|-------|---------|-------|
| **Text-Adventure** | Szenen, Inventar, Kampf-Würfel | `src/games/eldoria/eldoria.cpp` |
| **Action/Runner** | Game-Loop, Kollision, Sprites | `src/games/neonrun/neonrun.cpp` |
| **Sprite-Grafik** | Austauschbare Themes | `src/games/neonrun/sprites.h` |

## Schneller Test-Workflow

Um schnell zu iterieren ohne jedes Mal das Menü zu wechseln:

1. Ändere `BOOT_GAME` in `src/games/games.h` auf deinen Spiel-Index:
   ```cpp
   inline constexpr uint8_t BOOT_GAME = 2;  // dein Spiel direkt starten
   ```
2. `pio run -t upload`
3. Testen → Code ändern → wiederholen

## Display-Spezifikationen

| Eigenschaft | Wert |
|-------------|------|
| Auflösung | 320 × 170 Pixel |
| Farbtiefe | 16-bit RGB565 |
| Rotation | 1 (Landscape) |
| Display-Treiber | ST7789 (8-bit parallel) |

## Nützliche TFT_eSPI-Befehle

```cpp
auto& tft = gfx.tft();

// Bildschirm löschen
tft.fillScreen(TFT_BLACK);

// Text
tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Vordergrund, Hintergrund
tft.drawString("Hallo", 10, 10, 2);      // Font 1-8

// Formen
tft.fillRect(x, y, w, h, TFT_RED);
tft.drawRect(x, y, w, h, TFT_WHITE);
tft.fillCircle(cx, cy, r, TFT_GREEN);
tft.drawLine(x1, y1, x2, y2, TFT_BLUE);

// Pixel
drawPixel(x, y, color);

// Rotation
tft.setRotation(0);  // 0=Portrait, 1=Landscape, 2=Portrait-180, 3=Landscape-180
```

## Weiterführende Links

- [TFT_eSPI Dokumentation](https://github.com/Bodmer/TFT_eSPI)
- [ESP32-S3 Arduino Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO Dokumentation](https://docs.platformio.org/)
- [LilyGo T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)

## Hilfe

- **Fragen?** → [GitHub Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Bug melden** → [GitHub Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **Projekt-Repository** → [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)
