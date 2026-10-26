# TDisplayS3 Adventure – Mini-Game-Plattform für das LilyGo T-Display-S3

Eine Sammlung von Spielen für das **LilyGo T-Display-S3** (ESP32-S3, 320×170 Display,
2 Taster), aufgebaut als **modulare Mini-Game-Engine**: Spiele kennen keine GPIOs
und keine Display-Treiber, sondern nur die Schnittstellen in `src/core/`.
Weitere Boards lassen sich durch eine einzelne Datei in `src/boards/` und einen
Eintrag in `platformio.ini` anbinden.

Nach dem Flashen startet ein **Launcher-Menü**; dort wird das gewünschte Spiel
ausgewählt. Jedes Spiel läuft komplett über die **zwei Taster** des Boards
(**A = BOOT = GPIO0** und **B = frei belegbar = GPIO14**, beide aktiv LOW).

## Enthaltene Spiele

| Spiel | Genre | Kurzbeschreibung |
|---|---|---|
| **Eldoria** | Text-Adventure (Fantasy) | Die gestohlene **Mondkrone** des Drachen Morgath zurückholen – mit Inventar, Kampf-Würfeln und mehreren Enden (20–40 Min.) |
| **Neon Run** | Action/Runner (Cyberpunk) | Auf 3 Spuren durch die Neonstadt laufen: Firewall-Balken ausweichen, Datenchips sammeln, Drohnen meiden |
| **Neon Shard** | Point-and-Click (Cyberpunk) | Comic-Geschichte: Courier Vex findet einen Datensplitter mit einer freien KI – frei werden oder verkaufen? 2 Enden |
| **Chip Slide** | Schiebepuzzle | 15 Halbleiter-Bauteile (Widerstand, Diode, LED, IC …) auf den Bauplan sortieren – flüssige Gleit-Animation, Halten = weiterschieben |

## Projektstruktur

```
include/tft_setup.h          TFT_eSPI-Setup für das Board (kein Library-Eingriff nötig)
partitions_16mb.csv          16 MB Flash: 3 MB App + ~13 MB FATFS
platformio.ini               Build-Konfiguration (ein env pro Board)
src/
├── main.cpp                 Einstiegspunkt: startet den Launcher (Menü)
├── core/                    Die Engine (board-unabhängig)
│   ├── board_config.h       Bindet das aktive Board ein (aus platformio.ini)
│   ├── buttons.h            Taster mit Entprellung + Short/Long-Erkennung
│   ├── input.h/.cpp         Logische Tasten (Key::A / Key::B) -> InputState
│   ├── display.h/.cpp       Display-Wrapper um TFT_eSPI (gfx)
│   ├── game.h               Game-Interface: init / update / draw / quit
│   └── engine.h/.cpp        Launcher-Loop (startet Spiel, wechselt bei wantsMenu)
├── boards/
│   └── board_tdisplay_s3.h  Alle GPIOs/Pins/Display-Geometrie des Boards
└── games/
    ├── games.h              Game-Registry (neue Spiele hier eintragen)
    ├── eldoria/             Eldoria (eldoria.h/.cpp)
    ├── neonrun/             Neon Run (neonrun.h/.cpp + sprites.h/.cpp)
    │   └── sprites.h/.cpp   Austauschbare Grafik: NeonPalette + SpriteRenderer
    ├── neonshard/           Neon Shard (neonshard.h/.cpp + art.h/.cpp)
    └── chipslide/           Chip Slide – Halbleiter-Schiebepuzzle
```

## Bedienung

### Im Launcher-Menü (immer gleich)

| Aktion | Taster |
|---|---|
| Vorheriges Spiel | **A (GPIO0) kurz** |
| Nächstes Spiel | **B (GPIO14) kurz** |
| Spiel starten | **B (GPIO14) lang halten (~0,7 s)** |

### In den Spielen

| Spiel | Steuerung |
|---|---|
| **Eldoria** | A kurz = hoch / vorherige Option, B kurz = runter / nächste Option, B lang = OK / Würfeln / Fliehen / Neustart, A lang = Inventar öffnen |
| **Neon Run** | A kurz = Spur nach links, B kurz = Spur nach rechts (an Titel/Game-Over/Sieg: beliebiger Druck startet), B lang = zurück zum Menü |
| **Neon Shard** | A kurz = vorheriges Ziel, B kurz = nächstes Ziel, B lang = Aktion / Dialog weiter, A lang = Dialog schneller Vorlauf |
| **Chip Slide** | A kurz = Schiebe-Richtung wechseln (Pfeil im HUD), B kurz = Stein in Pfeilrichtung schieben, B halten = automatisch weiterschieben, A lang = zurück zum Menü |

> ⚠️ **Achtung:** GPIO0 ist der BOOT-Taster. Zum **Flashen** weiterhin:
> BOOT halten → RST drücken → loslassen → Upload starten.

## Neues Spiel hinzufügen

1. `src/games/<name>/<name>.h/.cpp` anlegen, `class <Name>Game : public Game`
   implementieren (`init`, `update(const InputState&, uint32_t)`, `draw`).
2. In `src/games/games.h` einbinden und in `makeGame()` eintragen.
3. Optional in `platformio.ini` ein zweites `env:` für ein anderes Board anlegen.

Details siehe [Wiki: Spielentwicklung](docs/wiki/Spielentwicklung.md) und
[Wiki: Schnellstart](docs/wiki/Schnellstart-Spielideen.md).

## Grafik modular austauschen (Sprite-Themes)

Neon Run zeichnet nicht direkt, sondern über `SpriteRenderer`
(`src/games/neonrun/sprites.h`). Ein Theme implementiert nur:

| Methode | Inhalt |
|---|---|
| `palette()` | `NeonPalette` (alle Spielfarben als RGB565) |
| `drawBackground()` | Himmel, Sterne, Strasse (scroll-abhängig) |
| `drawPlayer()` | Spielfigur |
| `drawBarObstacle()` | Firewall-Balken mit Lücke |
| `drawDrone()` / `drawCoin()` | Gegner / Sammelobjekt |

`NeonVectorTheme` ist das Standard-Theme (reine Vektorgrafik, kein Flash-Verbrauch).
Ein Bitmap-Theme (z. B. `PROGMEM`-Sprite-Arrays via `tft.pushImage()`) implementiert
dieselben Methoden und wird in `init()` des Spiels aktiviert:

```cpp
gSprites = &meinBitmapTheme;   // in NeonRunGame::init()
```

Spiel-Logik, Kollisionen und Steuerung bleiben dabei unangetastet.

## Neues Board unterstützen

1. `src/boards/board_<name>.h` anlegen (Pins, `LONG_A_MS`/`LONG_B_MS`,
   `SCR_W`/`SCR_H`, `TFT_ROTATION`).
2. In `src/core/board_config.h` einbinden.
3. In `platformio.ini` ein `[env:<name>]` mit `-D BOARD_<NAME>` anlegen.

## Build & Flash (PlatformIO)

Voraussetzung: [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI).

```bash
pio run                 # baut für das Default-Board (env:t_display_s3)
pio run -t upload       # baut und flasht (BOOT+RST-Sequenz beachten)
pio device monitor      # serieller Monitor (115200 Baud)
```

Die TFT_eSPI-Konfiguration liegt in `include/tft_setup.h` (Setup206,
ST7789 8-bit parallel). `TFT_eSPI.h` findet sie automatisch über den
PlatformIO-Include-Pfad – die Library selbst muss **nicht** bearbeitet werden.

**Wichtig für das T-Display-S3:** `include/tft_setup.h` enthält zusätzlich
`#define USE_HSPI_PORT` – das ist der kritische Fix aus der LilyGo-gepflegten
TFT_eSPI, ohne den das 8-bit-Parallel-Display auf dem S3 schwarz bleibt
(siehe [TFT_eSPI issue #3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)).
Wir pinnen TFT_eSPI auf **2.5.43** – neuere Versionen haben auf diesem Board
bekannte Probleme (LilyGo-Wiki).

## Board-Einstellungen (automatisch via platformio.ini)

- **Board:** `esp32-s3-devkitc-1` (ESP32-S3, 240 MHz)
- **Flash:** 16 MB, QIO, `partitions_16mb.csv` (3 MB App / ~13 MB FATFS)
- **PSRAM:** 8 MB Octal (`qio_opi`) – eingeschaltet
- **USB CDC On Boot:** aktiviert (`ARDUINO_USB_CDC_ON_BOOT=1`)

## Verifizierte Hardware-Pins (aus dem LilyGo-Repo)

| Funktion | Pin |
|---|---|
| TFT ST7789 (8-bit parallel) | CS=6, DC=7, WR=8, RD=9, RST=5, BL=38, D0–D7=39,40,41,42,45,46,47,48 |
| Taster A (BOOT) | GPIO0 (aktiv LOW) |
| Taster B (frei) | GPIO14 (aktiv LOW) |
| Power-Hold (muss HIGH sein!) | GPIO15 |
| Batterie-ADC | GPIO4 (ungenutzt) |

## Spielprinzipien

### Eldoria (Text-Adventure)

- **3 Lebenspunkte** (HP-Herzen oben links), **10 Gold** Startkapital
- **Kampf:** Kurzer Druck auf A oder B würfelt einen W6. Mit der **Kampfaxt**
  (Markt, 8 Gold) gibt es +1 auf den Wurf. Troll: Treffer bei ≥ 4, Drache: ≥ 5.
- **Inventar:** Gegenstände werden automatisch geprüft (z. B. Seil an der
  Gletscherspalte, Amulett am Elfentor, Schlüssel am Turm, Apfel beim Troll)
- **Schnellster Siegweg:** Dorf → Markt (Axt kaufen) → Wald → Aelindril
  (Amulett) → Gletscherspalte (Seil) → Brücke (Troll besiegen) → Elfentor →
  Höhle → Drachen wecken → Kampf gewinnen → Krone → Thronsaal
- **Geheimer Weg:** Höhle → Krone stehlen (hinter dem Goldberg) → fliehen
  (eigenes Sieg-Ende, ohne den Drachen zu besiegen)
- **Elfen-Endweg:** Amulett + Aelindril als Flag nutzen, um Morgath
  einschlafen zu lassen (im Inventar sichtbar)

**Story-Übersicht (alle Szenen):**

`Titel → Zwergenhall → Wald → Markt / Berge → Aelindril → Brunnen / Rätsel →
Gletscherspalte → Umweg → Brücke (Troll) → Elfentor → Elfenhain → Höhle →
Seitengang → Turm → Kampf (Drache) → Thronsaal / Game Over / Sieg-Flucht`

### Neon Run (Action/Runner)

- 3 Spuren, seitlich scrollende Neonstadt
- **Firewall-Balken** mit Lücke ausweichen (3 Treffer = Game Over, blinkende
  Unverwundbarkeit danach)
- **Datenchips** sammeln – bei 25 Chips ist der Run gewonnen (Bestzeit wird
  als Highscore gehalten)
- **Drohnen** meiden; Grafik über austauschbare Sprite-Themes (Vektor oder Bitmap)

### Neon Shard (Point-and-Click)

- Comic-Geschichte in 6 Szenen (Gasse, Bar, U-Bahn, Helix-Turm, 2 Enden)
- Ziel-Icons per A/B anwählen, mit **B lang** interagieren
- Dialoge mit verzweigten Entscheidungen (Splitter behalten/verkaufen,
  Agentin Chen ablehnen oder bestechen)
- Die Szenengrafik liegt RLE-kodiert vor (Generator: `tools/gen_neonshard_art.py`)

### Chip Slide (Schiebepuzzle)

- 4×4-Feld mit 15 Bauteilen + 1 Lücke; Ziel ist der **Bauplan** (Miniatur im HUD)
- Alle 15 Symbole (R, D, LED, TR, IC, C, BAT, L, Q, SW, RLY, F, ANT, SPK, GND)
  werden prozedural gezeichnet – kein Bitmap-Verbrauch
- Der gemischte Zustand entsteht durch Zufallszüge der Lücke (immer lösbar)
- Richtig platzierte Bauteile leuchten **grün**; fertige Schaltung wird mit
  pulsierendem Rahmen gefeiert
- Zug-Zähler und Fortschritt (x/15) im HUD; **B halten** schiebt automatisch
  weiter, bis die Richtung oder das Spielfeld es nicht mehr hergibt

## Troubleshooting

| Problem | Lösung |
|---|---|
| Schwarzer Screen, Backlight an, keine Grafik | `USE_HSPI_PORT` in `include/tft_setup.h` prüfen; TFT_eSPI **nicht** auf > 2.5.43 updaten (TFT_eSPI#3329) |
| Schwarzer Screen, Backlight aus | Power-Hold GPIO15 wird in `Engine::begin()` vor `gfx.init()` gesetzt – Reihenfolge nicht ändern |
| Weißer Screen | `TFT_INVERSION_ON` in `include/tft_setup.h` prüfen |
| Build-Fehler in TFT_eSPI | `pio pkg update` ausführen; TFT_eSPI ist auf 2.5.43 gepinnt |
| Board wird nicht erkannt | Anderes USB-Kabel/Port testen; BOOT+RST-Sequenz wiederholen |
| Taster reagieren nicht / komisch | Beide Taster sind aktiv LOW – in `buttons.h` bereits berücksichtigt |
| „Sketch too big" | Partition Scheme in `platformio.ini` (`partitions_16mb.csv`) prüfen |
| Nach Upload kein Serial-Monitor | `pio device monitor` nutzen; CDC on Boot ist in `platformio.ini` gesetzt |

## Erweiterungsideen

- Grafisches Spielmenü im Launcher (`Engine` + `wantsMenu` ist vorbereitet)
- Speichern/Laden via Preferences (NVS) – z. B. Highscores, Eldoria-Spielstand
- Soundeffekte über I2S/PCM5102 (z. B. Würfel-Sound, Fanfare beim Sieg)
- Beschleuniger-Steuerung (Tilt) für Chip Slide – Game-Interface ist vorbereitet
- Farbige Hintergründe pro Biom in Eldoria (Wald grün, Berg grau, Höhle schwarz)
- Weitere Spiele in `src/games/` – Engine und Board-Layer sind wiederverwendbar
