/*
    Copyright 2023 Zaden Ruggiero-Bouné

    This file is part of OpenGCC.

    OpenGCC is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    OpenGCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   OpenGCC If not, see http://www.gnu.org/licenses/.
*/

/** \file config.hpp
 * \brief Controller pinout constants
 */

#ifndef REV1_H_
#define REV1_H_

#include "hardware/i2c.h"
#include "pico/types.h"

#include <array>

/// \brief D-pad left pin
constexpr uint DPAD_LEFT_PIN = 0;

/// \brief D-pad right pin
constexpr uint DPAD_RIGHT_PIN = 1;

/// \brief D-pad down pin
constexpr uint DPAD_DOWN_PIN = 2;

/// \brief D-pad up pin
constexpr uint DPAD_UP_PIN = 3;

/// \brief Z button pin
constexpr uint Z_PIN = 4;

/// \brief Right trigger button pin
constexpr uint RT_DIGITAL_PIN = 5;

/// \brief Left trigger button pin
constexpr uint LT_DIGITAL_PIN = 6;

/// \brief A button pin
constexpr uint A_PIN = 8;

/// \brief B button pin
constexpr uint B_PIN = 9;

/// \brief X button pin
constexpr uint X_PIN = 10;

/// \brief Y button pin
constexpr uint Y_PIN = 11;

/// \brief Start button pin
constexpr uint START_PIN = 12;

/// \brief Right stick SDA pin
constexpr uint R_SDA_PIN = 14;

/// \brief Right stick SCL pin
constexpr uint R_SCL_PIN = 15;

/// \brief Left stick SDA pin
constexpr uint L_SDA_PIN = 24;

/// \brief Left stick SCL pin
constexpr uint L_SCL_PIN = 25;

/// \brief Left trigger slider pin
constexpr uint LT_ANALOG_PIN = 26;

/// \brief Right trigger slider pin
constexpr uint RT_ANALOG_PIN = 27;

/// \brief Left trigger slider ADC channel
constexpr uint LT_ANALOG_ADC_INPUT = LT_ANALOG_PIN - 26;

/// \brief Right trigger slider ADC channel
constexpr uint RT_ANALOG_ADC_INPUT = RT_ANALOG_PIN - 26;

/// \brief Mask on ADC channels to return only triggers
constexpr uint TRIGGER_ADC_MASK =
    (1 << LT_ANALOG_ADC_INPUT) | (1 << RT_ANALOG_ADC_INPUT);

/// \brief Mask on GPIO to return only digital inputs
constexpr uint16_t PHYSICAL_BUTTONS_MASK =
    (1 << DPAD_LEFT_PIN) | (1 << DPAD_RIGHT_PIN) | (1 << DPAD_DOWN_PIN) | (1 << DPAD_UP_PIN) |
    (1 << Z_PIN) | (1 << RT_DIGITAL_PIN) | (1 << LT_DIGITAL_PIN) | (1 << A_PIN) | (1 << B_PIN) |
    (1 << X_PIN) | (1 << Y_PIN) | (1 << START_PIN);

// I2C addresses of sensors for X & Y addresses
constexpr uint32_t X_I2C_ADDR = 0x32;
constexpr uint32_t Y_I2C_ADDR = 0x33;

// Constants to transfer 0/1 via DMA
constexpr uint32_t ZERO = 0x0;
constexpr uint32_t ONE = 0x1;

// Si7210 configuration data
constexpr std::array<uint8_t, 2> SI7210_AUTO_INCREMENT_CONFIG = {0xC5, 0x01};
constexpr std::array<uint8_t, 2> SI7210_IDLE_CONFIG = {0xC9, 0xFE};
constexpr std::array<uint8_t, 2> SI7210_IDLE_TIME_CONFIG = {0xC8, 0x00};
constexpr std::array<uint8_t, 2> SI7210_START_CONFIG = {0xC4, 0x00};
constexpr std::array<uint16_t, 3> SI7210_READ_DATA_COMMANDS = {
    I2C_IC_DATA_CMD_RESTART_BITS | 0xC1,
    I2C_IC_DATA_CMD_RESTART_BITS | I2C_IC_DATA_CMD_CMD_BITS,
    I2C_IC_DATA_CMD_STOP_BITS | I2C_IC_DATA_CMD_CMD_BITS
};

// DMA control block
struct control_block {
    const volatile void *read_address;
    volatile void *write_address;
    uint transfer_count;
    uint32_t control_register;
};

// Raw stick
struct raw_stick {
    uint16_t x;
    uint16_t y;
};

#endif // REV1_H_ 
