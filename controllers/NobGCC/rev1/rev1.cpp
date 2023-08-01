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

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"

#include "rev1.hpp"

#include <array>

constexpr uint8_t X_I2C_ADDR = 0x32;
constexpr uint8_t Y_I2C_ADDR = 0x33;

constexpr std::array<uint8_t, 2> autoinc_config = {0xC5, 0x01};
constexpr std::array<uint8_t, 2> idle_config = {0xC9, 0xFE};
constexpr std::array<uint8_t, 2> idle_time_config = {0xC8, 0x00};
constexpr std::array<uint8_t, 2> start_config = {0xC4, 0x00};
constexpr uint8_t data_reg = 0xC1;

std::array<uint8_t, 2> stick_raw;

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
    return ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
}

// Configure an Si7210 sensor to read continuously
void setup_si7210_sensor(i2c_inst_t* i2c, uint8_t addr) {
    i2c_write_blocking(i2c, addr, autoinc_config.data(), 2, false);
    i2c_write_blocking(i2c, addr, idle_config.data(), 2, false);
    i2c_write_blocking(i2c, addr, idle_time_config.data(), 2, false);
    i2c_write_blocking(i2c, addr, start_config.data(), 2, false);
}

// Setup an i2c block
void setup_i2c(i2c_inst_t* i2c, uint sda, uint scl) {
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    i2c_init(i2c, 400000);
}

void init_sticks() {
    // Initialize I2C blocks
    setup_i2c(i2c0, L_SDA_PIN, L_SCL_PIN);
    setup_i2c(i2c1, R_SDA_PIN, R_SCL_PIN);

    // Write configuration registers
    setup_si7210_sensor(i2c0, X_I2C_ADDR);
    setup_si7210_sensor(i2c0, Y_I2C_ADDR);
    setup_si7210_sensor(i2c1, X_I2C_ADDR);
    setup_si7210_sensor(i2c1, Y_I2C_ADDR);
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
    dma_channel_configure(triggers_dma_1, &triggers_config_1,
                          triggers_raw.data(), &adc_hw->fifo, 0xFFFFFFFF, true);
    dma_channel_configure(triggers_dma_2, &triggers_config_2,
                          triggers_raw.data(), &adc_hw->fifo, 0xFFFFFFFF,
                          false);

    // Start ADC
    adc_run(true);
}

// Read data from Si7210 sensor
void read_si7210_data(i2c_inst_t* i2c, uint8_t addr, uint16_t &out) {
    i2c_write_blocking(i2c, addr, &data_reg, 1, true);
    i2c_read_blocking(i2c, addr, stick_raw.data(), stick_raw.size(), false);
    
    out = (stick_raw[0] & 0x7F) * 256 + stick_raw[1];
}

// Read x & y-axis from a stick
void get_stick(i2c_inst_t* i2c, uint16_t &x_out, uint16_t &y_out) {
    read_si7210_data(i2c, X_I2C_ADDR, x_out);
    read_si7210_data(i2c, Y_I2C_ADDR, y_out);
}

void get_sticks(uint16_t &lx_out, uint16_t &ly_out, uint16_t &rx_out, uint16_t &ry_out) {
    get_stick(i2c0, lx_out, ly_out);
    get_stick(i2c1, rx_out, ry_out);
}

void get_left_stick(uint16_t &x_out, uint16_t &y_out) {
    get_stick(i2c0, x_out, y_out);
}

void get_right_stick(uint16_t &x_out, uint16_t &y_out) {
    get_stick(i2c1, x_out, y_out);
}

void get_triggers(uint8_t &l_out, uint8_t &r_out) {
    l_out = triggers_raw[0];
    r_out = triggers_raw[1];
}
