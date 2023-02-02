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

/// \brief Grouping of axes for a single analog stick
struct stick {
    /// \brief X axis
    uint8_t x;
    /// \brief Y axis
    uint8_t y;
};

/// \brief Current controller state
struct controller_state {
    /// \brief tate of digital inputs
    uint16_t buttons = 0;
    /// \brief hether the physical left trigger button is pressed
    bool lt_pressed = false;
    /** \brief Whether the left trigger button has been remapped to a jump
     * button (X or Y)
     */
    bool lt_is_jump;
    /// \brief Whether the physical right trigger button is pressed
    bool rt_pressed = false;
    /** \brief Whether the right trigger button has been remapped to a jump
     * button (X or Y)
     */
    bool rt_is_jump;
    /// \brief tate of A-stick
    stick a_stick;
    /// \brief tate of C-stick
    stick c_stick;
    /// \brief eft trigger analog value
    uint8_t l_trigger;
    /// \brief ight trigger analog value
    uint8_t r_trigger;
    /// \brief true`if origin has not been set, `false` if it has
    bool origin = true;
    /// \brief true` if safe mode is active, `false` if it is not
    bool safe_mode = true;
    /// \brief tate of digital inputs of the in-progress combo
    uint16_t active_combo = 0;
    /// \brief larm ID for triggering a combo
    alarm_id_t combo_alarm;
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