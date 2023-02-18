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

#include "main.hpp"

#include "hardware/clocks.h"
#include "joybus.hpp"
#include "pico/multicore.h"
#include "pico/stdlib.h"

controller_state state;

int main() {
    // Initialize clocks
    clocks_init();

    // Configure system PLL to 128 MHZ
    set_sys_clock_pll(1536 * MHZ, 6, 2);

    // Launch analog on core 1
    multicore_launch_core1(analog_main);

    // Wait for core 1 push to the intercore FIFO, signaling core 0 to proceed
    //
    // We don't use the SDK's multicore_lockout_* functions for this because
    // it would be a race condition for core 1 to launch and interrupt core 0
    // before core 0 enables the RX IRQ
    uint32_t val = multicore_fifo_pop_blocking();
    while (val != INTERCORE_SIGNAL) {
        val = multicore_fifo_pop_blocking();
    }

    // Setup buttons to be read
    init_buttons();

    // Load configuration
    controller_configuration &config = controller_configuration::get_instance();

    // Now that we have a configuration, select a profile if combo is held
    uint16_t startup_buttons;
    get_buttons(startup_buttons);
    switch (startup_buttons) {
        case (1 << START) | (1 << DPAD_LEFT):
            config.select_profile(0);
            break;
        case (1 << START) | (1 << DPAD_RIGHT):
            config.select_profile(1);
            break;
    }

    // if (config.l_stick_coefficients.x_coefficients ==
    //         std::array<double, 4>{0, 0, 0, 0} ||
    //     config.l_stick_coefficients.y_coefficients ==
    //         std::array<double, 4>{0, 0, 0, 0}) {
    //     config.calibrate_stick(config.l_stick_coefficients, state.r_stick,
    //                            get_left_stick);
    // }
    // if (config.r_stick_coefficients.x_coefficients ==
    //         std::array<double, 4>{0, 0, 0, 0} ||
    //     config.r_stick_coefficients.y_coefficients ==
    //         std::array<double, 4>{0, 0, 0, 0}) {
    //     config.calibrate_stick(config.r_stick_coefficients, state.l_stick,
    //                            get_right_stick);
    // }

    uint16_t physical_buttons;
    get_buttons(physical_buttons);
    read_digital(physical_buttons);

    joybus_init(pio0, DATA);

    while (true) {
        get_buttons(physical_buttons);
        read_digital(physical_buttons);
        check_combos(physical_buttons);
    }

    return 0;
}

// Read buttons
void read_digital(uint16_t physical_buttons) {
    controller_configuration &config = controller_configuration::get_instance();

    // Output sent to console
    uint16_t remapped_buttons = (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // Apply remaps
    remap(physical_buttons, remapped_buttons, START, config.mapping(12));
    remap(physical_buttons, remapped_buttons, Y, config.mapping(11));
    remap(physical_buttons, remapped_buttons, X, config.mapping(10));
    remap(physical_buttons, remapped_buttons, B, config.mapping(9));
    remap(physical_buttons, remapped_buttons, A, config.mapping(8));
    remap(physical_buttons, remapped_buttons, LT_DIGITAL, config.mapping(6));
    remap(physical_buttons, remapped_buttons, RT_DIGITAL, config.mapping(5));
    remap(physical_buttons, remapped_buttons, Z, config.mapping(4));
    remap(physical_buttons, remapped_buttons, DPAD_UP, config.mapping(3));
    remap(physical_buttons, remapped_buttons, DPAD_DOWN, config.mapping(2));
    remap(physical_buttons, remapped_buttons, DPAD_RIGHT, config.mapping(1));
    remap(physical_buttons, remapped_buttons, DPAD_LEFT, config.mapping(0));

    // If triggers are pressed (post remapping)
    // Apply digital trigger modes
    apply_trigger_mode_digital(remapped_buttons, LT_DIGITAL,
                               config.l_trigger_mode(),
                               config.r_trigger_mode());
    apply_trigger_mode_digital(remapped_buttons, RT_DIGITAL,
                               config.r_trigger_mode(),
                               config.l_trigger_mode());

    // Update state
    state.buttons = remapped_buttons;
}

// Update remapped_buttons based on a physical button and its mapping
void remap(uint16_t physical_buttons, uint16_t &remapped_buttons,
           uint8_t to_remap, uint8_t mapping) {
    uint16_t mask = ~(1 << mapping);
    bool pressed = (physical_buttons & (1 << to_remap)) != 0;
    remapped_buttons = (remapped_buttons & mask) | (pressed << mapping);
}

// Modify digital value based on the trigger mode
void apply_trigger_mode_digital(uint16_t &buttons, uint8_t bit_to_set,
                                trigger_mode mode, trigger_mode other_mode) {
    switch (mode) {
        case both:
        case trigger_plug:
        case analog_multiplied:
            if (other_mode == analog_on_digital) {
                buttons = buttons & ~(1 << bit_to_set);
            }
            break;
        case analog_only:
        case analog_on_digital:
            buttons = buttons & ~(1 << bit_to_set);
            break;
    }
}

// Check if any button combos are being pressed
void check_combos(uint32_t physical_buttons) {
    // Check if the active combo is still being pressed
    if (state.active_combo != 0) {
        if (state.active_combo == physical_buttons) {
            if (absolute_time_diff_us(state.combo_trigger_timestamp,
                                      get_absolute_time()) > 0) {
                execute_combo();
            }
            return;
        } else {
            state.combo_trigger_timestamp = nil_time;
            state.active_combo = 0;
        }
    }

    // Check if the combo is to toggle safe mode
    if (physical_buttons == ((1 << START) | (1 << Y) | (1 << A) | (1 << Z))) {
        state.active_combo = physical_buttons;
        state.combo_trigger_timestamp = make_timeout_time_ms(3000);
        return;
    }

    // If not in safe mode, check other combos
    if (!state.safe_mode) {
        switch (physical_buttons) {
            case (1 << START) | (1 << X) | (1 << A):
            case (1 << START) | (1 << X) | (1 << Z):
            case (1 << START) | (1 << X) | (1 << LT_DIGITAL):
            case (1 << START) | (1 << X) | (1 << RT_DIGITAL):
            case (1 << START) | (1 << Y) | (1 << Z):
                state.active_combo = physical_buttons;
                state.combo_trigger_timestamp = make_timeout_time_ms(3000);
                break;
        }
    }
}

// Run the handler function for a combo
void execute_combo() {
    controller_configuration &config = controller_configuration::get_instance();

    state.display_alert();

    switch (state.active_combo) {
        case (1 << START) | (1 << Y) | (1 << A) | (1 << Z):
            state.toggle_safe_mode();
            break;
        case (1 << START) | (1 << X) | (1 << A):
            config.swap_mappings();
            break;
        case (1 << START) | (1 << X) | (1 << Z):
            config.configure_triggers();
            break;
        case (1 << START) | (1 << X) | (1 << LT_DIGITAL):
            config.calibrate_stick(config.l_stick_coefficients, state.r_stick,
                                   get_left_stick);
            break;
        case (1 << START) | (1 << X) | (1 << RT_DIGITAL):
            config.calibrate_stick(config.r_stick_coefficients, state.l_stick,
                                   get_right_stick);
            break;
        case (1 << START) | (1 << Y) | (1 << Z):
            controller_configuration::factory_reset();
            break;
    }

    state.active_combo = 0;
    state.combo_trigger_timestamp = nil_time;
}

void analog_main() {
    // Setup sticks and triggers to be read
    init_sticks();
    init_triggers();

    // Allow core 0 to continue (start responding to console polls) after
    // reading analog inputs once
    read_triggers();
    read_sticks();
    multicore_fifo_push_blocking(INTERCORE_SIGNAL);

    // Enable lockout
    multicore_lockout_victim_init();

    while (true) {
        read_triggers();
        read_sticks();
    }
}

// Read analog triggers & apply analog trigger modes
void read_triggers() {
    controller_configuration &config = controller_configuration::get_instance();

    uint8_t lt;
    uint8_t rt;

    get_triggers(lt, rt);

    apply_trigger_mode_analog(state.l_trigger, lt,
                              config.l_trigger_threshold_value(),
                              state.buttons & (1 << LT_DIGITAL) != 0,
                              config.mapping(LT_DIGITAL) == LT_DIGITAL,
                              config.l_trigger_mode(), config.r_trigger_mode());
    apply_trigger_mode_analog(state.r_trigger, rt,
                              config.r_trigger_threshold_value(),
                              state.buttons & (1 << RT_DIGITAL) != 0,
                              config.mapping(RT_DIGITAL) == RT_DIGITAL,
                              config.r_trigger_mode(), config.l_trigger_mode());
}

// Modify analog value based on the trigger mode
void apply_trigger_mode_analog(uint8_t &out, uint8_t analog_value,
                               uint8_t threshold_value, bool digital_value,
                               bool enable_analog, trigger_mode mode,
                               trigger_mode other_mode) {
    // Prevent accidental trigger-tricking
    if (analog_value <= TRIGGER_TRICK_THRESHOLD) {
        analog_value = 0;
    }

    switch (mode) {
        case digital_only:
            out = 0;
            break;
        case both:
        case analog_only:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else {
                out = analog_value * enable_analog;
            }
            break;
        case trigger_plug:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else if (analog_value > threshold_value) {
                out = threshold_value * enable_analog;
            } else {
                out = analog_value * enable_analog;
            }
            break;
        case analog_on_digital:
        case both_on_digital:
            out = threshold_value * digital_value * enable_analog;
            break;
        case analog_multiplied:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else {
                float multiplier = (threshold_value * TRIGGER_MULTIPLIER_M) +
                                   TRIGGER_MULTIPLIER_B;
                float multiplied_value = analog_value * multiplier;
                if (multiplied_value > 255) {
                    out = 255 * enable_analog;
                } else {
                    out =
                        static_cast<uint8_t>(multiplied_value) * enable_analog;
                }
            }
            break;
    }
}

void read_sticks() {
    double lx;
    double ly;
    double rx;
    double ry;

    get_sticks(lx, ly, rx, ry, SAMPLES_PER_READ);
}
