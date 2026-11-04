# TDisplayS3 Adventure – Agent Guidance

## Build & Flash

```bash
pio run -e t_display_s3    # build for LilyGo T-Display-S3 (default)
pio run -e jc3248w535      # build for Guition 3.5" touch board
pio run -t upload          # build + flash to device
pio device monitor         # serial monitor @ 115200 baud
```

**Flash sequence for T-Display-S3:** Hold BOOT → press RST → release both.

## Critical Technical Constraints (Do Not Ignore)

### TFT_eSPI Version Pinning
TFT_eSPI is pinned to `2.5.43` in `platformio.ini`. **Never update**—newer versions have known issues on the T-Display-S3 (black screen, unstable parallel display). See [TFT_eSPI#3329](https://github.com/Bodmer/TFT_eSPI/issues/3329).

### Display Initialization Order
On boards with power-hold GPIO (`HAS_POWER_HOLD == true`), **`boardPowerOn()` MUST run before `gfx.init()`** or the display stays black. The order in `Engine::begin()` is intentional and should not be changed.

### T-Display-S3 Parallel Display Fix
The include directory contains `include/tft_setup.h` with `#define USE_HSPI_PORT`. This is required for 8-bit parallel display support on ESP32-S3. Without it, the screen stays black despite backlight being ON.

## Board Configurations

| Board | env | Display | Rotation | Buttons | Touch |
|-------|-----|---------|----------|---------|-------|
| T-Display-S3 | `t_display_s3` | ST7789 8-bit parallel | Portrait (170×320) | BOOT + GPIO14 | No |
| JC3248W535 | `jc3248w535` | AXS15231B over QSPI | Landscape (480×320) | BOOT only | Yes (integrated) |

- Board-specific constants live in `src/boards/board_<name>.h`. Only these files contain GPIO numbers.
- Core code should use abstractions: `board::PIN_BTN_A`, `board::SCR_W`, etc.
- The JC3248W535 touch panel is polled (no INT/RST) via I2C at `0x3B` @ 400kHz.

## Game Development

### Adding a New Game

1. Create `src/games/<name>/<name>.h/.cpp`:
   - Implement `class <Name>Game : public Game` with `name()`, `init()`, `update()`, `draw()`, `quit()`.
2. Include in `src/games/games.h` and add to `makeGame()` table.
3. (Optional) Add an `[env:<new_board>]` section to `platformio.ini` if supporting multiple boards.

### Adding a New Board

1. Create `src/boards/board_<name>.h` with GPIOs, timing (`LONG_A_MS`, `LONG_B_MS`), display geometry (`TFT_ROTATION`, `SCR_W/H`).
2. Include it in `src/core/board_config.h`.
3. Add `[env:<name>]` to `platformio.ini` with `-D BOARD_<NAME>` and matching display library.

### Graphics Themes (Neon Run)

Graphics are swappable via `SpriteRenderer` interface in `src/games/neonrun/sprites.h`:
- Implement `palette()`, `drawBackground()`, `drawPlayer()`, obstacle/enemy/coin draw methods.
- Vector theme (`NeonVectorTheme`) uses procedural drawing (no flash).
- Bitmap theme: store sprite arrays in `PROGMEM` and activate via `gSprites = &myBitmapTheme;`.

### Sprout Sprite Overrides

Procedurally generated art lives in `src/games/farm/art.h/.cpp` from `tools/gen_farm_art.py`. To use real PNG sprites:
1. Drop 16×16 RGB565 PNGs into `assets/farm/`.
2. The generator auto-loads them; missing ones fall back to procedural art.

## Survival Game Setup

The Survival game requires WiFi + Mistral AI credentials for the AI storyteller mode:

```bash
cp include/secrets.h.example include/secrets.h
# Edit with WIFI_SSID, WIFI_PASSWORD, MISTRAL_API_KEY
```

Without `include/secrets.h`, the game runs in offline fallback mode (built-in scenarios). The status bar shows `OFF` instead of `AI`.

Note: `include/secrets.h` is gitignored—never commit real credentials.

### Umlaut Handling

Survival's FreeSans9pt7b font supports ASCII only. German UTF-8 umlauts are folded at runtime:
- Ä/Ä → ae/Ae, Ö/Ö → oe/Oe, Ü/Ü → ue/Ue, ß → ss

Do not attempt to load external .vlw smooth fonts—they're unsupported by Arduino_GFX facade.

## QSPI Display Rendering (JC3248W535)

The AXS15231B panel over QSPI cannot address arbitrary Y per draw command. All rendering must go through a full-screen `Arduino_Canvas` frame buffer in PSRAM. The engine calls `gfx.present()` to flush once per frame (~60 FPS). This is the only reliable render path; direct pixel writes won't appear on screen.

## Testing & Verification

- A single game test: open `src/main.cpp`, modify to start that game directly instead of the launcher.
- Integration tests for touch boards require verifying touch coordinates are correctly mapped after rotation changes (`touchNotifyRotation`).
- Codegen: run `python3 tools/gen_farm_art.py` to regenerate Sprout assets.

## Common Pitfalls

| Problem | Solution |
|---------|----------|
| Black screen, backlight on | Verify `USE_HSPI_PORT` in `include/tft_setup.h`; ensure power-hold GPIO set before gfx.init() |
| White screen | Check `TFT_INVERSION_ON` flag in `include/tft_setup.h` |
| Build errors in TFT_eSPI | Run `pio pkg update` (pinned version should fix itself) |
| "Sketch too big" for JC3248W535 | Verify partition scheme: `partitions_16mb.csv` reserves ~13 MB for FATFS |
| Touch not responding on JC3248W535 | Confirm touch polling is enabled; coordinates may need rotation sync after setRotation() |
