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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <array>

#include "hardware/flash.h"

enum trigger_mode {
    both,
    digital_only,
    analog_only,
    trigger_plug,
    analog_on_digital,
    both_on_digital,
    analog_multiplied,
    last_trigger_mode = analog_multiplied,
};

constexpr uint8_t TRIGGER_THRESHOLD_MIN = 49;
constexpr uint8_t TRIGGER_THRESHOLD_MAX = 227;

struct controller_configuration {
    std::array<uint8_t, 13> mappings;
    trigger_mode l_trigger_mode;
    uint8_t l_trigger_threshold_value;
    trigger_mode r_trigger_mode;
    uint8_t r_trigger_threshold_value;

    static controller_configuration& get_instance();
    controller_configuration(controller_configuration const&) = delete;
    controller_configuration(controller_configuration&&) = delete;
    controller_configuration& operator=(const controller_configuration&) =
        delete;
    controller_configuration& operator=(controller_configuration&&) = delete;

    void persist();
    void swap_mappings();
    void configure_triggers();

   private:
    controller_configuration();

    static uint32_t read_page();
    static uint32_t write_page();
};

constexpr uint32_t CONFIG_FLASH_BASE =
    PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
constexpr uint32_t CONFIG_SRAM_BASE =
    XIP_NOCACHE_NOALLOC_BASE + CONFIG_FLASH_BASE;

constexpr uint32_t PAGES_PER_SECTOR = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;
constexpr uint32_t LAST_PAGE = PAGES_PER_SECTOR - 1;

constexpr uint32_t CONFIG_SIZE = sizeof(controller_configuration);

#endif  // _CONFIGURATION_H_