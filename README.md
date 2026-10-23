# Eldoria – Fantasy-Adventure für das LilyGo T-Display-S3

Ein komplettes Text-Adventure mit gezeichneten Portraits, Kampf-Würfeln, Inventar
und Gold-System. Steuerung komplett über die **zwei Taster** des Boards
(**BOOT = GPIO0** und **frei belegbarer Taster = GPIO14**). Ziel: die gestohlene
**Mondkrone** des Drachen Morgath zurückholen – oder den Drachen besiegen.

Spielzeit: je nach Weg ca. **20–40 Minuten**, mehrere Enden (Sieg, Flucht, Game Over).

## Bedienung

| Aktion | Taster |
|---|---|
| Nächste Textseite / Option hoch | **B (GPIO14) kurz** |
| Vorherige Textseite / Option runter | **A (GPIO0) kurz** |
| Option auswählen / Kampf würfeln / Neustart | **B (GPIO14) lang halten (~0,7 s)** |
| Inventar öffnen | **A (GPIO0) lang halten (~1,2 s)** |
| Im Kampf fliehen | **B lang halten** |

> ⚠️ **Achtung:** GPIO0 ist der BOOT-Taster. Zum **Flashen** weiterhin:
> BOOT halten → RST drücken → loslassen → Upload starten.

## Bibliotheken

1. **TFT_eSPI** (Bodmer) – über den **Library Manager** installieren.
2. Danach `User_Setup_Select.h` in der TFT_eSPI-Installation bearbeiten:
   - `Arduino/libraries/TFT_eSPI/User_Setup_Select.h`
   - Zeile `// #include <User_Setup.h>` auskommentieren (falls aktiv)
   - Zeile `#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>` **einkommentieren**
3. **Tipp:** Wer Probleme mit TFT_eSPI > 2.0.14 hat (weißer/schwarzer Screen),
   kann die fertige, von LilyGo gepflegte TFT_eSPI aus dem Repo
   [Xinyuan-LilyGO/T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
   (`lib/TFT_eSPI`) nehmen und die installierte ersetzen.

## Board-Einstellungen (Arduino IDE)

- **Board:** `LilyGo T-Display-S3` (im ESP32-Core ≥ 3.0 unter „ESP32S3 Dev Module"
  mit manuellem Partition-Schema, falls nicht gelistet)
- **Partition Scheme:** `16M Flash (3MB APP/9.5MB FATFS)` – das Board hat 16 MB Flash
- **PSRAM:** `Enabled` (8 MB Octal PSRAM – nicht zwingend nötig für dieses Spiel)
- **USB CDC On Boot:** `Enabled`
- **Upload Mode:** `UART0 / Hardware CDC`
- **CPU Frequency:** 240 MHz

## Verifizierte Hardware-Pins (aus dem LilyGo-Repo)

| Funktion | Pin |
|---|---|
| TFT ST7789 (8-bit parallel) | CS=6, DC=7, WR=8, RD=9, RST=5, BL=38, D0–D7=39,40,41,42,45,46,47,48 |
| Taster A (BOOT) | GPIO0 (aktiv LOW) |
| Taster B (frei) | GPIO14 (aktiv LOW) |
| Power-Hold (muss HIGH sein!) | GPIO15 |
| Batterie-ADC | GPIO4 (im Sketch ungenutzt) |

## Spielprinzip

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

## Story-Übersicht (alle Szenen)

`Titel → Zwergenhall → Wald → Markt / Berge → Aelindril → Brunnen / Rätsel →
Gletscherspalte → Umweg → Brücke (Troll) → Elfentor → Elfenhain → Höhle →
Seitengang → Turm → Kampf (Drache) → Thronsaal / Game Over / Sieg-Flucht`

## Troubleshooting

| Problem | Lösung |
|---|---|
| Weißer/schwarzer Screen, keine Grafik | `Setup206` in `User_Setup_Select.h` prüfen; TFT_eSPI ≤ 2.0.14 oder LilyGo-TFT_eSPI verwenden |
| Board wird nicht erkannt | Anderes USB-Kabel/Port testen; BOOT+RST-Sequenz wiederholen |
| Taster reagieren nicht / komisch | Beide Taster sind aktiv LOW – im Sketch bereits berücksichtigt |
| „Sketch too big" | Partition Scheme `16M Flash (3MB APP/9.5MB FATFS)` wählen |
| Nach Upload kein Serial-Monitor | „USB CDC On Boot" auf `Enabled` stellen |

## Erweiterungsideen

- Speichern/Laden via Preferences (NVS)
- Mehr Szenen (der `Scene`-Struct ist leicht erweiterbar)
- Soundeffekte über I2S/PCM5102 (z. B. Würfel-Sound, Fanfare beim Sieg)
- Farbige Hintergründe pro Biom (Wald grün, Berg grau, Höhle schwarz)
