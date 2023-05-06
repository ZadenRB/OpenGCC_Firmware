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

#ifndef STATE_H_
#define STATE_H_

#include "hardware/pio.h"
#include "pico/time.h"

/** \file state.hpp
 * \brief Controller's volatile state
 *
 * Structures and global variables relevant to the controller state that do not
 * persist between power cycles.
 */

/// \brief Number of microseconds to sample sensors for before updating state
constexpr uint SAMPLE_DURATION = 1000;

/** \brief Threshold below which trigger will report 0 to prevent
 * trigger-tricking
 */
constexpr uint8_t TRIGGER_TRICK_THRESHOLD = 5;

/// \brief Grouping of axes for a single analog stick
struct stick {
    /// \brief x-axis
    uint8_t x;
    /// \brief Y axis
    uint8_t y;
};

/// \brief Grouping of analog sticks
struct sticks {
    /// \brief State of left stick
    stick l_stick;
    /// \brief State of right stick
    stick r_stick;
};

/// \brief Grouping of analog triggers
struct triggers {
    /// \brief state of left trigger
    uint8_t l_trigger;
    /// \brief state of right trigger
    uint8_t r_trigger;
};

/// \brief Current controller state
struct controller_state {
    /// \brief State of digital inputs
    uint16_t buttons = 0;
    /// \brief Whether left trigger digital is pressed (post remap)
    bool lt_pressed = false;
    /// \brief Whether right trigger digital is pressed (post remap)
    bool rt_pressed = false;
    /// \brief State of sticks
    sticks analog_sticks;
    /// \brief State of triggers (analog)
    triggers analog_triggers;
    /// \brief `true` if origin has not been set, `false` if it has
    bool origin = true;
    /// \brief `true` if safe mode is active, `false` if it is not
    bool safe_mode = true;
    /// \brief State of digital inputs of the in-progress combo
    uint16_t active_combo = 0;
    /// \brief Alarm ID for triggering a combo
    absolute_time_t combo_trigger_timestamp = nil_time;

    /// \brief Max out triggers for 1.5 seconds to indicate an alert
    void display_alert();

    /// \brief Toggle safe mode
    void toggle_safe_mode();
};

/** \brief Global state
 * \note Not inherently thread-safe, take care with any state shared between
 * cores.
 */
extern controller_state state;

#endif  // STATE_H_