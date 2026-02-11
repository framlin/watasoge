# STM32G4 Entwicklungsumgebung — Konfiguration

## Grundsätze

- **Kein STM32CubeMX** für Code-Generierung. Claude Code übernimmt die Erzeugung von HAL-Konfiguration, Startup-Code, Linker-Script und CMake-Files.
- **Kein VSCode.** Entwicklung erfolgt ausschließlich über Claude Code (CLI).
- **Flashen und Debuggen** über OpenOCD.

## Zielplattform

| Eigenschaft | Wert |
|---|---|
| Board | NUCLEO-G431KB |
| MCU | STM32G431KB |
| Core | Cortex-M4F, 170 MHz |
| Flash | 128 KB (0x08000000) |
| RAM | 32 KB (0x20000000) |
| FPU | FPv4-SP-D16 (single-precision) |
| User-LED (LD2) | PB8 (default, SB7 ON) |
| Jumper JP1 | IDD-Messung (muss aufgesteckt bleiben für Normalbetrieb) |

## Toolchain

| Tool | Pfad | Version |
|---|---|---|
| arm-none-eabi-gcc | `/opt/homebrew/bin/arm-none-eabi-gcc` | 10.3.1 (2021.10) |
| arm-none-eabi-objcopy | `/opt/homebrew/bin/arm-none-eabi-objcopy` | — |
| arm-none-eabi-size | `/opt/homebrew/bin/arm-none-eabi-size` | — |
| arm-none-eabi-gdb | `/opt/homebrew/bin/arm-none-eabi-gdb` | — |
| CMake | `/opt/homebrew/bin/cmake` | 4.0.3 |
| Ninja | `/opt/homebrew/bin/ninja` | — |
| Make | `/usr/bin/make` | — |
| OpenOCD | `/opt/homebrew/bin/openocd` | 0.12.0 |

## Compiler-Flags (Cortex-M4F)

```
-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
```

## STM32Cube Firmware-Paket (HAL-Quelle)

```
~/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.1/
```

### Wichtige Unterverzeichnisse

| Inhalt | Pfad (relativ zum Repository) |
|---|---|
| HAL-Treiber (Src) | `Drivers/STM32G4xx_HAL_Driver/Src/` |
| HAL-Treiber (Inc) | `Drivers/STM32G4xx_HAL_Driver/Inc/` |
| CMSIS Device Headers | `Drivers/CMSIS/Device/ST/STM32G4xx/Include/` |
| CMSIS Core Headers | `Drivers/CMSIS/Include/` |
| Startup-Files (GCC) | `Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/gcc/` |
| System-Template | `Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/system_stm32g4xx.c` |
| NUCLEO-G431KB Beispiele | `Projects/NUCLEO-G431KB/` |
| Linker-Script Vorlagen | `Projects/NUCLEO-G431KB/Templates/STM32CubeIDE/` |

### Zentrale Header

- `stm32g431xx.h` — Register-Definitionen für STM32G431
- `stm32g4xx.h` — Familien-Header
- `system_stm32g4xx.h` — SystemInit, SystemCoreClock

### Compile-Defines

```
STM32G431xx
USE_HAL_DRIVER
```

## OpenOCD

| Konfiguration | Pfad |
|---|---|
| Scripts-Verzeichnis | `/opt/homebrew/share/openocd/scripts/` |
| Board-Config | `board/st_nucleo_g4.cfg` |
| Target-Config | `target/stm32g4x.cfg` |

### Flash-Befehl

```bash
openocd -f board/st_nucleo_g4.cfg -c "program build/Debug/firmware.elf verify reset exit"
```

## Referenzprojekte (als Vorlage)

| Projekt | Pfad | Besonderheit |
|---|---|---|
| HC-SR04 (ce31) | `~/studium/ce31/lab_1/NUCLEO-STM32L432KC_HC-SR04-TIM1/` | Modulare CMake-Struktur, CMakePresets |
| trill_bar (cobebo) | `~/tinker/cobebo/firmware/trill_bar/` | Monolithisches CMake, Post-Build .hex/.bin |

### Bevorzugtes CMake-Muster: Monolithisch (cobebo-Stil)

- Alle Quellen und Konfiguration in einem `CMakeLists.txt`
- Post-Build erzeugt `.elf`, `.hex`, `.bin`
- CMakePresets.json für Debug/Release
- Toolchain-File in `cmake/gcc-arm-none-eabi.cmake`

## Weitere STM32-Ressourcen auf diesem Rechner

| Ressource | Pfad |
|---|---|
| STM32CubeMX (installiert) | `/Applications/STMicroelectronics/STM32CubeMX.app` |
| STM32CubeIDE (installiert) | `/Applications/STMicroelectronics/STM32CubeIDE.app` |
| STM32CubeProgrammer | `/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer` |
| Hersteller-PDFs | `~/tinker/mcu-docs/stm32g431kb/` |
| MCU-Wissensbasis | `~/obsidian/mcu.zettelkasten/` |
| PlatformIO STM32-Framework | `~/.platformio/packages/framework-arduinoststm32/` |
| STM32L4 HAL (Referenz) | `~/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/` |
