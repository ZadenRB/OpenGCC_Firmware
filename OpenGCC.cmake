function(init_controller CONTROLLER)
    add_executable(${CONTROLLER})

    target_sources(${CONTROLLER} PRIVATE
        controller.hpp
        controller.cpp
    )

    target_link_libraries(${CONTROLLER} OpenGCC)

    pico_add_extra_outputs(${CONTROLLER})
    pico_set_binary_type(${CONTROLLER} copy_to_ram)
endfunction()
