add_executable(PhobGCC)

target_sources(PhobGCC PRIVATE
    PhobGCC.hpp
    PhobGCC.cpp
)

target_link_libraries(PhobGCC
    hardware_spi
)

target_compile_definitions(OpenGCC INTERFACE
    PICO_FLASH_SIZE_BYTES=16777216
)

pico_add_extra_outputs(PhobGCC)
