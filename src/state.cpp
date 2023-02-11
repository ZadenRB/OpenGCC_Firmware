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

#include "state.hpp"

#include "pico/multicore.h"

void controller_state::display_alert() {
    multicore_lockout_start_blocking();
    this->l_trigger = 255;
    this->r_trigger = 255;
    busy_wait_ms(1500);
    this->l_trigger = 0;
    this->r_trigger = 0;
    multicore_lockout_end_blocking();
}

void controller_state::toggle_safe_mode() {
    this->safe_mode = !this->safe_mode;
    this->display_alert();
}