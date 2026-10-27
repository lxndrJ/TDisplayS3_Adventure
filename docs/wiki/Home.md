---
layout: default
title: "Wiki Home"
description: "TDisplayS3 Adventure wiki: documentation for the modular mini-game engine for the LilyGo T-Display-S3 (ESP32-S3). Quickstart, game development, API reference, board support, troubleshooting."
---

# TDisplayS3 Adventure Wiki

Welcome to the wiki of the **TDisplayS3 Adventure** project – a modular game framework for the LilyGo T-Display-S3 (ESP32-S3).

## Contents

- [🏠 Home](Home.html) – this page
- [🚀 Quickstart: Turn game ideas into reality](Schnellstart-Spielideen.html) – **3 steps to your first game**
- [🎮 Game Development](Spielentwicklung.html) – Game interface, input, display, best practices
- [📖 API Reference](API-Referenz.html) – All classes, methods and constants
- [🖥️ Board Support](Board-Support.html) – Add a new board
- [🔧 Troubleshooting](Troubleshooting.html) – Common problems & solutions

## Project

- **Repository:** [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)
- **Live docs (GitHub Pages):** [chipnplay.pandango.de](https://chipnplay.pandango.de/)
- **Hardware:** LilyGo T-Display-S3 (ESP32-S3, 320×170 display, 2 buttons)
- **Framework:** PlatformIO + Arduino + TFT_eSPI

## Modules

The project is built as a **modular mini-game engine**:

```
src/
├── core/          Engine (board-independent)
│   ├── game.h     Game interface
│   ├── engine.*   Launcher loop
│   ├── input.*    Button input
│   ├── display.*  Display wrapper
│   └── buttons.h  Button debouncing
├── boards/        Board-specific configuration
│   └── board_tdisplay_s3.h
└── games/         The games
    ├── games.h      Game registry
    ├── eldoria/     Text adventure (fantasy)
    ├── neonrun/     Action runner (cyberpunk)
    │   └── sprites.*  Swappable graphics themes
    ├── neonshard/   Point-and-click (cyberpunk)
    ├── chipslide/   Sliding puzzle (15 components)
    ├── neondash/    Jump'n'run (cyberpunk)
    └── neonroll/    Marble puzzle (cyberpunk)
```

## Quick Links

| I want to... | Page |
|--------------|------|
| Create my first game | [Quickstart](Schnellstart-Spielideen.html) |
| Understand the engine | [Game Development](Spielentwicklung.html) |
| Look up a function | [API Reference](API-Referenz.html) |
| Add a new board | [Board Support](Board-Support.html) |
| Fix a problem | [Troubleshooting](Troubleshooting.html) |

## Available Games

| Game | Genre | Highlights |
|------|-------|-----------|
| **Eldoria** | Text adventure | Scenes, inventory, combat dice, multiple endings |
| **Neon Run** | Action/runner | 3 lanes, dodge firewalls, collect chips, sprite themes |
| **Neon Shard** | Point-and-click | Comic story with branching dialogs and 2 endings |
| **Chip Slide** | Sliding puzzle | Sort 15 semiconductor components, procedural symbols, smooth slide animation |
| **Neon Dash** | Jump'n'run | One-button jump over obstacles, pace rises continuously, data chip bonus |
| **Neon Roll** | Marble puzzle | Kick a glowing ball through neon mazes into the goal, impulse + friction physics, 8 themed levels |

## Help

- **Questions?** → [GitHub Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Report a bug** → [GitHub Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **Project repository** → [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)

---

[← Back to Docs Home](/)
