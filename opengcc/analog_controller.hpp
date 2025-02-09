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

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <pico/types.h>

/** \file controller.hpp
 * \brief Functionality that is varies in implementation between controllers
 */

/// \brief Initialize button reading functionality
void init_buttons();

/** \brief Get physical button states in the order they are sent to the console
 * 
 * \return Bitset of button states
 */
uint16_t get_buttons();

/// \brief Grouping of axes for a single analog stick's raw values
struct raw_stick {
  /// \brief X-axis
  uint16_t x;
  /// \brief X-axis
  uint16_t y;
  /// \brief `true` if this data hasn't been read yet, `false` otherwise
  bool fresh;
};

/// \brief Grouping of analog sticks raw values
struct raw_sticks {
  /// \brief Left stick
  raw_stick l_stick;
  /// \brief Right stick
  raw_stick r_stick;
};

/// \brief Initialize stick reading functionality
void init_sticks();

/** \brief Get the value of both sticks
 *
 * \return Stick values
 */
raw_sticks get_sticks();

/** \brief Get the value of the left stick
 *
 * \return Left stick axis values
 */
raw_stick get_left_stick();

/** \brief Get the value of the right stick
 *
 * \return Right stick axis values
 */
raw_stick get_right_stick();

/// \brief Grouping of trigger values
struct raw_triggers {
  /// \brief Left trigger
  uint8_t l;
  /// \brief Right trigger
  uint8_t r;
};

/// \brief Initialize trigger reading functionality
void init_triggers();

/** \brief Get the raw value of the triggers
 *
 * \return Trigger values
 */
raw_triggers get_triggers();

#endif  // CONTROLLER_H_
