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

#ifndef JOYBUS_UF2_H_
#define JOYBUS_UF2_H_

#include <array>

#include "hardware/flash.h"
#include "hardware/pio.h"

#define AIRCR_Register (*((volatile uint32_t *)(PPB_BASE + 0x0ED0C)))

/** \file joybus_uf2.hpp
 * \brief Implementation of UF2 processing over Joybus protocol (EXPERIMENTAL)
 */

/// \brief Maximum possible firmware size
constexpr uint32_t MAX_NUM_BLOCKS =
    (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) / FLASH_PAGE_SIZE;

/// \brief First invalid program address
constexpr uint32_t NON_PROGRAMMABLE_ADDRESS =
    XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

/// \brief UF2 block
struct uf2_block {
    /// \brief Magic number 0
    uint32_t magic_start_0;
    /// \brief Magic number 1
    uint32_t magic_start_1;
    /// \brief Flag bits
    uint32_t flags;
    /// \brief Address to program this block to
    uint32_t target_addr;
    /// \brief Size of this block
    uint32_t payload_size;
    /// \brief Block number
    uint32_t block_no;
    /// \brief Total number of blocks
    uint32_t num_blocks;
    /// \brief Total file size (or family ID)
    uint32_t file_size;
    /// \brief Data of this block
    std::array<uint8_t, 476> data;
    /// \brief Ending magic number
    uint32_t magic_end;
};

/** \brief Setup DMA and variables for Joybus UF2
 * \param joybus_pio Joybus PIO state machine
 * \param rx_sm Joybus RX state machine
 */
void joybus_uf2_init(PIO joybus_pio, uint rx_sm);

/// \brief Enter UF2 mode waiting for blocks to be received
void joybus_uf2_enter();

/// \brief Process a UF2 block
void handle_joybus_uf2_block();

/// \brief Restart controller
void reset();

#endif  // JOYBUS_UF2_H_