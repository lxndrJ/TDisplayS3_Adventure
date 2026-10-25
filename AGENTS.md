# Agent Guidelines

Anleitung für KI-Coding-Agenten (Claude, Copilot, Cursor, Mistral etc.), die an diesem Projekt arbeiten.

## Projekt-Übersicht

**TDisplayS3 Adventure** – Modulares Spiele-Framework für das LilyGo T-Display-S3 (ESP32-S3).

- **Hardware:** ESP32-S3, 320×170 Display (ST7789, 8-bit parallel), 2 Taster, 16 MB Flash, 8 MB PSRAM
- **Framework:** PlatformIO + Arduino + TFT_eSPI 2.5.43 (gepinnt!)
- **Sprache:** C++17 (`-std=gnu++17`)
- **Dokumentation:** Deutsch

## Build & Flash

```bash
pio run                 # baut für env:t_display_s3
pio run -t upload       # baut und flasht (BOOT+RST-Sequenz beachten!)
pio device monitor      # serieller Monitor (115200 Baud)
pio run -e <env>        # baut für spezifisches Board
```

> ⚠️ **Flash-Sequenz:** BOOT halten → RST drücken → BOOT loslassen → Upload starten

## Projektstruktur

```
src/
├── core/              Engine (board-unabhängig) – NIEMALS direkt ändern ohne Absprache
│   ├── game.h         Game-Interface (init/update/draw/quit)
│   ├── engine.*       Launcher-Loop (~60 FPS)
│   ├── input.*        InputState (Key::A/Key::B)
│   ├── display.*      Display-Wrapper (gfx.tft())
│   ├── buttons.h      Taster-Entprellung (aktiv LOW)
│   └── board_config.h Bindet aktives Board ein
├── boards/            Board-spezifische Konfiguration (GPIOs, Pins)
│   └── board_tdisplay_s3.h
└── games/             Die Spiele
    ├── games.h        Game-Registry (neue Spiele hier eintragen)
    ├── eldoria/       Text-Adventure (Fantasy)
    └── neonrun/       Action-Runner (Cyberpunk) + Sprite-Themes
```

## Kritische Regeln

| Regel | Warum |
|-------|-------|
| **Keine GPIO-Nummern in Spielen!** | Nur `board::PIN_BTN_A`, `board::SCR_W` etc. verwenden |
| **Display nur über `gfx.tft()`** | Niemals eigene TFT_eSPI-Instanz erstellen |
| **Input nur über `InputState`** | Niemals `digitalRead()` direkt aufrufen |
| **Framebuffer für Animationen** | `TFT_eSprite` nutzen – direktes Zeichnen flackert |
| **TFT_eSPI auf 2.5.43 pinnen** | Neuere Versionen haben bekannte Probleme (TFT_eSPI#3329) |
| **Power-Hold GPIO15 vor Display-Init** | Sonst bleibt Screen schwarz (in `Engine::begin()`) |
| **Beide Taster sind aktiv LOW** | Bereits in `buttons.h` berücksichtigt |

## Neues Spiel anlegen (Checkliste)

1. **Ordner erstellen:** `src/games/<name>/`
2. **Header:** `src/games/<name>/<name>.h`
   ```cpp
   #ifndef GAMES_<NAME>_<NAME>_H
   #define GAMES_<NAME>_<NAME>_H
   #include "core/game.h"
   class <Name>Game : public Game {
   public:
     const char* name() const override { return "<Name>"; }
     void init() override;
     void update(const InputState& in, uint32_t dtMs) override;
     void draw() override;
   };
   #endif
   ```
3. **Implementierung:** `src/games/<name>/<name>.cpp`
4. **Registry:** In `src/games/games.h` einbinden + in `makeGame()` eintragen + `GAME_COUNT` erhöhen
5. **Test:** `pio run -t upload`

## Input-Handling

```cpp
// In update(const InputState& in, uint32_t dtMs):
if (in.pressed[0])        { /* A kurz (Loslassen) */ }
if (in.pressed[1])        { /* B kurz (Loslassen) */ }
if (in.longPressed[0])    { /* A lang (~1200ms) */ }
if (in.longPressed[1])    { /* B lang (~700ms) */ }
if (in.down[0])           { /* A gehalten */ }
uint32_t hold = in.holdMs[1];  // Haltezeit in ms

// Zurück zum Menü:
wantsMenu = true;
```

## Display-Zeichnen

```cpp
#include "core/display.h"

// Einfach:
auto& tft = gfx.tft();
tft.fillScreen(TFT_BLACK);
tft.drawString("Score: 42", 10, 10, 2);

// Mit Framebuffer (empfohlen für Animationen):
TFT_eSprite fb_{&gfx.tft()};  // Member-Variable
// in init():
fb_.createSprite(gfx.width(), gfx.height());
// in draw():
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, r, TFT_RED);
fb_.pushSprite(0, 0);  // Einmalig pushen!
// in quit():
fb_.deleteSprite();
```

## Wichtige Konstanten

| Konstante | Wert | Bedeutung |
|-----------|------|-----------|
| `gfx.width()` | 170 | Display-Breite (Portrait) |
| `gfx.height()` | 320 | Display-Höhe (Portrait) |
| `board::PIN_BTN_A` | 0 | BOOT-Taster (aktiv LOW) |
| `board::PIN_BTN_B` | 14 | Zweiter Taster (aktiv LOW) |
| `board::PIN_POWER_ON` | 15 | Power-Hold (muss HIGH) |
| `board::LONG_A_MS` | 1200 | ms bis Long-Press A |
| `board::LONG_B_MS` | 700 | ms bis Long-Press B |
| `board::DEBOUNCE_MS` | 30 | Entprellzeit |

## Typische Aufgaben

| Aufgabe | Vorgehen |
|---------|----------|
| Neues Spiel | Siehe Checkliste oben + `docs/wiki/Schnellstart-Spielideen.md` |
| Grafik-Theme ändern | `src/games/neonrun/sprites.h` – neues Theme implementieren |
| Neues Board | `src/boards/board_<name>.h` + Eintrag in `board_config.h` + `platformio.ini` |
| Bug in Engine | `src/core/` – nur nach Rücksprache ändern! |
| Dokumentation | `docs/wiki/` – Markdown-Dateien |

## Git-Workflow

```bash
# Branch erstellen
git checkout -b vibe/<feature-slug>-1d87f7

# Änderungen committen
git add -A
git commit -m "<typ>: <kurze Beschreibung>

# Typen: feat, fix, docs, refactor, test, chore"

# Pushen
git push -u origin vibe/<feature-slug>-1d87f7

# PR öffnen (Draft)
gh pr create --repo lxndrJ/TDisplayS3_Adventure --draft --head <branch> --title "..." --body-file /tmp/pr-body.md
```

## Was Agenten NICHT tun sollen

- ❌ TFT_eSPI updaten (auf 2.5.43 gepinnt)
- ❌ `Engine::begin()` Reihenfolge ändern (Power-Hold vor Display-Init)
- ❌ Direkte GPIO-Zugriffe in Spielen (`pinMode`, `digitalRead`, `digitalWrite`)
- ❌ `delay()` in `update()` oder `draw()` (blockiert die Engine)
- ❌ Große Stack-Variablen (Stack Overflow auf ESP32)
- ❌ `String` in Performance-kritischen Pfaden (Heap-Fragmentierung)

## Weiterführende Dokumentation

- [Wiki Home](docs/wiki/Home.md)
- [Schnellstart: Spielideen umsetzen](docs/wiki/Schnellstart-Spielideen.md)
- [Spielentwicklung](docs/wiki/Spielentwicklung.md)
- [API-Referenz](docs/wiki/API-Referenz.md)
- [Board-Support](docs/wiki/Board-Support.md)
- [Troubleshooting](docs/wiki/Troubleshooting.md)
- [Agent-Integration](docs/wiki/Agent-Integration.md)
