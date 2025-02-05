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
#include <tuple>

#include "configuration.hpp"
#include "controller.hpp"
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
 */
void apply_trigger_mode_digital(uint16_t& buttons, uint8_t bit_to_set,
                                trigger_mode mode);

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
 * \param configured_value Configured value
 * \param digital_value Digital value for this trigger
 * \param enable_analog Whether analog output is enabled
 * \param mode Current trigger mode
 *
 * \return New analog value after applying trigger mode
 */
uint8_t apply_trigger_mode_analog(uint8_t analog_value,
                                  uint8_t configured_value, bool digital_value,
                                  bool enable_analog, trigger_mode mode);

/// \brief Read analog sticks and update state
void read_sticks();

/** \brief Process raw stick data, running it through linearization and rempping
 * 
 * \param stick_data Raw stick data
 * \param previous_stick Last stick state
 * \param stick_coefficients Coefficients used to linearize stick
 * \param snapback_state Current state of snapback for the stick
 * \param range Maximum range around center
 * 
 * \return Stick data for use in state
 */
stick process_raw_stick(raw_stick stick_data, stick previous_stick,
                        stick_coefficients coefficients,
                        stick_snapback_state& snapback_state, uint8_t range);

/** \brief Linearize an axis using the given coefficients
 *
 * \param raw_axis Raw axis value to linearize
 * \param axis_coefficients Coefficients to use for axis linearization
 *
 * \return Linearized axis value
 */
double linearize_axis(uint16_t raw_axis,
                      std::array<double, NUM_COEFFICIENTS> coefficients);

/** \brief Remap linearized stick data for snapback and cardinals
 * 
 * \param linearized_x Linearized x-axis value
 * \param linearized_y Linearized y-axis value
 * \param snapback_state Current state of snapback for the stick
 * \param range Maximum range around center
 * 
 * \return Stick data for use in state
 */
stick remap_stick(double x, double y, stick_snapback_state& snapback_state,
                  uint8_t range);

/** \brief Filter a stick for snapback
 * 
 * \param linearized_x Linearized x-axis value
 * \param linearized_y Linearized y-axis value
 * \param snapback_state Current state of snapback for the stick
 * 
 * \return Stick data with snapback removed
 */
precise_stick unsnap_stick(double linearized_x, double linearized_y,
                           stick_snapback_state& snapback_state);

/** \brief Filter an axis for snapback
 * 
 * \param linearized_axis Linearized axis value
 * \param axis_displacement Displacement from center of the axis
 * \param axis_distance Distance from center of the axis
 * \param other_axis_distance Distance from center of the other axis
 * \param now Current timestamp
 * \param snapback_state Snapback state for this axis
 * 
 * \return
 */
double unsnap_axis(double linearized_axis, double axis_displacement,
                   double axis_distance, double other_axis_distance,
                   absolute_time_t now, axis_snapback_state& snapback_state);

#endif  // MAIN_H_
