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

#include "joybus_uf2_bootloader.hpp"

#include "console_communication.hpp"
#include "hardware/dma.h"
#include "pico/time.h"

uf2_block block;
uint32_t num_blocks;
uint32_t blocks_programmed;
bool erased[4096];
int dma_channel = -1;

void joybus_uf2_bootloader_init(PIO joybus_pio, uint rx_sm, uint sm, uint dma) {
    // IRQ
    irq_handler_t current_handler = irq_get_exclusive_handler(PIO0_IRQ_0);
    if (current_handler != NULL) {
        irq_remove_handler(PIO0_IRQ_0, current_handler);
    }
    irq_set_exclusive_handler(PIO0_IRQ_0, handle_joybus_uf2_block);

    // DMA
    if (dma_channel == -1) {
        dma_channel = dma_claim_unused_channel(true);
    } else {
        dma_channel_abort(dma_channel);
    }

    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_read_increment(&dma_config,
                                      false);  // Always read from same address
    channel_config_set_write_increment(&dma_config,
                                       true);  // Increment write address
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
    channel_config_set_ring(&dma_config, true, 9);  // Wrap after 512 bytes
    channel_config_set_dreq(&dma_config,
                            pio_get_dreq(joybus_pio, rx_sm, false));
    channel_config_set_chain_to(&dma_config, dma_channel);

    // Start DMA
    dma_channel_configure(dma_channel, &dma_config, &block,
                          &joybus_pio->rxf[rx_sm], 0xFFFFFFFF, true);

    // Reset variables
    num_blocks = 0;
    blocks_programmed = 0;
}

void joybus_uf2_bootloader_enter() {
    // TODO: Get rid of timeouts in this function
    while (num_blocks == 0 || blocks_programmed != num_blocks) {
    }

    // Reset once all blocks have been programmed
    sleep_ms(10);
    reset();
}

void handle_joybus_uf2_block() {
    // Reasons to fail (reset)
    // Block family name was incorrect, or payload size was not 256
    if (block.file_size != 0xE48BFF56 || block.payload_size != 256) {
        // Send error
        uint8_t buf[1] = {0x01};
        send_data(buf, 1);
        sleep_ms(10);
        reset();
    }

    // Block was outside of the valid address range
    if (block.target_addr < XIP_BASE ||
        block.target_addr >= NON_PROGRAMMABLE_ADDRESS) {
        // Send error
        uint8_t buf[1] = {0x02};
        send_data(buf, 1);
        sleep_ms(10);
        reset();
    }

    // Firmware being sent was too large
    if (block.num_blocks > MAX_NUM_BLOCKS) {
        // Send error
        uint8_t buf[1] = {0x03};
        send_data(buf, 1);
        sleep_ms(10);
        reset();
    }

    // Number of blocks changed
    if (block.num_blocks != num_blocks && num_blocks != 0) {
        // Send error
        uint8_t buf[1] = {0x04};
        send_data(buf, 1);
        sleep_ms(10);
        reset();
    }

    // Block contained the UF2_FLAG_NOT_MAIN_FLASH marking
    if (block.flags & 0x00000001 != 0) {
        // Send error
        uint8_t buf[1] = {0x05};
        send_data(buf, 1);
        sleep_ms(10);
        reset();
    }

    // Handle valid block
    int target_sector = (block.target_addr - XIP_BASE) / 4096;
    if (!erased[target_sector]) {
        // Erase the sector if it hasn't already been erased
        erased[target_sector] = true;
        flash_range_erase(target_sector * 4096, 4096);
    }

    bool should_program = false;
    for (int i = 0; i < 256; ++i) {
        if (block.data[i] !=
            *reinterpret_cast<uint8_t *>(block.target_addr - XIP_BASE +
                                         XIP_NOCACHE_NOALLOC_BASE + i)) {
            should_program = true;
            break;
        }
    }

    if (should_program) {
        // Program page if it hasn't already been programmed
        flash_range_program(block.target_addr - XIP_BASE, block.data,
                            block.payload_size);
        blocks_programmed++;
    }

    // Send acknowledgement
    uint8_t buf[1] = {0x00};
    send_data(buf, 1);
}

// Run the handler function for a combo
void reset() { AIRCR_Register = 0x5FA0004; }