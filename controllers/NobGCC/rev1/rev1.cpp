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

#include "rev1.hpp"

#include <array>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"

std::array<control_block, 12> l_stick_control_blocks = {};
std::array<control_block, 12> r_stick_control_blocks = {};
control_block l_stick_reset_block = {};
control_block r_stick_reset_block = {};

uint32_t l_stick_raw = 0;
uint32_t r_stick_raw = 0;
std::array<uint8_t, 4> r_stick_temporary = {};
std::array<uint8_t, 4> l_stick_temporary = {};

std::array<uint8_t, 2> triggers_raw = {};

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
  return ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
}

// Configure an Si7210 sensor to read continuously
void setup_si7210_sensor(i2c_inst_t *i2c, uint8_t addr) {
  // The proper way to wake the sensor is a 0-byte write, but RP2040's I2C interface does not support 0-byte writes
  // Instead we "write" 0x00 to a read-only register
  i2c_write_blocking(i2c, addr, SI7210_WAKEUP_CONFIG.data(), 2, false);

  // Configure continuous measurement mode
  i2c_write_blocking(i2c, addr, SI7210_IDLE_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, SI7210_IDLE_TIME_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, SI7210_BURST_CONFIG.data(), 2, false);

  // Get value to write to OTP read enable byte
  uint8_t otp_enable_byte = 0;
  i2c_write_blocking(i2c, addr, &SI7210_OTP_ENABLE_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &otp_enable_byte, 1, false);
  uint8_t otp_enable_value = otp_enable_byte | 0x02;
  std::array<uint8_t, 2> otp_enable_config = {SI7210_OTP_ENABLE_ADDR,
                                              otp_enable_value};

  // Configure coefficients for averaging with temperature compensation
  uint8_t a0 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A0_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a0, 1, false);
  std::array<uint8_t, 2> a0_config = {SI7210_A0_ADDR, a0};
  i2c_write_blocking(i2c, addr, a0_config.data(), 2, false);

  uint8_t a1 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A1_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a1, 1, false);
  std::array<uint8_t, 2> a1_config = {SI7210_A1_ADDR, a1};
  i2c_write_blocking(i2c, addr, a1_config.data(), 2, false);

  uint8_t a2 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A2_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a2, 1, false);
  std::array<uint8_t, 2> a2_config = {SI7210_A2_ADDR, a2};
  i2c_write_blocking(i2c, addr, a2_config.data(), 2, false);

  uint8_t a3 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A3_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a3, 1, false);
  std::array<uint8_t, 2> a3_config = {SI7210_A3_ADDR, a3};
  i2c_write_blocking(i2c, addr, a3_config.data(), 2, false);

  uint8_t a4 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A4_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a4, 1, false);
  std::array<uint8_t, 2> a4_config = {SI7210_A4_ADDR, a4};
  i2c_write_blocking(i2c, addr, a4_config.data(), 2, false);

  uint8_t a5 = 0;
  i2c_write_blocking(i2c, addr, SI7210_READ_A5_CONFIG.data(), 2, false);
  i2c_write_blocking(i2c, addr, otp_enable_config.data(), 2, false);
  i2c_write_blocking(i2c, addr, &SI7210_OTP_DATA_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &a5, 1, false);
  std::array<uint8_t, 2> a5_config = {SI7210_A5_ADDR, a5};
  i2c_write_blocking(i2c, addr, a5_config.data(), 2, false);

  // Start measurement loop
  uint8_t start_byte = 0;
  i2c_write_blocking(i2c, addr, &SI7210_START_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &start_byte, 1, false);
  uint8_t start_value = start_byte & 0xF0;
  std::array<uint8_t, 2> start_config = {SI7210_START_ADDR, start_value};
  i2c_write_blocking(i2c, addr, start_config.data(), 2, false);

  // Configure address auto-increment
  uint8_t auto_increment_byte = 0;
  i2c_write_blocking(i2c, addr, &SI7210_AUTO_INCREMENT_ADDR, 1, true);
  i2c_read_blocking(i2c, addr, &auto_increment_byte, 1, false);
  uint8_t auto_increment_value = auto_increment_byte | 0x01;
  std::array<uint8_t, 2> auto_increment_config = {SI7210_AUTO_INCREMENT_ADDR,
                                                  auto_increment_value};
  i2c_write_blocking(i2c, addr, auto_increment_config.data(), 2, false);
}

// Setup an i2c block
void setup_i2c(i2c_inst_t *i2c, uint sda, uint scl) {
  gpio_set_function(sda, GPIO_FUNC_I2C);
  gpio_set_function(scl, GPIO_FUNC_I2C);
  gpio_pull_up(sda);
  gpio_pull_up(scl);
  i2c_init(i2c, 400000);
}

void init_stick(i2c_inst_t *i2c, uint sda, uint scl,
                std::array<control_block, 12> &control_blocks,
                control_block &reset_block,
                std::array<uint8_t, 4> &stick_temporary, uint32_t &stick_raw) {
  // Initialize I2C block
  setup_i2c(i2c, sda, scl);

  // Write sensor configuration registers
  setup_si7210_sensor(i2c, X_I2C_ADDR);
  setup_si7210_sensor(i2c, Y_I2C_ADDR);

  // Claim DMA channels
  uint transfer_channel = dma_claim_unused_channel(true);
  uint control_channel = dma_claim_unused_channel(true);

  // Create DMA configurations
  dma_channel_config control_config =
      dma_channel_get_default_config(control_channel);
  channel_config_set_write_increment(&control_config, true);
  channel_config_set_ring(&control_config, true, 4);
  uint32_t control_config_control_value =
      channel_config_get_ctrl_value(&control_config);

  dma_channel_config i2c_register_write_config =
      dma_channel_get_default_config(transfer_channel);
  channel_config_set_chain_to(&i2c_register_write_config, control_channel);
  uint32_t i2c_register_write_config_control_value =
      channel_config_get_ctrl_value(&i2c_register_write_config);

  dma_channel_config i2c_write_config =
      dma_channel_get_default_config(transfer_channel);
  channel_config_set_dreq(&i2c_write_config, i2c_get_dreq(i2c, true));
  channel_config_set_chain_to(&i2c_write_config, control_channel);
  channel_config_set_transfer_data_size(&i2c_write_config, DMA_SIZE_16);
  uint32_t i2c_write_config_control_value =
      channel_config_get_ctrl_value(&i2c_write_config);

  dma_channel_config i2c_read_config =
      dma_channel_get_default_config(transfer_channel);
  channel_config_set_read_increment(&i2c_read_config, false);
  channel_config_set_write_increment(&i2c_read_config, true);
  channel_config_set_dreq(&i2c_read_config, i2c_get_dreq(i2c, false));
  channel_config_set_chain_to(&i2c_read_config, control_channel);
  channel_config_set_transfer_data_size(&i2c_read_config, DMA_SIZE_8);
  uint32_t i2c_read_config_control_value =
      channel_config_get_ctrl_value(&i2c_read_config);

  dma_channel_config buffer_to_buffer_config =
      dma_channel_get_default_config(transfer_channel);
  channel_config_set_chain_to(&buffer_to_buffer_config, control_channel);
  channel_config_set_bswap(&buffer_to_buffer_config, true);
  uint32_t buffer_to_buffer_config_control_value =
      channel_config_get_ctrl_value(&buffer_to_buffer_config);

  dma_channel_config reset_control_config =
      dma_channel_get_default_config(transfer_channel);
  channel_config_set_write_increment(&reset_control_config, true);
  uint32_t reset_control_config_control_value =
      channel_config_get_ctrl_value(&reset_control_config);

  // Initialize control blocks
  control_blocks = {{
      {&ZERO, &i2c->hw->enable, 1, i2c_register_write_config_control_value},
      {&X_I2C_ADDR, &i2c->hw->tar, 1, i2c_register_write_config_control_value},
      {&ONE, &i2c->hw->enable, 1, i2c_register_write_config_control_value},
      {SI7210_READ_DATA_COMMANDS.data(), &i2c->hw->data_cmd, 3,
       i2c_write_config_control_value},
      {&i2c->hw->data_cmd, stick_temporary.data(), 2,
       i2c_read_config_control_value},
      {&ZERO, &i2c->hw->enable, 1, i2c_register_write_config_control_value},
      {&Y_I2C_ADDR, &i2c->hw->tar, 1, i2c_register_write_config_control_value},
      {&ONE, &i2c->hw->enable, 1, i2c_register_write_config_control_value},
      {SI7210_READ_DATA_COMMANDS.data(), &i2c->hw->data_cmd, 3,
       i2c_write_config_control_value},
      {&i2c->hw->data_cmd, stick_temporary.data() + 2, 2,
       i2c_read_config_control_value},
      {stick_temporary.data(), &stick_raw, 1,
       buffer_to_buffer_config_control_value},
      {&reset_block, &dma_hw->ch[control_channel].read_addr, 4,
       reset_control_config_control_value},
  }};

  // Initialize reset block
  reset_block = {control_blocks.data(), &dma_hw->ch[transfer_channel].read_addr,
                 4, control_config_control_value};

  // Start stick-reading DMA
  dma_channel_configure(control_channel, &control_config,
                        &dma_hw->ch[transfer_channel].read_addr,
                        control_blocks.data(), 4, true);
}

void init_sticks() {
  init_stick(i2c0, L_SDA_PIN, L_SCL_PIN, l_stick_control_blocks,
             l_stick_reset_block, l_stick_temporary, l_stick_raw);
  init_stick(i2c1, R_SDA_PIN, R_SCL_PIN, r_stick_control_blocks,
             r_stick_reset_block, r_stick_temporary, r_stick_raw);
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
                        &adc_hw->fifo, 0xFFFFFFFE, true);
  dma_channel_configure(triggers_dma_2, &triggers_config_2, triggers_raw.data(),
                        &adc_hw->fifo, 0xFFFFFFFE, false);

  // Start ADC
  adc_run(true);
}

// Read x & y-axis from a stick
void get_stick(uint32_t stick_raw, uint16_t &x_out, uint16_t &y_out) {
  x_out = (stick_raw >> 16) & 0x7FFF;
  y_out = stick_raw & 0x00007FFF;
}

void get_left_stick(uint16_t &x_out, uint16_t &y_out) {
  get_stick(l_stick_raw, x_out, y_out);
}

void get_right_stick(uint16_t &x_out, uint16_t &y_out) {
  get_stick(r_stick_raw, x_out, y_out);
}

void get_sticks(uint16_t &lx_out, uint16_t &ly_out, uint16_t &rx_out,
                uint16_t &ry_out) {
  get_left_stick(lx_out, ly_out);
  get_right_stick(rx_out, ry_out);
}

void get_triggers(uint8_t &l_out, uint8_t &r_out) {
  l_out = triggers_raw[0];
  r_out = triggers_raw[1];
}
