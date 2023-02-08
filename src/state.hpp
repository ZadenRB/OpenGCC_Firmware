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
   NobGCC. If not, see http://www.gnu.org/licenses/.
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

/** \brief Number of samples to collect from sensors and average before updating
 * state
 */
constexpr uint SAMPLES_PER_READ = 1000;

/** \brief Threshold below which trigger will report 0 to prevent
 * trigger-tricking
 */
constexpr uint8_t TRIGGER_TRICK_THRESHOLD = 5;

/// \brief Grouping of axes for a single analog stick
struct stick {
    /// \brief X axis
    uint8_t x;
    /// \brief Y axis
    uint8_t y;
};

/// \brief Current controller state
struct controller_state {
    /// \brief State of digital inputs
    uint16_t buttons = 0;
    /// \brief Whether the physical left trigger button is pressed
    bool left_trigger_pressed = false;
    /** \brief Whether the left trigger button has been remapped to a jump
     * button (X or Y)
     */
    bool left_trigger_jump;
    /// \brief Whether the physical right trigger button is pressed
    bool right_trigger_pressed = false;
    /** \brief Whether the right trigger button has been remapped to a jump
     * button (X or Y)
     */
    bool right_trigger_jump;
    /// \brief State of A-stick
    stick a_stick;
    /// \brief State of C-stick
    stick c_stick;
    /// \brief Left trigger analog value
    uint8_t l_trigger;
    /// \brief Right trigger analog value
    uint8_t r_trigger;
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
 * \note Not thread-safe. Must ensure each field is only ever written
 * from one core or in critical sections.
 */
extern controller_state state;

/// \brief PIO being used for Joybus protocol
extern PIO joybus_pio;

/// \brief State machine being used for Joybus TX
extern uint tx_sm;

/// \brief State machine being used for Joybus RX
extern uint rx_sm;

/// \brief Offset into PIO instruction memory of the RX program
extern uint rx_offset;

/// \brief DMA channel being used for Joybus TX
extern uint tx_dma;

#endif  // STATE_H_