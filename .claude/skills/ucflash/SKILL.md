---
name: ucflash
description: |
  Flasht die STM32G431KB-Firmware auf das NUCLEO-G431KB Board via OpenOCD und ST-LINK V3.
  Programmiert, verifiziert und setzt das Target zurück. Verwende diesen Skill, wenn die
  Firmware auf das Board übertragen werden soll.
  Triggers: "flashen", "flash", "programmieren", "auf Board laden", "ucflash".
allowed-tools: Bash, Read
---

# ucflash — Firmware flashen

Flasht die kompilierte Firmware auf das NUCLEO-G431KB Board.

## Voraussetzungen

- **OpenOCD** installiert (`brew install openocd`)
- **NUCLEO-G431KB** per USB angeschlossen (ST-LINK V3)
- Firmware wurde zuvor gebaut (ELF-Datei vorhanden)

## Flash-Befehl

```bash
openocd -f board/st_nucleo_g4.cfg \
  -c "program build/Debug/blinky.elf verify reset exit"
```

Für Release-Build:

```bash
openocd -f board/st_nucleo_g4.cfg \
  -c "program build/Release/blinky.elf verify reset exit"
```

## Ablauf im Skill

1. Prüfen, ob die ELF-Datei existiert (`build/Debug/blinky.elf`).
   Falls nicht: Benutzer darauf hinweisen, dass zuerst gebaut werden muss (ucbuild).
2. OpenOCD-Befehl mit **absolutem Pfad** zur ELF-Datei ausführen.
   Relative Pfade funktionieren nicht zuverlässig, da OpenOCD im eigenen
   Arbeitsverzeichnis läuft.
3. Ausgabe prüfen:
   - `Programming Finished` + `Verified OK` + `Resetting Target` = Erfolg
   - Fehlermeldung analysieren und Benutzer informieren

## Häufige Fehler

| Fehler | Ursache | Lösung |
|--------|---------|--------|
| `couldn't open *.elf` | Relativer Pfad oder Datei fehlt | Absoluten Pfad verwenden, ggf. zuerst bauen |
| `Error connecting DP` | Board nicht angeschlossen oder USB-Problem | USB-Kabel prüfen, Board neu einstecken |
| `STLINK ... not found` | ST-LINK Treiber-Problem | `brew reinstall openocd`, USB-Verbindung prüfen |
| `Target voltage: 0.0` | Board hat keine Stromversorgung | USB-Kabel mit Daten+Strom verwenden |

## Board-Informationen

- **Debug-Probe:** ST-LINK V3 (integriert auf NUCLEO-G431KB)
- **Interface:** SWD (Serial Wire Debug)
- **OpenOCD Config:** `board/st_nucleo_g4.cfg` (mitgeliefert mit OpenOCD)
- **Target Voltage:** ~3.3 V

## Wichtige Hinweise

- Nach dem Flashen führt OpenOCD automatisch einen Reset durch. Die Firmware
  startet sofort.
- Der `verify`-Schritt vergleicht den Flash-Inhalt mit der ELF-Datei und
  meldet Fehler bei Abweichungen.
- Während des Flash-Vorgangs darf das USB-Kabel nicht abgezogen werden.
