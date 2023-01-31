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

#ifndef _COMMON_H_
#define _COMMON_H_

#include "configuration.hpp"
#include "hardware/pio.h"
#include "pico/time.h"

/** \file common.hpp
 * \brief Defines global variables, structures, and constants
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

/// \brief C-Stick y-axis pin
constexpr uint CY = 13;

/// \brief C-Stick x-axis pin
constexpr uint CX = 14;

/// \brief Data line pin
constexpr uint DATA = 18;

/// \brief A-Stick y-axis pin
constexpr uint AY = 24;

/// \brief A-Stick x-axis pin
constexpr uint AX = 25;

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

/// \brief Mask on button state to return only jump buttons
constexpr uint16_t JUMP_MASK = (1 << X) | (1 << Y);

/// \brief Mask on ADC channels to return only triggers
constexpr uint TRIGGER_ADC_MASK =
    (1 << LT_ANALOG_ADC_INPUT) | (1 << RT_ANALOG_ADC_INPUT);

/// \brief Grouping of axes for a single analog stick
struct stick {
    uint8_t x;  /// X axis
    uint8_t y;  /// Y axis
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
    /// \briefhether the physical right trigger button is pressed
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

/** \brief Magic number for second core to signal first core to finish setup and
 * begin transmission with console
 */
constexpr uint32_t INTERCORE_SIGNAL = 0x623F16E4;

#endif  // _COMMON_H_