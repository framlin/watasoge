# Watasoge Firmware

## Projektübersicht

Wavetable-basierter Klangerzeuger (Synthesizer).

## Hardware

- **Mikrocontroller:** Raspberry Pi Pico (RP2040)
- **Audio-DAC:** PCM5102
- **Audio-Interface:** I2S (via PIO emuliert)

## Architektur

Der RP2040 nutzt seine programmierbaren I/O-Blöcke (PIO), um das I2S-Protokoll zu emulieren und Audiodaten an den PCM5102 DAC zu übertragen.

## Wavetables

Format: 256 Samples, 16-Bit signed PCM, 44.1 kHz. Quellen: AKWF (CC0), Mutable Instruments (MIT).

→ [Detaillierte Wavetable-Dokumentation](doc/wavetables_for_pi_pico.md)
