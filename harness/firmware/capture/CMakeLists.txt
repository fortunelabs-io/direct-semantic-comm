cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(capture_engine C ASM)

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

add_executable(capture_engine
    startup.s
    timebase.c
    sensor.c
    main.c
)

target_compile_options(capture_engine PRIVATE
    ${MCU_FLAGS}
    -Wall -Wextra -std=c11
    -ffunction-sections -fdata-sections
    -O2 -g
)

target_link_options(capture_engine PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F411CEU6_FLASH.ld
    -Wl,--gc-sections
    -nostartfiles
    -specs=nosys.specs
)

add_custom_command(TARGET capture_engine POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:capture_engine>
        $<TARGET_FILE_DIR:capture_engine>/capture_engine.bin
    COMMENT "Generating capture_engine.bin for flashing"
)
