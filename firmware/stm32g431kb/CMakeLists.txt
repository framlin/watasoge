cmake_minimum_required(VERSION 3.22)

project(blinky LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- MCU flags (global, gilt auch für Submodule) ---
set(MCU_FLAGS
    -mthumb
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)
add_compile_options(${MCU_FLAGS})

# --- Defines ---
add_compile_definitions(
    STM32G431xx
    USE_HAL_DRIVER
)

# --- fracanto submodule ---
set(FRACANTO_HAL_BACKEND "stm32_fdcan" CACHE STRING "" FORCE)
set(FRACANTO_DRV_PCM5102A ON CACHE BOOL "" FORCE)
set(FRACANTO_DRV_SSD1306 OFF CACHE BOOL "" FORCE)
set(FRACANTO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(fracanto)

# STM32 HAL headers für fracanto HAL-Backend
target_include_directories(fracanto_hal_stm32_fdcan PRIVATE
    Core/Inc
    Drivers/STM32G4xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32G4xx/Include
    Drivers/CMSIS/Include
)

# --- Sources ---
set(SOURCES
    Core/Src/main.c
    Core/Src/watasoge_module.c
    Core/Src/synthesis.c
    Core/Src/stm32g4xx_hal_msp.c
    Core/Src/stm32g4xx_it.c
    Core/Src/system_stm32g4xx.c
    Core/Src/syscalls.c
    Core/Src/sysmem.c
    startup_stm32g431xx.s

    # HAL Drivers
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_sai.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_sai_ex.c
    Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c
)

# --- Include paths ---
set(INCLUDES
    Core/Inc
    Drivers/STM32G4xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32G4xx/Include
    Drivers/CMSIS/Include
)

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

# --- Compiler flags ---
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -fdata-sections
    -ffunction-sections
    $<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>
)

# --- Linker flags ---
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32G431KBTX_FLASH.ld)

target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -lc -lm -lnosys
)

# --- fracanto link ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    fracanto_core
    fracanto_hal_stm32_fdcan
    fracanto_drv_pcm5102a
)

# --- Post-build: size, hex, bin ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Building .hex and .bin — printing size"
)
