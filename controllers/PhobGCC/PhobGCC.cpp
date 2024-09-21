/*
    Copyright 2023 Zaden Ruggiero-Bouné

    This file is part of OpenGCC.

    OpenGCC is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    OpenGCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   OpenGCC. If not, see http://www.gnu.org/licenses/.
*/

#include "PhobGCC.hpp"

#include <array>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "opengcc/controller.hpp"
#include "opengcc/state.hpp"

std::array<uint8_t, 3> stick_raw;

alignas(16) std::array<uint8_t, 2> triggers_raw;

void init_buttons() {
  // Set buttons as pull-up inputs
  gpio_pull_up(DPAD_LEFT_PIN);
  gpio_pull_up(DPAD_RIGHT_PIN);
  gpio_pull_up(DPAD_DOWN_PIN);
  gpio_pull_up(DPAD_UP_PIN);
  gpio_pull_up(Z_PIN);
  gpio_pull_up(RT_DIGITAL_PIN);
  gpio_pull_up(LT_DIGITAL_PIN);
  gpio_pull_up(A_PIN);
  gpio_pull_up(B_PIN);
  gpio_pull_up(X_PIN);
  gpio_pull_up(Y_PIN);
  gpio_pull_up(START_PIN);

  // Wait for pull-ups to stabilize
  busy_wait_us(100);
}

uint16_t get_buttons() {
  // Get all pins and invert values (pulled down means pressed), bitwise and
  // with mask to only include buttons
  return (gpio_get(DPAD_LEFT_PIN) << DPAD_LEFT) |
         (gpio_get(DPAD_RIGHT_PIN) << DPAD_RIGHT) |
         (gpio_get(DPAD_DOWN_PIN) << DPAD_DOWN) |
         (gpio_get(DPAD_UP_PIN) << DPAD_UP) | (gpio_get(Z_PIN) << Z) |
         (gpio_get(RT_DIGITAL_PIN) << RT_DIGITAL) |
         (gpio_get(LT_DIGITAL_PIN) << LT_DIGITAL) | (gpio_get(A_PIN) << A) |
         (gpio_get(B_PIN) << B) | (gpio_get(X_PIN) << X) |
         (gpio_get(Y_PIN) << Y) | (gpio_get(START_PIN) << START);
}

// Setup an i2c block
void setup_spi(spi_inst_t *spi, uint clk, uint tx, uint rx) {
  gpio_set_function(clk, GPIO_FUNC_SPI);
  gpio_set_function(tx, GPIO_FUNC_SPI);
  gpio_set_function(rx, GPIO_FUNC_SPI);
  spi_init(spi, 3000000);
}

void init_sticks() {
  // Initialize SPI block
  setup_spi(spi0, SPI_CLK_PIN, SPI_TX_PIN, SPI_RX_PIN);

  // Setup chip selects
  gpio_init(L_CS_PIN);
  gpio_set_dir(L_CS_PIN, GPIO_OUT);
  gpio_put(L_CS_PIN, 1);

  gpio_init(R_CS_PIN);
  gpio_set_dir(R_CS_PIN, GPIO_OUT);
  gpio_put(R_CS_PIN, 1);
}

void init_triggers() {
  // Configure ADC
  adc_init();
  adc_gpio_init(LT_ANALOG_PIN);
  adc_gpio_init(RT_ANALOG_PIN);
  adc_select_input(LT_ANALOG_ADC_INPUT);
  adc_set_round_robin(TRIGGER_ADC_MASK);
  adc_fifo_setup(true, true, 1, false, true);

  // Claim ADC DMA channels
  uint triggers_dma_1 = dma_claim_unused_channel(true);
  uint triggers_dma_2 = dma_claim_unused_channel(true);

  // Setup base configuration
  dma_channel_config triggers_base_config =
      dma_channel_get_default_config(triggers_dma_1);
  channel_config_set_read_increment(&triggers_base_config,
                                    false);  // Always read from same address
  channel_config_set_write_increment(&triggers_base_config,
                                     true);  // Increment write address
  channel_config_set_transfer_data_size(&triggers_base_config, DMA_SIZE_8);
  channel_config_set_ring(&triggers_base_config, true,
                          1);  // Wrap after 2 bytes
  channel_config_set_dreq(&triggers_base_config, DREQ_ADC);

  // Setup channel specific configurations
  dma_channel_config triggers_config_1 = triggers_base_config;
  channel_config_set_chain_to(&triggers_config_1, triggers_dma_2);
  dma_channel_config triggers_config_2 = triggers_base_config;
  channel_config_set_chain_to(&triggers_config_2, triggers_dma_1);

  // Apply configurations
  dma_channel_configure(triggers_dma_1, &triggers_config_1, triggers_raw.data(),
                        &adc_hw->fifo, 0xFFFFFFFF, true);
  dma_channel_configure(triggers_dma_2, &triggers_config_2, triggers_raw.data(),
                        &adc_hw->fifo, 0xFFFFFFFF, false);

  // Start ADC
  adc_run(true);
}

// Read data from MCP3202 ADC, 12 bit
uint16_t read_mcp3202_data(uint cs_pin, bool read_y_axis) {
  // Setup config data based on which axis we're reading
  uint8_t config_byte = 0b11010000 | (read_y_axis << 5);

  // Select stick
  gpio_put(cs_pin, 0);

  // Read and push result to out
  spi_read_blocking(spi0, config_byte, stick_raw.data(), stick_raw.size());

  // Deselect stick
  gpio_put(cs_pin, 1);

  return (((stick_raw[0] & 0b00000111) << 9) | stick_raw[1] << 1 |
          stick_raw[2] >> 7);
}

// Read x & y-axis from a stick
raw_stick get_stick(uint cs_pin) {
  return {read_mcp3202_data(cs_pin, false), read_mcp3202_data(cs_pin, true),
          true};
}

raw_sticks get_sticks() { return {get_stick(L_CS_PIN), get_stick(R_CS_PIN)}; }

raw_stick get_left_stick() { return get_stick(L_CS_PIN); }

raw_stick get_right_stick() { return get_stick(R_CS_PIN); }

raw_triggers get_triggers() { return {triggers_raw[0], triggers_raw[1]}; }
