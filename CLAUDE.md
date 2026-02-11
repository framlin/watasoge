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
        │   ├── Inc/               # main.h, hal_conf.h, it.h
        │   └── Src/               # main.c, it.c, hal_msp.c, system, syscalls, sysmem
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

Blinky-Firmware für STM32G431KB implementiert und auf dem NUCLEO-G431KB verifiziert. LD2 (PB8) blinkt mit 4 Hz.

- **Systemtakt:** 170 MHz (HSI 16 MHz → PLL, PLLM=4, PLLN=85, PLLR=2)
- **Build-System:** CMake 3.22 + Ninja, arm-none-eabi-gcc 10.3
- **HAL:** STM32Cube_FW_G4_V1.6.1 (via Symlink)
- **Flash-Nutzung:** 5780 Bytes (4.4%), **RAM:** 1592 Bytes (4.9%)

### Build & Flash

```bash
cd firmware/stm32g431kb
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
```

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals, Programming Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | Aufbereitete Notizen zu STM32G4, RP2040, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode, Playback-Dokumentation |
