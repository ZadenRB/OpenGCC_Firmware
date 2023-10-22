function(add_controller CONTROLLER CONTROLLER_TARGET)
    add_subdirectory(${CONTROLLER})

    get_target_property(OPENGCC_SOURCE_DIR OpenGCC SOURCE_DIR)
    get_directory_property(CONTROLLER_SOURCE_DIR DIRECTORY ${CONTROLLER} SOURCE_DIR)
    set_property(
        SOURCE ${OPENGCC_SOURCE_DIR}/opengcc/main.cpp
        DIRECTORY ${CONTROLLER_SOURCE_DIR}
        PROPERTY COMPILE_DEFINITIONS
        CONFIG_H="${CONTROLLER_SOURCE_DIR}/config.hpp"
    )
    set_property(
        SOURCE ${OPENGCC_SOURCE_DIR}/opengcc/joybus.cpp
        DIRECTORY ${CONTROLLER_SOURCE_DIR}
        PROPERTY COMPILE_DEFINITIONS
        CONFIG_H="${CONTROLLER_SOURCE_DIR}/config.hpp"
    )

    target_link_libraries(${CONTROLLER_TARGET} OpenGCC)
    pico_set_binary_type(${CONTROLLER_TARGET} copy_to_ram)
endfunction()
