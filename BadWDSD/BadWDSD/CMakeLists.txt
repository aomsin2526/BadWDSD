cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include ($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(BadWDSD CXX C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(BadWDSD
    Main.c Hold.c Led.c Timer.c Xdr.c Gpio.c Uart.c Sc.c DebugUart.c Aes.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(BadWDSD pico_stdlib hardware_resets hardware_irq pico_multicore pico_rand pico_sync)

# WS2812
pico_generate_pio_header(BadWDSD ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/build/generated)
target_link_libraries(BadWDSD hardware_pio)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(BadWDSD)
