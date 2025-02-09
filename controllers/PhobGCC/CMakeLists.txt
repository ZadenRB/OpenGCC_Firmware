init_controller(PhobGCC)

target_link_libraries(PhobGCC
    hardware_spi
)

target_compile_definitions(PhobGCC PRIVATE
    PICO_FLASH_SIZE_BYTES=16777216
    JOYBUS_IN_PIN=28
    JOYBUS_OUT_PIN=28
    NORMALIZATION_ALGORITHM=POLYNOMIAL
)
