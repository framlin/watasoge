# Watasoge Firmware — STM32G431KB

## Hardware

- **MCU:** STM32G431KB (ARM Cortex-M4F, 170 MHz, FPU, 128 KB Flash, 32 KB RAM)
- **Board:** NUCLEO-G431KB (MB1430)
- **Audio-DAC:** PCM5102 (extern, via I2S/SAI, 48 kHz, 16-Bit Stereo)
- **CAN-Transceiver:** Waveshare SN65HVD230 (3.3V, 500 kbit/s Classic CAN)

## Aktueller Stand

fracanto-Modul mit Wavetable-Synthese, gesteuert über CAN-Bus. Audio-Ausgabe über fracanto Audio-Pipeline (Mainloop-Polling, DMA Circular). 220 integrierte Wavetables, ADSR-Envelope, 6 CC-Parameter.

- **SAI1 Block A:** I2S-Master-TX, 16-Bit Stereo, 48 kHz
- **DMA:** Circular-DMA (DMA1 Channel1), Double-Buffer (2×64 Frames)
- **FDCAN1:** Classic CAN, 500 kbit/s (PA11 RX, PA12 TX, AF9)
- **Pins:** PA8/PA9/PA10 (SAI1, AF14), PA11/PA12 (FDCAN1, AF9), PB8 (LED2)
- **Systemtakt:** 170 MHz (HSI 16 MHz → PLL), FDCAN-Kernel-Takt: PCLK1
- **NVIC-Prioritäten:** DMA(2) > FDCAN(4) > SysTick(8)
- **Flash:** 96.892 Bytes (74%), RAM: 4.208 Bytes (13%)

### Modul-Architektur

```
fracanto_module_tick(&mod)
├─ fracanto_node_tick()         CAN-RX → Dispatch → Callbacks
│  ├─ on_cv(port=0)            → synthesis_set_frequency (1V/Oct, C4 bei 0V)
│  ├─ on_cc(port=0, param=0-5) → Wave/Attack/Decay/Release/Sustain/Volume
│  ├─ on_gate(port=0)          → synthesis_gate_on/off (ADSR)
│  └─ on_trigger(port=0)       → synthesis_trigger (Re-Trigger)
├─ module_process_audio()       Pipeline-Tick → watasoge_process_audio
│  └─ synthesis_fill_buffer()   Wavetable (Hermite, Diff, LP) + Volume
└─ watasoge_tick()              LED-Heartbeat, CAN-Timeout-Safety-Mute
```

### Build & Flash & Test

```bash
cmake --preset Debug
cmake --build build/Debug
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/blinky.elf verify reset exit"
cd tests && make test    # 44 Host-Side Unit-Tests
```

### Abhängigkeiten

- **Toolchain:** arm-none-eabi-gcc 10.3 (Homebrew)
- **HAL:** STM32Cube_FW_G4_V1.6.1 (`~/STM32Cube/Repository/`)
- **fracanto:** Git-Submodul (fracanto_core + fracanto_hal_stm32_fdcan + fracanto_drv_pcm5102a)
- **Build:** CMake 3.22+, Ninja
- **Flash:** OpenOCD mit ST-LINK V3
