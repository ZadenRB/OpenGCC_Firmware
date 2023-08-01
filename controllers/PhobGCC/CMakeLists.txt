add_executable(PhobGCC)

target_sources(PhobGCC PRIVATE
    PhobGCC.hpp
    PhobGCC.cpp
)

target_link_libraries(PhobGCC
    hardware_spi
)
