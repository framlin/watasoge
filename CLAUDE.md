# Watasoge

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Entwicklungsplattform ist bis auf Weiteres das **NUCLEO-G431KB** (STM32G431KB). Audio-DAC: PCM5102.

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
        │   ├── Inc/               # main.h, input.h, synthesis.h, karplus.h, player.h, player_config.h, output.h, audio_config.h, svf.h, delay_line.h, hal_conf.h, it.h, wavetables_integrated.h
        │   └── Src/               # main.c, input.c, synthesis.c, karplus.c, player.c, player_config.c, output.c, it.c, hal_msp.c, system, syscalls, sysmem
        └── Drivers/               # Symlink → ~/STM32Cube/Repository/.../Drivers
```

## Hardware

- **MCU:** STM32G431KB (Cortex-M4F, 170 MHz, 128 KB Flash, 32 KB RAM)
- **Board:** NUCLEO-G431KB
- **Audio-DAC:** PCM5102, 16-Bit, 44.1 kHz, I2S via SAI

## Wavetable-Daten

Die Datei `firmware/stm32g431kb/Core/Inc/wavetables_integrated.h` enthält 220 Wavetables im Integrated-Wavetable-Synthesis-Format (Franck & Välimäki, DAFx-12 / Mutable Instruments Plaits):

- **Format:** 128 Samples + 4 Guard, int16_t, integriert (kumulative Summe)
- **Bank A:** 64 Waves — additive Synthese (Sinus, Comb, Quadra, Tri-Stack, Drawbars, Formant, Digital-Formant, Pulse)
- **Bank B:** 156 Waves — aus Audio-Samples extrahiert (Sub-Bass, Bass, Stabs, Pads, Gitarre, Kicks, Claps, Snares, HiHats)
- **Quelle:** Generiert in `~/tinker/audio-samples/` via `generate_integrated_wavetables.py`

## Projektstand

Zwei Synthese-Engines, beide auf Hardware verifiziert:

- **Integrated Wavetable Playback:** 220 Wavetables (MI-Plaits-Pipeline), Hermite-Interpolation, Differenzierung + One-Pole-LP
- **Karplus-Strong String Synthesis:** Physical Modelling nach MI Rings/Plaits, Delay-Line + ZDF-SVF + Allpass-Dispersion

Gate-gesteuerter Player mit Pitch-CV-Eingang (PA0 Gate/EXTI, PA1 CV/ADC1, 1V/Oct). 21 Instrumentengruppen (11 Wavetable, 4 KS-melodisch, 6 KS-perkussiv). Audio-Ausgabe über SAI1/I2S an PCM5102-DAC via Circular-DMA.

Firmware modular aufgebaut: `input`, `synthesis`, `karplus`, `player`, `player_config`, `output`, `main` (Applikation) + `audio_config`, `svf`, `delay_line` (DSP-Infrastruktur).

Details zu Modulen, Signalfluss, Algorithmen und Peripherie-Konfiguration: siehe `firmware/stm32g431kb/CLAUDE.md`.

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
| Watasoge-Zettelkasten | `~/obsidian/watasoge.zettelkasten/` | Implementierungspläne, Projektnotizen |
