/*
    Copyright 2022 Zaden Ruggiero-Bouné

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

#include "configuration.hpp"
#include "pico/stdlib.h"

// Pin assignments
const int DPAD_LEFT = 0;
const int DPAD_RIGHT = 1;
const int DPAD_DOWN = 2;
const int DPAD_UP = 3;
const int Z = 4;
const int RT_DIGITAL = 5;
const int LT_DIGITAL = 6;
const int A = 8;
const int B = 9;
const int X = 10;
const int Y = 11;
const int START = 12;
const int CY = 13;
const int CX = 14;
const int DATA = 18;
const int AY = 24;
const int AX = 25;

// Origin bit
const int ALWAYS_HIGH = 7;
const int ORIGIN = 13;

const uint32_t PHYSICAL_BUTTONS_MASK =
    (1 << DPAD_LEFT) | (1 << DPAD_RIGHT) | (1 << DPAD_DOWN) | (1 << DPAD_UP) |
    (1 << Z) | (1 << RT_DIGITAL) | (1 << LT_DIGITAL) | (1 << A) | (1 << B) |
    (1 << X) | (1 << Y) | (1 << START);

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
    uint16_t buttons;
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

// Core 0
void handle_console_request();
void send_mode(uint8_t mode);
inline void read_digital(uint16_t physical_buttons);
inline void remap(uint16_t physical_buttons, uint16_t *remapped_buttons,
                  uint8_t to_remap, uint8_t mapping);
inline void check_combos(uint32_t physical_buttons);
inline int64_t execute_combo(alarm_id_t alarm_id, void *user_data);
void toggle_safe_mode();
void swap_mappings();

// Core 1
void analog_main();
inline void read_triggers();
inline void read_sticks(int ax_raw[], int ay_raw[], int cx_raw[], int cy_raw[]);