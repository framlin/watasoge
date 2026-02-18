# Watasoge Firmware — STM32G431KB

## Hardware

- **MCU:** STM32G431KB (STM32G4-Serie)
- **Board:** NUCLEO-G431KB (MB1430)
- **CPU:** ARM Cortex-M4F, 170 MHz
- **FPU:** Single-precision Hardware-FPU
- **Flash:** 128 KB
- **SRAM:** 32 KB (davon 10 KB CCM-RAM)
- **Audio-DAC:** PCM5102 (extern, via I2S/SAI)

## Aktueller Stand

Zwei Synthese-Engines: Integrated Wavetable Playback (verifiziert) und Karplus-Strong String Synthesis (auf Hardware verifiziert). Audio-Ausgabe über SAI1/I2S an PCM5102-DAC. Gate-gesteuerter Player: externes Gate-Signal an PA0 (EXTI, Rising+Falling) triggert Note-ON (steigende Flanke) und Note-OFF (fallende Flanke). Player bestimmt WAS gespielt wird (Ton, Engine, Parameter), das Gate bestimmt WANN und WIE LANGE. 21 Instrumentengruppen: 11 Wavetable, 4 KS-melodisch, 6 KS-perkussiv. Audio-Quelle wird automatisch zwischen den Engines umgeschaltet. Flash-Nutzung: 84.440 Bytes (64,5%), RAM: 9.124 Bytes (27,9%).

- **SAI1 Block A:** I2S-Master-TX, 16-Bit Stereo, ~44.1 kHz (SYSCLK-basiert, ~44.27 kHz)
- **DMA:** Circular-DMA (DMA1 Channel1), Half-/Complete-Callbacks
- **Audio-Buffer:** 128 Stereo-Frames (256 int16_t)
- **Wavetable-Playback:** Float-Phase-Accumulator [0,1), Hermite-4-Punkt-Interpolation über integrierte Wavetables, Differenzierung + One-Pole-LP (adaptives Anti-Aliasing), frequenzabhängige Skalierung, Output-Gain ±24000 (~75% Full-Scale)
- **Karplus-Strong:** Delay-Line (1024 floats) + Allpass-Delay-Line (256 floats), Hermite-Interpolation, ZDF-SVF Lowpass (Q=0.5), RT60-basiertes Decay, Allpass-Dispersion, Curved-Bridge-Nichtlinearität, DC-Blocker, Noise-Burst-Excitation (via `dl_write` für korrekte zirkuläre Konvention), Per-Sample-Parameterinterpolation, SVF-Delay-Kompensation per LUT
- **Gain-Envelope:** Dual-Modus: Smoothstep-Fade (256 Samples, ~5,8 ms) für melodische Sustain-Töne, exponentieller Decay für perkussive Hits (kategorieabhängig: Kicks 0.9995, Snares 0.9993, HiHats 0.9985). Pipeline läuft auch bei Mute weiter (Differentiator bleibt eingeschwungen).
- **Gate-Input:** PA0 (EXTI, Rising+Falling, Pulldown) — externes Gate-Signal für Note-ON/OFF
- **Pins:** PA0 (Gate-Input), PA8 (SAI1_SCK_A, AF14), PA9 (SAI1_FS_A, AF14), PA10 (SAI1_SD_A, AF14)
- **MCK:** Deaktiviert (PCM5102 erzeugt MCLK intern)

### Firmware-Struktur

```
stm32g431kb/
├── CMakeLists.txt                # CMake 3.22, C11, Cortex-M4F Flags
├── CMakePresets.json             # Debug (-O0 -g3) / Release (-Os), Ninja
├── cmake/
│   └── gcc-arm-none-eabi.cmake   # Toolchain-File
├── STM32G431KBTX_FLASH.ld       # Linker-Script (READONLY entfernt für GCC 10.3)
├── startup_stm32g431xx.s        # Startup (Kopie aus STM32Cube Repo)
├── Core/
│   ├── Inc/
│   │   ├── main.h               # LED2_PIN (PB8), Error_Handler
│   │   ├── input.h              # Gate-Input API: input_init(), gate_on/off_pending()
│   │   ├── audio_config.h       # Gemeinsame Konstanten: SAMPLE_RATE, OUTPUT_GAIN, PI_F
│   │   ├── svf.h                # Wiederverwendbarer ZDF-SVF: svf_state_t, svf_coeff_t, svf_process()
│   │   ├── delay_line.h         # Delay-Line-Operationen: dl_write(), dl_read_hermite(), dl_allpass()
│   │   ├── synthesis.h          # synthesis_init(), fill_buffer(), set_frequency/wave/mute/decay(), trigger()
│   │   ├── karplus.h            # karplus_init(), fill_buffer(), set_frequency/damping/brightness/dispersion(), trigger()
│   │   ├── player.h             # player_group_t enum (21 Gruppen), player_init(group), note_on(), note_off(), beat_pending()
│   │   ├── player_config.h      # group_def_t, ks_params_t, Note-Count-Konstanten, extern-Deklarationen
│   │   ├── output.h             # output_init(fill_buffer_fn), output_set_source(fill_buffer_fn)
│   │   ├── stm32g4xx_hal_conf.h # HAL-Module: GPIO, RCC, FLASH, PWR, CORTEX, DMA, EXTI, SAI
│   │   ├── wavetables_integrated.h # 220 integrierte Wavetables (MI-Plaits-Stil, ~58 KB)
│   │   └── stm32g4xx_it.h       # Interrupt-Prototypen
│   └── Src/
│       ├── main.c               # Orchestrierung: Clock, GPIO, Input-Init, Gate-Polling Loop (Player + LED)
│       ├── input.c              # Gate-Input: PA0 EXTI-Konfiguration, HAL_GPIO_EXTI_Callback, Polling-Flags
│       ├── synthesis.c          # Signalerzeugung: IWT Playback (Hermite, Diff, LP), Dual-Envelope
│       ├── karplus.c            # Karplus-Strong: nutzt svf.h + delay_line.h, Allpass, Curved Bridge, Excitation
│       ├── player.c             # Gate-gesteuerter Sequencer: 21 Gruppen, note_on/off, automatische Quellenwahl
│       ├── player_config.c      # Gruppen-Definitionen, KS-Presets, Frequenz-Arrays (Daten)
│       ├── output.c             # Audio-Ausgabe: SAI/DMA, Dependency-Injection für fill_buffer_fn
│       ├── stm32g4xx_it.c       # SysTick → HAL_IncTick(), DMA1_Ch1 → HAL_DMA_IRQHandler(), EXTI0 → HAL_GPIO_EXTI_IRQHandler()
│       ├── stm32g4xx_hal_msp.c  # SYSCFG/PWR, SAI1 MspInit (GPIO AF14, DMA1 Circular)
│       ├── system_stm32g4xx.c   # SystemInit (FPU), SystemCoreClockUpdate
│       ├── syscalls.c           # Newlib Stubs
│       └── sysmem.c             # _sbrk
└── Drivers/                     # Symlink → <STM32Cube_FW_G4>/Drivers
```

### Modul-Architektur

```
main.c  ──init──→  input.c           (Gate-Eingang PA0, EXTI)
   │                    │
   │                    └──pollt──→  gate_on/off_pending()
   │
   ├───init──→  synthesis.c          (Wavetable-Signalerzeugung)
   │                    ↑
   ├───init──→  karplus.c            (Karplus-Strong-Signalerzeugung)
   │                ↑
   ├───init──→  output.c             (Audio-Ausgabe, umschaltbare Quelle)
   │                │
   │                └──ruft auf──→  fill_buffer_fn (synthesis oder karplus)
   │
   └───init+gate──→  player.c        (Gate-gesteuerter Sequencer)
                        │
                        ├──steuert──→  synthesis_set_wave/frequency/mute()
                        ├──steuert──→  karplus_set_frequency/damping/brightness/dispersion()
                        └──schaltet──→  output_set_source(synthesis|karplus)
```

- **audio_config** (`audio_config.h`): Gemeinsame Audio-Konstanten (`SAMPLE_RATE`, `OUTPUT_GAIN`, `PI_F`), zentral statt dupliziert in synthesis.c/karplus.c.
- **svf** (`svf.h`): Wiederverwendbarer ZDF State Variable Filter (Inline-Header). `svf_state_t` (Filter-State), `svf_coeff_t` (Koeffizienten), `svf_compute_coeff()` (Berechnung), `svf_process()` (Lowpass-Ausgabe). Wird von karplus.c genutzt.
- **delay_line** (`delay_line.h`): Wiederverwendbare Delay-Line-Operationen (Inline-Header). `dl_write()` (zirkuläres Schreiben), `dl_read_hermite()` (Hermite-4-Punkt-Interpolation), `dl_allpass()` (Allpass-Filter). Wird von karplus.c genutzt.
- **synthesis**: Erzeugt Audio-Samples via Integrated Wavetable Playback (220 Waves aus Flash), Float-Phase-Accumulator, Hermite-Interpolation, Differenzierung + One-Pole-LP, frequenzabhängige Skalierung, Dual-Envelope (Smoothstep-Fade für Sustain, exponentieller Decay für Perkussion), füllt Stereo-Buffer (L=R). API: `synthesis_fill_buffer()`, `synthesis_set_frequency()`, `synthesis_set_wave()`, `synthesis_set_mute()`, `synthesis_set_decay()`, `synthesis_trigger()`.
- **karplus**: Karplus-Strong String Synthesis nach MI Rings/Plaits-Vorbild. Nutzt `svf.h` für Loop-Filter und Excitation-Filter, `delay_line.h` für Ringbuffer-Operationen. RT60-basiertes Decay, Allpass-Dispersion (256 floats) für Inharmonizität, Curved-Bridge-Nichtlinearität für Sitar-Buzz, DC-Blocker, Stabilitäts-Clamp, Noise-Burst-Excitation (XorShift32 PRNG), Per-Sample-Parameterinterpolation, SVF-Delay-Kompensation per LUT. API: `karplus_fill_buffer()`, `karplus_set_frequency()`, `karplus_set_damping()`, `karplus_set_brightness()`, `karplus_set_dispersion()`, `karplus_trigger()`.
- **input** (`input.c/.h`): Gate-Eingang PA0 als EXTI (Rising+Falling, Pulldown). `HAL_GPIO_EXTI_Callback()` setzt volatile Flags. Polling-API: `input_gate_on_pending()`, `input_gate_off_pending()`. NVIC-Priorität 5.
- **player**: Gate-gesteuerter Sequencer. Steuert 21 Instrumentengruppen, schaltet automatisch die Audio-Quelle in output um. Beat-Flag für LED-Synchronisation. API: `player_init(group)`, `player_note_on()`, `player_note_off()`, `player_beat_pending()`. Note-OFF mutet melodische Wavetables und rückt Note-Counter vor; perkussive/KS klingen natürlich aus.
- **player_config** (`player_config.c/.h`): Gruppen-Definitionen (`groups[]`), KS-Presets (`ks_presets[]`), Frequenz-Arrays (`c_major_freqs[]`, `ks_scale_freqs[]`), Note-Count-Konstanten. Daten getrennt von Sequencer-Logik.
- **output**: Kapselt SAI1/I2S/DMA. Umschaltbarer Funktionspointer `fill_buffer_fn` für Audio-Quellenwahl per Dependency Injection. DMA-Callbacks rufen die aktive Quelle. API: `output_init(fill_buffer_fn)`, `output_set_source(fn)`.
- **main**: Initialisierungsreihenfolge (HAL → Clock → GPIO → input → synthesis → karplus → output(synthesis_fill_buffer) → player), non-blocking Main-Loop (Gate-Polling → `player_note_on()`/`player_note_off()` + beat-synchrone LED).

### Clock-Konfiguration

HSI 16 MHz → PLL (PLLM=4, PLLN=85, PLLR=2) → **170 MHz SYSCLK**. Voltage Scale 1 Boost, Flash Latency 4 WS.

### Build & Flash

```bash
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
```

### Abhängigkeiten

- **Toolchain:** arm-none-eabi-gcc 10.3 (Homebrew)
- **HAL:** STM32Cube_FW_G4_V1.6.1
- **Build:** CMake 3.22+, Ninja
- **Flash:** OpenOCD mit ST-LINK V3

## Synthesizer-Architektur

Zwei Synthese-Engines:

### 1. Integrated Wavetable Synthesis

Wavetable-Playback nach Franck & Välimäki (DAFx-12) / Mutable Instruments Plaits.

- 128 Samples + 4 Guard (Hermite-Interpolation), int16_t
- Integrierte Wellenformen (kumulative Summe), Differenzierung bei Wiedergabe
- Anti-Aliasing durch Differenzierung + One-pole Tiefpass
- 220 Waves in ~60.1 KB Flash (56.7 KB Wavedaten + ~3.4 KB Metadaten)
- Header-Datei: `Core/Inc/wavetables_integrated.h`

### 2. Karplus-Strong String Synthesis

Physical-Modelling-Synthese nach MI Rings/Plaits (Plaits-Variante als Vorlage).

- Delay-Line: 1024 floats (4 KB RAM), tiefste Frequenz ~43 Hz (F1)
- Allpass-Dispersion: 256 floats (1 KB RAM) für inharmonische Obertöne
- Loop-Filter: ZDF-SVF Lowpass (Zero-Delay-Feedback, Trapezoidal Integration), Q=0.5
- Decay: RT60-basiert über `semitones_to_ratio()` LUT (257 Einträge)
- SVF-Delay-Kompensation: LUT (129 Einträge) korrigiert SVF-Phasenverzögerung
- Nichtlinearitäten: Allpass-Dispersion (>0) oder Curved Bridge (<0, Sitar-Buzz)
- Excitation: White-Noise-Burst (XorShift32 PRNG), SVF-gefiltert, Dauer = 1/f0
- Stabilität: DC-Blocker + Hard-Clamp [-20, +20]

### Speicherbudget

| Ressource | Gesamt | Wavetables | Karplus-Strong | Verfügbar |
|---|---|---|---|---|
| Flash | 128 KB | ~60.1 KB | ~3–4 KB (Code+LUT) | ~64 KB |
| SRAM | 32 KB | — (Flash-only) | ~5,2 KB (Delay-Lines+State) | ~25 KB |

## Relevante Peripherie für Audio

- **SAI** (Serial Audio Interface): I2S-Ausgabe an PCM5102
- **DMA**: Doppelpuffer-Übertragung für unterbrechungsfreies Audio-Streaming
- **TIM**: Timer für Sample-Rate-Generierung (44.1 kHz)
- **DAC** (intern, 12-Bit): Nicht für Audio-Ausgabe vorgesehen (PCM5102 übernimmt)
- **CORDIC**: Hardware-Beschleuniger für trigonometrische Funktionen
- **FMAC**: Filter Math Accelerator — potenziell nutzbar für Tiefpass-Filterung

## Playback-Algorithmus (implementiert in `synthesis.c`)

Pipeline pro Sample: Phase-Accumulator → Hermite-Interpolation → Differenzierung → One-Pole-LP → Skalierung → Gain-Envelope → int16

```c
// Initialisierung bei Wave-Wechsel (Startup-Transient-Fix):
diff.previous = (float)wave[0];
diff.lp = 0.0f;

// Frequenzabhängige Parameter:
f0 = freq / 44100.0f;
coeff = fminf(128.0f * f0, 1.0f);
scale = 1.0f / (f0 * 131072.0f);

// Gain-Envelope (Dual-Modus):
// Sustain: gain rampt linear 0↔1, dann g = gain² × (3 - 2×gain) (Smoothstep)
// Decay:   gain *= decay_rate pro Sample (exponentiell), trigger() setzt gain=1
// Pipeline läuft auch bei mute weiter (Differentiator bleibt eingeschwungen)

// Pro Sample:
// phase [0,1) → Hermite über 4 Tabellenwerte → Differenzierung → LP → scale × 24000 × g → int16
```

## Karplus-Strong Algorithmus (implementiert in `karplus.c`)

Pipeline pro Sample: Delay-Line Read (Hermite) → [Dispersion oder Curved Bridge] → SVF Lowpass → Damping → DC-Block → Clamp → Delay-Line Write

```c
// Trigger: Delay-Line mit gefiltertem White Noise füllen (Dauer = 1/f0)
// Noise → SVF LP (Cutoff abhängig von Brightness)

// Delay-Länge: N = SAMPLE_RATE / f0, Hermite-Interpolation für fraktionalen Anteil
// SVF-Delay-Kompensation: delay *= (1 - svf_shift_lookup(cutoff_semitones))

// RT60-Decay:
// rt60 = 0.07 * semitones_to_ratio(damping² * 96) * SAMPLE_RATE
// damping_coefficient = semitones_to_ratio(-120 * delay / rt60)

// Dispersion > 0: Allpass-Delay-Line (Inharmonizität, Glocken/Gamelan)
// Dispersion < 0: Curved Bridge (amplitudenabhängige Delay-Modulation, Sitar-Buzz)
```

## Online-Ressourcen

- **Produktseite MCU:** https://www.st.com/en/microcontrollers-microprocessors/stm32g431kb.html
- **Produktseite Board:** https://www.st.com/en/evaluation-tools/nucleo-g431kb.html
- **STM32CubeG4 (HAL/LL):** https://www.st.com/en/embedded-software/stm32cubeg4.html
- **STM32CubeG4 GitHub:** https://github.com/STMicroelectronics/STM32CubeG4
- **STM32CubeIDE:** https://www.st.com/en/development-tools/stm32cubeide.html
- **ARM Cortex-M4 TRM:** https://developer.arm.com/documentation/100166/latest/
- **MI Eurorack (Rings/Plaits KS-Referenz):** https://github.com/pichenettes/eurorack
