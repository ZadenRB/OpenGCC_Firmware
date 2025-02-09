/*
    Copyright 2023-2025 Zaden Ruggiero-Bouné

    This file is part of OpenGCC.

    OpenGCC is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    OpenGCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   OpenGCC If not, see http://www.gnu.org/licenses/.
*/

#include "joybus.hpp"

#if JOYBUS_IN_PIN == JOYBUS_OUT_PIN
#include "single_pin_joybus.pio.h"
const pio_program program = single_pin_joybus_program;
const uint stop_bit_offset = single_pin_joybus_offset_read_stop_bit;
#else
#include "joybus.pio.h"
const pio_program program = joybus_program;
const uint stop_bit_offset = joybus_offset_read_stop_bit;
#endif
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "state.hpp"

PIO joybus_pio;
uint joybus_sm;
uint joybus_offset;
uint joybus_dma;

std::array<uint8_t, 10> tx_buf = {};
std::array<uint8_t, 2> request = {};

uint jump_instruction;

void joybus_init(PIO pio, uint in_pin, uint out_pin) {
  // Joybus PIO
  joybus_pio = pio;

  joybus_offset = pio_add_program(joybus_pio, &program);
  jump_instruction = pio_encode_jmp(joybus_offset + stop_bit_offset);

  joybus_sm = pio_claim_unused_sm(joybus_pio, true);

  // Joybus TX DMA
  joybus_dma = dma_claim_unused_channel(true);
  dma_channel_config tx_config = dma_channel_get_default_config(joybus_dma);
  channel_config_set_dreq(&tx_config,
                          pio_get_dreq(joybus_pio, joybus_sm, true));
  channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
  dma_channel_set_config(joybus_dma, &tx_config, false);
  dma_channel_set_write_addr(joybus_dma, &joybus_pio->txf[joybus_sm], false);

  // Joybus RX IRQ
  irq_set_exclusive_handler(PIO0_IRQ_0, handle_console_request);
  irq_set_enabled(PIO0_IRQ_0, true);
  pio_set_irq0_source_enabled(
      joybus_pio,
      static_cast<pio_interrupt_source>(pis_sm0_rx_fifo_not_empty + joybus_sm),
      true);

  joybus_program_init(joybus_pio, joybus_sm, joybus_offset, in_pin, out_pin);
}

void handle_console_request() {
  // Disable IRQ to avoid interrupt reentrancy
  irq_set_enabled(PIO0_IRQ_0, false);

  uint32_t cmd = pio_sm_get(joybus_pio, joybus_sm);

  // Determine request length based on command
  uint request_len;
  switch (cmd) {
    case 0x40:
    case 0x42:
    case 0x43:
      request_len = 2;
      break;
    default:
      request_len = 0;
      break;
  }

  // Collect the rest of the console request
  for (int i = 0; i < request_len; ++i) {
    // Wait a max of 48us for a new item to be pushed into the RX FIFO,
    // otherwise assume we caught the middle of a command & return
    absolute_time_t timeout_at = make_timeout_time_us(48);
    while (pio_sm_is_rx_fifo_empty(joybus_pio, joybus_sm)) {
      if (absolute_time_diff_us(timeout_at, get_absolute_time()) > 0) {
        // Clear ISR (mov isr, null)
        pio_sm_exec(joybus_pio, joybus_sm, pio_encode_mov(pio_isr, pio_null));
        // Re-enable IRQ now that FIFO is emptied
        irq_set_enabled(PIO0_IRQ_0, true);
        return;
      }
    }
    request[i] = pio_sm_get(joybus_pio, joybus_sm);
  }

  // Make state machine process stop bit
  pio_sm_exec(joybus_pio, joybus_sm, jump_instruction);

  // Re-enable IRQ now that FIFO is emptied
  irq_set_enabled(PIO0_IRQ_0, true);

  // Set & send response
  switch (cmd) {
    case 0xFF:
    case 0x00:
      // Device identifier
      tx_buf[0] = 0x09;
      tx_buf[1] = 0x00;
      tx_buf[2] = 0x03;
      send_data(3);
      return;
    case 0x40:
      if (request[0] > 0x04) {
        request[0] = 0x00;
      }
      break;
    case 0x41:
      state.origin = false;
      request[0] = 0x06;
      break;
    case 0x42:
    case 0x43:
      request[0] = 0x05;
      break;
    default:
      // Continue reading if command was unknown
      return;
  }

  send_mode(request[0]);
}

void send_data(uint32_t length) {
  dma_channel_transfer_from_buffer_now(joybus_dma, tx_buf.data(), length);
}

void send_mode(uint8_t mode) {
  // Copy state that could be updated from other core
  sticks sticks_copy = state.analog_sticks;
  triggers triggers_copy = state.analog_triggers;

  if (mode != 0x06 && !state.center_set) {
    // Set centers
    state.l_trigger_center = triggers_copy.l_trigger;
    state.r_trigger_center = triggers_copy.r_trigger;
    state.center_set = true;

    // Ensure this poll is responded to with origin values
    sticks_copy.l_stick.x = 0x7F;
    sticks_copy.l_stick.y = 0x7F;
    sticks_copy.r_stick.x = 0x7F;
    sticks_copy.r_stick.y = 0x7F;
    triggers_copy.l_trigger = 0x00;
    triggers_copy.r_trigger = 0x00;
  }

  // Fill tx_buf based on mode and initiate send
  switch (mode) {
    case 0x00:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = sticks_copy.r_stick.x;
      tx_buf[5] = sticks_copy.r_stick.y;
      tx_buf[6] =
          (triggers_copy.l_trigger & 0xF0) | (triggers_copy.r_trigger >> 4);
      tx_buf[7] = 0x00;
      send_data(8);
      break;
    case 0x01:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = (sticks_copy.r_stick.x & 0xF0) | (sticks_copy.r_stick.y >> 4);
      tx_buf[5] = triggers_copy.l_trigger;
      tx_buf[6] = triggers_copy.r_trigger;
      tx_buf[7] = 0x00;
      send_data(8);
      break;
    case 0x02:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = (sticks_copy.r_stick.x & 0xF0) | (sticks_copy.r_stick.y >> 4);
      tx_buf[5] =
          (triggers_copy.l_trigger & 0xF0) | (triggers_copy.r_trigger >> 4);
      tx_buf[6] = 0x00;
      tx_buf[7] = 0x00;
      send_data(8);
      break;
    case 0x03:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = sticks_copy.r_stick.x;
      tx_buf[5] = sticks_copy.r_stick.y;
      tx_buf[6] = triggers_copy.l_trigger;
      tx_buf[7] = triggers_copy.r_trigger;
      send_data(8);
      break;
    case 0x04:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = sticks_copy.r_stick.x;
      tx_buf[5] = sticks_copy.r_stick.y;
      tx_buf[6] = 0x00;
      tx_buf[7] = 0x00;
      send_data(8);
      break;
    case 0x05:
      tx_buf[0] = state.buttons >> 8;
      tx_buf[1] = state.buttons & 0x00FF;
      tx_buf[2] = sticks_copy.l_stick.x;
      tx_buf[3] = sticks_copy.l_stick.y;
      tx_buf[4] = sticks_copy.r_stick.x;
      tx_buf[5] = sticks_copy.r_stick.y;
      tx_buf[6] = triggers_copy.l_trigger;
      tx_buf[7] = triggers_copy.r_trigger;
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
