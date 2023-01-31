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

#include "common.hpp"
#include "hardware/gpio.h"
#include "pico/multicore.h"

controller_configuration::controller_configuration() {
    uint32_t read_page = controller_configuration::read_page();
    if (read_page == -1) {
        // If there's no stored configuration, load defaults & persist
        this->mappings[0] = 0b0000;
        this->mappings[1] = 0b0001;
        this->mappings[2] = 0b0010;
        this->mappings[3] = 0b0011;
        this->mappings[4] = 0b0100;
        this->mappings[5] = 0b0101;
        this->mappings[6] = 0b0110;
        this->mappings[7] = 0b0000;
        this->mappings[8] = 0b1000;
        this->mappings[9] = 0b1001;
        this->mappings[10] = 0b1010;
        this->mappings[11] = 0b1011;
        this->mappings[12] = 0b1100;
        this->l_trigger_mode = both;
        this->l_trigger_threshold_value = TRIGGER_THRESHOLD_MIN;
        this->r_trigger_mode = both;
        this->r_trigger_threshold_value = TRIGGER_THRESHOLD_MIN;
        this->persist();
    }

    // Load stored configuration
    *this = *reinterpret_cast<controller_configuration *>(
        CONFIG_SRAM_BASE + (read_page * FLASH_PAGE_SIZE));
}

controller_configuration &controller_configuration::get_instance() {
    static controller_configuration instance;
    return instance;
}

void controller_configuration::persist() {
    uint32_t write_page = this->write_page();
    if (write_page == 0 && this->read_page() != -1) {
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

    flash_range_program(CONFIG_FLASH_BASE + (write_page * FLASH_PAGE_SIZE),
                        buf.data(), FLASH_PAGE_SIZE);
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

void controller_configuration::swap_mappings() {
    // Wait for all buttons to be released
    while ((~gpio_get_all() & PHYSICAL_BUTTONS_MASK) != 0) {
        tight_loop_contents();
    }

    // Wait for first button press
    uint32_t first_button = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
    while (!(first_button != 0 && (first_button & (first_button - 1)) == 0)) {
        first_button = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
    }

    // Wait for all buttons to be released
    while ((~gpio_get_all() & PHYSICAL_BUTTONS_MASK) != 0) {
        tight_loop_contents();
    }

    // Wait for second button press
    uint32_t second_button = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
    while (!(second_button != 0 && (second_button & (second_button - 1)) == 0 &&
             second_button != first_button)) {
        second_button = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
    }

    // Get first button's mapping
    uint8_t first_mapping_index = 0;
    while ((first_button >> first_mapping_index) > 1) {
        ++first_mapping_index;
    }
    uint8_t first_mapping = this->mappings[first_mapping_index];

    // Get second button's mapping
    uint8_t second_mapping_index = 0;
    while ((second_button >> second_mapping_index) > 1) {
        ++second_mapping_index;
    }
    uint8_t second_mapping = this->mappings[second_mapping_index];

    this->mappings[first_mapping_index] = second_mapping;
    this->mappings[second_mapping_index] = first_mapping;
    this->persist();
}

void controller_configuration::configure_triggers() {
    // Lock core 1 to prevent analog trigger output from being displayed
    multicore_lockout_start_blocking();

    // Initialize variables
    uint32_t last_combo = 0;
    uint32_t same_combo_count = 0;

    while (true) {
        // Get buttons
        uint32_t physical_buttons = ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;

        // If exit combo is pressed, unlock core 1, save changes, and exit
        if (physical_buttons == (1 << START) | (1 << B) | (1 << Z)) {
            multicore_lockout_end_blocking();
            this->persist();
            return;
        }

        // Mask out non-trigger buttons
        uint32_t trigger_pressed =
            physical_buttons & ((1 << LT_DIGITAL) | (1 << RT_DIGITAL));
        if (trigger_pressed != 0 &&
            (trigger_pressed & (trigger_pressed - 1)) == 0) {
            // If one (and only one) trigger is pressed, set it's mode & offset
            // as the values to change
            trigger_mode *mode;
            uint8_t *offset;
            if (trigger_pressed == (1 << LT_DIGITAL)) {
                mode = &this->l_trigger_mode;
                offset = &this->l_trigger_threshold_value;
            } else if (trigger_pressed == (1 << RT_DIGITAL)) {
                mode = &this->r_trigger_mode;
                offset = &this->r_trigger_threshold_value;
            }

            // Mask out trigger buttons
            uint32_t combo =
                physical_buttons & ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL));
            if (combo != 0 && combo == last_combo) {
                // If the same buttons are pressed, increment count
                ++same_combo_count;

                // Throttle for 500ms initially, then 200ms
                sleep_ms(same_combo_count > 1 ? 200 : 500);
            } else {
                // Otherwise reset count
                same_combo_count = 0;
            }

            // Set last combo to current combo
            last_combo = combo;

            // Update mode/offset based on combo
            switch (combo) {
                case (1 << A):
                    *mode = static_cast<trigger_mode>((*mode + 1) %
                                                      (last_trigger_mode + 1));
                    break;
                case (1 << DPAD_UP):
                    *offset = *offset + 1U;
                    break;
                case (1 << DPAD_RIGHT):
                    *offset = *offset + 10U;
                    break;
                case (1 << DPAD_DOWN):
                    *offset = *offset - 1U;
                    break;
                case (1 << DPAD_LEFT):
                    *offset = *offset - 10U;
                    break;
                default:
                    // If no valid combo was pressed, reset last combo and
                    // counter to avoid throttling
                    last_combo = 0;
                    same_combo_count = 0;
                    break;
            }

            // Display mode on left trigger & offset on right trigger
            state.triggers.l = *mode;
            state.triggers.r = *offset;
        } else {
            // Otherwise reset last combo and counter, and display nothing
            state.triggers.l = 0;
            state.triggers.r = 0;
            last_combo = 0;
            same_combo_count = 0;
        }
    }
}