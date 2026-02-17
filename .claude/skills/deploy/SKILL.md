---
name: deploy
description: |
  Erstellt oder aktualisiert den deployment-Branch, bereinigt von persönlichen Pfaden und
  maschinenspezifischen Informationen. Durchsucht alle Markdown- und Konfigurationsdateien
  nach lokalen Pfaden, generalisiert oder entfernt sie, und committet das Ergebnis auf den
  deployment-Branch. Verwende diesen Skill, wenn das Repository für die Weitergabe vorbereitet
  werden soll.
  Triggers: "deploy", "deployment", "deployment-branch", "persönliche Pfade entfernen".
allowed-tools: Bash, Read, Edit, Write, Glob, Grep
---

# deploy — Deployment-Branch bereinigen

Erstellt oder aktualisiert den `deployment`-Branch, bereinigt von persönlichen Pfaden und maschinenspezifischen Informationen.

## Persönliche Pfadmuster

Folgende Muster gelten als persönlich und dürfen im deployment-Branch nicht vorkommen:

| Muster | Beschreibung |
|---|---|
| `/Users/framlin/` | Absoluter Home-Pfad |
| `~/tinker/` | Lokale Projektverzeichnisse |
| `~/obsidian/` | Lokale Wissensbasis |
| `~/studium/` | Lokale Studium-Verzeichnisse |
| `~/STM32Cube/` | Lokales STM32Cube-Repository |
| `/opt/homebrew/` | Homebrew-Installationspfade |
| `/Applications/STMicroelectronics/` | Lokale IDE-Installationen |
| `~/.platformio/` | PlatformIO-Verzeichnis |

## Generalisierungsregeln

Persönliche Pfade werden je nach Kontext unterschiedlich behandelt:

### Generalisieren (Pfad durch Platzhalter ersetzen)

| Persönlicher Pfad | Generalisiert |
|---|---|
| `~/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.1/...` | `<STM32Cube_FW_G4>/...` |
| `~/tinker/audio-samples/` | `<audio-samples>/` |
| `/opt/homebrew/bin/<tool>` | `<tool>` (nur Toolname ohne Pfad) |
| `/Users/framlin/.../build/Debug/blinky.elf` | `build/Debug/blinky.elf` (relativer Pfad) |

### Entfernen (Zeile oder Sektion komplett löschen)

- Einzelne Referenzzeilen auf lokale Dateisystempfade (z.B. Hersteller-PDFs, Zettelkasten)
- Ganze Sektionen die ausschließlich lokale Ressourcen auflisten (z.B. "Externe Referenzen", "Dokumentation", "Wissensbasis", "Wavetable-Referenzen")
- Dateien die fast vollständig aus lokalen Pfaden bestehen (z.B. `STM32G4_CONFIG.md`)

## Ablauf

### Phase 1: Vorbereitung

1. Sicherstellen, dass der aktuelle Branch `main` ist. Falls nicht: abbrechen und Benutzer informieren.
2. Sicherstellen, dass keine uncommitteten Änderungen auf `main` vorliegen (`git status --porcelain`). Falls doch: abbrechen und Benutzer informieren.
3. Branch `deployment` erstellen oder aktualisieren:
   - Falls `deployment` existiert: `git checkout deployment && git reset --hard main`
   - Falls nicht: `git checkout -b deployment`

### Phase 2: Scan

4. Alle getackten Dateien im Repository nach den persönlichen Pfadmustern durchsuchen:
   ```
   git ls-files | xargs grep -l -E '/Users/framlin/|~/tinker/|~/obsidian/|~/studium/|~/STM32Cube/|/opt/homebrew/|/Applications/STMicroelectronics/|~/\.platformio/'
   ```
5. Ergebnis dem Benutzer als Liste präsentieren: Dateiname, Zeilennummer, gefundener Pfad.
6. Für jede betroffene Datei eine Bereinigungsstrategie vorschlagen:
   - **Generalisieren:** Pfad durch Platzhalter ersetzen (siehe Tabelle oben)
   - **Zeile entfernen:** Einzelne Referenzzeile löschen
   - **Sektion entfernen:** Ganze Markdown-Sektion löschen (Überschrift bis nächste gleichrangige Überschrift)
   - **Datei entfernen:** `git rm` für Dateien die fast vollständig aus persönlichen Pfaden bestehen

### Phase 3: Bereinigung

7. **Benutzer um Bestätigung bitten** bevor Änderungen durchgeführt werden.
8. **Zuerst:** `.claude/skills/deploy/SKILL.md` vom deployment-Branch entfernen (`git rm`).
   Diese Datei enthält selbst persönliche Pfade als Dokumentation und ist ein reines
   Entwicklungswerkzeug, das auf dem deployment-Branch nicht benötigt wird.
9. Weitere Änderungen gemäß der bestätigten Strategie durchführen.
10. Nach allen Änderungen erneut nach persönlichen Pfadmustern suchen (Verifikation).
    - Es dürfen **keine** Treffer mehr vorhanden sein — keine Ausnahmen.
    - Falls noch Treffer: dem Benutzer zeigen und nachbessern.
    - Falls keine Treffer: weiter zu Phase 4.

### Phase 3b: README sicherstellen

11. Prüfen, ob `README.md` im Repository-Root vorhanden ist.
    - Falls vorhanden: keine Aktion nötig (wurde von `main` übernommen).
    - Falls nicht vorhanden: `README.md` aus `~/obsidian/watasoge.zettelkasten/io/input/` kopieren (neueste Datei mit Alias "README"), dabei den Obsidian-Frontmatter (YAML-Header zwischen `---`) entfernen.

### Phase 4: Commit

12. Alle Änderungen stagen und committen:
    ```
    git add -A
    git commit -m "deployment-Branch aktualisiert"
    ```
13. `git diff main..deployment --stat` dem Benutzer zeigen.
14. Zurück auf `main` wechseln: `git checkout main`.

### Phase 5: Push

15. Push auf Gitea (origin):
    ```
    git push origin deployment --force-with-lease
    ```
16. GitHub-Remote sicherstellen:
    - Prüfen, ob Remote `github` existiert (`git remote get-url github`).
    - Falls nicht: `git remote add github git@github.com:framlin/watasoge.git`
17. Push **nur den deployment-Branch** auf GitHub:
    ```
    git push github deployment --force-with-lease
    ```
    **Niemals** `main` oder andere Branches auf GitHub pushen.
18. Ergebnis dem Benutzer bestätigen (Gitea- und GitHub-URLs).

## Remotes

| Remote | URL | Branches |
|---|---|---|
| `origin` (Gitea) | `ssh://git@jupiter:2222/mbaaba/watasoge.git` | `main`, `deployment` |
| `github` (GitHub) | `git@github.com:framlin/watasoge.git` | **nur `deployment`** |

## Wichtige Hinweise

- **Diese Datei (`deploy/SKILL.md`) wird vom deployment-Branch entfernt** (`git rm`), da sie selbst persönliche Pfadmuster als Dokumentation enthält. Sie ist ein reines Entwicklungswerkzeug und wird nur auf `main` benötigt.
- **Firmware-Quellcode** (C-Dateien, Header, CMakeLists.txt, Toolchain-File, Linker-Script) darf **nicht** verändert werden. Persönliche Pfade kommen nur in Dokumentations- und Konfigurationsdateien vor (.md, SKILL.md).
- Der Skill verändert **niemals** den `main`-Branch.
- Der `deployment`-Branch wird bei jedem Aufruf von `main` neu aufgebaut (`reset --hard main`), sodass Änderungen auf `main` immer korrekt übernommen werden.
- `--force-with-lease` ist nötig, weil der Branch bei jedem Aufruf per `reset --hard main` neu aufgebaut wird.
- Auf GitHub landet **ausschließlich** der `deployment`-Branch. `main` und andere Branches werden **nie** auf GitHub gepusht.
