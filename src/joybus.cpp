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

#include "joybus.hpp"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "joybus.pio.h"
#include "joybus_uf2.hpp"
#include "state.hpp"

PIO joybus_pio;
uint joybus_tx_sm;
uint joybus_rx_sm;
uint joybus_rx_offset;
uint joybus_tx_dma;

std::array<uint8_t, 10> tx_buf = {};

void joybus_init(PIO pio, uint data_pin) {
    // Pull up data line
    gpio_pull_up(data_pin);

    // Joybus PIO
    joybus_pio = pio;

    // Joybus RX
    joybus_rx_offset = pio_add_program(joybus_pio, &joybus_rx_program);
    joybus_rx_sm = pio_claim_unused_sm(joybus_pio, true);

    // Joybus RX IRQ
    irq_set_exclusive_handler(PIO0_IRQ_0, handle_console_request);
    pio_set_irq0_source_enabled(joybus_pio,
                                static_cast<pio_interrupt_source>(
                                    pis_sm0_rx_fifo_not_empty + joybus_rx_sm),
                                true);

    // Joybus TX
    uint tx_offset = pio_add_program(joybus_pio, &joybus_tx_program);
    joybus_tx_sm = pio_claim_unused_sm(joybus_pio, true);
    joybus_tx_program_init(joybus_pio, joybus_tx_sm, tx_offset, data_pin);

    // Joybus TX DMA
    joybus_tx_dma = dma_claim_unused_channel(true);
    dma_channel_config tx_config =
        dma_channel_get_default_config(joybus_tx_dma);
    channel_config_set_read_increment(&tx_config, true);
    channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
    channel_config_set_dreq(&tx_config,
                            pio_get_dreq(joybus_pio, joybus_tx_sm, true));
    dma_channel_set_config(joybus_tx_dma, &tx_config, false);
    dma_channel_set_write_addr(joybus_tx_dma, &joybus_pio->txf[joybus_tx_sm],
                               false);

    pio_sm_put_blocking(joybus_pio, joybus_tx_sm, FIFO_EMPTY);

    irq_set_enabled(PIO0_IRQ_0, true);

    joybus_rx_program_init(joybus_pio, joybus_rx_sm, joybus_rx_offset,
                           data_pin);

    pio_restart_sm_mask(joybus_pio, (1 << joybus_rx_sm) | (1 << joybus_tx_sm));
}

void handle_console_request() {
    uint32_t cmd = pio_sm_get(joybus_pio, joybus_rx_sm);

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
        while (pio_sm_is_rx_fifo_empty(joybus_pio, joybus_rx_sm)) {
            if (absolute_time_diff_us(timeout_at, get_absolute_time()) > 0) {
                // Clear ISR (mov isr, null)
                pio_sm_exec(joybus_pio, joybus_rx_sm,
                            pio_encode_mov(pio_isr, pio_null));
                return;
            }
        }
        request[i] = pio_sm_get(joybus_pio, joybus_rx_sm);
    }

    // Move to code to process stop bit
    pio_sm_exec(
        joybus_pio, joybus_rx_sm,
        pio_encode_jmp(joybus_rx_offset + joybus_rx_offset_read_stop_bit));

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
            request[0] = 0x06;
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

void send_data(uint32_t length) {
    dma_channel_transfer_from_buffer_now(joybus_tx_dma, tx_buf.data(), length);
    pio_interrupt_clear(joybus_pio, TX_SYS_IRQ);
}

void send_mode(uint8_t mode) {
    switch (mode) {
        case 0x00:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = state.r_stick.x;
            tx_buf[5] = state.r_stick.y;
            tx_buf[6] = (state.l_trigger & 0xF0) | (state.r_trigger >> 4);
            tx_buf[7] = 0x00;
            send_data(8);
            break;
        case 0x01:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = (state.r_stick.x & 0xF0) | (state.r_stick.y >> 4);
            tx_buf[5] = state.l_trigger;
            tx_buf[6] = state.r_trigger;
            tx_buf[7] = 0x00;
            send_data(8);
            break;
        case 0x02:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = (state.r_stick.x & 0xF0) | (state.r_stick.y >> 4);
            tx_buf[5] = (state.l_trigger & 0xF0) | (state.r_trigger >> 4);
            tx_buf[6] = 0x00;
            tx_buf[7] = 0x00;
            send_data(8);
            break;
        case 0x03:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = state.r_stick.x;
            tx_buf[5] = state.r_stick.y;
            tx_buf[6] = state.l_trigger;
            tx_buf[7] = state.r_trigger;
            send_data(8);
            break;
        case 0x04:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = state.r_stick.x;
            tx_buf[5] = state.r_stick.y;
            tx_buf[6] = 0x00;
            tx_buf[7] = 0x00;
            send_data(8);
            break;
        case 0x05:
            tx_buf[0] = state.buttons >> 8;
            tx_buf[1] = state.buttons & 0x00FF;
            tx_buf[2] = state.l_stick.x;
            tx_buf[3] = state.l_stick.y;
            tx_buf[4] = state.r_stick.x;
            tx_buf[5] = state.r_stick.y;
            tx_buf[6] = state.l_trigger;
            tx_buf[7] = state.r_trigger;
            tx_buf[8] = 0x00;
            tx_buf[9] = 0x00;
            send_data(10);
            break;
        case 0x06:
            // Origin state
            tx_buf[0] = 0x00;
            tx_buf[1] = 0x80;
            tx_buf[2] = 0x7F;
            tx_buf[3] = 0x7F;
            tx_buf[4] = 0x7F;
            tx_buf[5] = 0x7F;
            tx_buf[6] = 0x00;
            tx_buf[7] = 0x00;
            tx_buf[8] = 0x00;
            tx_buf[9] = 0x00;
            send_data(10);
            break;
    }
}