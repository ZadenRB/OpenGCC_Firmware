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

#include "configuration.hpp"

#include "board.hpp"
#include "calibration.hpp"
#include "hardware/gpio.h"
#include "pico/multicore.h"

controller_configuration::controller_configuration() {
    uint32_t read_page = controller_configuration::read_page();
    if (read_page == -1) {
        // If there's no stored configuration, load defaults & persist
        // Set up default profile
        configuration_profile default_profile;
        default_profile.mappings[0] = 0b0000;
        default_profile.mappings[1] = 0b0001;
        default_profile.mappings[2] = 0b0010;
        default_profile.mappings[3] = 0b0011;
        default_profile.mappings[4] = 0b0100;
        default_profile.mappings[5] = 0b0101;
        default_profile.mappings[6] = 0b0110;
        default_profile.mappings[7] = 0b0000;
        default_profile.mappings[8] = 0b1000;
        default_profile.mappings[9] = 0b1001;
        default_profile.mappings[10] = 0b1010;
        default_profile.mappings[11] = 0b1011;
        default_profile.mappings[12] = 0b1100;
        default_profile.l_trigger_mode = both;
        default_profile.l_trigger_threshold_value = TRIGGER_THRESHOLD_MIN;
        default_profile.r_trigger_mode = both;
        default_profile.r_trigger_threshold_value = TRIGGER_THRESHOLD_MIN;

        // Set all profiles to default
        for (size_t i = 0; i < profiles.size(); ++i) {
            profiles[i] = default_profile;
        }
        current_profile = 0;

        // TODO: add default coefficient values

        // Persist
        persist();
        return;
    }

    // Load stored configuration
    controller_configuration *config_in_flash =
        reinterpret_cast<controller_configuration *>(
            CONFIG_SRAM_BASE + (read_page * FLASH_PAGE_SIZE));
    profiles = config_in_flash->profiles;
    current_profile = config_in_flash->current_profile;
    // TODO: Load coefficients
}

controller_configuration &controller_configuration::get_instance() {
    static controller_configuration instance;
    return instance;
}

uint32_t controller_configuration::read_page() {
    for (uint32_t page = 0; page < PAGES_PER_SECTOR; ++page) {
        uint32_t read_address = CONFIG_SRAM_BASE + (page * FLASH_PAGE_SIZE);
        if (*reinterpret_cast<uint8_t *>(read_address) == 0xFF) {
            // Return last initialized flash (-1 if no flash is initialized)
            --page;
            return page;
        }
    }

    // If we never find uninitialized flash, return the last page
    return LAST_PAGE;
}

uint32_t controller_configuration::write_page() {
    return (controller_configuration::read_page() + 1) % PAGES_PER_SECTOR;
}

void controller_configuration::persist() {
    uint32_t w_page = write_page();
    if (w_page == 0 && read_page() != -1) {
        flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
    }

    uint8_t *config_bytes = reinterpret_cast<uint8_t *>(this);
    std::array<uint8_t, FLASH_PAGE_SIZE> buf = {0};
    for (std::size_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
        if (i < CONFIG_SIZE) {
            buf[i] = config_bytes[i];
        } else {
            buf[i] = 0xFF;
        }
    }

    flash_range_program(CONFIG_FLASH_BASE + (w_page * FLASH_PAGE_SIZE),
                        buf.data(), FLASH_PAGE_SIZE);
}

std::array<uint8_t, 13> controller_configuration::mappings() {
    return profiles[current_profile].mappings;
}

uint8_t controller_configuration::mapping(size_t index) {
    return profiles[current_profile].mappings[index];
}

trigger_mode controller_configuration::l_trigger_mode() {
    return profiles[current_profile].l_trigger_mode;
}

uint8_t controller_configuration::l_trigger_threshold_value() {
    return profiles[current_profile].l_trigger_threshold_value;
}

trigger_mode controller_configuration::r_trigger_mode() {
    return profiles[current_profile].r_trigger_mode;
}

uint8_t controller_configuration::r_trigger_threshold_value() {
    return profiles[current_profile].r_trigger_threshold_value;
}

void controller_configuration::select_profile(size_t profile) {
    current_profile = profile;
    persist();
}

void controller_configuration::swap_mappings() {
    // Initialize variables
    bool buttons_released = false;
    uint32_t first_button = 0;
    uint32_t second_button = 0;

    while (true) {
        // Get buttons
        uint32_t physical_buttons = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = physical_buttons == 0;
            continue;
        }

        if (first_button == 0 && physical_buttons != 0 &&
            (physical_buttons & (physical_buttons - 1)) == 0) {
            // Wait for first single button press
            first_button = physical_buttons;
            buttons_released = false;
        } else if (second_button == 0 && physical_buttons != 0 &&
                   (physical_buttons & (physical_buttons - 1)) == 0) {
            // Wait for second single button press
            second_button = physical_buttons;

            // Once both buttons are selected, swap
            // Get first button's mapping
            uint8_t first_mapping_index = 0;
            while ((first_button >> first_mapping_index) > 1) {
                ++first_mapping_index;
            }
            uint8_t first_mapping =
                profiles[current_profile].mappings[first_mapping_index];

            // Get second button's mapping
            uint8_t second_mapping_index = 0;
            while ((second_button >> second_mapping_index) > 1) {
                ++second_mapping_index;
            }
            uint8_t second_mapping =
                profiles[current_profile].mappings[second_mapping_index];

            // Swap mappings
            profiles[current_profile].mappings[first_mapping_index] =
                second_mapping;
            profiles[current_profile].mappings[second_mapping_index] =
                first_mapping;
            persist();
            state.display_alert();
            return;
        }
    }
}

void controller_configuration::configure_triggers() {
    // Lock core 1 to prevent analog trigger output from being displayed
    multicore_lockout_start_blocking();

    // Set triggers to 0 initially
    state.l_trigger = 0;
    state.r_trigger = 0;

    // Initialize variables
    uint32_t last_combo = 0;
    uint32_t same_combo_count = 0;
    bool buttons_released = false;

    while (true) {
        // Get buttons
        uint32_t physical_buttons = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = physical_buttons == 0;
            continue;
        }

        if (physical_buttons == ((1 << START) | (1 << X) | (1 << Z))) {
            persist();
            multicore_lockout_end_blocking();
            state.display_alert();
            return;
        }

        // Mask out non-trigger buttons
        uint32_t trigger_pressed =
            physical_buttons & ((1 << LT_DIGITAL) | (1 << RT_DIGITAL));
        if (trigger_pressed != 0 &&
            (trigger_pressed & (trigger_pressed - 1)) == 0) {
            // If only one trigger is pressed, set it to modify
            trigger_mode *mode;
            uint8_t *offset;
            if (trigger_pressed == (1 << LT_DIGITAL)) {
                mode = &(profiles[current_profile].l_trigger_mode);
                offset = &(profiles[current_profile].l_trigger_threshold_value);
            } else if (trigger_pressed == (1 << RT_DIGITAL)) {
                mode = &(profiles[current_profile].r_trigger_mode);
                offset = &(profiles[current_profile].r_trigger_threshold_value);
            }

            // Mask out trigger buttons
            uint32_t combo =
                physical_buttons & ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL));
            if (combo != 0 && combo == last_combo) {
                // If the same buttons are pressed, increment count
                ++same_combo_count;

                // Throttle for 500ms initially, then 200ms
                busy_wait_ms(same_combo_count > 1 ? 200 : 500);
            } else {
                // Otherwise reset count
                same_combo_count = 0;
            }

            // Set last combo to current combo
            last_combo = combo;

            uint new_offset = *offset;
            // Update mode/offset based on combo
            switch (combo) {
                case (1 << A):
                    *mode = static_cast<trigger_mode>((*mode + 1) %
                                                      (last_trigger_mode + 1));
                    break;
                case (1 << DPAD_UP):
                    new_offset += 1U;
                    break;
                case (1 << DPAD_RIGHT):
                    new_offset += 10U;
                    break;
                case (1 << DPAD_DOWN):
                    new_offset -= 1U;
                    break;
                case (1 << DPAD_LEFT):
                    new_offset -= 10U;
                    break;
                default:
                    // If no valid combo was pressed, reset combo counter
                    last_combo = 0;
                    same_combo_count = 0;
                    break;
            }

            if (new_offset < TRIGGER_THRESHOLD_MIN) {
                *offset = (TRIGGER_THRESHOLD_MAX + 1) -
                          (TRIGGER_THRESHOLD_MIN - new_offset);
            } else if (new_offset > TRIGGER_THRESHOLD_MAX) {
                *offset = (TRIGGER_THRESHOLD_MIN - 1) +
                          (new_offset - TRIGGER_THRESHOLD_MAX);
            } else {
                *offset = new_offset;
            }

            // Display mode on left trigger & offset on right trigger
            state.l_trigger = *mode;
            state.r_trigger = *offset;
        } else {
            // Otherwise reset last combo and counter, and display nothing
            state.l_trigger = 0;
            state.r_trigger = 0;
            last_combo = 0;
            same_combo_count = 0;
        }
    }
}

void controller_configuration::calibrate_stick(
    stick_coefficients &to_calibrate, stick &display_stick,
    const std::array<uint32_t, 2> &x_raw,
    const std::array<uint32_t, 2> &y_raw) {
    // Initialize variables
    stick_calibration calibration(display_stick);
    bool buttons_released = false;

    while (true) {
        // Show current step on display stick
        calibration.display_step();

        // Get buttons
        uint32_t physical_buttons = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = physical_buttons == 0;
            continue;
        }

        // If calibration is complete, generate coefficients and exit
        if (calibration.done()) {
            calibration.generate_coefficients(to_calibrate);
            state.display_alert();
            return;
        }

        // Wait for buttons to be released whenever some button is pressed
        if (physical_buttons != 0) {
            buttons_released = false;
        }

        switch (physical_buttons) {
            case (1 << B):
                calibration.undo_measurement();
                break;
            case (1 << A): {
                uint x_high_total = 0;
                uint x_low_total = 0;
                uint y_high_total = 0;
                uint y_low_total = 0;
                for (uint i = 0; i < SAMPLES_PER_READ; ++i) {
                    x_high_total += x_raw[0] + 3;
                    x_low_total += x_raw[1];
                    y_high_total += y_raw[0] + 3;
                    y_low_total += y_raw[1];
                }
                double measured_x =
                    x_high_total /
                    static_cast<double>(x_low_total + x_high_total);
                double measured_y =
                    y_high_total /
                    static_cast<double>(y_low_total + y_high_total);
                calibration.record_measurement(measured_x, measured_y);
                break;
            }
            case (1 << X):
                calibration.skip_measurement();
                break;
        }
    }
}

void controller_configuration::factory_reset() {
    flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
    state.display_alert();
    restart_controller();
}