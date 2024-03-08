# OpenGCC
This is a GameCube Controller firmware designed to be extensible to easily support any RP2040-based controller hardware.

## Supported Controllers
* [NobGCC](https://github.com/ZadenRB/NobGCC-HW)
* [PhobGCC 2](https://github.com/PhobGCC/PhobGCC-HW)

## Features
* Analog stick calibration
* Button remapping
* Trigger modes
* Dual settings profiles
* Notch calibration (Planned)
* Snapback filtering (Planned)

## Building the Firmware

1. Follow the "Install the Toolchain" instructions for your platform in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) (MacOS and Windows instructions are under "Building on other platforms")
2. Download the repository
3. `cd <repository directory>`
4. `git submodule update --init --recursive` to retrieve the [Pico SDK](https://github.com/raspberrypi/pico-sdk).
5. `mkdir build` to create the build directory
6. `cd build`
7. `cmake ..` to write build files. Use `cmake .. -DCMAKE_BUILD_TYPE=Debug` if adding features to create a debug build instead, and `cmake .. -DCMAKE_BUILD_TYPE=Release` to revert to a release build.
8. `make` to build all firmware targets. Compiled firmware will appear in `build/controllers` under each controller's subdirectory. To build a specific firmware, run `make <controller_name>` instead.
9. Plug in your controller in Mass Storage (Flash) Mode and drag the UF2 file onto the device.

## Documentation

Documentation is generated by running `doxygen` in the project directory.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md)
