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

Integrated Wavetable Playback nach MI-Plaits-Vorbild über SAI1/I2S an PCM5102-DAC. 220 Wavetables aus `Core/Inc/wavetables_integrated.h` verfügbar. Default: 440 Hz, Wave 0 (`a_sine_00`). LED (PB8) blinkt weiterhin mit 4 Hz als Lebenszeichen. Firmware modularisiert in synthesis (Signalerzeugung), output (Audio-Ausgabe) und main (Orchestrierung).

- **SAI1 Block A:** I2S-Master-TX, 16-Bit Stereo, ~44.1 kHz (SYSCLK-basiert, ~44.27 kHz)
- **DMA:** Circular-DMA (DMA1 Channel1), Half-/Complete-Callbacks
- **Audio-Buffer:** 128 Stereo-Frames (256 int16_t)
- **Wavetable-Playback:** Float-Phase-Accumulator [0,1), Hermite-4-Punkt-Interpolation über integrierte Wavetables, Differenzierung + One-Pole-LP (adaptives Anti-Aliasing), frequenzabhängige Skalierung, Output-Gain ±24000 (~75% Full-Scale)
- **Pins:** PA8 (SAI1_SCK_A, AF14), PA9 (SAI1_FS_A, AF14), PA10 (SAI1_SD_A, AF14)
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
│   │   ├── synthesis.h          # synthesis_init(), synthesis_fill_buffer(), set_frequency(), set_wave()
│   │   ├── output.h             # output_init()
│   │   ├── stm32g4xx_hal_conf.h # HAL-Module: GPIO, RCC, FLASH, PWR, CORTEX, DMA, EXTI, SAI
│   │   ├── wavetables_integrated.h # 220 integrierte Wavetables (MI-Plaits-Stil, ~58 KB)
│   │   └── stm32g4xx_it.h       # Interrupt-Prototypen
│   └── Src/
│       ├── main.c               # Orchestrierung: Clock, GPIO, Init-Reihenfolge, LED-Loop
│       ├── synthesis.c          # Signalerzeugung: Integrated Wavetable Playback (Hermite, Diff, LP)
│       ├── output.c             # Audio-Ausgabe: SAI/DMA-Handles, Buffer, Init, Callbacks
│       ├── stm32g4xx_it.c       # SysTick → HAL_IncTick(), DMA1_Ch1 → HAL_DMA_IRQHandler()
│       ├── stm32g4xx_hal_msp.c  # SYSCFG/PWR, SAI1 MspInit (GPIO AF14, DMA1 Circular)
│       ├── system_stm32g4xx.c   # SystemInit (FPU), SystemCoreClockUpdate
│       ├── syscalls.c           # Newlib Stubs
│       └── sysmem.c             # _sbrk
└── Drivers/                     # Symlink → ~/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.1/Drivers
```

### Modul-Architektur

```
main.c  ──init──→  synthesis.c    (Signalerzeugung)
   │                    ↑
   └───init──→  output.c          (Audio-Ausgabe)
                    │
                    └──ruft auf──→  synthesis_fill_buffer()
```

- **synthesis**: Erzeugt Audio-Samples. Integrated Wavetable Playback (220 Waves aus Flash), Float-Phase-Accumulator, Hermite-Interpolation, Differenzierung + One-Pole-LP, frequenzabhängige Skalierung, füllt Stereo-Buffer (L=R). API: `synthesis_set_frequency()`, `synthesis_set_wave()`.
- **output**: Kapselt SAI1/I2S/DMA. SAI- und DMA-Handles (global, extern-referenziert von hal_msp.c und it.c), DMA-Callbacks rufen `synthesis_fill_buffer()`.
- **main**: Initialisierungsreihenfolge (HAL → Clock → GPIO → synthesis → output) und LED-Loop.

### Clock-Konfiguration

HSI 16 MHz → PLL (PLLM=4, PLLN=85, PLLR=2) → **170 MHz SYSCLK**. Voltage Scale 1 Boost, Flash Latency 4 WS.

### Build & Flash

```bash
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
```

### Build-Ergebnis (Debug)

- Flash: 70.832 Bytes (54.1% von 128 KB) — davon ~58 KB Wavetable-Daten
- RAM: 2.376 Bytes (7.3% von 32 KB)

### Abhängigkeiten

- **Toolchain:** arm-none-eabi-gcc 10.3 (Homebrew)
- **HAL:** STM32Cube_FW_G4_V1.6.1 (`~/STM32Cube/Repository/`)
- **Build:** CMake 3.22+, Ninja
- **Flash:** OpenOCD mit ST-LINK V3

## Synthesizer-Architektur

Wavetable-basierter Klangerzeuger mit Integrated Wavetable Synthesis (Franck & Välimäki, DAFx-12), wie in Mutable Instruments Plaits verwendet.

### Wavetable-Format

- 128 Samples + 4 Guard (Hermite-Interpolation), int16_t
- Integrierte Wellenformen (kumulative Summe), Differenzierung bei Wiedergabe
- Anti-Aliasing durch Differenzierung + One-pole Tiefpass
- 220 Waves in ~60.1 KB Flash (56.7 KB Wavedaten + ~3.4 KB Metadaten)
- Header-Datei: `Core/Inc/wavetables_integrated.h`

### Speicherbudget

| Ressource | Gesamt | Wavetables | Verfügbar für Firmware |
|---|---|---|---|
| Flash | 128 KB | ~60.1 KB | ~67.9 KB |
| SRAM | 32 KB | — (Flash-only) | 32 KB |

## Relevante Peripherie für Audio

- **SAI** (Serial Audio Interface): I2S-Ausgabe an PCM5102
- **DMA**: Doppelpuffer-Übertragung für unterbrechungsfreies Audio-Streaming
- **TIM**: Timer für Sample-Rate-Generierung (44.1 kHz)
- **DAC** (intern, 12-Bit): Nicht für Audio-Ausgabe vorgesehen (PCM5102 übernimmt)
- **CORDIC**: Hardware-Beschleuniger für trigonometrische Funktionen
- **FMAC**: Filter Math Accelerator — potenziell nutzbar für Tiefpass-Filterung

## Dokumentation

### Lokale Hersteller-PDFs (`~/tinker/mcu-docs/stm32g431kb/`)

| Dokument | Datei | Inhalt |
|---|---|---|
| Datasheet | `stm32g431kb-datasheet.pdf` | Pinout, elektrische Spezifikationen, Peripherie-Übersicht |
| Reference Manual | `rm0440-reference-manual.pdf` | Register-Beschreibungen aller Peripherie (SAI, DMA, TIM, etc.) |
| Programming Manual | `pm0214-programming-manual.pdf` | Cortex-M4 Instruction Set, NVIC, SysTick, FPU |
| Board User Manual | `nucleo-g431kb-user-manual.pdf` | NUCLEO-G431KB Schaltplan, Jumper-Konfiguration, Pin-Mapping |

### Wissensbasis (`~/obsidian/mcu.zettelkasten/`)

Obsidian-Zettelkasten mit Topic-Struktur für MCU-Wissen. Relevante Topics:

- `io/output/topics/STM32G4.md` — Notizen zur STM32G4-Serie
- `io/output/topics/Peripherals.md` — Peripherie-Notizen (SAI, DMA, Timer)
- `io/output/topics/Toolchains.md` — Build-Toolchains, Debugger

### Wavetable-Referenzen (`~/tinker/audio-samples/`)

| Datei | Inhalt |
|---|---|
| `INTEGRATED_WAVETABLE_PLAYBACK.md` | Playback-Algorithmus, Startup-Transient-Fix, Guard-Sample-Drift |
| `NOTES.md` | Wavetable-Extraktion, MI-Analyse, Speicherbudget |
| `mi-reference/plaits_wavetable_oscillator.h` | Mutable Instruments Plaits — Differentiator, Hermite, Oszillator |
| `mi-reference/plaits_wavetable_engine.cc` | Plaits — 8x8x3 Wave-Terrain, trilineare Interpolation |
| `generate_integrated_wavetables.py` | Generator-Script für die integrierten Wavetables |

## Playback-Algorithmus (implementiert in `synthesis.c`)

Pipeline pro Sample: Phase-Accumulator → Hermite-Interpolation → Differenzierung → One-Pole-LP → Skalierung → int16

```c
// Initialisierung bei Wave-Wechsel (Startup-Transient-Fix):
diff.previous = (float)wave[0];
diff.lp = 0.0f;

// Frequenzabhängige Parameter:
f0 = freq / 44100.0f;
coeff = fminf(128.0f * f0, 1.0f);
scale = 1.0f / (f0 * 131072.0f);

// Pro Sample:
// phase [0,1) → Hermite über 4 Tabellenwerte → Differenzierung → LP → scale × 24000 → int16
```

Siehe `~/tinker/audio-samples/INTEGRATED_WAVETABLE_PLAYBACK.md` für Details zu bekannten Fallstricken.

## Online-Ressourcen

- **Produktseite MCU:** https://www.st.com/en/microcontrollers-microprocessors/stm32g431kb.html
- **Produktseite Board:** https://www.st.com/en/evaluation-tools/nucleo-g431kb.html
- **STM32CubeG4 (HAL/LL):** https://www.st.com/en/embedded-software/stm32cubeg4.html
- **STM32CubeG4 GitHub:** https://github.com/STMicroelectronics/STM32CubeG4
- **STM32CubeIDE:** https://www.st.com/en/development-tools/stm32cubeide.html
- **ARM Cortex-M4 TRM:** https://developer.arm.com/documentation/100166/latest/
