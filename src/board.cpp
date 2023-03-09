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
   NobGCC-SW. If not, see http://www.gnu.org/licenses/.
*/

#include "board.hpp"

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "read_pwm.pio.h"

// Align as 16 here so that DMA ring configuration works properly
alignas(16) std::array<uint32_t, 2> lx_raw;
alignas(16) std::array<uint32_t, 2> ly_raw;
alignas(16) std::array<uint32_t, 2> rx_raw;
alignas(16) std::array<uint32_t, 2> ry_raw;

alignas(16) std::array<uint8_t, 2> triggers_raw;

void init_buttons() {
    // Set buttons as pull-up inputs
    gpio_pull_up(DPAD_LEFT);
    gpio_pull_up(DPAD_RIGHT);
    gpio_pull_up(DPAD_DOWN);
    gpio_pull_up(DPAD_UP);
    gpio_pull_up(Z);
    gpio_pull_up(RT_DIGITAL);
    gpio_pull_up(LT_DIGITAL);
    gpio_pull_up(A);
    gpio_pull_up(B);
    gpio_pull_up(X);
    gpio_pull_up(Y);
    gpio_pull_up(START);

    // Wait for pull-ups to stabilize
    busy_wait_us(100);
}

uint16_t get_buttons() {
    // Get all pins and invert values (pulled down means pressed), bitwise and
    // with mask to only include buttons
    return ~gpio_get_all() & PHYSICAL_BUTTONS_MASK;
}

void init_sticks() {
    // Set PWM inputs as pull-up
    gpio_pull_up(LX);
    gpio_pull_up(LY);
    gpio_pull_up(RX);
    gpio_pull_up(RY);

    // Configure PWM PIO
    PIO pwm_pio = pio1;
    uint read_pwm_offset = pio_add_program(pwm_pio, &read_pwm_program);

    // Configure SMs
    uint lx_sm = pio_claim_unused_sm(pwm_pio, true);
    uint ly_sm = pio_claim_unused_sm(pwm_pio, true);
    uint rx_sm = pio_claim_unused_sm(pwm_pio, true);
    uint ry_sm = pio_claim_unused_sm(pwm_pio, true);

    // Claim PWM DMA channels
    int lx_dma_1 = dma_claim_unused_channel(true);
    int lx_dma_2 = dma_claim_unused_channel(true);
    int ly_dma_1 = dma_claim_unused_channel(true);
    int ly_dma_2 = dma_claim_unused_channel(true);
    int rx_dma_1 = dma_claim_unused_channel(true);
    int rx_dma_2 = dma_claim_unused_channel(true);
    int ry_dma_1 = dma_claim_unused_channel(true);
    int ry_dma_2 = dma_claim_unused_channel(true);

    // Setup shared configurations
    dma_channel_config dma_base_config =
        dma_channel_get_default_config(lx_dma_1);
    channel_config_set_read_increment(&dma_base_config,
                                      false);  // Always read from same address
    channel_config_set_write_increment(&dma_base_config,
                                       true);  // Increment write address
    channel_config_set_ring(&dma_base_config, true,
                            3);  // Wrap on 8 byte address boundary

    // Configure SM specific configurations
    dma_channel_config lx_dma_base_config = dma_base_config;
    channel_config_set_dreq(&lx_dma_base_config,
                            pio_get_dreq(pwm_pio, lx_sm, false));

    dma_channel_config ly_dma_base_config = dma_base_config;
    channel_config_set_dreq(&ly_dma_base_config,
                            pio_get_dreq(pwm_pio, ly_sm, false));

    dma_channel_config rx_dma_base_config = dma_base_config;
    channel_config_set_dreq(&rx_dma_base_config,
                            pio_get_dreq(pwm_pio, rx_sm, false));

    dma_channel_config ry_dma_base_config = dma_base_config;
    channel_config_set_dreq(&ry_dma_base_config,
                            pio_get_dreq(pwm_pio, ry_sm, false));

    // Setup channel specific configurations
    dma_channel_config lx_dma_1_config = lx_dma_base_config;
    channel_config_set_chain_to(&lx_dma_1_config, lx_dma_2);
    dma_channel_config lx_dma_2_config = lx_dma_base_config;
    channel_config_set_chain_to(&lx_dma_2_config, lx_dma_1);

    dma_channel_config ly_dma_1_config = ly_dma_base_config;
    channel_config_set_chain_to(&ly_dma_1_config, ly_dma_2);
    dma_channel_config ly_dma_2_config = ly_dma_base_config;
    channel_config_set_chain_to(&ly_dma_2_config, ly_dma_1);

    dma_channel_config rx_dma_1_config = rx_dma_base_config;
    channel_config_set_chain_to(&rx_dma_1_config, rx_dma_2);
    dma_channel_config rx_dma_2_config = rx_dma_base_config;
    channel_config_set_chain_to(&rx_dma_2_config, rx_dma_1);

    dma_channel_config ry_dma_1_config = ry_dma_base_config;
    channel_config_set_chain_to(&ry_dma_1_config, ry_dma_2);
    dma_channel_config ry_dma_2_config = ry_dma_base_config;
    channel_config_set_chain_to(&ry_dma_2_config, ry_dma_1);

    // Apply configurations
    dma_channel_configure(lx_dma_1, &lx_dma_1_config, lx_raw.data(),
                          &pwm_pio->rxf[lx_sm], 0xFFFFFFFD,
                          true);  // Configure and start LX DMA 1
    dma_channel_configure(lx_dma_2, &lx_dma_2_config, lx_raw.data(),
                          &pwm_pio->rxf[lx_sm], 0xFFFFFFFD,
                          false);  // Configure but don't start LX DMA 2

    dma_channel_configure(ly_dma_1, &ly_dma_1_config, ly_raw.data(),
                          &pwm_pio->rxf[ly_sm], 0xFFFFFFFD,
                          true);  // Configure and start LY DMA 1
    dma_channel_configure(ly_dma_2, &ly_dma_2_config, ly_raw.data(),
                          &pwm_pio->rxf[ly_sm], 0xFFFFFFFD,
                          false);  // Configure but don't start LY DMA 2

    dma_channel_configure(rx_dma_1, &rx_dma_1_config, rx_raw.data(),
                          &pwm_pio->rxf[rx_sm], 0xFFFFFFFD,
                          true);  // Configure and start RX DMA 1
    dma_channel_configure(rx_dma_2, &rx_dma_2_config, rx_raw.data(),
                          &pwm_pio->rxf[rx_sm], 0xFFFFFFFD,
                          false);  // Configure but don't start RX DMA 2

    dma_channel_configure(ry_dma_1, &ry_dma_1_config, ry_raw.data(),
                          &pwm_pio->rxf[ry_sm], 0xFFFFFFFD,
                          true);  // Configure and start RY DMA 1
    dma_channel_configure(ry_dma_2, &ry_dma_2_config, ry_raw.data(),
                          &pwm_pio->rxf[ry_sm], 0xFFFFFFFD,
                          false);  // Configure but don't start RY DMA 2

    // Start Joybus PIO state machines
    read_pwm_program_init(pwm_pio, lx_sm, read_pwm_offset, LX);
    read_pwm_program_init(pwm_pio, ly_sm, read_pwm_offset, LY);
    read_pwm_program_init(pwm_pio, rx_sm, read_pwm_offset, RX);
    read_pwm_program_init(pwm_pio, ry_sm, read_pwm_offset, RY);
}

void init_triggers() {
    // Configure ADC
    adc_init();
    adc_gpio_init(LT_ANALOG);
    adc_gpio_init(RT_ANALOG);
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

void get_sticks(double &lx_out, double &ly_out, double &rx_out, double &ry_out,
                uint sample_for_us) {
    double lx_high = 0;
    double lx_low = 0;
    double ly_high = 0;
    double ly_low = 0;
    double rx_high = 0;
    double rx_low = 0;
    double ry_high = 0;
    double ry_low = 0;
    uint num_samples = 0;
    absolute_time_t timeout_at = make_timeout_time_us(sample_for_us);

    // Collect inputs for sample_for_us microseconds
    while (absolute_time_diff_us(timeout_at, get_absolute_time()) < 0) {
        lx_high += lx_raw[0];
        lx_low += lx_raw[1];
        ly_high += ly_raw[0];
        ly_low += ly_raw[1];
        rx_high += rx_raw[0];
        rx_low += rx_raw[1];
        ry_high += ry_raw[0];
        ry_low += ry_raw[1];
        ++num_samples;
    }

    // Add correction factors
    lx_high += PWM_HIGH_CORRECTION * num_samples;
    lx_low += PWM_LOW_CORRECTION * num_samples;
    ly_high += PWM_HIGH_CORRECTION * num_samples;
    ly_low += PWM_LOW_CORRECTION * num_samples;
    rx_high += PWM_HIGH_CORRECTION * num_samples;
    rx_low += PWM_LOW_CORRECTION * num_samples;
    ry_high += PWM_HIGH_CORRECTION * num_samples;
    ry_low += PWM_LOW_CORRECTION * num_samples;

    // Set outputs to average
    lx_out = lx_high / (lx_high + lx_low);
    ly_out = ly_high / (ly_high + ly_low);
    rx_out = rx_high / (rx_high + rx_low);
    ry_out = ry_high / (ry_high + ry_low);
}

void get_left_stick(double &x_out, double &y_out, uint sample_for_us) {
    double x_high = 0;
    double x_low = 0;
    double y_high = 0;
    double y_low = 0;
    uint num_samples = 0;
    absolute_time_t timeout_at = make_timeout_time_us(sample_for_us);

    // Collect inputs for sample_for_us microseconds
    while (absolute_time_diff_us(timeout_at, get_absolute_time()) < 0) {
        x_high += lx_raw[0];
        x_low += lx_raw[1];
        y_high += ly_raw[0];
        y_low += ly_raw[1];
        ++num_samples;
    }

    // Add correction factors
    x_high += PWM_HIGH_CORRECTION * num_samples;
    x_low += PWM_LOW_CORRECTION * num_samples;
    y_high += PWM_HIGH_CORRECTION * num_samples;
    y_low += PWM_LOW_CORRECTION * num_samples;

    // Set outputs to average
    x_out = x_high / (x_high + x_low);
    y_out = y_high / (y_high + y_low);
}

void get_right_stick(double &x_out, double &y_out, uint sample_for_us) {
    double x_high = 0;
    double x_low = 0;
    double y_high = 0;
    double y_low = 0;
    uint num_samples = 0;
    absolute_time_t timeout_at = make_timeout_time_us(sample_for_us);

    // Collect inputs for sample_for_us microseconds
    while (absolute_time_diff_us(timeout_at, get_absolute_time()) < 0) {
        x_high += rx_raw[0];
        x_low += rx_raw[1];
        y_high += ry_raw[0];
        y_low += ry_raw[1];
        ++num_samples;
    }

    // Add correction factors
    x_high += PWM_HIGH_CORRECTION * num_samples;
    x_low += PWM_LOW_CORRECTION * num_samples;
    y_high += PWM_HIGH_CORRECTION * num_samples;
    y_low += PWM_LOW_CORRECTION * num_samples;

    // Set outputs to average
    x_out = x_high / (x_high + x_low);
    y_out = y_high / (y_high + y_low);
}

void get_triggers(uint8_t &l_out, uint8_t &r_out) {
    l_out = triggers_raw[0];
    r_out = triggers_raw[1];
}