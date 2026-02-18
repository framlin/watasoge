---
name: ucconfig
description: |
  STM32G431KB Firmware-Konfiguration für das watasoge-Projekt. Verwaltet die Build-Infrastruktur:
  CMakeLists.txt, CMakePresets.json, Toolchain-File, Linker-Script, Startup-File, HAL-Konfiguration
  und den Drivers-Symlink. Verwende diesen Skill, wenn HAL-Module hinzugefügt/entfernt werden,
  neue Peripherie konfiguriert wird, oder sich an der Build-Konfiguration etwas ändern muss.
  Triggers: "HAL konfigurieren", "Peripherie hinzufügen", "CMake ändern", "Linker-Script",
  "HAL-Modul", "Build-Konfiguration", "ucconfig".
allowed-tools: Read, Write, Edit, Glob, Grep, Bash
---

# ucconfig — STM32G431KB Build-Konfiguration

Verwaltet die gesamte Build-Infrastruktur der STM32G431KB-Firmware im watasoge-Projekt.

## Projektlayout

```
firmware/stm32g431kb/
├── CMakeLists.txt                    # Build-Definition
├── CMakePresets.json                 # Debug/Release Presets (Ninja)
├── cmake/
│   └── gcc-arm-none-eabi.cmake       # Toolchain-File
├── STM32G431KBTX_FLASH.ld           # Linker-Script
├── startup_stm32g431xx.s            # Startup (Kopie aus STM32Cube Repo)
├── Core/
│   ├── Inc/
│   │   ├── main.h                   # Pin-Definitionen, Projekt-Header
│   │   ├── stm32g4xx_hal_conf.h     # HAL-Modulauswahl
│   │   └── stm32g4xx_it.h           # Interrupt-Prototypen
│   └── Src/
│       ├── main.c                   # Applikationslogik
│       ├── stm32g4xx_it.c           # Interrupt-Handler
│       ├── stm32g4xx_hal_msp.c      # MSP-Init (Clock-Enable für Peripherie)
│       ├── system_stm32g4xx.c       # SystemInit, SystemCoreClockUpdate
│       ├── syscalls.c               # Newlib Stubs
│       └── sysmem.c                 # _sbrk
└── Drivers/                         # Symlink → <STM32Cube_FW_G4>/Drivers
```

## Hardware

- **MCU:** STM32G431KB, Cortex-M4F, 170 MHz
- **Board:** NUCLEO-G431KB
- **Flash:** 128 KB, **RAM:** 32 KB (+ 10 KB CCMSRAM)
- **Toolchain:** arm-none-eabi-gcc 10.3 (Homebrew)
- **HAL:** STM32Cube_FW_G4_V1.6.1

## Aktuelle Clock-Konfiguration

HSI 16 MHz → PLL (PLLM=4, PLLN=85, PLLR=2) → 170 MHz SYSCLK.
Voltage Scale 1 Boost, Flash Latency 4 WS.

## HAL-Modul hinzufügen

Wenn neue Peripherie benötigt wird (z.B. SAI, TIM, SPI), sind drei Dateien zu ändern:

### 1. `Core/Inc/stm32g4xx_hal_conf.h` — Modul aktivieren

```c
#define HAL_SAI_MODULE_ENABLED
```

Und den entsprechenden Include-Block hinzufügen:

```c
#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32g4xx_hal_sai.h"
#endif
```

### 2. `CMakeLists.txt` — HAL-Treiber-Quelldateien eintragen

Im `SOURCES`-Block die benötigten HAL-Quelldateien ergänzen:

```cmake
Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_sai.c
Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_sai_ex.c
```

### 3. `Core/Src/stm32g4xx_hal_msp.c` — MSP-Callbacks

Für Peripherie mit Clock- oder Pin-Konfiguration die `HAL_<PERIPH>_MspInit()` und
`HAL_<PERIPH>_MspDeInit()` Callbacks implementieren.

## HAL-Treiber-Quelldateien finden

Die verfügbaren HAL-Quelldateien liegen unter:

```
Drivers/STM32G4xx_HAL_Driver/Src/
```

Dort existieren jeweils `stm32g4xx_hal_<modul>.c` und ggf. `stm32g4xx_hal_<modul>_ex.c`.

## Aktuell kompilierte HAL-Treiber

- `hal.c`, `hal_rcc.c`, `hal_rcc_ex.c`
- `hal_gpio.c`, `hal_cortex.c`
- `hal_pwr.c`, `hal_pwr_ex.c`
- `hal_flash.c`, `hal_flash_ex.c`, `hal_flash_ramfunc.c`
- `hal_exti.c`, `hal_dma.c`, `hal_dma_ex.c`

## Linker-Script

`STM32G431KBTX_FLASH.ld` — Kopie aus STM32Cube Repository mit entfernten `READONLY`-Keywords
(GCC 10.3 unterstützt diese nicht). Bei Upgrade auf GCC 11+ können die `READONLY`-Keywords
wieder eingefügt werden.

## Compiler-Flags

```
-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
-Wall -fdata-sections -ffunction-sections
```

Linker: `--specs=nano.specs -Wl,--gc-sections -lc -lm -lnosys`

## Wichtige Hinweise

- **Drivers-Symlink:** Zeigt auf `<STM32Cube_FW_G4>/Drivers`.
  Nicht löschen oder durch Kopie ersetzen.
- **syscalls.c:** Enthält keine `_exit`-Funktion — diese wird von `libnosys` bereitgestellt.
  Keine eigene `_exit` hinzufügen, sonst gibt es Multiple-Definition-Fehler beim Linken.
- **startup_stm32g431xx.s:** Unveränderte Kopie aus dem STM32Cube Repository.
  Bei HAL-Updates ggf. durch neuere Version ersetzen.
- Nach Konfigurationsänderungen immer einen Clean-Build durchführen:
  `rm -rf build/Debug && cmake --preset Debug && cmake --build build/Debug`

## Referenzen

- HAL-Quelldateien: `Drivers/STM32G4xx_HAL_Driver/Src/`
- HAL-Header: `Drivers/STM32G4xx_HAL_Driver/Inc/`
- CMSIS-Header: `Drivers/CMSIS/Device/ST/STM32G4xx/Include/`
