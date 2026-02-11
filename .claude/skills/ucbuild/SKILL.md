---
name: ucbuild
description: |
  Baut die STM32G431KB-Firmware des watasoge-Projekts mit CMake und arm-none-eabi-gcc.
  Konfiguriert den Build (cmake --preset), kompiliert (cmake --build) und zeigt das Ergebnis
  (Flash/RAM-Nutzung, erzeugte Artefakte). Verwende diesen Skill nach Code-Änderungen,
  um die Firmware zu kompilieren und auf Fehler zu prüfen.
  Triggers: "bauen", "build", "kompilieren", "firmware bauen", "ucbuild".
allowed-tools: Bash, Read, Glob
---

# ucbuild — Firmware bauen

Baut die STM32G431KB-Firmware des watasoge-Projekts.

## Verzeichnis

```
firmware/stm32g431kb/
```

Alle cmake-Befehle werden in diesem Verzeichnis ausgeführt.

## Standard-Build (Debug)

### 1. Konfigurieren (nur beim ersten Mal oder nach CMakeLists.txt-Änderungen)

```bash
cmake --preset Debug
```

### 2. Bauen

```bash
cmake --build build/Debug
```

### Ausgabe bei Erfolg

Der Post-Build-Step gibt automatisch aus:
- **arm-none-eabi-size:** text/data/bss/Flash/RAM-Nutzung
- Erzeugt `build/Debug/blinky.elf`, `blinky.hex`, `blinky.bin`

## Release-Build

```bash
cmake --preset Release
cmake --build build/Release
```

Verwendet `-Os` statt `-O0 -g3`.

## Clean-Build

Bei Konfigurationsänderungen (CMakeLists.txt, Toolchain, HAL-Module) ist ein Clean-Build nötig:

```bash
rm -rf build/Debug && cmake --preset Debug && cmake --build build/Debug
```

## Ablauf im Skill

1. Prüfen, ob `build/Debug/` existiert. Falls nicht: `cmake --preset Debug` ausführen.
2. `cmake --build build/Debug` ausführen.
3. Bei Erfolg: Flash/RAM-Nutzung aus der size-Ausgabe dem Benutzer mitteilen.
4. Bei Fehler: Fehlermeldung analysieren und dem Benutzer eine Lösung vorschlagen.

## Häufige Fehler

| Fehler | Ursache | Lösung |
|--------|---------|--------|
| `multiple definition of _exit` | `_exit` in syscalls.c UND in libnosys | `_exit` aus syscalls.c entfernen |
| `undefined reference to HAL_*` | HAL-Quelldatei fehlt in CMakeLists.txt | Datei im SOURCES-Block ergänzen |
| `No such file or directory: stm32g4xx_hal_*.h` | HAL-Modul nicht in hal_conf.h aktiviert | `#define HAL_*_MODULE_ENABLED` setzen |
| `READONLY` Linker-Fehler | GCC 10.3 unterstützt READONLY nicht | Keyword aus .ld-Datei entfernen |
| Ninja not found | Ninja Build-System fehlt | `brew install ninja` |

## Toolchain-Voraussetzungen

- **arm-none-eabi-gcc** 10.3 (`/opt/homebrew/bin/arm-none-eabi-gcc`)
- **CMake** ≥ 3.22
- **Ninja** Build-System

## Build-Artefakte

```
build/Debug/
├── blinky.elf    # ELF mit Debug-Symbolen (für OpenOCD/GDB)
├── blinky.hex    # Intel HEX
├── blinky.bin    # Raw Binary
└── blinky.map    # Linker Map-File
```
