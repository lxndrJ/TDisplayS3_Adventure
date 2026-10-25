# 🤖 Agent-Integration

Anleitung für KI-Coding-Agenten (Claude, GitHub Copilot, Cursor, Mistral etc.), die an diesem Projekt arbeiten.

## Schnell-Referenz

| Was | Datei |
|-----|-------|
| Agent-Richtlinien | [AGENTS.md](../../AGENTS.md) |
| Copilot-Instructions | [.github/copilot-instructions.md](../../.github/copilot-instructions.md) |
| Contributing | [CONTRIBUTING.md](../../CONTRIBUTING.md) |
| API-Referenz | [API-Referenz](API-Referenz) |
| Schnellstart neues Spiel | [Schnellstart-Spielideen](Schnellstart-Spielideen) |

## Projekt verstehen (in 2 Minuten)

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Dein Spiel │────▶│    Engine   │────▶│    Board    │
│  (games/)   │     │   (core/)   │     │  (boards/)  │
└─────────────┘     └─────────────┘     └─────────────┘
       │                    │                   │
   implementiert      nutzt InputState      definiert GPIOs
   Game-Interface     und gfx.tft()         und Display-Geometrie
```

**Kernprinzip:** Spiele kennen keine GPIOs und keine Display-Treiber – nur die Schnittstellen in `src/core/`.

## Erste Schritte für Agenten

### 1. Projekt-Struktur kennenlernen

```bash
# Wichtige Dateien lesen
cat AGENTS.md                    # Diese Regeln
cat src/core/game.h              # Game-Interface
cat src/games/games.h            # Game-Registry
cat src/games/neonrun/neonrun.h  # Beispiel: Action-Spiel
cat src/games/eldoria/eldoria.h  # Beispiel: Text-Adventure
```

### 2. Build testen

```bash
pio run  # Muss fehlerfrei bauen!
```

### 3. Änderung planen

| Aufgabe | Vorgehen |
|---------|----------|
| Neues Spiel | `src/games/<name>/` anlegen, in `games.h` registrieren |
| Bestehendes Spiel erweitern | `src/games/<name>/<name>.cpp` bearbeiten |
| Engine-Feature | `src/core/` – **nur nach Rücksprache!** |
| Neues Board | `src/boards/board_<name>.h` + `platformio.ini` |
| Dokumentation | `docs/wiki/*.md` |

## Kritische Regeln (Nie brechen!)

### 1. Keine GPIO-Nummern in Spielen

```cpp
// ❌ FALSCH
if (digitalRead(0) == LOW) { ... }
pinMode(14, INPUT_PULLUP);

// ✅ RICHTIG
if (in.pressed[0]) { ... }  // Über InputState
// Pins: board::PIN_BTN_A, board::PIN_BTN_B
```

### 2. Display nur über `gfx`

```cpp
// ❌ FALSCH
TFT_eSPI myTft;
myTft.init();

// ✅ RICHTIG
#include "core/display.h"
auto& tft = gfx.tft();
```

### 3. Framebuffer für Animationen

```cpp
// ❌ FALSCH – Flackern!
tft.fillScreen(TFT_BLACK);
tft.fillCircle(x, y, 20, TFT_RED);

// ✅ RICHTIG – Flackerfrei
TFT_eSprite fb_{&gfx.tft()};  // Member
// init: fb_.createSprite(gfx.width(), gfx.height());
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, 20, TFT_RED);
fb_.pushSprite(0, 0);  // Einmalig pushen!
```

### 4. Kein `delay()` im Game-Loop

```cpp
// ❌ FALSCH
void update(const InputState& in, uint32_t dtMs) {
  delay(500);  // Blockiert die Engine!
}

// ✅ RICHTIG – Delta-Time nutzen
void update(const InputState& in, uint32_t dtMs) {
  position_ += speed_ * (dtMs / 1000.0f);
}
```

### 5. TFT_eSPI nicht updaten

```ini
; platformio.ini – gepinnt!
lib_deps =
  bodmer/TFT_eSPI@2.5.43
```

Neuere Versionen haben bekannte Probleme (siehe [TFT_eSPI#3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)).

## Neues Spiel erstellen (Schritt-für-Schritt)

### Schritt 1: Dateien anlegen

```bash
mkdir -p src/games/meinspiel
```

### Schritt 2: Header (`src/games/meinspiel/meinspiel.h`)

```cpp
#ifndef GAMES_MEINSPIEL_MEINSPIEL_H
#define GAMES_MEINSPIEL_MEINSPIEL_H

#include "core/game.h"
#include "core/display.h"

class MeinSpielGame : public Game {
public:
  const char* name() const override { return "Mein Spiel"; }
  void init() override;
  void update(const InputState& in, uint32_t dtMs) override;
  void draw() override;
  void quit() override;

private:
  int score_ = 0;
  TFT_eSprite fb_{&gfx.tft()};
};

#endif
```

### Schritt 3: Implementierung (`src/games/meinspiel/meinspiel.cpp`)

```cpp
#include "meinspiel.h"

void MeinSpielGame::init() {
  fb_.setColorDepth(16);
  fb_.createSprite(gfx.width(), gfx.height());
  score_ = 0;
}

void MeinSpielGame::update(const InputState& in, uint32_t dtMs) {
  if (in.pressed[0]) score_ += 10;       // A kurz
  if (in.longPressed[1]) wantsMenu = true; // B lang → Menü
}

void MeinSpielGame::draw() {
  fb_.fillScreen(TFT_BLACK);
  fb_.setTextColor(TFT_WHITE);
  fb_.drawString("Score: " + String(score_), 10, 10, 4);
  fb_.pushSprite(0, 0);
}

void MeinSpielGame::quit() {
  fb_.deleteSprite();
}
```

### Schritt 4: Registrieren (`src/games/games.h`)

```cpp
#include "meinspiel/meinspiel.h"  // Hinzufügen

inline Game* makeGame(uint8_t index) {
  static EldoriaGame  eldoria;
  static NeonRunGame  neonRun;
  static MeinSpielGame meinSpiel;  // Hinzufügen
  switch (index) {
    case 0:  return &eldoria;
    case 1:  return &neonRun;
    case 2:  return &meinSpiel;    // Hinzufügen
    default: return nullptr;
  }
}

inline constexpr uint8_t GAME_COUNT = 3;  // 2 → 3
```

### Schritt 5: Testen

```bash
pio run -t upload
```

## Git-Workflow für Agenten

```bash
# 1. Branch erstellen
git checkout -b vibe/<feature-slug>-1d87f7

# 2. Änderungen machen + testen
pio run  # Muss bauen!

# 3. Committen
git add -A
git commit -m "feat: Neues Spiel 'Mein Spiel' hinzufügen"

# 4. Pushen
git push -u origin vibe/<feature-slug>-1d87f7

# 5. PR öffnen (Draft)
cat > /tmp/pr-body.md <<'EOF'
## Summary
- Was wurde geändert?

## Verification
- [ ] `pio run` baut fehlerfrei
- [ ] Auf Hardware getestet (wenn möglich)
EOF
gh pr create --repo lxndrJ/TDisplayS3_Adventure --draft \
  --head vibe/<feature-slug>-1d87f7 \
  --title "feat: <Beschreibung>" \
  --body-file /tmp/pr-body.md
```

## Häufige Aufgaben

| Aufgabe | Dateien | Anleitung |
|---------|---------|-----------|
| Neues Spiel | `src/games/<name>/` | Siehe oben |
| Bestehendes Spiel erweitern | `src/games/<name>/<name>.cpp` | Direkt bearbeiten |
| Sprite-Theme ändern | `src/games/neonrun/sprites.*` | Neues Theme implementieren |
| Neues Board | `src/boards/board_<name>.h`, `platformio.ini` | [Board-Support](Board-Support) |
| Engine-Feature | `src/core/` | **Nur nach Rücksprache!** |
| Dokumentation | `docs/wiki/*.md` | Markdown bearbeiten |
| Bugfix | Betroffene Datei | [Troubleshooting](Troubleshooting) konsultieren |

## Debugging

### Serieller Monitor

```bash
pio device monitor  # 115200 Baud
```

### Debug-Ausgabe im Code

```cpp
Serial.printf("Score: %d, Phase: %d\n", score_, phase_);
Serial.println(F("Debug: Spieler kollidiert"));  // F() spart RAM
```

### Häufige Fehler

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Schwarzer Screen, Backlight an | `USE_HSPI_PORT` fehlt oder falsche Pins | `include/tft_setup.h` prüfen |
| Schwarzer Screen, Backlight aus | Power-Hold GPIO15 nicht HIGH | `Engine::begin()` Reihenfolge prüfen |
| Flackernde Animation | Direkt auf Display gezeichnet | Framebuffer (`TFT_eSprite`) nutzen |
| Taster reagiert nicht | Aktiv HIGH statt LOW | `buttons.h` ist aktiv LOW – Pin prüfen |
| Build-Fehler TFT_eSPI | Falsche Version | Auf 2.5.43 pinnen |
| "Sketch too big" | Partition zu klein | `partitions_16mb.csv` prüfen |

## Speicher-Optimierung

```cpp
// PROGMEM für konstante Daten (spart RAM)
const uint16_t sprite[] PROGMEM = { 0xF800, 0x07E0, ... };
tft.pushImage(x, y, w, h, sprite);

// Große Objekte im PSRAM
TFT_eSprite fb_{&gfx.tft()};  // Liegt automatisch im PSRAM

// RAM-Status prüfen
Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
```

## Was Agenten NICHT tun sollen

- ❌ TFT_eSPI updaten (gepinnt auf 2.5.43)
- ❌ `Engine::begin()` Reihenfolge ändern (Power-Hold vor Display-Init)
- ❌ Direkte GPIO-Zugriffe in Spielen
- ❌ `delay()` in `update()` oder `draw()`
- ❌ Große Stack-Variablen (Stack Overflow!)
- ❌ `String` in Performance-kritischen Pfaden
- ❌ `using namespace std;`
- ❌ Tabs statt 2 Leerzeichen

## Checkliste vor jedem Commit

- [ ] `pio run` baut fehlerfrei
- [ ] Code folgt den Regeln in `AGENTS.md`
- [ ] Keine GPIO-Nummern in Spielen
- [ ] Framebuffer für Animationen verwendet
- [ ] Dokumentation aktualisiert (falls nötig)
- [ ] Commit-Message folgt Konvention (`<typ>: <beschreibung>`)

## Hilfe & Dokumentation

| Ressource | Link |
|-----------|------|
| Wiki Home | [Home](Home) |
| API-Referenz | [API-Referenz](API-Referenz) |
| Spielentwicklung | [Spielentwicklung](Spielentwicklung) |
| Board-Support | [Board-Support](Board-Support) |
| Troubleshooting | [Troubleshooting](Troubleshooting) |
| AGENTS.md | [Agent Guidelines](../../AGENTS.md) |
| GitHub Issues | [Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues) |
| GitHub Discussions | [Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions) |
