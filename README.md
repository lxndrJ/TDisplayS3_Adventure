# TDisplayS3 Adventure – Mini-Game Platform for the LilyGo T-Display-S3

A collection of games for the **LilyGo T-Display-S3** (ESP32-S3, 320×170 display,
2 buttons), built as a **modular mini-game engine**: games know neither GPIOs
nor display drivers, only the interfaces in `src/core/`. Additional boards can
be added via a single file in `src/boards/` and an entry in `platformio.ini`.

After flashing, a **launcher menu** starts; pick the game you want to play. Every
game is controlled entirely with the board's **two buttons**
(**A = BOOT = GPIO0** and **B = free = GPIO14**, both active LOW).

**Live documentation:** [chipnplay.pandango.de](https://chipnplay.pandango.de/)

### Documentation (GitHub Pages)

| Page | URL |
|------|-----|
| Wiki Home | [chipnplay.pandango.de/wiki/](https://chipnplay.pandango.de/wiki/Home.html) |
| Quickstart: Write Your Own Game | [Quickstart](https://chipnplay.pandango.de/wiki/Schnellstart-Spielideen.html) |
| Game Development Guide | [Game Development](https://chipnplay.pandango.de/wiki/Spielentwicklung.html) |
| API Reference | [API Reference](https://chipnplay.pandango.de/wiki/API-Referenz.html) |
| Board Support | [Board Support](https://chipnplay.pandango.de/wiki/Board-Support.html) |
| Troubleshooting | [Troubleshooting](https://chipnplay.pandango.de/wiki/Troubleshooting.html) |

## Included Games

| Game | Genre | Short description |
|---|---|---|
| **Eldoria** | Text adventure (fantasy) | Recover the stolen **Moon Crown** of the dragon Morgath – with inventory, combat dice and multiple endings (20–40 min) |
| **Neon Run** | Action/runner (cyberpunk) | Run across 3 lanes through the neon city: dodge firewall bars, collect data chips, avoid drones |
| **Neon Shard** | Point-and-click (cyberpunk) | Comic story: courier Vex finds a data shard with a free AI – set it free or sell it? 2 endings |
| **Chip Slide** | Sliding puzzle | Sort 15 semiconductor components (resistor, diode, LED, IC …) onto the blueprint – smooth slide animation, hold to keep sliding |
| **Neon Dash** | Jump'n'run (cyberpunk) | One button = jump over firewall pillars and floating drones – the pace keeps rising, data chips as bonus |
| **Neon Roll** | Marble puzzle (cyberpunk) | Kick a glowing ball through neon mazes into the goal socket – impulse + friction physics, 8 themed levels |

## Project Structure

```
include/tft_setup.h          TFT_eSPI setup for the board (no library edits needed)
partitions_16mb.csv          16 MB flash: 3 MB app + ~13 MB FATFS
platformio.ini               Build configuration (one env per board)
src/
├── main.cpp                 Entry point: starts the launcher (menu)
├── core/                    The engine (board-independent)
│   ├── board_config.h       Includes the active board (from platformio.ini)
│   ├── buttons.h            Button debouncing + short/long detection
│   ├── input.h/.cpp         Logical keys (Key::A / Key::B) -> InputState
│   ├── display.h/.cpp       Display wrapper around TFT_eSPI (gfx)
│   ├── game.h               Game interface: init / update / draw / quit
│   └── engine.h/.cpp        Launcher loop (starts game, switches on wantsMenu)
├── boards/
│   └── board_tdisplay_s3.h  All GPIOs/pins/display geometry of the board
└── games/
    ├── games.h              Game registry (register new games here)
    ├── eldoria/             Eldoria (eldoria.h/.cpp)
    ├── neonrun/             Neon Run (neonrun.h/.cpp + sprites.h/.cpp)
    │   └── sprites.h/.cpp   Swappable graphics: NeonPalette + SpriteRenderer
    ├── neonshard/           Neon Shard (neonshard.h/.cpp + art.h/.cpp)
    ├── chipslide/           Chip Slide – semiconductor sliding puzzle
    ├── neondash/            Neon Dash – cyberpunk jump'n'run
    └── neonroll/            Neon Roll – neon marble puzzle
```

## Controls

### In the launcher menu (always the same)

| Action | Button |
|---|---|
| Previous game | **A (GPIO0) short** |
| Next game | **B (GPIO14) short** |
| Start game | **B (GPIO14) long (~0.7 s)** |

### In the games

| Game | Controls |
|---|---|
| **Eldoria** | A short = up / previous option, B short = down / next option, B long = OK / roll / flee / restart, A long = open inventory |
| **Neon Run** | A short = lane left, B short = lane right (on title/game-over/win: any press starts), A long = restart, B long = back to menu |
| **Neon Shard** | A short = previous target, B short = next target, B long = action / continue dialog, A long = fast-forward dialog |
| **Chip Slide** | A short = change slide direction (arrow in the HUD), B short = slide tile in arrow direction, B hold = keep sliding automatically, A long = back to menu |
| **Neon Dash** | A or B short = jump (on title/game-over: any press starts/restarts), B long = back to menu |
| **Neon Roll** | A short/hold = kick ball in arrow direction, B short = rotate direction 90°, A long = back to menu |

> ⚠️ **Note:** GPIO0 is the BOOT button. To **flash**, keep using:
> hold BOOT → press RST → release → start upload.

## Adding a New Game

1. Create `src/games/<name>/<name>.h/.cpp`, implement `class <Name>Game : public Game`
   (`init`, `update(const InputState&, uint32_t)`, `draw`).
2. Include it in `src/games/games.h` and add it to `makeGame()`.
3. Optionally add a second `env:` in `platformio.ini` for another board.

See [Game Development](https://chipnplay.pandango.de/wiki/Spielentwicklung.html)
and [Quickstart](https://chipnplay.pandango.de/wiki/Schnellstart-Spielideen.html)
for details.

## Swapping Graphics Modularly (Sprite Themes)

Neon Run does not draw directly, but through `SpriteRenderer`
(`src/games/neonrun/sprites.h`). A theme implements only:

| Method | Content |
|---|---|
| `palette()` | `NeonPalette` (all game colors as RGB565) |
| `drawBackground()` | Sky, stars, road (scroll-dependent) |
| `drawPlayer()` | Player figure |
| `drawBarObstacle()` | Firewall bar with a gap |
| `drawDrone()` / `drawCoin()` | Enemy / collectible |

`NeonVectorTheme` is the standard theme (pure vector graphics, no flash usage).
A bitmap theme (e.g. `PROGMEM` sprite arrays via `tft.pushImage()`) implements
the same methods and is activated in the game's `init()`:

```cpp
gSprites = &myBitmapTheme;   // in NeonRunGame::init()
```

Game logic, collisions and controls stay untouched.

## Adding a New Board

1. Create `src/boards/board_<name>.h` (pins, `LONG_A_MS`/`LONG_B_MS`,
   `SCR_W`/`SCR_H`, `TFT_ROTATION`).
2. Include it in `src/core/board_config.h`.
3. Add an `[env:<name>]` with `-D BOARD_<NAME>` in `platformio.ini`.

## Build & Flash (PlatformIO)

Requires [PlatformIO](https://platformio.org/) (VS Code extension or CLI).

```bash
pio run                 # build for the default board (env:t_display_s3)
pio run -t upload       # build and flash (mind the BOOT+RST sequence)
pio device monitor      # serial monitor (115200 baud)
```

The TFT_eSPI configuration lives in `include/tft_setup.h` (Setup206,
ST7789 8-bit parallel). `TFT_eSPI.h` finds it automatically via the
PlatformIO include path – the library itself must **not** be edited.

**Important for the T-Display-S3:** `include/tft_setup.h` additionally contains
`#define USE_HSPI_PORT` – this is the critical fix from LilyGo's TFT_eSPI fork;
without it the 8-bit parallel display on the S3 stays black
(see [TFT_eSPI issue #3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)).
We pin TFT_eSPI to **2.5.43** – newer versions have known problems on this
board (LilyGo wiki).

## Board Settings (automatic via platformio.ini)

- **Board:** `esp32-s3-devkitc-1` (ESP32-S3, 240 MHz)
- **Flash:** 16 MB, QIO, `partitions_16mb.csv` (3 MB app / ~13 MB FATFS)
- **PSRAM:** 8 MB octal (`qio_opi`) – enabled
- **USB CDC on boot:** enabled (`ARDUINO_USB_CDC_ON_BOOT=1`)

## Verified Hardware Pins (from the LilyGo repo)

| Function | Pin |
|---|---|
| TFT ST7789 (8-bit parallel) | CS=6, DC=7, WR=8, RD=9, RST=5, BL=38, D0–D7=39,40,41,42,45,46,47,48 |
| Button A (BOOT) | GPIO0 (active LOW) |
| Button B (free) | GPIO14 (active LOW) |
| Power-hold (must stay HIGH!) | GPIO15 |
| Battery ADC | GPIO4 (unused) |

## Gameplay

### Eldoria (text adventure)

- **3 hit points** (HP hearts top left), **10 gold** starting capital
- **Combat:** A short press on A or B rolls a W6. The **battle axe**
  (market, 8 gold) gives +1 to the roll. Troll: hit at ≥ 4, dragon: ≥ 5.
- **Inventory:** items are checked automatically (e.g. rope at the glacier
  gap, amulet at the elven gate, key at the tower, apple at the troll)
- **Fastest win path:** Village → Market (buy axe) → Forest → Aelindril
  (amulet) → Glacier gap (rope) → Bridge (beat the troll) → Elven gate →
  Cave → wake the dragon → win the fight → crown → throne hall
- **Secret path:** Cave → steal the crown (behind the gold mound) → flee
  (own victory ending, without beating the dragon)
- **Elven ending:** use amulet + Aelindril as a flag to lull Morgath to sleep
  (visible in the inventory)

**Story overview (all scenes):**

`Title → Dwarvenhall → Forest → Market / Mountains → Aelindril → Well / Riddle →
Glacier gap → Detour → Bridge (troll) → Elven gate → Elven court → Cave →
Side passage → Tower → Fight (dragon) → Throne hall / Game Over / Victory-flee`

### Neon Run (action/runner)

- 3 lanes, side-scrolling neon city
- **Firewall bars** with a gap to dodge (3 hits = game over, blinking
  invulnerability afterwards)
- **Data chips** to collect – at 25 chips the run is won (best score is kept)
- **Drones** to avoid; graphics via swappable sprite themes (vector or bitmap)

### Neon Shard (point-and-click)

- Comic story across 6 scenes (alley, bar, subway, Helix tower, 2 endings)
- Select target icons with A/B, interact with **B long**
- Dialogs with branching decisions (keep/sell the shard, refuse or bribe
  agent Chen)
- Scene art is stored RLE-encoded (generator: `tools/gen_neonshard_art.py`)

### Chip Slide (sliding puzzle)

- 4×4 field with 15 components + 1 gap; goal is the **blueprint** (miniature in the HUD)
- All 15 symbols (R, D, LED, TR, IC, C, BAT, L, Q, SW, RLY, F, ANT, SPK, GND)
  are drawn procedurally – no bitmap usage
- The shuffled state is created by random gap moves (always solvable)
- Correctly placed components glow **green**; the finished circuit is
  celebrated with a pulsing frame
- Move counter and progress (x/15) in the HUD; **hold B** to keep sliding
  until the direction or the board no longer allows it

### Neon Dash (jump'n'run)

- Fixed player on the left, obstacles approach from the right: **firewall
  pillars** (16–36 px tall) and **floating drones** (sinusoidal, low enough to
  hit, high enough to jump over)
- **One button**: A or B short = jump (physics-based with gravity, ~70 px jump
  height, no double jump)
- **Gets faster continuously**: 150 px/s → 470 px/s (+2.6 px/s per second);
  spawn distances shrink accordingly but stay fair (> 200 px)
- **Data chips** float above tall pillars – collecting one gives +25 m bonus
- Score = meters run + chip bonus; highscore (RAM only) is kept
- HUD: speed bar, current meters, highscore; control hint at the bottom

### Neon Roll (marble puzzle)

- Glowing ball rolls through procedurally generated **neon mazes** (10×14 cells)
- **No full control**: A short/hold = impulse ("kick") in the current direction,
  B short = rotate direction 90° clockwise
- Physics: momentum, exponential friction, wall bounce (restitution 0.62),
  speed cap — the ball glides and ricochets until it rests
- **Goal capture**: the ball must be slow enough (< 110 px/s) and inside
  the glowing goal socket — fast balls pass through!
- **Bumpers** (diamond cells) bounce the ball with extra energy
- **8 levels** with different graphic themes: Neon Grid (cyan), Magenta Maze,
  Toxic Green, Solar Flare, Ice Cave, Purple Haze, Laser Red, Gold Rush
- Wall density and bumper count scale with the level; every level is
  verified solvable via BFS flood-fill at generation time
- Trail glow behind the ball, direction ghost arrow shows the next kick
- HUD: level + theme name, kick counter, direction arrow; best-kick tracking

## Troubleshooting

| Problem | Solution |
|---|---|
| Black screen, backlight on, no graphics | Check `USE_HSPI_PORT` in `include/tft_setup.h`; do **not** update TFT_eSPI beyond 2.5.43 (TFT_eSPI#3329) |
| Black screen, backlight off | Power-hold GPIO15 is set in `Engine::begin()` before `gfx.init()` – do not change the order |
| White screen | Check `TFT_INVERSION_ON` in `include/tft_setup.h` |
| Build errors in TFT_eSPI | Run `pio pkg update`; TFT_eSPI is pinned to 2.5.43 |
| Board not detected | Try another USB cable/port; repeat the BOOT+RST sequence |
| Buttons not responding / erratic | Both buttons are active LOW – already handled in `buttons.h` |
| "Sketch too big" | Check the partition scheme in `platformio.ini` (`partitions_16mb.csv`) |
| No serial monitor after upload | Use `pio device monitor`; CDC on boot is set in `platformio.ini` |

## Ideas for the Future

- Graphical game menu in the launcher (`Engine` + `wantsMenu` is ready)
- Save/load via Preferences (NVS) – e.g. highscores, Eldoria save state
- Sound effects via I2S/PCM5102 (e.g. dice sound, victory fanfare)
- Accelerometer (tilt) control for Chip Slide – the game interface is ready
- Colored backgrounds per biome in Eldoria (forest green, mountain grey, cave black)
- More games in `src/games/` – engine and board layer are reusable
