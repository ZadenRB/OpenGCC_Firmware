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

#include "configuration.hpp"

#include "board.hpp"
#include "calibration.hpp"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "read_pwm.pio.h"

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

        // Set coefficients to zero
        l_stick_coefficients.x_coefficients = {0, 0, 0, 0};
        l_stick_coefficients.y_coefficients = {0, 0, 0, 0};
        r_stick_coefficients.x_coefficients = {0, 0, 0, 0};
        r_stick_coefficients.y_coefficients = {0, 0, 0, 0};

        // Persist
        persist();
        return;
    }

    // Load stored configuration
    controller_configuration *config_in_flash =
        reinterpret_cast<controller_configuration *>(
            CONFIG_SRAM_BASE + (read_page * FLASH_PAGE_SIZE));
    for (size_t i = 0; i < profiles.size(); ++i) {
        profiles[i] = config_in_flash->profiles[i];
    }
    current_profile = config_in_flash->current_profile;
    l_stick_coefficients = config_in_flash->l_stick_coefficients;
    r_stick_coefficients = config_in_flash->r_stick_coefficients;
}

controller_configuration &controller_configuration::get_instance() {
    static controller_configuration instance;
    return instance;
}

void controller_configuration::reload_instance() {
    get_instance() = controller_configuration();
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

    // Get current configuration as bytes
    uint8_t *config_bytes = reinterpret_cast<uint8_t *>(this);
    std::array<uint8_t, FLASH_PAGE_SIZE> buf = {0};
    // Fill buffer with the configuration bytes and pad with 0xFF
    for (std::size_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
        if (i < CONFIG_SIZE) {
            buf[i] = config_bytes[i];
        } else {
            buf[i] = 0xFF;
        }
    }

    // Write to flash
    flash_range_program(CONFIG_FLASH_BASE + (w_page * FLASH_PAGE_SIZE),
                        buf.data(), FLASH_PAGE_SIZE);
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
    bool buttons_released = false;
    uint32_t first_button = 0;
    uint32_t second_button = 0;

    while (true) {
        // Get buttons
        uint16_t physical_buttons = get_buttons();

        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = physical_buttons == 0;
            if (buttons_released) {
                busy_wait_ms(DEBOUNCE_TIME);
            }
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
            buttons_released = false;

            // If the second button is the same as the first, let it be held for
            // 3 seconds to quit, otherwise discard it
            if (second_button == first_button) {
                absolute_time_t timeout_at = make_timeout_time_ms(3000);
                // Ensure button is held for 3 seconds
                while (absolute_time_diff_us(timeout_at, get_absolute_time()) <
                       0) {
                    physical_buttons = get_buttons();
                    if (physical_buttons != second_button) {
                        second_button = 0;
                        break;
                    }
                }

                // If second button was held, exit
                if (second_button != 0) {
                    state.display_alert();
                    return;
                }

                continue;
            }

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

    bool buttons_released = false;

    while (true) {
        // Get buttons
        uint16_t physical_buttons = get_buttons();
        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Quit if combo is pressed
        if (physical_buttons == ((1 << START) | (1 << X) | (1 << Z))) {
            persist();
            multicore_lockout_end_blocking();
            state.display_alert();
            return;
        }

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = (physical_buttons &
                                ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL))) == 0;
            if (buttons_released) {
                busy_wait_ms(DEBOUNCE_TIME);
            }
            continue;
        }

        // Mask out non-trigger buttons
        uint32_t trigger_pressed =
            physical_buttons & ((1 << LT_DIGITAL) | (1 << RT_DIGITAL));

        // If a combo might be pressed mark buttons as unreleased
        if (trigger_pressed != 0 &&
            (physical_buttons &
             ((1 << A) | (1 << B) | (1 << DPAD_UP) | (1 << DPAD_RIGHT) |
              (1 << DPAD_DOWN) | (1 << DPAD_LEFT))) != 0) {
            buttons_released = false;
        }

        if (trigger_pressed != 0 &&
            (trigger_pressed & (trigger_pressed - 1)) == 0) {
            // If only one trigger is pressed, set it as the one to modify
            trigger_mode *mode;
            uint8_t *threshold;
            if (trigger_pressed == (1 << LT_DIGITAL)) {
                mode = &(profiles[current_profile].l_trigger_mode);
                threshold =
                    &(profiles[current_profile].l_trigger_threshold_value);
            } else if (trigger_pressed == (1 << RT_DIGITAL)) {
                mode = &(profiles[current_profile].r_trigger_mode);
                threshold =
                    &(profiles[current_profile].r_trigger_threshold_value);
            }

            // Mask out trigger buttons
            uint32_t combo =
                physical_buttons & ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL));

            int new_threshold = *threshold;
            int new_mode = *mode;
            // Update mode/threshold based on combo
            switch (combo) {
                case (1 << A):
                    new_mode += 1U;
                    break;
                case (1 << B):
                    new_mode -= 1U;
                    break;
                case (1 << DPAD_UP):
                    new_threshold += 1U;
                    break;
                case (1 << DPAD_RIGHT):
                    new_threshold += 10U;
                    break;
                case (1 << DPAD_DOWN):
                    new_threshold -= 1U;
                    break;
                case (1 << DPAD_LEFT):
                    new_threshold -= 10U;
                    break;
            }

            // Wrap trigger mode
            if (new_mode < first_trigger_mode) {
                *mode = last_trigger_mode;
            } else if (new_mode > last_trigger_mode) {
                *mode = first_trigger_mode;
            } else {
                *mode = static_cast<trigger_mode>(new_mode);
            }

            // Wrap threshold
            if (new_threshold < TRIGGER_THRESHOLD_MIN) {
                *threshold = (TRIGGER_THRESHOLD_MAX + 1) -
                             (TRIGGER_THRESHOLD_MIN - new_threshold);
            } else if (new_threshold > TRIGGER_THRESHOLD_MAX) {
                *threshold = (TRIGGER_THRESHOLD_MIN - 1) +
                             (new_threshold - TRIGGER_THRESHOLD_MAX);
            } else {
                *threshold = static_cast<uint8_t>(new_threshold);
            }

            // Display mode on left trigger & offset on right trigger
            state.l_trigger = *mode;
            state.r_trigger = *threshold;
        } else {
            // Otherwise display nothing
            state.l_trigger = 0;
            state.r_trigger = 0;
        }
    }
}

void controller_configuration::calibrate_stick(
    stick_coefficients &to_calibrate, stick &display_stick,
    std::function<void(double &, double &, size_t)> get_stick) {
    // Lock core 1 to prevent stick output from being displayed
    multicore_lockout_start_blocking();

    stick_calibration calibration(display_stick);
    bool buttons_released = false;

    while (!calibration.done()) {
        // Show current step on display stick
        calibration.display_step();

        // Get buttons
        uint16_t physical_buttons = get_buttons();
        state.buttons =
            physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

        // Wait for buttons to be released if they haven't been
        if (!buttons_released) {
            buttons_released = physical_buttons == 0;
            if (buttons_released) {
                busy_wait_ms(DEBOUNCE_TIME);
            }
            continue;
        }

        // Wait for buttons to be released whenever a combo is pressed
        if ((physical_buttons & ((1 << B) | (1 << Z) | (1 << X))) != 0) {
            buttons_released = false;
        }

        // Handle combos
        switch (physical_buttons) {
            case (1 << B):
                calibration.undo_measurement();
                break;
            case (1 << Z): {
                double measured_x;
                double measured_y;
                get_stick(measured_x, measured_y, SAMPLE_DURATION);
                calibration.record_measurement(measured_x, measured_y);
                break;
            }
            case (1 << X):
                calibration.skip_measurement();
                break;
        }
    }

    to_calibrate = calibration.generate_coefficients();

    persist();
    multicore_lockout_end_blocking();
    state.display_alert();
}

void controller_configuration::factory_reset() {
    flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
    reload_instance();
    state = controller_state();
    state.display_alert();
}