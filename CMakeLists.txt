cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(NobGCC C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

if (TARGET tinyusb_device)
    add_executable(NobGCC)

    target_sources(NobGCC PRIVATE
        src/common.hpp
        src/configuration.hpp
        src/configuration.cpp
        src/console_communication.hpp
        src/console_communication.cpp
        src/joybus_uf2_bootloader.hpp
        src/joybus_uf2_bootloader.cpp
        src/main.hpp
        src/main.cpp
        src/q2x14.hpp
        src/q2x14.cpp
    )

    target_compile_definitions(NobGCC PRIVATE
        PICO_FLASH_SIZE_BYTES=16000000
    )

    target_link_libraries(NobGCC
        hardware_adc
        hardware_clocks
        hardware_dma
        hardware_flash
        hardware_gpio
        hardware_irq
        hardware_pio
        hardware_pll
        hardware_xosc
        pico_float
        pico_multicore
        pico_stdlib
        pico_time
    )

    pico_set_binary_type(NobGCC copy_to_ram)

    pico_generate_pio_header(NobGCC ${CMAKE_CURRENT_LIST_DIR}/src/pio/joybus.pio)
    pico_generate_pio_header(NobGCC ${CMAKE_CURRENT_LIST_DIR}/src/pio/read_pwm.pio)

    pico_enable_stdio_usb(NobGCC 1)
    pico_enable_stdio_uart(NobGCC 0)

    pico_add_extra_outputs(NobGCC)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building NobGCC because TinyUSB submodule is not initialized in the SDK")
endif()