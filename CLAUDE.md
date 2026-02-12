# Watasoge

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Entwicklungsplattform ist bis auf Weiteres das **NUCLEO-G431KB** (STM32G431KB). Audio-DAC: PCM5102.

## Projektstruktur

```
watasoge/
├── CLAUDE.md                      # Diese Datei
├── data/
│   └── wavetables_integrated.h    # 220 integrierte Wavetables (MI-Plaits-Stil, ~60 KB)
└── firmware/
    ├── rp2040/                    # Firmware für Raspberry Pi Pico
    │   ├── CLAUDE.md
    │   └── doc/
    │       └── wavetables_for_pi_pico.md
    └── stm32g431kb/               # Firmware für STM32G431KB / NUCLEO-G431KB
        ├── CLAUDE.md
        ├── CMakeLists.txt         # CMake Build (Ninja, arm-none-eabi-gcc)
        ├── CMakePresets.json      # Debug/Release Presets
        ├── cmake/
        │   └── gcc-arm-none-eabi.cmake
        ├── STM32G431KBTX_FLASH.ld # Linker-Script
        ├── startup_stm32g431xx.s  # Startup (Kopie aus STM32Cube Repo)
        ├── Core/
        │   ├── Inc/               # main.h, synthesis.h, output.h, hal_conf.h, it.h
        │   └── Src/               # main.c, synthesis.c, output.c, it.c, hal_msp.c, system, syscalls, sysmem
        └── Drivers/               # Symlink → ~/STM32Cube/Repository/.../Drivers
```

## Hardware-Plattformen

| Plattform | CPU | Flash | RAM | Audio-Interface |
|---|---|---|---|---|
| Raspberry Pi Pico (RP2040) | Dual Cortex-M0+, 133 MHz | 2 MB | 264 KB | I2S via PIO |
| STM32G431KB (NUCLEO-G431KB) | Cortex-M4F, 170 MHz | 128 KB | 32 KB | I2S via SAI |

Gemeinsam: PCM5102 Audio-DAC, 16-Bit, 44.1 kHz.

## Wavetable-Daten

Die Datei `data/wavetables_integrated.h` enthält 220 Wavetables im Integrated-Wavetable-Synthesis-Format (Franck & Välimäki, DAFx-12 / Mutable Instruments Plaits):

- **Format:** 128 Samples + 4 Guard, int16_t, integriert (kumulative Summe)
- **Bank A:** 64 Waves — additive Synthese (Sinus, Comb, Quadra, Tri-Stack, Drawbars, Formant, Digital-Formant, Pulse)
- **Bank B:** 156 Waves — aus Audio-Samples extrahiert (Sub-Bass, Bass, Stabs, Pads, Gitarre, Kicks, Claps, Snares, HiHats)
- **Quelle:** Generiert in `~/tinker/audio-samples/` via `generate_integrated_wavetables.py`

## Projektstand

Integrated Wavetable Playback nach Mutable Instruments Plaits implementiert und auf dem NUCLEO-G431KB verifiziert. 220 Wavetables aus `data/wavetables_integrated.h` werden über die Plaits-Pipeline abgespielt: Hermite-Interpolation → Differenzierung → One-Pole-Tiefpass → Skalierung. Default: 440 Hz, Wave 0 (`a_sine_00`). LED (PB8) blinkt weiterhin mit 4 Hz als Lebenszeichen.

Firmware modularisiert in drei Module:
- **synthesis** (`synthesis.c/.h`) — Signalerzeugung: Integrated Wavetable Playback (Hermite, Differentiator, adaptiver LP), `synthesis_fill_buffer()`, `synthesis_set_frequency()`, `synthesis_set_wave()`
- **output** (`output.c/.h`) — Audio-Ausgabe: SAI/DMA-Konfiguration, Buffer, DMA-Callbacks → ruft `synthesis_fill_buffer()`
- **main** (`main.c`) — Orchestrierung: Clock, GPIO, Init-Reihenfolge, LED-Loop

Technische Details:
- **Audio-Ausgabe:** SAI1 Block A, I2S-Master-TX, 16-Bit Stereo, ~44.1 kHz
- **Wavetable-Playback:** Float-Phase-Accumulator [0,1), Hermite-4-Punkt-Interpolation, Differenzierung + One-Pole-LP (Anti-Aliasing), frequenzabhängige Skalierung
- **DMA:** Circular-DMA (DMA1 Channel1), Half-/Complete-Callbacks für lückenloses Streaming
- **Pins:** PA8 (SCK), PA9 (FS/LRCLK), PA10 (SD/DATA)
- **Systemtakt:** 170 MHz (HSI 16 MHz → PLL, PLLM=4, PLLN=85, PLLR=2)
- **Build-System:** CMake 3.22 + Ninja, arm-none-eabi-gcc 10.3
- **HAL:** STM32Cube_FW_G4_V1.6.1 (via Symlink)
- **Flash-Nutzung:** 70.832 Bytes (54.1%), **RAM:** 2.376 Bytes (7.3%)

### Build & Flash

```bash
cd firmware/stm32g431kb
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
```

## Skills

Projektspezifische Skills unter `.claude/skills/`:

| Skill | Beschreibung |
|-------|-------------|
| **ucconfig** | STM32G431KB Build-Konfiguration: HAL-Module, CMakeLists.txt, Linker-Script, Toolchain |
| **ucbuild** | Firmware bauen mit CMake + Ninja + arm-none-eabi-gcc |
| **ucflash** | Firmware flashen via OpenOCD auf NUCLEO-G431KB |

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals, Programming Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | Aufbereitete Notizen zu STM32G4, RP2040, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode, Playback-Dokumentation |
