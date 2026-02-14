# Watasoge

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Entwicklungsplattform ist bis auf Weiteres das **NUCLEO-G431KB** (STM32G431KB). Audio-DAC: PCM5102.

## Projektstruktur

```
watasoge/
├── CLAUDE.md                      # Diese Datei
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
        │   ├── Inc/               # main.h, synthesis.h, karplus.h, player.h, output.h, hal_conf.h, it.h, wavetables_integrated.h
        │   └── Src/               # main.c, synthesis.c, karplus.c, player.c, output.c, it.c, hal_msp.c, system, syscalls, sysmem
        └── Drivers/               # Symlink → ~/STM32Cube/Repository/.../Drivers
```

## Hardware-Plattformen

| Plattform | CPU | Flash | RAM | Audio-Interface |
|---|---|---|---|---|
| Raspberry Pi Pico (RP2040) | Dual Cortex-M0+, 133 MHz | 2 MB | 264 KB | I2S via PIO |
| STM32G431KB (NUCLEO-G431KB) | Cortex-M4F, 170 MHz | 128 KB | 32 KB | I2S via SAI |

Gemeinsam: PCM5102 Audio-DAC, 16-Bit, 44.1 kHz.

## Wavetable-Daten

Die Datei `firmware/stm32g431kb/Core/Inc/wavetables_integrated.h` enthält 220 Wavetables im Integrated-Wavetable-Synthesis-Format (Franck & Välimäki, DAFx-12 / Mutable Instruments Plaits):

- **Format:** 128 Samples + 4 Guard, int16_t, integriert (kumulative Summe)
- **Bank A:** 64 Waves — additive Synthese (Sinus, Comb, Quadra, Tri-Stack, Drawbars, Formant, Digital-Formant, Pulse)
- **Bank B:** 156 Waves — aus Audio-Samples extrahiert (Sub-Bass, Bass, Stabs, Pads, Gitarre, Kicks, Claps, Snares, HiHats)
- **Quelle:** Generiert in `~/tinker/audio-samples/` via `generate_integrated_wavetables.py`

## Projektstand

Zwei Synthese-Engines implementiert:

### 1. Integrated Wavetable Playback (verifiziert)

Implementiert nach Mutable Instruments Plaits und auf dem NUCLEO-G431KB verifiziert. 220 Wavetables werden über die Plaits-Pipeline abgespielt: Hermite-Interpolation → Differenzierung → One-Pole-Tiefpass → Skalierung.

### 2. Karplus-Strong String Synthesis (auf Hardware verifiziert)

Karplus-Strong Synthese nach Mutable Instruments Rings/Plaits-Vorbild. Erzeugt gezupfte Saiten, inharmonische Klänge (Glocken, Gamelan), Sitar-Buzz und perkussive Sounds.

- **Signal-Fluss:** Noise-Burst-Excitation → Delay-Line (1024 floats, Hermite-Interpolation) → ZDF-SVF Lowpass (Q=0.5) → RT60-Damping → DC-Blocker → Hard-Clamp → Feedback
- **Dispersion:** Allpass-Delay-Line (256 floats) für Inharmonizität (Dispersion > 0), Curved-Bridge-Nichtlinearität für Sitar-Buzz (Dispersion < 0)
- **Parameter:** Frequency, Damping (RT60-basiert), Brightness (SVF-Cutoff), Dispersion [-1..+1]
- **RAM-Bedarf:** ~5,2 KB (Delay-Lines + State)

Player-Modul spielt alle Waves sequenziell ab, gruppiert in 21 Instrumentengruppen:
- **Wavetable-Gruppen (11 Gruppen):** Melodisch (6 Gruppen, Waves 0–115) mit C-Dur-Tonleiter C1–C2, perkussiv (5 Gruppen, Waves 116–219) mit Decay-Envelope
- **KS-Melodisch (4 Gruppen):** KS_STRING (warm), KS_BRIGHT (brillant), KS_INHARMONIC (Glocken), KS_SITAR (Buzz) — C-Dur-Tonleiter C3–C4
- **KS-Perkussiv (6 Gruppen):** KS_KICK (C2, tief), KS_SNARE (A3, Curved Bridge), KS_HIHAT (E6, metallisch), KS_TOM (G2, resonant), KS_COWBELL (E5, inharmonisch), KS_CLAVE (A5, holzig) — je 16 Hits bei 120 BPM
- Audio-Quelle wird automatisch zwischen Wavetable und Karplus-Strong umgeschaltet (`output_set_source()`)
- LED blinkt synchron zu den Beats

Firmware modularisiert in fünf Module:
- **synthesis** (`synthesis.c/.h`) — Signalerzeugung: Integrated Wavetable Playback (Hermite, Differentiator, adaptiver LP), Dual-Envelope
- **karplus** (`karplus.c/.h`) — Karplus-Strong String Synthesis: Delay-Line, ZDF-SVF Loop-Filter, Allpass-Dispersion, Curved Bridge, Noise-Burst-Excitation, Per-Sample-Parameterinterpolation
- **player** (`player.c/.h`) — Sequenzielles Abspielen: Tick-basierte Zustandsmaschine, 16 Instrumentengruppen (11 Wavetable + 5 KS), automatische Quellenwahl
- **output** (`output.c/.h`) — Audio-Ausgabe: SAI/DMA-Konfiguration, umschaltbarer Funktionspointer (`fill_buffer_fn`) für Quellenwahl
- **main** (`main.c`) — Orchestrierung: Clock, GPIO, Init-Reihenfolge, non-blocking Main-Loop (Player + beat-synchrone LED)

Technische Details:
- **Audio-Ausgabe:** SAI1 Block A, I2S-Master-TX, 16-Bit Stereo, ~44.1 kHz
- **Wavetable-Playback:** Float-Phase-Accumulator [0,1), Hermite-4-Punkt-Interpolation, Differenzierung + One-Pole-LP (Anti-Aliasing), frequenzabhängige Skalierung
- **Karplus-Strong:** Ringbuffer-Delay-Line (1024 floats) + Allpass (256 floats), Hermite-Interpolation, ZDF-SVF Lowpass, RT60-basiertes Decay, SVF-Delay-Kompensation per LUT, DC-Blocker, Stabilitäts-Clamp
- **Gain-Envelope:** Dual-Modus: Smoothstep-Fade für Sustain, exponentieller Decay für Perkussion
- **DMA:** Circular-DMA (DMA1 Channel1), Half-/Complete-Callbacks für lückenloses Streaming
- **Pins:** PA8 (SCK), PA9 (FS/LRCLK), PA10 (SD/DATA)
- **Systemtakt:** 170 MHz (HSI 16 MHz → PLL, PLLM=4, PLLN=85, PLLR=2)
- **Build-System:** CMake 3.22 + Ninja, arm-none-eabi-gcc 10.3
- **HAL:** STM32Cube_FW_G4_V1.6.1 (via Symlink)

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
| MI-Quellcode | `~/tinker/mutable_instruments/MI_eurorack_git/` | Rings/Plaits KS-Referenzimplementierung (MIT-Lizenz) |
| Watasoge-Zettelkasten | `~/obsidian/watasoge.zettelkasten/` | Implementierungspläne, Projektnotizen |
