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

#include <array>

#include "hardware/flash.h"
#include "hardware/pio.h"

#define AIRCR_Register (*((volatile uint32_t *)(PPB_BASE + 0x0ED0C)))

constexpr uint32_t MAX_NUM_BLOCKS =
    (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) / FLASH_PAGE_SIZE;
constexpr uint32_t NON_PROGRAMMABLE_ADDRESS =
    XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

struct uf2_block {
    // 32 byte header
    uint32_t magic_start_0;
    uint32_t magic_start_1;
    uint32_t flags;
    uint32_t target_addr;
    uint32_t payload_size;
    uint32_t block_no;
    uint32_t num_blocks;
    uint32_t file_size;  // or familyID;
    std::array<uint8_t, 476> data;
    uint32_t magic_end;
};

void joybus_uf2_init(PIO joybus_pio, uint rx_sm);
void joybus_uf2_enter();
void handle_joybus_uf2_block();
void reset();