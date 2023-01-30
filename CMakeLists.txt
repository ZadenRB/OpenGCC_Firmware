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
        src/curve_fitting.hpp
        src/curve_fitting.cpp
        src/joybus.hpp
        src/joybus.cpp
        src/joybus_uf2.hpp
        src/joybus_uf2.cpp
        src/main.hpp
        src/main.cpp
        src/q2x14.hpp
        src/q2x14.cpp
    )

    target_compile_definitions(NobGCC PRIVATE
        PICO_FLASH_SIZE_BYTES=16777216
    )

    target_link_libraries(NobGCC
        hardware_adc
        hardware_dma
        hardware_pio
        pico_multicore
        pico_stdlib
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