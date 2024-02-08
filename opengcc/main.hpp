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

#ifndef MAIN_H_
#define MAIN_H_

#include <array>

#include "controller.hpp"
#include "configuration.hpp"
#include "state.hpp"

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
void remap(uint16_t& remapped_buttons, uint16_t physical_buttons,
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

/// \brief Main analog input loop, run on second core
void analog_main();

/// \brief Process analog trigger values
void read_triggers();

/** \brief Update analog trigger value based on trigger mode
 *
 * \param analog_value Current analog trigger value
 * \param threshold_value Configured threshold value
 * \param digital_value Digital value for this trigger
 * \param enable_analog Whether analog output is enabled
 * \param mode Current trigger mode
 * \param other_mode Current trigger mode of other trigger
 *
 * \return New analog value after applying trigger mode
 */
uint8_t apply_trigger_mode_analog(uint8_t analog_value, uint8_t threshold_value,
                                  bool digital_value, bool enable_analog,
                                  trigger_mode mode, trigger_mode other_mode);

/// \brief Read analog sticks and update state
void read_sticks();

/** \brief Process raw stick data, running it through linearization and rempping
 * 
 * \param x_raw Raw x-axis value
 * \param y_raw Raw y-axis value
 * \param stick_coefficients Coefficients used to linearize stick
 * \param range Maximum range around center
 * 
 * \return Stick data for use in state
 */
stick process_raw_stick(uint16_t x_raw, uint16_t y_raw, stick_coefficients coefficients, uint8_t range);

/** \brief Linearize an axis using the given coefficients
 *
 * \param axis_raw Raw axis value to linearize
 * \param axis_coefficients Coefficients to use for axis linearization
 *
 * \return Linearized axis value
 */
double linearize_axis(uint16_t axis_raw, std::array<double, NUM_COEFFICIENTS> coefficients);

/** \brief Remap linearized stick data for notches and cardinals
 * 
 * \param linearized_x Linearized x-axis value
 * \param linearized_y Linearized y-axis value
 * \param range Maximum range around center
 * 
 * \return Stick data for use in state
*/
stick remap_stick(double linearized_x, double linearized_y, uint8_t range);

#endif  // MAIN_H_
