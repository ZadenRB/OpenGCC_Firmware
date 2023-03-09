/*
    Copyright 2023 Zaden Ruggiero-Bouné

    This file is part of NobGCC-SW.

    NobGCC-SW is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    NobGCC-SW is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   NobGCC-SW If not, see http://www.gnu.org/licenses/.
*/

#ifndef BOARD_H_
#define BOARD_H_

#include "configuration.hpp"

/** \file board.hpp
 * \brief Board-specific constants
 */

/// \brief D-pad left pin
constexpr uint DPAD_LEFT = 0;

/// \brief D-pad right pin
constexpr uint DPAD_RIGHT = 1;

/// \brief D-pad down pin
constexpr uint DPAD_DOWN = 2;

/// \brief D-pad up pin
constexpr uint DPAD_UP = 3;

/// \brief Z button pin
constexpr uint Z = 4;

/// \brief Right trigger button pin
constexpr uint RT_DIGITAL = 5;

/// \brief Left trigger button pin
constexpr uint LT_DIGITAL = 6;

/// \brief A button pin
constexpr uint A = 8;

/// \brief B button pin
constexpr uint B = 9;

/// \brief X button pin
constexpr uint X = 10;

/// \brief Y button pin
constexpr uint Y = 11;

/// \brief Start button pin
constexpr uint START = 12;

/// \brief Right stick y-axis pin
constexpr uint RY = 13;

/// \brief Right stick x-axis pin
constexpr uint RX = 14;

/// \brief Data line pin
constexpr uint DATA = 18;

/// \brief Left stick y-axis pin
constexpr uint LY = 24;

/// \brief Left stick x-axis pin
constexpr uint LX = 25;

/// \brief Left trigger slider pin
constexpr uint LT_ANALOG = 26;

/// \brief Right trigger slider pin
constexpr uint RT_ANALOG = 27;

/// \brief Left trigger slider ADC channel
constexpr uint LT_ANALOG_ADC_INPUT = LT_ANALOG - 26;

/// \brief Right trigger slider ADC channel
constexpr uint RT_ANALOG_ADC_INPUT = RT_ANALOG - 26;

/// \brief Bit which is always high in controller state
constexpr uint ALWAYS_HIGH = 7;

/// \brief Origin bit in controller state
constexpr uint ORIGIN = 13;

/// \brief Mask on GPIO to return only digital inputs
constexpr uint16_t PHYSICAL_BUTTONS_MASK =
    (1 << DPAD_LEFT) | (1 << DPAD_RIGHT) | (1 << DPAD_DOWN) | (1 << DPAD_UP) |
    (1 << Z) | (1 << RT_DIGITAL) | (1 << LT_DIGITAL) | (1 << A) | (1 << B) |
    (1 << X) | (1 << Y) | (1 << START);

/// \brief Mask on ADC channels to return only triggers
constexpr uint TRIGGER_ADC_MASK =
    (1 << LT_ANALOG_ADC_INPUT) | (1 << RT_ANALOG_ADC_INPUT);

/** \brief Magic number for second core to signal first core to finish setup and
 * begin transmission with console
 */
constexpr uint32_t INTERCORE_SIGNAL = 0x623F16E4;

/// \brief Initialize button reading functionality
void init_buttons();

/** \brief Get physical button states in the order they are sent to the console
 * 
 * \return Bitset of button states
 */
uint16_t get_buttons();

/// \brief Initialize stick reading functionality
void init_sticks();

/** \brief Get the value of both sticks
 *
 * \param lx_out Output for left stick x-axis
 * \param ly_out Output for left stick y-axis
 * \param rx_out Output for right stick x-axis
 * \param ry_out Output for right stick y-axis
 * \param sample_for_us Number of microseconds to sample for
 */
void get_sticks(double &lx_out, double &ly_out, double &rx_out, double &ry_out,
                uint sample_for_us);

/** \brief Get the value of the left stick
 *
 * \param x_out Output for left stick x-axis
 * \param y_out Output for left stick y-axis
 * \param sample_for_us Number of microseconds to sample for
 */
void get_left_stick(double &x_out, double &y_out, uint sample_for_us);

/** \brief Get the value of the right stick
 *
 * \param x_out Output for right stick x-axis
 * \param y_out Output for right stick y-axis
 * \param sample_for_us Number of microseconds to sample for
 */
void get_right_stick(double &x_out, double &y_out, uint sample_for_us);

/// \brief Initialize trigger reading functionality
void init_triggers();

/** \brief Get the raw value of the triggers
 *
 * \param l_out Output for left trigger
 * \param r_out Output for right trigger
 */
void get_triggers(uint8_t &l_out, uint8_t &r_out);

#endif  // BOARD_H_