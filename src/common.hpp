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

#include <stdint.h>

#include "configuration.hpp"
#include "hardware/pio.h"
#include "pico/time.h"

// Pin assignments
constexpr int DPAD_LEFT = 0;
constexpr int DPAD_RIGHT = 1;
constexpr int DPAD_DOWN = 2;
constexpr int DPAD_UP = 3;
constexpr int Z = 4;
constexpr int RT_DIGITAL = 5;
constexpr int LT_DIGITAL = 6;
constexpr int A = 8;
constexpr int B = 9;
constexpr int X = 10;
constexpr int Y = 11;
constexpr int START = 12;
constexpr int CY = 13;
constexpr int CX = 14;
constexpr int DATA = 18;
constexpr int AY = 24;
constexpr int AX = 25;
constexpr int LT_ANALOG = 26;
constexpr int RT_ANALOG = 27;
constexpr int LT_ANALOG_ADC_INPUT = LT_ANALOG - 26;
constexpr int RT_ANALOG_ADC_INPUT = RT_ANALOG - 26;
// Origin bit
constexpr int ALWAYS_HIGH = 7;
constexpr int ORIGIN = 13;

constexpr uint16_t PHYSICAL_BUTTONS_MASK =
    (1 << DPAD_LEFT) | (1 << DPAD_RIGHT) | (1 << DPAD_DOWN) | (1 << DPAD_UP) |
    (1 << Z) | (1 << RT_DIGITAL) | (1 << LT_DIGITAL) | (1 << A) | (1 << B) |
    (1 << X) | (1 << Y) | (1 << START);
constexpr uint16_t JUMP_MASK = (1 << X) | (1 << Y);
constexpr uint TRIGGER_ADC_MASK =
    (1 << LT_ANALOG_ADC_INPUT) | (1 << RT_ANALOG_ADC_INPUT);

struct stick {
    uint8_t x;
    uint8_t y;
};

struct trigger {
    uint8_t r;
    uint8_t l;
};

// State - in memory
struct controller_state {
    uint16_t buttons = 0;
    bool lt_pressed = false;
    bool lt_is_jump;
    bool rt_pressed = false;
    bool rt_is_jump;
    stick a_stick;
    stick c_stick;
    trigger triggers;
    bool origin = 1;
    bool safe_mode = 1;
    uint16_t active_combo = 0;
    alarm_id_t combo_alarm;
    controller_config config;
};

struct raw_stick {
    uint16_t x;
    uint16_t y;
};

struct raw_trigger {
    uint16_t lt;
    uint16_t rt;
};

extern controller_state state;
extern PIO joybus_pio;
extern uint tx_sm;
extern uint rx_sm;
extern uint rx_offset;
extern uint tx_dma;

constexpr int INTERCORE_SIGNAL = 0x623F16E4;