init_controller(NobGCC_rev1)

target_link_libraries(NobGCC_rev1
    hardware_i2c
)

target_compile_definitions(NobGCC_rev1 PRIVATE
    PICO_FLASH_SIZE_BYTES=16777216
    JOYBUS_IN_PIN=18
    JOYBUS_OUT_PIN=19
    NORMALIZATION_ALGORITHM=POLYNOMIAL
)
