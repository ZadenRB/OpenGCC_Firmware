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

#include "console_communication.hpp"

#include "common.hpp"
#include "hardware/dma.h"
#include "joybus.pio.h"
#include "joybus_uf2_bootloader.hpp"
#include "pico/time.h"

uint8_t tx_buf[10];

// Process a request from the console
void handle_console_request() {
    pio_interrupt_clear(joybus_pio, RX_SYS_IRQ);

    uint8_t request[8];
    for (int i = 0; !pio_sm_is_rx_fifo_empty(joybus_pio, rx_sm) && i < 8; ++i) {
        request[i] = (uint8_t)pio_sm_get(joybus_pio, rx_sm);
    }

    switch (request[0]) {
        case 0xFF:
            // TODO: reset
        case 0x00: {
            // Device identifier
            tx_buf[0] = 0x09;
            tx_buf[1] = 0x00;
            tx_buf[2] = 0x03;
            send_data(3);
            return;
        }
        case 0x40:
            if (request[1] > 0x04) {
                request[1] = 0x00;
            }
            break;
        case 0x41:
            state.origin = 0;
            request[1] = 0x05;
            break;
        case 0x42:
            // TODO: calibrate
        case 0x43:
            request[1] = 0x05;
            break;
        case 0x44: {
            // Firmware version X.Y.Z {X, Y, Z}
            tx_buf[0] = 0x00;
            tx_buf[1] = 0x00;
            tx_buf[2] = 0x00;
            send_data(3);
            return;
        }
        case 0x45: {
            uint32_t requested_firmware_size = request[1] | (request[2] << 8) |
                                               (request[3] << 16) |
                                               (request[4] << 24);
            if (requested_firmware_size <=
                PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) {
                tx_buf[0] = 0x00;
                send_data(1);
                joybus_uf2_bootloader_enter();
            } else {
                tx_buf[0] = 0x01;
                send_data(1);
            }
            return;
        }
        default:
            // Continue reading if command was unknown
            pio_interrupt_clear(joybus_pio, RX_WAIT_IRQ);
            return;
    }

    send_mode(request[1]);
    return;
}

// Send buffer of data
void send_data(uint32_t length) {
    dma_channel_config tx_config = dma_get_channel_config(tx_dma);
    dma_channel_configure(tx_dma, &tx_config, &joybus_pio->txf[tx_sm], tx_buf,
                          length, true);
    pio_interrupt_clear(joybus_pio, TX_WAIT_IRQ);
}

// Send controller state in given mode
void send_mode(uint8_t mode) {
    uint32_t length;
    switch (mode) {
        case 0x00:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = state.c_stick.x;
            tx_buf[5] = state.c_stick.y;
            tx_buf[6] = (state.triggers.l & 0xF0) | (state.triggers.r >> 4);
            tx_buf[7] = 0x00;
            break;
        case 0x01:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = (state.c_stick.x & 0xF0) | (state.c_stick.y >> 4);
            tx_buf[5] = state.triggers.l;
            tx_buf[6] = state.triggers.r;
            tx_buf[7] = 0x00;
            break;
        case 0x02:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = (state.c_stick.x & 0xF0) | (state.c_stick.y >> 4);
            tx_buf[5] = (state.triggers.l & 0xF0) | (state.triggers.r >> 4);
            tx_buf[6] = 0x00;
            tx_buf[7] = 0x00;
            break;
        case 0x03:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = state.c_stick.x;
            tx_buf[5] = state.c_stick.y;
            tx_buf[6] = state.triggers.l;
            tx_buf[7] = state.triggers.r;
            break;
        case 0x04:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = state.c_stick.x;
            tx_buf[5] = state.c_stick.y;
            tx_buf[6] = 0x00;
            tx_buf[7] = 0x00;
        case 0x05:
            length = 10;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = state.c_stick.x;
            tx_buf[5] = state.c_stick.y;
            tx_buf[6] = state.triggers.l;
            tx_buf[7] = state.triggers.r;
            tx_buf[8] = 0x00;
            tx_buf[9] = 0x00;
            break;
    }
    send_data(length);
}