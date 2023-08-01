cmake_minimum_required(VERSION 3.18)

include(pico-sdk/pico_sdk_init.cmake)
include(OpenGCC.cmake)

project(OpenGCC C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -flto")

pico_sdk_init()

if (TARGET tinyusb_device)
    add_library(OpenGCC INTERFACE)

    target_sources(OpenGCC INTERFACE
        opengcc/calibration.hpp
        opengcc/calibration.cpp
        opengcc/configuration.hpp
        opengcc/configuration.cpp
        opengcc/controller.hpp
        opengcc/curve_fitting.hpp
        opengcc/curve_fitting.tpp
        opengcc/joybus.hpp
        opengcc/joybus.cpp
        opengcc/main.hpp
        opengcc/main.cpp
        opengcc/state.hpp
        opengcc/state.cpp
    )

    target_include_directories(OpenGCC INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(OpenGCC INTERFACE
        PICO_FLASH_SIZE_BYTES=16777216
    )

    target_link_libraries(OpenGCC INTERFACE
        hardware_adc
        hardware_dma
        hardware_flash
        hardware_pio
        pico_multicore
        pico_stdlib
    )

    pico_set_binary_type(OpenGCC copy_to_ram)

    pico_generate_pio_header(OpenGCC ${CMAKE_CURRENT_SOURCE_DIR}/opengcc/pio/joybus.pio)
    pico_generate_pio_header(OpenGCC ${CMAKE_CURRENT_SOURCE_DIR}/opengcc/pio/single_pin_joybus.pio)

    pico_enable_stdio_usb(OpenGCC 1)
    pico_enable_stdio_uart(OpenGCC 0)

    # Add your controller here
    add_controller(controllers/NobGCC/rev1)
    add_controller(controllers/PhobGCC)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building OpenGCC because TinyUSB submodule is not initialized in the SDK")
endif()
