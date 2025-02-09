/*
    Copyright 2023-2025 Zaden Ruggiero-Bouné

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

#ifndef PHOBGCC_H_
#define PHOBGCC_H_

#include "pico/types.h"

/// \brief D-pad left pin
constexpr uint DPAD_LEFT_PIN = 8;

/// \brief D-pad right pin
constexpr uint DPAD_RIGHT_PIN = 11;

/// \brief D-pad down pin
constexpr uint DPAD_DOWN_PIN = 10;

/// \brief D-pad up pin
constexpr uint DPAD_UP_PIN = 9;

/// \brief Z button pin
constexpr uint Z_PIN = 20;

/// \brief Right trigger button pin
constexpr uint RT_DIGITAL_PIN = 21;

/// \brief Left trigger button pin
constexpr uint LT_DIGITAL_PIN = 22;

/// \brief A button pin
constexpr uint A_PIN = 17;

/// \brief B button pin
constexpr uint B_PIN = 16;

/// \brief X button pin
constexpr uint X_PIN = 18;

/// \brief Y button pin
constexpr uint Y_PIN = 19;

/// \brief Start button pin
constexpr uint START_PIN = 5;

/// \brief Left trigger slider pin
constexpr uint LT_ANALOG_PIN = 27;

/// \brief Right trigger slider pin
constexpr uint RT_ANALOG_PIN = 26;

/// \brief Left trigger slider ADC channel
constexpr uint LT_ANALOG_ADC_INPUT = LT_ANALOG_PIN - 26;

/// \brief Right trigger slider ADC channel
constexpr uint RT_ANALOG_ADC_INPUT = RT_ANALOG_PIN - 26;

/// \brief SPI clock pin for analog sticks
constexpr uint SPI_CLK_PIN = 6;

/// \brief SPI TX pin for analog sticks
constexpr uint SPI_TX_PIN = 7;

/// \brief SPI RX pin for analog sticks
constexpr uint SPI_RX_PIN = 4;

/// \brief Left stick chip select pin
constexpr uint L_CS_PIN = 24;

/// \brief Right stick chip select pin
constexpr uint R_CS_PIN = 23;

/// \brief Mask on ADC channels to return only triggers
constexpr uint TRIGGER_ADC_MASK =
    (1 << LT_ANALOG_ADC_INPUT) | (1 << RT_ANALOG_ADC_INPUT);

#endif  // PHOBGCC_H_
