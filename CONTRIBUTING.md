# Contributing

Danke, dass du zum **TDisplayS3 Adventure** Projekt beitragen möchtest – egal ob Mensch oder KI-Agent!

## Entwicklungsumgebung

| Komponente | Version/Hinweis |
|------------|-----------------|
| PlatformIO | VS Code Extension oder CLI (empfohlen) |
| Python | 3.x (für PlatformIO CLI) |
| Board | LilyGo T-Display-S3 (ESP32-S3) |
| USB | Kabel mit Datenübertragung (nicht nur Ladekabel!) |

### Ersteinrichtung

```bash
# Repository klonen
git clone https://github.com/lxndrJ/TDisplayS3_Adventure.git
cd TDisplayS3_Adventure

# Abhängigkeiten installieren (automatisch beim ersten Build)
pio run

# Board anschließen und testen
pio run -t upload
pio device monitor
```

## Git-Workflow

### Branch-Namenskonvention

```
vibe/<kurzer-slug>-<hash>     # Für KI-Agenten (empfohlen)
feature/<beschreibung>        # Für menschliche Contributor
fix/<bug-beschreibung>        # Für Bugfixes
docs/<thema>                  # Für Dokumentations-Änderungen
```

### Commit-Messages

```
<typ>: <kurze Beschreibung>

<optional: längere Erklärung>
```

**Typen:**
- `feat` – Neues Feature (z.B. neues Spiel)
- `fix` – Bugfix
- `docs` – Dokumentations-Änderungen
- `refactor` – Code-Umstrukturierung (keine Funktionsänderung)
- `test` – Tests hinzufügen/ändern
- `chore` – Build, Abhängigkeiten, Konfiguration

**Beispiele:**
```
feat: Neues Spiel "Space Invaders" hinzufügen
fix: Taster-Entprellung für GPIO14 verbessern
docs: Wiki um API-Referenz erweitern
refactor: SpriteRenderer in eigenes Modul auslagern
```

### Pull Requests

1. **Branch pushen:** `git push -u origin <branch>`
2. **PR öffnen:** `gh pr create --draft` oder über GitHub Web-UI
3. **Beschreibung ausfüllen:** Siehe PR-Template unten
4. **Ready for Review** setzen, wenn fertig

**PR-Template:**

```markdown
## Summary
- Was wurde geändert und warum?

## Changes
- Datei 1: Änderung X
- Datei 2: Änderung Y

## Verification
- [ ] `pio run` baut fehlerfrei
- [ ] Auf Hardware getestet (wenn möglich)
- [ ] Dokumentation aktualisiert

## Screenshots/Videos (bei UI-Änderungen)
- Foto/Video vom Display

Closes #<issue-nummer>  (falls zutreffend)
```

## Code-Style

### C++

- **Standard:** C++17 (`-std=gnu++17`)
- **Einrückung:** 2 Leerzeichen (keine Tabs!)
- **Klammern:** Gleiche Zeile (K&R-Style)
  ```cpp
  if (condition) {
    // code
  }
  ```
- **Kommentare:** Deutsch, prägnant
- **Keine `using namespace std;`**
- **Member-Variablen:** `snake_case_` mit Unterstrich am Ende
- **Konstanten:** `UPPER_CASE`
- ** Klassen:** `PascalCase`

### Beispiel

```cpp
// Gut
class MeinSpielGame : public Game {
public:
  const char* name() const override { return "Mein Spiel"; }

  void init() override {
    score_ = 0;
    fb_.createSprite(gfx.width(), gfx.height());
  }

  void update(const InputState& in, uint32_t dtMs) override {
    if (in.pressed[1]) score_ += 10;
  }

private:
  uint16_t score_ = 0;
  TFT_eSprite fb_{&gfx.tft()};
};
```

### Dateien

- Header-Guards: `#ifndef PFAD_DATEI_H` / `#define PFAD_DATEI_H`
- Includes: Projekt-Includes zuerst, dann System/Library
- Eine Klasse pro Datei (außer kleine Helfer)

## Testing

### Manueller Test (Hardware)

```bash
# Build & Flash
pio run -t upload

# Serieller Monitor
pio device monitor
```

**Checkliste:**
- [ ] Build erfolgreich (`pio run`)
- [ ] Upload erfolgreich
- [ ] Display zeigt erwartetes Bild
- [ ] Beide Taster reagieren korrekt
- [ ] Spiel-Wechsel funktioniert (wantsMenu)
- [ ] Kein Flackern bei Animationen

### Debug-Ausgabe

```cpp
Serial.printf("Score: %d, HP: %d\n", score_, hp_);
Serial.println(F("Debug: Spieler gestorben"));  // F() spart RAM
```

## Dokumentation

- **Wiki:** `docs/wiki/*.md` (Markdown)
- **README:** `README.md` (Projekt-Übersicht)
- **Code-Kommentare:** Nur wo nötig (Code sollte selbsterklärend sein)

### Wiki-Seiten aktualisieren

Wenn du Engine-Features änderst:
1. `docs/wiki/API-Referenz.md` aktualisieren
2. `docs/wiki/Spielentwicklung.md` anpassen
3. `AGENTS.md` bei neuen Regeln ergänzen

## Review-Prozess

1. **Automatisch:** CI baut Projekt (falls eingerichtet)
2. **Manuell:** Maintainer testet auf Hardware
3. **Feedback:** Änderungswünsche als Kommentare im PR
4. **Merge:** Nach Approve durch Maintainer

## Hilfe

- **Dokumentation:** [Wiki](docs/wiki/Home.md)
- **Fragen:** [GitHub Discussions](https://github.com/lxndrJ/TDisplayS3_Adventure/discussions)
- **Bugs:** [GitHub Issues](https://github.com/lxndrJ/TDisplayS3_Adventure/issues)
- **Agent-Guidelines:** [AGENTS.md](AGENTS.md)
