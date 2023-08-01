function(add_controller CONTROLLER)
    add_subdirectory(${CONTROLLER})

    get_target_property(OPENGCC_SOURCE_DIR OpenGCC SOURCE_DIR)
    get_directory_property(CONTROLLER_SOURCE_DIR DIRECTORY ${CONTROLLER} SOURCE_DIR)
    set_property(
        SOURCE ${OPENGCC_SOURCE_DIR}/opengcc/main.cpp
        DIRECTORY ${CONTROLLER_SOURCE_DIR}
        PROPERTY COMPILE_DEFINITIONS
        CONFIG_H="${CONTROLLER_SOURCE_DIR}/config.hpp"
    )

    get_property(controller_target DIRECTORY ${CONTROLLER_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    target_link_libraries(${controller_target} OpenGCC)
endfunction()
