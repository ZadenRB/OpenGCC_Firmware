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

#include "joybus.hpp"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "joybus.pio.h"
#include "joybus_uf2.hpp"
#include "state.hpp"

std::array<uint8_t, 10> tx_buf;

// Process a request from the console
void handle_console_request() {
    uint32_t cmd = pio_sm_get(joybus_pio, rx_sm);

    int request_len;

    switch (cmd) {
        case 0x40:
        case 0x42:
        case 0x43:
            request_len = 2;
            break;
        case 0x45:
            request_len = 4;
            break;
        default:
            request_len = 0;
            break;
    }

    std::array<uint8_t, 4> request;
    for (std::size_t i = 0; i < request_len; ++i) {
        // Wait a max of 48us for a new item to be pushed into the RX FIFO,
        // otherwise assume we caught the middle of a command & return
        absolute_time_t timeout_at = make_timeout_time_us(48);
        while (pio_sm_is_rx_fifo_empty(joybus_pio, rx_sm)) {
            if (absolute_time_diff_us(timeout_at, get_absolute_time()) > 0) {
                // Clear ISR (mov isr, null)
                pio_sm_exec(joybus_pio, rx_sm,
                            pio_encode_mov(pio_isr, pio_null));
                return;
            }
        }
        request[i] = pio_sm_get(joybus_pio, rx_sm);
    }

    // Move to code to process stop bit
    pio_sm_exec(joybus_pio, rx_sm,
                pio_encode_jmp(rx_offset + joybus_rx_offset_read_stop_bit));

    switch (cmd) {
        case 0xFF:
        case 0x00: {
            // Device identifier
            tx_buf[0] = 0x09;
            tx_buf[1] = 0x00;
            tx_buf[2] = 0x03;
            send_data(3);
            return;
        }
        case 0x40:
            if (request[0] > 0x04) {
                request[0] = 0x00;
            }
            break;
        case 0x41:
            state.origin = 0;
            request[0] = 0x05;
            break;
        case 0x42:
        case 0x43:
            request[0] = 0x05;
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
            uint32_t requested_firmware_size = request[0] | (request[1] << 8) |
                                               (request[2] << 16) |
                                               (request[3] << 24);
            if (requested_firmware_size <=
                PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) {
                tx_buf[0] = 0x00;
                send_data(1);
                joybus_uf2_enter();
            } else {
                tx_buf[0] = 0x01;
                send_data(1);
            }
            return;
        }
        default:
            // Continue reading if command was unknown
            return;
    }

    send_mode(request[0]);
    return;
}

// Send buffer of data
void send_data(uint32_t length) {
    dma_channel_config tx_config = dma_get_channel_config(tx_dma);
    dma_channel_configure(tx_dma, &tx_config, &joybus_pio->txf[tx_sm],
                          tx_buf.data(), length, true);
    pio_interrupt_clear(joybus_pio, TX_SYS_IRQ);
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
            tx_buf[6] = (state.l_trigger & 0xF0) | (state.r_trigger >> 4);
            tx_buf[7] = 0x00;
            break;
        case 0x01:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = (state.c_stick.x & 0xF0) | (state.c_stick.y >> 4);
            tx_buf[5] = state.l_trigger;
            tx_buf[6] = state.r_trigger;
            tx_buf[7] = 0x00;
            break;
        case 0x02:
            length = 8;
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.a_stick.x;
            tx_buf[3] = state.a_stick.y;
            tx_buf[4] = (state.c_stick.x & 0xF0) | (state.c_stick.y >> 4);
            tx_buf[5] = (state.l_trigger & 0xF0) | (state.r_trigger >> 4);
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
            tx_buf[6] = state.l_trigger;
            tx_buf[7] = state.r_trigger;
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
            tx_buf[6] = state.l_trigger;
            tx_buf[7] = state.r_trigger;
            tx_buf[8] = 0x00;
            tx_buf[9] = 0x00;
            break;
    }
    send_data(length);
}