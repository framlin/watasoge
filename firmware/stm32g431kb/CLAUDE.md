# Watasoge Firmware — STM32G431KB

## Hardware

- **MCU:** STM32G431KB (STM32G4-Serie)
- **Board:** NUCLEO-G431KB (MB1430)
- **CPU:** ARM Cortex-M4F, 170 MHz
- **FPU:** Single-precision Hardware-FPU
- **Flash:** 128 KB
- **SRAM:** 32 KB (davon 10 KB CCM-RAM)
- **Audio-DAC:** PCM5102 (extern, via I2S/SAI)

## Synthesizer-Architektur

Wavetable-basierter Klangerzeuger mit Integrated Wavetable Synthesis (Franck & Välimäki, DAFx-12), wie in Mutable Instruments Plaits verwendet.

### Wavetable-Format

- 128 Samples + 4 Guard (Hermite-Interpolation), int16_t
- Integrierte Wellenformen (kumulative Summe), Differenzierung bei Wiedergabe
- Anti-Aliasing durch Differenzierung + One-pole Tiefpass
- 220 Waves in ~60.1 KB Flash (56.7 KB Wavedaten + ~3.4 KB Metadaten)
- Header-Datei: `../../data/wavetables_integrated.h`

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

## Playback-Algorithmus (Kurzreferenz)

```c
// Initialisierung bei Wave-Wechsel:
diff.previous = (float)table[0];  // Startup-Transient vermeiden
diff.lp = 0.0f;

// Pro Sample:
float f0 = freq / SAMPLERATE;
float coeff = fminf(WAVETABLE_SIZE * f0, 1.0f);
float scale = 1.0f / (f0 * 131072.0f);
// Interpolation → Differenzierung → Tiefpass → Skalierung
```

Siehe `~/tinker/audio-samples/INTEGRATED_WAVETABLE_PLAYBACK.md` für Details zu bekannten Fallstricken.

## Online-Ressourcen

- **Produktseite MCU:** https://www.st.com/en/microcontrollers-microprocessors/stm32g431kb.html
- **Produktseite Board:** https://www.st.com/en/evaluation-tools/nucleo-g431kb.html
- **STM32CubeG4 (HAL/LL):** https://www.st.com/en/embedded-software/stm32cubeg4.html
- **STM32CubeG4 GitHub:** https://github.com/STMicroelectronics/STM32CubeG4
- **STM32CubeIDE:** https://www.st.com/en/development-tools/stm32cubeide.html
- **ARM Cortex-M4 TRM:** https://developer.arm.com/documentation/100166/latest/
