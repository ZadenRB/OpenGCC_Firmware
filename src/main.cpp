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

#include "main.hpp"

#include <cmath>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "joybus.hpp"
#include "joybus.pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "q2x14.hpp"
#include "read_pwm.pio.h"

controller_state state;
controller_configuration &config = controller_configuration::get_instance();

PIO joybus_pio;
uint rx_sm;
uint rx_offset;
pio_sm_config rx_config;
uint tx_sm;
uint tx_dma;

std::array<uint32_t, 2> ax_raw;
std::array<uint32_t, 2> ay_raw;
std::array<uint32_t, 2> cx_raw;
std::array<uint32_t, 2> cy_raw;

int main() {
    // Initialize clocks
    clocks_init();

    // Configure system PLL to 128 MHZ
    set_sys_clock_pll(1536 * MHZ, 6, 2);

    // Launch analog on core 1
    multicore_launch_core1(analog_main);

    // Wait for core 1 push to the intercore FIFO, signaling core 0 to proceed
    //
    // We don't use the SDK's multicore_lockout_* functions for this because
    // it would be a race condition for core 1 to launch and interrupt core 0
    // before core 0 enables the RX IRQ
    uint32_t val = multicore_fifo_pop_blocking();
    while (val != INTERCORE_SIGNAL) {
        val = multicore_fifo_pop_blocking();
    }

    // Configure pins
    gpio_init_mask(PHYSICAL_BUTTONS_MASK & (1 << DATA));
    gpio_set_dir_in_masked(PHYSICAL_BUTTONS_MASK & (1 << DATA));
    gpio_pull_up(DATA);
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

    // Joybus PIO
    joybus_pio = pio0;

    // Joybus RX
    rx_offset = pio_add_program(joybus_pio, &joybus_rx_program);
    rx_sm = pio_claim_unused_sm(joybus_pio, true);

    // Joybus RX IRQ
    irq_set_exclusive_handler(PIO0_IRQ_0, handle_console_request);
    pio_set_irq0_source_enabled(
        joybus_pio,
        static_cast<pio_interrupt_source>(pis_sm0_rx_fifo_not_empty + rx_sm),
        true);

    // Joybus TX
    uint tx_offset = pio_add_program(joybus_pio, &joybus_tx_program);
    tx_sm = pio_claim_unused_sm(joybus_pio, true);
    joybus_tx_program_init(joybus_pio, tx_sm, tx_offset, DATA);

    // Joybus TX DMA
    tx_dma = dma_claim_unused_channel(true);
    dma_channel_config tx_config = dma_channel_get_default_config(tx_dma);
    channel_config_set_read_increment(&tx_config, true);
    channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
    channel_config_set_dreq(&tx_config, pio_get_dreq(joybus_pio, tx_sm, true));
    dma_channel_set_config(tx_dma, &tx_config, false);

    // Load configuration
    state.lt_is_jump = ((1 << config.mappings[6]) & JUMP_MASK) != 0;
    state.rt_is_jump = ((1 << config.mappings[5]) & JUMP_MASK) != 0;

    irq_set_enabled(PIO0_IRQ_0, true);

    joybus_rx_program_init(joybus_pio, rx_sm, rx_offset, DATA);

    pio_restart_sm_mask(joybus_pio, (1 << rx_sm) | (1 << tx_sm));

    uint32_t raw_input;
    uint16_t physical_buttons;
    while (true) {
        raw_input = ~gpio_get_all();
        physical_buttons = (raw_input & PHYSICAL_BUTTONS_MASK);
        read_digital(physical_buttons);
        check_combos(physical_buttons);
    }

    return 0;
}

// Read buttons
void read_digital(uint16_t physical_buttons) {
    // Output sent to console
    uint16_t remapped_buttons = (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // Apply remaps
    remap(physical_buttons, remapped_buttons, START, config.mappings[12]);
    remap(physical_buttons, remapped_buttons, Y, config.mappings[11]);
    remap(physical_buttons, remapped_buttons, X, config.mappings[10]);
    remap(physical_buttons, remapped_buttons, B, config.mappings[9]);
    remap(physical_buttons, remapped_buttons, A, config.mappings[8]);
    remap(physical_buttons, remapped_buttons, LT_DIGITAL, config.mappings[6]);
    remap(physical_buttons, remapped_buttons, RT_DIGITAL, config.mappings[5]);
    remap(physical_buttons, remapped_buttons, Z, config.mappings[4]);
    remap(physical_buttons, remapped_buttons, DPAD_UP, config.mappings[3]);
    remap(physical_buttons, remapped_buttons, DPAD_DOWN, config.mappings[2]);
    remap(physical_buttons, remapped_buttons, DPAD_RIGHT, config.mappings[1]);
    remap(physical_buttons, remapped_buttons, DPAD_LEFT, config.mappings[0]);

    // If triggers are pressed (post remapping)
    state.lt_pressed = (remapped_buttons & (1 << LT_DIGITAL)) != 0;
    state.rt_pressed = (remapped_buttons & (1 << RT_DIGITAL)) != 0;

    // Apply digital trigger modes
    apply_trigger_mode_digital(remapped_buttons, LT_DIGITAL,
                               config.l_trigger_mode, config.r_trigger_mode);
    apply_trigger_mode_digital(remapped_buttons, RT_DIGITAL,
                               config.r_trigger_mode, config.l_trigger_mode);

    // Update state
    state.buttons = remapped_buttons;
}

// Update remapped_buttons based on a physical button and its mapping
void remap(uint16_t physical_buttons, uint16_t &remapped_buttons,
           uint8_t to_remap, uint8_t mapping) {
    uint16_t mask = ~(1 << mapping);
    bool pressed = (physical_buttons & (1 << to_remap)) != 0;
    remapped_buttons = (remapped_buttons & mask) | (pressed << mapping);
}

// Modify digital value based on the trigger mode
void apply_trigger_mode_digital(uint16_t &buttons, uint8_t bit_to_set,
                                trigger_mode mode, trigger_mode other_mode) {
    switch (mode) {
        case both:
        case trigger_plug:
        case analog_multiplied:
            if (other_mode == analog_on_digital) {
                buttons = buttons & ~(1 << bit_to_set);
            }
            break;
        case analog_only:
        case analog_on_digital:
            buttons = buttons & ~(1 << bit_to_set);
            break;
    }
}

// Check if any button combos are being pressed
void check_combos(uint32_t physical_buttons) {
    // Check if the active combo is still being pressed
    if (state.active_combo != 0) {
        if (state.active_combo == physical_buttons) {
            return;
        } else {
            cancel_alarm(state.combo_alarm);
            state.active_combo = 0;
        }
    }

    // Check if the combo is to toggle safe mode
    if (physical_buttons == ((1 << START) | (1 << Y) | (1 << A) | (1 << Z))) {
        state.active_combo = physical_buttons;
        state.combo_alarm = add_alarm_in_ms(3000, execute_combo, NULL, false);
        return;
    }

    // If not in safe mode, check other combos
    if (!state.safe_mode) {
        switch (physical_buttons) {
            case (1 << START) | (1 << B) | (1 << A):
            case (1 << START) | (1 << B) | (1 << Z):
            case (1 << START) | (1 << B) | (1 << LT_DIGITAL):
            case (1 << START) | (1 << B) | (1 << RT_DIGITAL):
                state.active_combo = physical_buttons;
                state.combo_alarm =
                    add_alarm_in_ms(3000, execute_combo, NULL, false);
                break;
        }
    }
}

// Run the handler function for a combo
int64_t execute_combo(alarm_id_t alarm_id, void *user_data) {
    switch (state.active_combo) {
        case (1 << START) | (1 << Y) | (1 << A) | (1 << Z):
            toggle_safe_mode();
            break;
        case (1 << START) | (1 << B) | (1 << A):
            config.swap_mappings();
            break;
        case (1 << START) | (1 << B) | (1 << Z):
            config.configure_triggers();
            break;
        case (1 << START) | (1 << B) | (1 << LT_DIGITAL):
            config.calibrate_stick(config.a_stick, state.c_stick, ax_raw,
                                   ay_raw);
        case (1 << START) | (1 << B) | (1 << RT_DIGITAL):
            config.calibrate_stick(config.c_stick, state.a_stick, cx_raw,
                                   cy_raw);
    }
    state.active_combo = 0;
    return 0;
}

// Toggle safe mode
void toggle_safe_mode() { state.safe_mode = !state.safe_mode; }

void analog_main() {
    // Enable lockout
    multicore_lockout_victim_init();

    // Set PWM inputs as pull up
    gpio_pull_up(AX);
    gpio_pull_up(AY);
    gpio_pull_up(CX);
    gpio_pull_up(CY);

    // Configure PWM PIO
    PIO pwm_pio = pio1;
    uint read_pwm_offset = pio_add_program(pwm_pio, &read_pwm_program);

    // Configure SMs
    uint ax_sm = pio_claim_unused_sm(pwm_pio, true);
    read_pwm_program_init(pwm_pio, ax_sm, read_pwm_offset, AX);

    uint ay_sm = pio_claim_unused_sm(pwm_pio, true);
    read_pwm_program_init(pwm_pio, ay_sm, read_pwm_offset, AY);

    uint cx_sm = pio_claim_unused_sm(pwm_pio, true);
    read_pwm_program_init(pwm_pio, cx_sm, read_pwm_offset, CX);

    uint cy_sm = pio_claim_unused_sm(pwm_pio, true);
    read_pwm_program_init(pwm_pio, cy_sm, read_pwm_offset, CY);

    // Claim PWM DMA channels
    int ax_dma_1 = dma_claim_unused_channel(true);
    int ax_dma_2 = dma_claim_unused_channel(true);
    int ay_dma_1 = dma_claim_unused_channel(true);
    int ay_dma_2 = dma_claim_unused_channel(true);
    int cx_dma_1 = dma_claim_unused_channel(true);
    int cx_dma_2 = dma_claim_unused_channel(true);
    int cy_dma_1 = dma_claim_unused_channel(true);
    int cy_dma_2 = dma_claim_unused_channel(true);

    // Setup shared configurations
    dma_channel_config dma_base_config =
        dma_channel_get_default_config(ax_dma_1);
    channel_config_set_read_increment(&dma_base_config,
                                      false);  // Always read from same address
    channel_config_set_write_increment(&dma_base_config,
                                       true);  // Increment write address
    channel_config_set_ring(&dma_base_config, true, 3);  // Wrap after 8 bytes

    // Configure SM specific configurations
    dma_channel_config ax_dma_base_config = dma_base_config;
    channel_config_set_dreq(&ax_dma_base_config,
                            pio_get_dreq(pwm_pio, ax_sm, false));

    dma_channel_config ay_dma_base_config = dma_base_config;
    channel_config_set_dreq(&ay_dma_base_config,
                            pio_get_dreq(pwm_pio, ay_sm, false));

    dma_channel_config cx_dma_base_config = dma_base_config;
    channel_config_set_dreq(&cx_dma_base_config,
                            pio_get_dreq(pwm_pio, cx_sm, false));

    dma_channel_config cy_dma_base_config = dma_base_config;
    channel_config_set_dreq(&cy_dma_base_config,
                            pio_get_dreq(pwm_pio, cy_sm, false));

    // Setup channel specific configurations
    dma_channel_config ax_dma_1_config = ax_dma_base_config;
    channel_config_set_chain_to(&ax_dma_1_config, ax_dma_2);
    dma_channel_config ax_dma_2_config = ax_dma_base_config;
    channel_config_set_chain_to(&ax_dma_2_config, ax_dma_1);

    dma_channel_config ay_dma_1_config = ay_dma_base_config;
    channel_config_set_chain_to(&ay_dma_1_config, ay_dma_2);
    dma_channel_config ay_dma_2_config = ay_dma_base_config;
    channel_config_set_chain_to(&ay_dma_2_config, ay_dma_1);

    dma_channel_config cx_dma_1_config = cx_dma_base_config;
    channel_config_set_chain_to(&cx_dma_1_config, cx_dma_2);
    dma_channel_config cx_dma_2_config = cx_dma_base_config;
    channel_config_set_chain_to(&cx_dma_2_config, cx_dma_1);

    dma_channel_config cy_dma_1_config = cy_dma_base_config;
    channel_config_set_chain_to(&cy_dma_1_config, cy_dma_2);
    dma_channel_config cy_dma_2_config = cy_dma_base_config;
    channel_config_set_chain_to(&cy_dma_2_config, cy_dma_1);

    // Apply configurations
    dma_channel_configure(ax_dma_1, &ax_dma_1_config, ax_raw.data(),
                          &pwm_pio->rxf[ax_sm], 0xFFFFFFFF,
                          true);  // Configure and start AX DMA 1
    dma_channel_configure(ax_dma_2, &ax_dma_2_config, ax_raw.data(),
                          &pwm_pio->rxf[ax_sm], 0xFFFFFFFF,
                          false);  // Configure but don't start AX DMA 2

    dma_channel_configure(ay_dma_1, &ay_dma_1_config, ay_raw.data(),
                          &pwm_pio->rxf[ay_sm], 0xFFFFFFFF,
                          true);  // Configure and start AY DMA 1
    dma_channel_configure(ay_dma_2, &ay_dma_2_config, ay_raw.data(),
                          &pwm_pio->rxf[ay_sm], 0xFFFFFFFF,
                          false);  // Configure but don't start AY DMA 2

    dma_channel_configure(cx_dma_1, &cx_dma_1_config, cx_raw.data(),
                          &pwm_pio->rxf[cx_sm], 0xFFFFFFFF,
                          true);  // Configure and start CX DMA 1
    dma_channel_configure(cx_dma_2, &cx_dma_2_config, cx_raw.data(),
                          &pwm_pio->rxf[cx_sm], 0xFFFFFFFF,
                          false);  // Configure but don't start CX DMA 2

    dma_channel_configure(cy_dma_1, &cy_dma_1_config, cy_raw.data(),
                          &pwm_pio->rxf[cy_sm], 0xFFFFFFFF,
                          true);  // Configure and start CY DMA 1
    dma_channel_configure(cy_dma_2, &cy_dma_2_config, cy_raw.data(),
                          &pwm_pio->rxf[cy_sm], 0xFFFFFFFF,
                          false);  // Configure but don't start CY DMA 2

    // Start all PWM state machines at the same time
    pio_enable_sm_mask_in_sync(pwm_pio, 0xF);

    // Configure ADC
    adc_init();
    adc_gpio_init(LT_ANALOG);
    adc_gpio_init(RT_ANALOG);
    adc_select_input(LT_ANALOG_ADC_INPUT);
    adc_set_round_robin(TRIGGER_ADC_MASK);
    adc_fifo_setup(true, true, 1, false, true);

    // ADC data destination
    std::array<uint8_t, 2> triggers_raw = {0};

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

    // Allow core 0 to continue (start responding to console polls) after
    // reading analog inputs once
    read_triggers(triggers_raw[0], triggers_raw[1]);
    read_sticks(ax_raw, ay_raw, cx_raw, cy_raw);
    multicore_fifo_push_blocking(INTERCORE_SIGNAL);

    while (1) {
        read_triggers(triggers_raw[0], triggers_raw[1]);
        read_sticks(ax_raw, ay_raw, cx_raw, cy_raw);
    }
}

// Read analog triggers & apply analog trigger modes
void read_triggers(uint8_t lt_raw, uint8_t rt_raw) {
    apply_trigger_mode_analog(state.l_trigger, lt_raw,
                              config.l_trigger_threshold_value,
                              state.lt_pressed, !state.lt_is_jump,
                              config.l_trigger_mode, config.r_trigger_mode);
    apply_trigger_mode_analog(state.r_trigger, rt_raw,
                              config.r_trigger_threshold_value,
                              state.rt_pressed, !state.rt_is_jump,
                              config.r_trigger_mode, config.l_trigger_mode);
}

// Modify analog value based on the trigger mode
void apply_trigger_mode_analog(uint8_t &out, uint8_t analog_value,
                               uint8_t threshold_value, bool digital_value,
                               bool enable_analog, trigger_mode mode,
                               trigger_mode other_mode) {
    switch (mode) {
        case digital_only:
            out = 0;
            break;
        case both:
        case analog_only:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else {
                out = analog_value * enable_analog;
            }
            break;
        case trigger_plug:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else if (analog_value > threshold_value) {
                out = threshold_value * enable_analog;
            } else {
                out = analog_value * enable_analog;
            }
            break;
        case analog_on_digital:
        case both_on_digital:
            out = threshold_value * digital_value * enable_analog;
            break;
        case analog_multiplied:
            if (other_mode == analog_on_digital) {
                out = 0;
            } else {
                float multiplier = (threshold_value * 0.01124f) + 0.44924f;
                float multiplied_value = round(analog_value * multiplier);
                if (multiplied_value > 255) {
                    out = 255 * enable_analog;
                } else {
                    out =
                        static_cast<uint8_t>(multiplied_value) * enable_analog;
                }
            }
            break;
    }
}

void read_sticks(std::array<uint32_t, 2> const &ax_raw,
                 std::array<uint32_t, 2> const &ay_raw,
                 std::array<uint32_t, 2> const &cx_raw,
                 std::array<uint32_t, 2> const &cy_raw) {
    uint ax_high_total = 0;
    uint ax_low_total = 0;
    uint ay_high_total = 0;
    uint ay_low_total = 0;
    uint cx_high_total = 0;
    uint cx_low_total = 0;
    uint cy_high_total = 0;
    uint cy_low_total = 0;
    for (uint i = 0; i < SAMPLES_PER_READ; ++i) {
        ax_high_total += ax_raw[0] + 3;
        ax_low_total += ax_raw[1];
        ay_high_total += ay_raw[0] + 3;
        ay_low_total += ay_raw[1];
        cx_high_total += cx_raw[0] + 3;
        cx_low_total += cx_raw[1];
        cy_high_total += cy_raw[0] + 3;
        cy_low_total += cy_raw[1];
    }
    Q2x14 ax = Q2x14(ax_high_total / (float)(ax_low_total + ax_high_total));
    Q2x14 ay = Q2x14(ay_high_total / (float)(ay_low_total + ay_high_total));
    Q2x14 cx = Q2x14(cx_high_total / (float)(cx_low_total + cy_high_total));
    Q2x14 cy = Q2x14(cy_high_total / (float)(cy_low_total + cy_high_total));
}
