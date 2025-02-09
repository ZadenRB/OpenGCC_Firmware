add_library(OpenGCC INTERFACE)

target_sources(OpenGCC INTERFACE
    analog_controller.hpp
    calibration.hpp
    calibration.cpp
    configuration.hpp
    configuration.cpp
    curve_fitting.hpp
    curve_fitting.tpp
    joybus.hpp
    joybus.cpp
    main.hpp
    main.cpp
    state.hpp
    state.cpp
)

target_include_directories(OpenGCC INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenGCC INTERFACE
    hardware_adc
    hardware_dma
    hardware_flash
    hardware_pio
    pico_multicore
    pico_stdlib
)

target_compile_definitions(OpenGCC INTERFACE
    NONE=0
    LINEAR=1
    POLYNOMIAL=2
)

pico_generate_pio_header(OpenGCC ${CMAKE_CURRENT_SOURCE_DIR}/pio/joybus.pio)
pico_generate_pio_header(OpenGCC ${CMAKE_CURRENT_SOURCE_DIR}/pio/single_pin_joybus.pio)
