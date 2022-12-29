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

#include "pico/stdlib.h"
#include "common.hpp"

/* Core 0 */
void read_digital(uint16_t physical_buttons);
void remap(uint16_t physical_buttons, uint16_t *remapped_buttons,
                  uint8_t to_remap, uint8_t mapping);
void apply_trigger_mode_digital(uint16_t *buttons, uint8_t bit_to_set, trigger_mode mode, trigger_mode other_mode);

// Combos
void check_combos(uint32_t physical_buttons);
int64_t execute_combo(alarm_id_t alarm_id, void *user_data);
void toggle_safe_mode();
void swap_mappings();

/* Core 1 */
void analog_main();
void read_triggers(uint8_t triggers_raw[]);
void apply_trigger_mode_analog(uint8_t *out, uint8_t analog_value, uint8_t threshold_value, bool digital_value, bool enable_analog, trigger_mode mode, trigger_mode other_mode);
void read_sticks(int ax_raw[], int ay_raw[], int cx_raw[], int cy_raw[]);