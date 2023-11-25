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

/// \brief D-pad left bit in controller state
constexpr uint DPAD_LEFT = 0;

/// \brief D-pad right bit in controller state
constexpr uint DPAD_RIGHT = 1;

/// \brief D-pad down bit in controller state
constexpr uint DPAD_DOWN = 2;

/// \brief D-pad up bit in controller state
constexpr uint DPAD_UP = 3;

/// \brief Z button bit in controller state
constexpr uint Z = 4;

/// \brief Right trigger button bit in controller state
constexpr uint RT_DIGITAL = 5;

/// \brief Left trigger button bit in controller state
constexpr uint LT_DIGITAL = 6;

/// \brief A button bit in controller state
constexpr uint A = 8;

/// \brief B button bit in controller state
constexpr uint B = 9;

/// \brief X button bit in controller state
constexpr uint X = 10;

/// \brief Y button bit in controller state
constexpr uint Y = 11;

/// \brief Start button bit in controller state
constexpr uint START = 12;

/// \brief Always high bit in controller state
constexpr uint ALWAYS_HIGH = 7;

/// \brief Origin bit in controller state
constexpr uint ORIGIN = 13;

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
