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

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <stdint.h>

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

/// \brief Initialize stick reading functionality
void init_sticks();

/** \brief Get the value of both sticks
 *
 * \param lx_out Output for left stick x-axis
 * \param ly_out Output for left stick y-axis
 * \param rx_out Output for right stick x-axis
 * \param ry_out Output for right stick y-axis
 */
void get_sticks(uint16_t &lx_out, uint16_t &ly_out, uint16_t &rx_out, uint16_t &ry_out);

/** \brief Get the value of the left stick
 *
 * \param x_out Output for left stick x-axis
 * \param y_out Output for left stick y-axis
 */
void get_left_stick(uint16_t &x_out, uint16_t &y_out);

/** \brief Get the value of the right stick
 *
 * \param x_out Output for right stick x-axis
 * \param y_out Output for right stick y-axis
 */
void get_right_stick(uint16_t &x_out, uint16_t &y_out);

/// \brief Initialize trigger reading functionality
void init_triggers();

/** \brief Get the raw value of the triggers
 *
 * \param l_out Output for left trigger
 * \param r_out Output for right trigger
 */
void get_triggers(uint8_t &l_out, uint8_t &r_out);

#endif  // CONTROLLER_H_
