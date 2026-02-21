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

Zwei Synthese-Engines implementiert:

### 1. Integrated Wavetable Playback (verifiziert)

Implementiert nach Mutable Instruments Plaits und auf dem NUCLEO-G431KB verifiziert. 220 Wavetables werden über die Plaits-Pipeline abgespielt: Hermite-Interpolation → Differenzierung → One-Pole-Tiefpass → Skalierung.

### 2. Karplus-Strong String Synthesis (auf Hardware verifiziert)

Karplus-Strong Synthese nach Mutable Instruments Rings/Plaits-Vorbild. Erzeugt gezupfte Saiten, inharmonische Klänge (Glocken, Gamelan), Sitar-Buzz und perkussive Sounds.

- **Signal-Fluss:** Noise-Burst-Excitation → Delay-Line (1024 floats, Hermite-Interpolation) → ZDF-SVF Lowpass (Q=0.5) → RT60-Damping → DC-Blocker → Hard-Clamp → Feedback
- **Dispersion:** Allpass-Delay-Line (256 floats) für Inharmonizität (Dispersion > 0), Curved-Bridge-Nichtlinearität für Sitar-Buzz (Dispersion < 0)
- **Parameter:** Frequency, Damping (RT60-basiert), Brightness (SVF-Cutoff), Dispersion [-1..+1]
- **RAM-Bedarf:** ~5,2 KB (Delay-Lines + State)

### 3. Pitch-CV Eingang (auf Hardware verifiziert)

1V/Octave Pitch-CV-Eingang über internen 12-Bit ADC (ADC1, Kanal 2, PA1). Polling-Modus: ADC wird bei Gate-ON gelesen, Umrechnung in Frequenz via `freq = 32.703 * exp2f(voltage)`. Bereich C1 (~32.7 Hz bei 0V) bis ~D#4 (~311 Hz bei 3.3V). Zwischenlösung bis externer 16-Bit-ADC verfügbar.

- **Gate (PA0)** bestimmt WANN (Note-ON/OFF)
- **CV (PA1)** bestimmt WELCHE TONHÖHE (1V/Oct)
- **Player** bestimmt WELCHER KLANG (Wave, Engine, Parameter)

Player-Modul spielt alle Waves sequenziell ab, gate-gesteuert über PA0, gruppiert in 21 Instrumentengruppen:
- **Wavetable-Gruppen (11 Gruppen):** Melodisch (6 Gruppen, Waves 0–115), perkussiv (5 Gruppen, Waves 116–219) mit Decay-Envelope
- **KS-Melodisch (4 Gruppen):** KS_STRING (warm), KS_BRIGHT (brillant), KS_INHARMONIC (Glocken), KS_SITAR (Buzz)
- **KS-Perkussiv (6 Gruppen):** KS_KICK (C2, tief), KS_SNARE (A3, Curved Bridge), KS_HIHAT (E6, metallisch), KS_TOM (G2, resonant), KS_COWBELL (E5, inharmonisch), KS_CLAVE (A5, holzig)
- **Gate-Steuerung:** Steigende Flanke an PA0 → ADC-Lesung → Pitch setzen → Note-ON, fallende Flanke → Note-OFF (melodische Gruppen werden gemutet, perkussive/KS klingen natürlich aus), Note-Counter rückt bei Gate-OFF vor
- **Pitch-CV:** Melodische Gruppen erhalten Tonhöhe vom CV-Eingang, perkussive Gruppen verwenden weiterhin feste Frequenzen
- Audio-Quelle wird automatisch zwischen Wavetable und Karplus-Strong umgeschaltet (`output_set_source()`)
- LED blinkt synchron zu den Note-ON-Events

Firmware modularisiert in Applikations- und DSP-Infrastruktur-Module:

Applikationsmodule:
- **input** (`input.c/.h`) — Gate-Eingang (PA0, EXTI) + Pitch-CV-Eingang (PA1, ADC1 Kanal 2, 12-Bit, Polling). Gate: volatile Flags, `input_gate_on_pending()`, `input_gate_off_pending()`. CV: `input_pitch_cv()` liest ADC und gibt Frequenz in Hz zurück (1V/Oct, C1=32.703 Hz bei 0V)
- **synthesis** (`synthesis.c/.h`) — Signalerzeugung: Integrated Wavetable Playback (Hermite, Differentiator, adaptiver LP), Dual-Envelope
- **karplus** (`karplus.c/.h`) — Karplus-Strong String Synthesis: nutzt `svf.h` und `delay_line.h` für DSP-Bausteine, Allpass-Dispersion, Curved Bridge, Noise-Burst-Excitation, Per-Sample-Parameterinterpolation
- **player** (`player.c/.h`) — Gate-gesteuerter Sequencer: 21 Instrumentengruppen (11 Wavetable + 10 KS), automatische Quellenwahl, `player_note_on()`/`player_note_off()`/`player_set_pitch()` API
- **player_config** (`player_config.c/.h`) — Gruppen-Definitionen, KS-Presets, Frequenz-Arrays, Note-Count-Konstanten (Daten getrennt von Logik)
- **output** (`output.c/.h`) — Audio-Ausgabe: SAI/DMA-Konfiguration, umschaltbarer Funktionspointer (`fill_buffer_fn`) per Dependency Injection
- **main** (`main.c`) — Orchestrierung: Clock, GPIO, Input-Init, non-blocking Main-Loop (Gate-ON → CV lesen → Pitch setzen → Note-ON; Gate-OFF → Note-OFF; beat-synchrone LED)

DSP-Infrastruktur (Inline-Header):
- **audio_config** (`audio_config.h`) — Gemeinsame Konstanten: `SAMPLE_RATE`, `OUTPUT_GAIN`, `PI_F`
- **svf** (`svf.h`) — Wiederverwendbarer ZDF State Variable Filter: `svf_state_t`, `svf_coeff_t`, `svf_compute_coeff()`, `svf_process()`
- **delay_line** (`delay_line.h`) — Wiederverwendbare Delay-Line-Operationen: `dl_write()`, `dl_read_hermite()`, `dl_allpass()`

Technische Details:
- **Audio-Ausgabe:** SAI1 Block A, I2S-Master-TX, 16-Bit Stereo, ~44.1 kHz
- **Wavetable-Playback:** Float-Phase-Accumulator [0,1), Hermite-4-Punkt-Interpolation, Differenzierung + One-Pole-LP (Anti-Aliasing), frequenzabhängige Skalierung
- **Karplus-Strong:** Ringbuffer-Delay-Line (1024 floats) + Allpass (256 floats), Hermite-Interpolation, ZDF-SVF Lowpass, RT60-basiertes Decay, SVF-Delay-Kompensation per LUT, DC-Blocker, Stabilitäts-Clamp
- **Gain-Envelope:** Dual-Modus: Smoothstep-Fade für Sustain, exponentieller Decay für Perkussion
- **DMA:** Circular-DMA (DMA1 Channel1), Half-/Complete-Callbacks für lückenloses Streaming
- **Pitch-CV:** ADC1 Kanal 2 (PA1), 12-Bit, Single-Conversion, Synchron-Takt (PCLK/4 = 42.5 MHz), Polling bei Gate-ON, 1V/Oct-Umrechnung (`CV_BASE_FREQ * exp2f(volts)`)
- **Pins:** PA0 (Gate-Input, EXTI), PA1 (Pitch-CV, ADC1_CH2, Analog), PA8 (SCK), PA9 (FS/LRCLK), PA10 (SD/DATA)
- **Systemtakt:** 170 MHz (HSI 16 MHz → PLL, PLLM=4, PLLN=85, PLLR=2)
- **Build-System:** CMake 3.22 + Ninja, arm-none-eabi-gcc 10.3
- **HAL:** STM32Cube_FW_G4_V1.6.1 (via Symlink), Module: GPIO, RCC, FLASH, PWR, CORTEX, DMA, EXTI, SAI, ADC

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
| **deploy** | Deployment-Branch erstellen/aktualisieren: persönliche Pfade entfernen, Push auf Gitea + GitHub |

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals, Programming Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | Aufbereitete Notizen zu STM32G4, RP2040, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode, Playback-Dokumentation |
| MI-Quellcode | `~/tinker/mutable_instruments/MI_eurorack_git/` | Rings/Plaits KS-Referenzimplementierung (MIT-Lizenz) |
| Watasoge-Zettelkasten | `~/obsidian/watasoge.zettelkasten/` | Implementierungspläne, Projektnotizen |

## Coding-Wissensbasis

Vor Beginn einer Coding-Aufgabe immer den Watasoge-Zettelkasten nach relevanten Notes mit dem Topic **Coding** durchsuchen. Vorgehen:

1. **Überblick verschaffen:** Alle Notes mit Topic `Coding` finden und deren Frontmatter (insbesondere Aliases) lesen. Notes vom Typ `Topic_Note` ignorieren — sie sind nur Platzhalter.
   ```bash
   grep -rl '\[\[Coding\]\]' ~/obsidian/watasoge.zettelkasten/io/input/ ~/obsidian/watasoge.zettelkasten/mem/ 2>/dev/null \
     | xargs grep -L 'type:.*Topic_Note' \
     | while read f; do echo "=== $f ==="; sed -n '1,/^---$/{ /^---$/!p; /^---$/p }; /^---$/,/^---$/p' "$f" | head -10; done
   ```
   Entscheidend sind die `aliases`-Felder — sie beschreiben den Inhalt der jeweiligen Note (z.B. `FMAC-Usage`, `SAI_DMA`, `Karplus-Strong`).

2. **Relevante Notes identifizieren:** Anhand der Aliases entscheiden, welche Notes zur aktuellen Aufgabe passen. Beispiel: Bei einer Filter-Implementierung sind Notes mit Alias `FMAC` relevant, bei Audio-Ausgabe solche mit `SAI` oder `DMA`.

3. **Notes lesen:** Die als relevant identifizierten Notes vollständig lesen, bevor mit der Planung oder Implementierung begonnen wird.

Dieser Schritt gehört in die Planungsphase jeder Coding-Aufgabe und stellt sicher, dass bereits erarbeitetes Wissen (Registerdetails, Fallstricke, Designentscheidungen) nicht verloren geht.
