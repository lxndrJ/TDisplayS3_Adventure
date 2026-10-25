# 🖥️ Board-Support

Anleitung zum Hinzufügen neuer Boards (ESP32-basiert mit Display + 2 Taster).

## Unterstützte Boards

| Board | Status | Umgebungsname |
|-------|--------|---------------|
| LilyGo T-Display-S3 | ✅ Standard | `t_display_s3` |

## Neues Board hinzufügen (4 Schritte)

### Schritt 1: Board-Datei anlegen

**`src/boards/board_<name>.h`** erstellen:

```cpp
#ifndef BOARD_<NAME>_H
#define BOARD_<NAME>_H

#include <Arduino.h>

// Alle GPIO-Nummern und Display-Parameter des Boards.
// Spiele und Engine dürfen NIEMALS direkte GPIO-Nummern verwenden,
// sondern nur diese Konstanten.
namespace board {

// ---- Taster ----
constexpr uint8_t PIN_BTN_A      = 0;   // Erster Taster (z.B. BOOT)
constexpr uint8_t PIN_BTN_B      = 14;  // Zweiter Taster
constexpr bool    HAS_POWER_HOLD = true; // GPIO für Display-Strom
constexpr uint8_t PIN_POWER_ON   = 15;  // Muss HIGH bleiben

// ---- Taster-Timing ----
constexpr uint32_t LONG_A_MS   = 1200;  // ms bis Long für Taster A
constexpr uint32_t LONG_B_MS   = 700;   // ms bis Long für Taster B
constexpr uint32_t DEBOUNCE_MS = 30;    // Entprellzeit in ms

// ---- Display-Geometrie ----
// WICHTIG: SCR_W/SCR_H in der Rotation, die TFT_ROTATION ergibt!
constexpr uint8_t  TFT_ROTATION = 1;    // 0=Portrait, 1=Landscape, ...
constexpr int16_t  SCR_W = 320;         // Breite in Sketch-Ausrichtung
constexpr int16_t  SCR_H = 170;         // Höhe in Sketch-Ausrichtung

}  // namespace board

#endif
```

### Schritt 2: In board_config.h einbinden

**`src/core/board_config.h`** bearbeiten:

```cpp
#ifndef CORE_BOARD_CONFIG_H
#define CORE_BOARD_CONFIG_H

#include <Arduino.h>

#if defined(BOARD_TDISPLAY_S3)
  #include "boards/board_tdisplay_s3.h"
#elif defined(BOARD_MEIN_BOARD)        // ← NEU
  #include "boards/board_mein_board.h" // ← NEU
#else
  #error "Kein Board gewaehlt - in platformio.ini ein -D BOARD_<NAME> setzen"
#endif

#endif
```

### Schritt 3: TFT_eSPI-Setup (falls nötig)

Falls das Board einen anderen Display-Treiber oder andere Pins hat:

**`include/tft_setup_<name>.h`** anlegen (oder bestehendes anpassen):

```cpp
#ifndef TFT_SETUP_<NAME>_H
#define TFT_SETUP_<NAME>_H

// Display-Treiber wählen (einer davon):
// #define ST7789_DRIVER   // 240x320, häufig bei ESP32-Boards
// #define ILI9341_DRIVER  // 240x320 SPI
// #define ST7735_DRIVER   // 128x160 klein

#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pins (an Board anpassen!)
#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  33
#define TFT_BL   32

// Bei SPI-Displays:
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1  // oft nicht verbunden

// Bei 8-bit Parallel (wie T-Display-S3):
// #define TFT_PARALLEL_8_BIT
// #define TFT_D0 39 ... TFT_D7 48
// #define TFT_WR 8
// #define TFT_RD 9

// SPI-Frequenz (nur bei SPI-Displays)
#define SPI_FREQUENCY  40000000

#endif
```

### Schritt 4: PlatformIO-Umgebung anlegen

**`platformio.ini`** erweitern:

```ini
[platformio]
default_envs = t_display_s3

; Bestehende Umgebung...
[env:t_display_s3]
; ...

; ============================================
; Neues Board: Mein ESP32 Board
; ============================================
[env:mein_board]
platform = espressif32@^6.5.0
board = esp32dev                    ; oder esp32-s3-devkitc-1, etc.
framework = arduino
board_build.mcu = esp32             ; oder esp32s3
board_build.f_cpu = 240000000L
monitor_speed = 115200
build_flags =
  -std=gnu++17
  -D BOARD_MEIN_BOARD               ; ← Board-Name definieren
  -D ARDUINO_USB_CDC_ON_BOOT=1
  -I include
; lib_deps =                        ; bei anderem Display ggf. anpassen
;   bodmer/TFT_eSPI@2.5.43
```

## Build & Flash für neues Board

```bash
# Build für bestimmtes Board
pio run -e mein_board

# Build & Flash
pio run -e mein_board -t upload

# Serieller Monitor
pio device monitor
```

## Board-spezifische Besonderheiten

### Power-Hold (Display-Strom)

Viele ESP32-Boards mit Display benötigen einen "Power-Hold"-Pin, der HIGH bleiben muss, damit das Display Strom bekommt:

```cpp
// In board_<name>.h:
constexpr bool    HAS_POWER_HOLD = true;
constexpr uint8_t PIN_POWER_ON   = 15;  // GPIO-Nummer
```

Die Engine setzt diesen Pin in `Engine::begin()` **vor** dem Display-Init:

```cpp
// Automatisch in Engine::begin():
boardPowerOn();  // GPIO15 HIGH + 10ms warten
```

> ⚠️ **Wichtig:** Diese Reihenfolge nicht ändern, sonst bleibt das Display schwarz!

### Taster aktiv LOW vs. HIGH

Die meisten Boards haben Taster aktiv LOW (gegen GND). Falls aktiv HIGH:

```cpp
// In buttons.h muss dann angepasst werden:
// bool raw = digitalRead(btn_.pin) == HIGH;  // statt LOW
```

Aktuell sind alle Boards aktiv LOW – bei aktiv HIGH bitte `ButtonTracker` anpassen.

## Pin-Referenz gängiger Boards

### LilyGo T-Display-S3 (Referenz)

| Funktion | GPIO | Hinweis |
|----------|------|---------|
| TFT 8-bit parallel | D0-D7: 39,40,41,42,45,46,47,48 | |
| TFT CS | 6 | |
| TFT DC | 7 | |
| TFT WR | 8 | |
| TFT RD | 9 | |
| TFT RST | 5 | |
| TFT Backlight | 38 | |
| Taster A (BOOT) | 0 | aktiv LOW |
| Taster B | 14 | aktiv LOW |
| Power-Hold | 15 | muss HIGH |
| Batterie-ADC | 4 | ungenutzt |

### Typische ESP32-DevKit (SPI-Display)

| Funktion | GPIO | Hinweis |
|----------|------|---------|
| TFT MOSI | 23 | VSPI default |
| TFT SCLK | 18 | VSPI default |
| TFT CS | 5 | |
| TFT DC | 27 | |
| TFT RST | 33 | |
| TFT BL | 32 | oder 4 |
| Taster A | 0 | BOOT (meist vorhanden) |
| Taster B | 35 | oder anderer freier Pin |

## Checkliste: Neues Board testen

- [ ] `pio run -e <board>` baut ohne Fehler
- [ ] Display zeigt nach Upload etwas an (nicht schwarz/weiß)
- [ ] Beide Taster reagieren (kurz + lang)
- [ ] Backlight ist an
- [ ] `pio device monitor` zeigt Serielle Ausgaben
- [ ] Spiel-Wechsel funktioniert (wantsMenu)
- [ ] Deep Sleep / Neustart funktioniert (falls implementiert)

## Troubleshooting Board-Support

| Problem | Lösung |
|---------|--------|
| Schwarzer Screen, Backlight an | Pin-Konfiguration in `tft_setup.h` prüfen; Treiber (ST7789 vs ILI9341) prüfen |
| Schwarzer Screen, Backlight aus | `HAS_POWER_HOLD` und `PIN_POWER_ON` prüfen |
| Weiße/komische Farben | `TFT_INVERSION_ON` oder `TFT_RGB_ORDER` anpassen |
| Taster A geht nicht | GPIO0 ist BOOT – zum Flashen BOOT+RST-Sequenz beachten |
| Build-Fehler "BOARD nicht definiert" | `-D BOARD_<NAME>` in platformio.ini prüfen |
| Display flackert | SPI-Frequenz reduzieren oder 8-bit parallel nutzen |

## Weiterführende Links

- [TFT_eSPI Unterstützte Displays](https://github.com/Bodmer/TFT_eSPI#supported-displays)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [LilyGo T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- [Projekt-Repository](https://github.com/lxndrJ/TDisplayS3_Adventure)
