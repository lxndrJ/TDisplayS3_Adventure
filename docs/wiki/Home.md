# TDisplayS3 Adventure Wiki

Willkommen im Wiki des **TDisplayS3 Adventure** Projekts – ein modulares Spiele-Framework für das LilyGo T-Display-S3 (ESP32-S3).

## Inhalt

- [🏠 Home](Home) – Diese Seite
- [🚀 Schnellstart: Neue Spielideen umsetzen](Schnellstart-Spielideen)
- [🎮 Spielentwicklung](Spielentwicklung)
- [🖥️ Board-Support](Board-Support)
- [🔧 Troubleshooting](Troubleshooting)

## Projekt

- **Repository:** [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)
- **Hardware:** LilyGo T-Display-S3 (ESP32-S3, 320×170 Display, 2 Taster)
- **Framework:** PlatformIO + Arduino + TFT_eSPI

## Module

Das Projekt ist als **modulare Mini-Game-Engine** aufgebaut:

```
src/
├── core/          Engine (board-unabhängig)
│   ├── game.h     Game-Interface
│   ├── engine.*   Launcher-Loop
│   ├── input.*    Taster-Eingaben
│   ├── display.*  Display-Wrapper
│   └── buttons.h  Taster-Entprellung
├── boards/        Board-spezifische Konfiguration
└── games/         Die Spiele
    ├── eldoria/   Text-Adventure
    └── neonrun/   Action-Runner
```
