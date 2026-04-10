# Watasoge

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Läuft als **fracanto-Modul**, gesteuert über CAN-Bus (CV-Pitch, CC-Wavetable/ADSR/Volume, Gate/Trigger). Entwicklungsplattform: **NUCLEO-G431KB** (STM32G431KB). Audio-DAC: PCM5102.

## Projektstruktur

```
watasoge/
├── CLAUDE.md                      # Diese Datei
├── README.md                      # Projektbeschreibung
├── tools/
│   └── play_scale.sh             # CAN-Audio-Test via fraclacan
└── firmware/
    └── stm32g431kb/               # Firmware für STM32G431KB / NUCLEO-G431KB
        ├── CLAUDE.md
        ├── CMakeLists.txt         # CMake Build (Ninja, arm-none-eabi-gcc)
        ├── CMakePresets.json      # Debug/Release Presets
        ├── cmake/
        │   └── gcc-arm-none-eabi.cmake
        ├── STM32G431KBTX_FLASH.ld # Linker-Script
        ├── startup_stm32g431xx.s  # Startup (Kopie aus STM32Cube Repo)
        ├── fracanto/              # Git-Submodul (fracanto-Framework)
        ├── Core/
        │   ├── Inc/               # main.h, audio_config.h, synthesis.h, watasoge_module.h, wavetables_integrated.h, hal_conf.h, it.h
        │   └── Src/               # main.c, watasoge_module.c, synthesis.c, it.c, hal_msp.c, system, syscalls, sysmem
        ├── tests/                 # Host-Side Unit-Tests (test_adsr.c, test_watasoge_module.c)
        └── Drivers/               # Symlink → ~/STM32Cube/Repository/.../Drivers
```

## Hardware

- **MCU:** STM32G431KB (Cortex-M4F, 170 MHz, 128 KB Flash, 32 KB RAM)
- **Board:** NUCLEO-G431KB
- **Audio-DAC:** PCM5102, 16-Bit, 48 kHz, I2S via SAI
- **CAN-Transceiver:** Waveshare SN65HVD230, 500 kbit/s Classic CAN

## Projektstand

Watasoge ist ein vollwertiger CAN-Node mit fracanto-Framework-Integration. Wavetable-Synthese wird über CAN-Nachrichten gesteuert. Auf Hardware verifiziert: C-Dur-Tonleiter + Wavetable-Wechsel über CAN-Bus hörbar.

Projektstand und Details: `~/obsidian/watasoge.zettelkasten/PROJECT.md`

## Skills

| Skill | Beschreibung |
|-------|-------------|
| **ucconfig** | STM32G431KB Build-Konfiguration |
| **ucbuild** | Firmware bauen mit CMake + Ninja + arm-none-eabi-gcc |
| **ucflash** | Firmware flashen via OpenOCD auf NUCLEO-G431KB |
| **deploy** | Deployment-Branch erstellen/aktualisieren |

## Build & Flash & Test

```bash
cd firmware/stm32g431kb
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
cd tests && make test    # 44 Host-Side Unit-Tests
```

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | STM32G4, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode |
| fracanto-Framework | `~/tinker/fracanto/` | CAN-Bus-Framework (Git-Submodul) |
| fraclacan | `~/tinker/fraclacan/` | CAN-Bus-Zugang via Raspberry Pi |
| Watasoge-Zettelkasten | `~/obsidian/watasoge.zettelkasten/` | Projektstand, Dokumentation |
