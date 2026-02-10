# Watasoge

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer) für Eurorack. Das Projekt evaluiert zwei Hardware-Plattformen mit gemeinsamem Audio-DAC (PCM5102).

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
        └── CLAUDE.md
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

Das Projekt befindet sich in der Dokumentations- und Planungsphase. Es existiert noch kein Firmware-Code.

## Externe Referenzen

| Ressource | Pfad | Inhalt |
|---|---|---|
| Hersteller-PDFs | `~/tinker/mcu-docs/` | Datasheets, Reference Manuals, Programming Manuals |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` | Aufbereitete Notizen zu STM32G4, RP2040, Peripherals, Toolchains |
| Audio-Samples | `~/tinker/audio-samples/` | Wavetable-Generierung, MI-Referenzcode, Playback-Dokumentation |
