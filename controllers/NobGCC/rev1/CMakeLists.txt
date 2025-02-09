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
    JOYBUS_IN_PIN=18
    JOYBUS_OUT_PIN=19
    NORMALIZATION_ALGORITHM=POLYNOMIAL

)

pico_add_extra_outputs(NobGCC_rev1)
