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

#ifndef MAIN_H_
#define MAIN_H_

#include <array>

#include "board.hpp"

/** \file main.hpp
 * \brief Main processor functions
 */

/** \brief Process digital inputs
 *
 * \param physical_buttons Physical button states
 */
void read_digital(uint16_t physical_buttons);

/** \brief Map a physical button to its remapped value
 *
 * \param physical_buttons Physical button states
 * \param remapped_buttons Output button states
 * \param to_remap Which bit to remap
 * \param mapping Which bit to map to
 */
void remap(uint16_t physical_buttons, uint16_t& remapped_buttons,
           uint8_t to_remap, uint8_t mapping);

/** \brief Update digital trigger value based on trigger mode
 *
 * \param buttons Output button states
 * \param bit_to_set Which bit of the output to set
 * \param mode Current trigger mode
 * \param other_mode Current trigger mode of other trigger
 */
void apply_trigger_mode_digital(uint16_t& buttons, uint8_t bit_to_set,
                                trigger_mode mode, trigger_mode other_mode);

/** \brief Check current physical button states to see if a combo is being
 * pressed
 *
 * If a valid combo is pressed, starts a countdown to execute the combo.
 *
 * If a valid combo was pressed, checks if it still is. If it is not, cancels
 * the combo timer.
 *
 * \param physical_buttons Physical button states
 */
void check_combos(uint32_t physical_buttons);

/// \brief Executes the current combo
void execute_combo();

/// \brief Toggle safe mode on or off
void toggle_safe_mode();

/// \brief Main analog input loop, run on second core
void analog_main();

/** \brief Process analog trigger values
 *
 * \param lt_raw Raw value of left trigger
 * \param rt_raw Raw value of right trigger
 */
void read_triggers(uint8_t lt_raw, uint8_t rt_raw);

/**
 * \brief Update analog trigger value based on trigger mode
 *
 * \param out Where to ouput the updated analog trigger value
 * \param analog_value Current analog trigger value
 * \param threshold_value Configured threshold value
 * \param digital_value Digital value for this trigger
 * \param enable_analog Whether analog output is enabled
 * \param mode Current trigger mode
 * \param other_mode Current trigger mode of other trigger
 */
void apply_trigger_mode_analog(uint8_t& out, uint8_t analog_value,
                               uint8_t threshold_value, bool digital_value,
                               bool enable_analog, trigger_mode mode,
                               trigger_mode other_mode);

/**
 * \brief Read and process analog sticks
 *
 * \param ax_raw Location of PWM high / low data for A-stick X
 * \param ay_raw Location of PWM high / low data for A-stick Y
 * \param cx_raw Location of PWM high / low data for C-stick X
 * \param cy_raw Location of PWM high / low data for C-stick Y
 */
void read_sticks(const std::array<uint32_t, 2>& ax_raw,
                 const std::array<uint32_t, 2>& ay_raw,
                 const std::array<uint32_t, 2>& cx_raw,
                 const std::array<uint32_t, 2>& cy_raw);

#endif  // MAIN_H_