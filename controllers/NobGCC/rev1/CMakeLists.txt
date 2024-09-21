add_executable(NobGCC_rev1)

target_sources(NobGCC_rev1 PRIVATE
    rev1.hpp
    rev1.cpp
)

target_link_libraries(NobGCC_rev1
    hardware_i2c
)

target_compile_definitions(OpenGCC INTERFACE
    PICO_FLASH_SIZE_BYTES=16777216
)

pico_add_extra_outputs(NobGCC_rev1)
pico_enable_stdio_usb(NobGCC_rev1 1)
pico_enable_stdio_uart(NobGCC_rev1 0)
