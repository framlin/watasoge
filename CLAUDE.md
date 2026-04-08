# Watasoge

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Entwicklungsplattform: **NUCLEO-G431KB** (STM32G431KB). Audio-DAC: PCM5102.

## Projektspezifisch

- Zettelkasten: `watasoge.zettelkasten` (Gitea: mbaaba/watasoge.zettelkasten)
- Projektstand und Details: `~/obsidian/watasoge.zettelkasten/PROJECT.md`

## Projektstruktur

```
watasoge/
├── CLAUDE.md                      # Diese Datei
├── README.md                      # Projektbeschreibung
└── firmware/
    └── stm32g431kb/               # Firmware für STM32G431KB / NUCLEO-G431KB
        ├── CLAUDE.md
        ├── CMakeLists.txt         # CMake Build (Ninja, arm-none-eabi-gcc)
        ├── CMakePresets.json      # Debug/Release Presets
        ├── cmake/
        │   └── gcc-arm-none-eabi.cmake
        ├── STM32G431KBTX_FLASH.ld # Linker-Script
        ├── startup_stm32g431xx.s  # Startup (Kopie aus STM32Cube Repo)
        ├── Core/
        │   ├── Inc/               # Header-Dateien
        │   └── Src/               # Quellcode
        └── Drivers/               # Symlink → ~/STM32Cube/Repository/.../Drivers
```

## Hardware

- **MCU:** STM32G431KB (Cortex-M4F, 170 MHz, 128 KB Flash, 32 KB RAM)
- **Board:** NUCLEO-G431KB
- **Audio-DAC:** PCM5102, 16-Bit, 44.1 kHz, I2S via SAI

## Skills

Projektspezifische Skills unter `.claude/skills/`:

| Skill | Beschreibung |
|-------|-------------|
| **ucconfig** | STM32G431KB Build-Konfiguration: HAL-Module, CMakeLists.txt, Linker-Script, Toolchain |
| **ucbuild** | Firmware bauen mit CMake + Ninja + arm-none-eabi-gcc |
| **ucflash** | Firmware flashen via OpenOCD auf NUCLEO-G431KB |
| **deploy** | Deployment-Branch erstellen/aktualisieren: persönliche Pfade entfernen, Push auf Gitea + GitHub |

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals, Programming Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | Aufbereitete Notizen zu STM32G4, RP2040, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode, Playback-Dokumentation |
| MI-Quellcode | `~/tinker/mutable_instruments/MI_eurorack_git/` | Rings/Plaits KS-Referenzimplementierung (MIT-Lizenz) |
| fracanto-Framework | `~/tinker/fracanto/` | Dreischichtiges C-Framework für CAN-Bus-vernetzte Eurorack-Module |
