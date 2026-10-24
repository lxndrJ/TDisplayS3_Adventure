# 🔧 Troubleshooting

Häufige Probleme und Lösungen für das TDisplayS3 Adventure Projekt.

## Display-Probleme

### Schwarzer Screen, Backlight an, keine Grafik

| Ursache | Lösung |
|---------|--------|
| `USE_HSPI_PORT` fehlt | In `include/tft_setup.h` prüfen – muss definiert sein! |
| TFT_eSPI zu neu | **Nicht** auf > 2.5.43 updaten (bekannte Probleme, siehe [TFT_eSPI#3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)) |
| Falscher Display-Treiber | `ST7789_DRIVER` in `tft_setup.h` prüfen |
| Falsche Pins | Alle TFT-Pins in `tft_setup.h` mit Board-Doku abgleichen |

**Schnell-Check:**
```cpp
// In tft_setup.h müssen stehen:
#define USE_HSPI_PORT
#define ST7789_DRIVER
#define TFT_PARALLEL_8_BIT
#define TFT_INVERSION_ON
```

### Schwarzer Screen, Backlight aus

| Ursache | Lösung |
|---------|--------|
| Power-Hold GPIO15 nicht HIGH | Wird in `Engine::begin()` vor `gfx.init()` gesetzt – **Reihenfolge nicht ändern!** |

**Code-Prüfung (`src/core/engine.h`):**
```cpp
void begin(Game* game) {
  boardPowerOn();  // ← MUSS vor gfx.init() stehen!
  gfx.init();
  // ...
}
```

### Weißer Screen / komische Farben

| Ursache | Lösung |
|---------|--------|
| Inversion falsch | `TFT_INVERSION_ON` in `include/tft_setup.h` prüfen |
| RGB/BGR vertauscht | `TFT_RGB_ORDER TFT_RGB` (nicht TFT_BGR!) in `tft_setup.h` |
| CGRAM Offset | `CGRAM_OFFSET` definiert lassen |

### Display flackert / Artefakte

| Ursache | Lösung |
|---------|--------|
| SPI zu schnell | `SPI_FREQUENCY` reduzieren (auf 40 MHz oder weniger) |
| Direkt auf Display zeichnen | **Framebuffer nutzen** (`TFT_eSprite`) – siehe [Spielentwicklung](Spielentwicklung#framebuffer) |
| DMA-Konflikte | Bei 8-bit parallel: `USE_HSPI_PORT` verwenden |

## Taster-Probleme

### Taster reagieren nicht / komisch

| Ursache | Lösung |
|---------|--------|
| Aktiv HIGH statt LOW | Beide Taster sind **aktiv LOW** – in `buttons.h` bereits berücksichtigt |
| Entprellung zu kurz | `DEBOUNCE_MS` in `board_tdisplay_s3.h` erhöhen (Standard: 30ms) |
| Falscher Pin | GPIO0 (BOOT) und GPIO14 prüfen |
| GPIO0 blockiert | BOOT-Taster: Flash-Sequenz beachten (siehe unten) |

### Taster A (GPIO0 / BOOT) geht nicht im Spiel

GPIO0 ist der **BOOT-Taster**. Zum Flashen:

```
1. BOOT-Taster halten
2. RST (Reset) drücken und loslassen
3. BOOT loslassen
4. Upload starten (pio run -t upload)
```

Im normalen Betrieb funktioniert GPIO0 als Eingang.

## Build-Probleme

### Build-Fehler in TFT_eSPI

```bash
# Abhängigkeiten aktualisieren
pio pkg update

# TFT_eSPI-Version prüfen (muss 2.5.43 sein)
pio pkg list | grep TFT_eSPI
```

Falls eine neuere Version installiert wurde:
```ini
; In platformio.ini pinnen:
lib_deps =
  bodmer/TFT_eSPI@2.5.43
```

### "Sketch too big" / Partition-Fehler

| Ursache | Lösung |
|---------|--------|
| App-Partition zu klein | `partitions_16mb.csv` prüfen (3 MB App) |
| Falsches Partition-Schema | In `platformio.ini`: `board_build.partitions = partitions_16mb.csv` |

**Partition-Datei prüfen (`partitions_16mb.csv`):**
```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,  ← 3 MB App
storage,  data, fat,     0x310000,0xCF0000,  ← ~13 MB FATFS
```

### Board wird nicht erkannt

```bash
# USB-Gerät prüfen
pio device list

# Auf Linux:
lsusb | grep -i esp
# oder
ls /dev/tty*
```

| Lösung | Befehl |
|--------|--------|
| Anderes USB-Kabel/Port testen | – |
| BOOT+RST-Sequenz wiederholen | Siehe oben |
| Treiber installieren (Windows) | [CP210x USB-UART Treiber](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| Berechtigungen (Linux) | `sudo usermod -a -G dialout $USER` (Neuanmeldung nötig) |

## Serieller Monitor

### Nach Upload kein Serial-Monitor

```bash
# CDC on Boot muss aktiviert sein (in platformio.ini):
build_flags =
  -D ARDUINO_USB_CDC_ON_BOOT=1

# Monitor starten:
pio device monitor
# oder mit Baudrate:
pio device monitor -b 115200
```

### Monitor zeigt nur Kauderwelsch

| Ursache | Lösung |
|---------|--------|
| Falsche Baudrate | `monitor_speed = 115200` in `platformio.ini` prüfen |
| Anderer Port | `pio device list` zeigt verfügbare Ports |

## Speicher-Probleme

### Guru Meditation Error / Crash

| Ursache | Lösung |
|---------|--------|
| Stack Overflow | Große lokale Variablen als `static` oder Member deklarieren |
| Null-Pointer | Immer auf `nullptr` prüfen |
| PSRAM nicht initialisiert | `board_build.arduino.memory_type = qio_opi` in `platformio.ini` |

**Debug-Ausgabe aktivieren:**
```ini
; In platformio.ini für mehr Debug-Info:
build_flags =
  -D CORE_DEBUG_LEVEL=5  ; 0=none, 5=verbose
```

### Framebuffer-Allokation schlägt fehl

```cpp
// Prüfen ob genügend PSRAM frei ist:
Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

// Framebuffer-Größe reduzieren oder auf PSRAM auslagern:
TFT_eSprite fb_{&gfx.tft()};
fb_.setColorDepth(8);  // 8-bit statt 16-bit (spart 50% Speicher)
fb_.createSprite(170, 160);  // Kleineren Bereich
```

## Performance-Probleme

### Niedrige Framerate / Ruckeln

| Ursache | Lösung |
|---------|--------|
| Zu viel in `draw()` | Logik nach `update()` verschieben |
| Direkt auf Display zeichnen | Framebuffer (`TFT_eSprite`) nutzen |
| Zu große Sprites | Sprite-Größe reduzieren |
| `delay()` in Game-Loop | Niemals `delay()` in `update()`/`draw()` verwenden! |

### Flackernde Animation

```cpp
// ❌ Schlecht: Direkt auf Display (flackert beim Löschen)
tft.fillScreen(TFT_BLACK);
tft.fillCircle(x, y, 20, TFT_RED);

// ✅ Gut: Framebuffer (flackerfrei)
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, 20, TFT_RED);
fb_.pushSprite(0, 0);  // Einmalig pushen
```

## Netzwerk/WiFi (zukünftig)

| Problem | Hinweis |
|---------|---------|
| WiFi funktioniert nicht | Noch nicht implementiert – siehe [Erweiterungsideen](https://github.com/lxndrJ/TDisplayS3_Adventure#erweiterungsideen) |
| Bluetooth | Noch nicht implementiert |

## Debug-Checkliste

Wenn nichts funktioniert, systematisch prüfen:

```bash
# 1. Build erfolgreich?
pio run

# 2. Upload erfolgreich?
pio run -t upload

# 3. Serial-Monitor aktiv?
pio device monitor

# 4. Richtiges Board ausgewählt?
pio run -e t_display_s3

# 5. USB-Verbindung OK?
pio device list

# 6. Code-Version aktuell?
git pull origin main
pio pkg update
```

## Hilfe bekommen

- **GitHub Issues:** [Probleme melden](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **GitHub Discussions:** [Fragen stellen](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Wiki:** [Weitere Dokumentation](Home)
- **LilyGo T-Display-S3:** [Offizielles Repo](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- **TFT_eSPI Issues:** [Display-Probleme](https://github.com/Bodmer/TFT_eSPI/issues)
