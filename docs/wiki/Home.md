# TDisplayS3 Adventure Wiki

Willkommen im Wiki des **TDisplayS3 Adventure** Projekts – ein modulares Spiele-Framework für das LilyGo T-Display-S3 (ESP32-S3).

## Inhalt

- [🏠 Home](Home) – Diese Seite
- [🚀 Schnellstart: Neue Spielideen umsetzen](Schnellstart-Spielideen) – **3 Schritte zum ersten Spiel**
- [🎮 Spielentwicklung](Spielentwicklung) – Game-Interface, Input, Display, Best Practices
- [📚 API-Referenz](API-Referenz) – Alle Klassen, Methoden und Konstanten
- [🖥️ Board-Support](Board-Support) – Neues Board hinzufügen
- [🔧 Troubleshooting](Troubleshooting) – Häufige Probleme & Lösungen

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
│   └── board_tdisplay_s3.h
└── games/         Die Spiele
    ├── games.h    Game-Registry
    ├── eldoria/   Text-Adventure (Fantasy)
    └── neonrun/   Action-Runner (Cyberpunk)
        └── sprites.*  Austauschbare Grafik-Themes
```

## Schnell-Links

| Ich möchte... | Seite |
|---------------|-------|
| Mein erstes Spiel erstellen | [Schnellstart](Schnellstart-Spielideen) |
| Die Engine verstehen | [Spielentwicklung](Spielentwicklung) |
| Eine Funktion nachschlagen | [API-Referenz](API-Referenz) |
| Ein neues Board anbinden | [Board-Support](Board-Support) |
| Ein Problem lösen | [Troubleshooting](Troubleshooting) |

## Aktuell vorhandene Spiele

| Spiel | Genre | Besonderheiten |
|-------|-------|----------------|
| **Eldoria** | Text-Adventure | Szenen, Inventar, Kampf-Würfel, mehrere Enden |
| **Neon Run** | Action/Runner | 3 Spuren, Firewall ausweichen, Chips sammeln, Sprite-Themes |

## Hilfe

- **Fragen?** → [GitHub Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Bug melden** → [GitHub Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **Projekt-Repository** → [lxndrJ/TDisplayS3_Adventure](https://github.com/lxndrJ/TDisplayS3_Adventure)
