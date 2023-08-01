add_executable(NobGCC_rev1)

target_sources(NobGCC_rev1 PRIVATE
    rev1.hpp
    rev1.cpp
)

target_link_libraries(NobGCC_rev1
    hardware_i2c
)
