---
layout: default
title: "Troubleshooting"
description: "Common problems and solutions for the TDisplayS3 Adventure project: display issues, button problems, build errors, serial monitor, memory and performance problems, debug checklist."
---

# 🔧 Troubleshooting

Common problems and solutions for the TDisplayS3 Adventure project.

## Display Problems

### Black Screen, Backlight On, No Graphics

| Cause | Solution |
|-------|----------|
| `USE_HSPI_PORT` missing | Check `include/tft_setup.h` - must be defined! |
| TFT_eSPI too new | **Do not** update past 2.5.43 (known problems, see [TFT_eSPI#3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)) |
| Wrong display driver | Check `ST7789_DRIVER` in `tft_setup.h` |
| Wrong pins | Match all TFT pins in `tft_setup.h` with the board docs |

**Quick check:**
```cpp
// In tft_setup.h these must be present:
#define USE_HSPI_PORT
#define ST7789_DRIVER
#define TFT_PARALLEL_8_BIT
#define TFT_INVERSION_ON
```

### Black Screen, Backlight Off

| Cause | Solution |
|-------|----------|
| Power-hold GPIO15 not HIGH | Set in `Engine::begin()` before `gfx.init()` - **do not change the order!** |

**Code check (`src/core/engine.h`):**
```cpp
void begin(Game* game) {
  boardPowerOn();  // <- MUST come before gfx.init()!
  gfx.init();
  // ...
}
```

### White Screen / Odd Colors

| Cause | Solution |
|-------|----------|
| Inversion wrong | Check `TFT_INVERSION_ON` in `include/tft_setup.h` |
| RGB/BGR swapped | `TFT_RGB_ORDER TFT_RGB` (not TFT_BGR!) in `tft_setup.h` |
| CGRAM offset | Leave `CGRAM_OFFSET` defined |

### Display Flickers / Artifacts

| Cause | Solution |
|-------|----------|
| SPI too fast | Reduce `SPI_FREQUENCY` (to 40 MHz or less) |
| Drawing directly to the display | **Use a framebuffer** (`TFT_eSprite`) - see [Game Development](Spielentwicklung.html#framebuffer-for-flicker-free-animation-recommended) |
| DMA conflicts | For 8-bit parallel: use `USE_HSPI_PORT` |

## Button Problems

### Buttons Not Responding / Erratic

| Cause | Solution |
|-------|----------|
| Active HIGH instead of LOW | Both buttons are **active LOW** - already handled in `buttons.h` |
| Debounce too short | Increase `DEBOUNCE_MS` in `board_tdisplay_s3.h` (default: 30ms) |
| Wrong pin | Check GPIO0 (BOOT) and GPIO14 |
| GPIO0 blocked | BOOT button: mind the flash sequence (see below) |

### Button A (GPIO0 / BOOT) Does Not Work in-Game

GPIO0 is the **BOOT button**. To flash:

```
1. Hold the BOOT button
2. Press and release RST (Reset)
3. Release BOOT
4. Start the upload (pio run -t upload)
```

In normal operation GPIO0 works as an input.

## Build Problems

### Build Errors in TFT_eSPI

```bash
# Update dependencies
pio pkg update

# Check the TFT_eSPI version (must be 2.5.43)
pio pkg list | grep TFT_eSPI
```

If a newer version got installed:
```ini
; Pin in platformio.ini:
lib_deps =
  bodmer/TFT_eSPI@2.5.43
```

### "Sketch too big" / Partition Error

| Cause | Solution |
|-------|----------|
| App partition too small | Check `partitions_16mb.csv` (3 MB app) |
| Wrong partition scheme | In `platformio.ini`: `board_build.partitions = partitions_16mb.csv` |

**Check the partition file (`partitions_16mb.csv`):**
```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,  <- 3 MB app
storage,  data, fat,     0x310000,0xCF0000,  <- ~13 MB FATFS
```

### Board Not Detected

```bash
# Check the USB device
pio device list

# On Linux:
lsusb | grep -i esp
# or
ls /dev/tty*
```

| Solution | Command |
|----------|---------|
| Try another USB cable/port | – |
| Repeat the BOOT+RST sequence | See above |
| Install drivers (Windows) | [CP210x USB-UART driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| Permissions (Linux) | `sudo usermod -a -G dialout $USER` (re-login required) |

## Serial Monitor

### No Serial Monitor After Upload

```bash
# CDC on boot must be enabled (in platformio.ini):
build_flags =
  -D ARDUINO_USB_CDC_ON_BOOT=1

# Start the monitor:
pio device monitor
# or with baud rate:
pio device monitor -b 115200
```

### Monitor Shows Only Gibberish

| Cause | Solution |
|-------|----------|
| Wrong baud rate | Check `monitor_speed = 115200` in `platformio.ini` |
| Wrong port | `pio device list` shows available ports |

## Memory Problems

### Guru Meditation Error / Crash

| Cause | Solution |
|-------|----------|
| Stack overflow | Declare large local variables as `static` or members |
| Null pointer | Always check for `nullptr` |
| PSRAM not initialized | `board_build.arduino.memory_type = qio_opi` in `platformio.ini` |

**Enable debug output:**
```ini
; In platformio.ini for more debug info:
build_flags =
  -D CORE_DEBUG_LEVEL=5  ; 0=none, 5=verbose
```

### Framebuffer Allocation Fails

```cpp
// Check whether enough PSRAM is free:
Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

// Reduce the framebuffer size or move to PSRAM:
TFT_eSprite fb_{&gfx.tft()};
fb_.setColorDepth(8);  // 8-bit instead of 16-bit (saves 50% memory)
fb_.createSprite(170, 160);  // smaller area
```

## Performance Problems

### Low Frame Rate / Stuttering

| Cause | Solution |
|-------|----------|
| Too much in `draw()` | Move logic into `update()` |
| Drawing directly to the display | Use a framebuffer (`TFT_eSprite`) |
| Sprites too large | Reduce sprite size |
| `delay()` in the game loop | Never use `delay()` in `update()`/`draw()`! |

### Flickering Animation

```cpp
// ❌ Bad: directly to the display (flickers when clearing)
tft.fillScreen(TFT_BLACK);
tft.fillCircle(x, y, 20, TFT_RED);

// ✅ Good: framebuffer (flicker-free)
fb_.fillScreen(TFT_BLACK);
fb_.fillCircle(x, y, 20, TFT_RED);
fb_.pushSprite(0, 0);  // push once
```

## Network/WiFi (Future)

| Problem | Note |
|---------|------|
| WiFi does not work | Not implemented yet - see [Ideas for the Future](https://github.com/lxndrJ/TDisplayS3_Adventure#ideas-for-the-future) |
| Bluetooth | Not implemented yet |

## Debug Checklist

If nothing works, check systematically:

```bash
# 1. Build successful?
pio run

# 2. Upload successful?
pio run -t upload

# 3. Serial monitor active?
pio device monitor

# 4. Correct board selected?
pio run -e t_display_s3

# 5. USB connection OK?
pio device list

# 6. Code up to date?
git pull origin main
pio pkg update
```

## Getting Help

- **GitHub Issues:** [Report problems](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **GitHub Discussions:** [Ask questions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Wiki:** [More documentation](Home.html)
- **LilyGo T-Display-S3:** [Official repo](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- **TFT_eSPI Issues:** [Display problems](https://github.com/Bodmer/TFT_eSPI/issues)

---

[← Wiki Home](Home.html) · [Docs Home](/)
