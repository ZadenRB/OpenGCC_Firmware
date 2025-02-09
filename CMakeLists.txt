cmake_minimum_required(VERSION 3.18)

include(pico-sdk/pico_sdk_init.cmake)
include(OpenGCC.cmake)

project(OpenGCC C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -flto")

pico_sdk_init()

if (TARGET tinyusb_device)
    add_subdirectory(opengcc)

    # Add your controller here
    add_subdirectory(controllers/NobGCC/rev1)
    add_subdirectory(controllers/PhobGCC)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building OpenGCC because TinyUSB submodule is not initialized in the SDK")
endif()
