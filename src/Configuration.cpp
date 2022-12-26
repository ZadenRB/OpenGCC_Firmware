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

uint32_t config_read_page() {
    for (int page = 0; page < PAGES_PER_SECTOR; ++page) {
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

uint32_t config_write_page() {
    return (config_read_page() + 1) % PAGES_PER_SECTOR;
}

controller_config load_config() {
    uint32_t read_page = config_read_page();
    if (read_page == -1) {
        // If there's no stored configuration, load defaults & persist
        controller_config config;
        config.mappings[0] = 0b0000;
        config.mappings[1] = 0b0001;
        config.mappings[2] = 0b0010;
        config.mappings[3] = 0b0011;
        config.mappings[4] = 0b0100;
        config.mappings[5] = 0b0101;
        config.mappings[6] = 0b0110;
        config.mappings[7] = 0b0000;
        config.mappings[8] = 0b1000;
        config.mappings[9] = 0b1001;
        config.mappings[10] = 0b1010;
        config.mappings[11] = 0b1011;
        config.mappings[12] = 0b1100;
        persist_config(&config);
        return config;
    }

    // Load stored configuration
    return *reinterpret_cast<controller_config *>(
        CONFIG_SRAM_BASE + (read_page * FLASH_PAGE_SIZE));
}

void persist_config(controller_config *config) {
    uint32_t write_page = config_write_page();
    if (write_page == 0 && config_read_page() != -1) {
        flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
    }

    uint8_t *config_bytes = reinterpret_cast<uint8_t *>(config);
    uint8_t buf[FLASH_PAGE_SIZE] = {0};
    for (int i = 0; i < FLASH_PAGE_SIZE; ++i) {
        if (i < CONFIG_SIZE) {
            buf[i] = config_bytes[i];
        } else {
            buf[i] = 0xFF;
        }
    }

    flash_range_program(CONFIG_FLASH_BASE + (write_page * FLASH_PAGE_SIZE), buf,
                        FLASH_PAGE_SIZE);
}