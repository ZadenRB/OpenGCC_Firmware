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
   OpenGCC If not, see http://www.gnu.org/licenses/.
*/

#include "main.hpp"

#include <cmath>

#include CONFIG_H
#include "calibration.hpp"
#include "configuration.hpp"
#include "hardware/clocks.h"
#include "joybus.hpp"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "state.hpp"

controller_state state;

int main() {
  // Initialize clocks
  clocks_init();

  // Configure system PLL to 128 MHZ
  set_sys_clock_pll(1536 * MHZ, 6, 2);

  // Setup buttons, sticks, and triggers to be read
  init_buttons();
  init_sticks();
  init_triggers();

  // Load configuration
  controller_configuration &config = controller_configuration::get_instance();

  // Now that we have a configuration, select a profile if combo is held
  uint16_t startup_buttons = get_buttons();
  switch (startup_buttons) {
    case (1 << START) | (1 << A):
      config.select_profile(0);
      break;
    case (1 << START) | (1 << B):
      config.select_profile(1);
      break;
  }

  state.l_stick_coefficients =
      stick_calibration(config.l_stick_range,
                        config.l_stick_calibration_measurement)
          .generate_coefficients();
  state.r_stick_coefficients =
      stick_calibration(config.r_stick_range,
                        config.r_stick_calibration_measurement)
          .generate_coefficients();

  // Read buttons, sticks, and triggers once before starting communication
  read_digital(startup_buttons);
  read_triggers();
  read_sticks();

  // Start console communication
  joybus_init(pio0, JOYBUS_IN_PIN, JOYBUS_OUT_PIN);

  // Launch analog on core 1
  multicore_launch_core1(analog_main);

  while (true) {
    uint16_t physical_buttons = get_buttons();
    read_digital(physical_buttons);
    check_combos(physical_buttons);
  }

  return 0;
}

void read_digital(uint16_t physical_buttons) {
  controller_configuration &config = controller_configuration::get_instance();

  // Output sent to console
  uint16_t remapped_buttons = (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

  // Apply remaps
  remap(remapped_buttons, physical_buttons, START, config.mapping(12));
  remap(remapped_buttons, physical_buttons, Y, config.mapping(11));
  remap(remapped_buttons, physical_buttons, X, config.mapping(10));
  remap(remapped_buttons, physical_buttons, B, config.mapping(9));
  remap(remapped_buttons, physical_buttons, A, config.mapping(8));
  remap(remapped_buttons, physical_buttons, LT_DIGITAL, config.mapping(6));
  remap(remapped_buttons, physical_buttons, RT_DIGITAL, config.mapping(5));
  remap(remapped_buttons, physical_buttons, Z, config.mapping(4));
  remap(remapped_buttons, physical_buttons, DPAD_UP, config.mapping(3));
  remap(remapped_buttons, physical_buttons, DPAD_DOWN, config.mapping(2));
  remap(remapped_buttons, physical_buttons, DPAD_RIGHT, config.mapping(1));
  remap(remapped_buttons, physical_buttons, DPAD_LEFT, config.mapping(0));

  state.lt_pressed = (remapped_buttons & (1 << LT_DIGITAL)) != 0;
  state.rt_pressed = (remapped_buttons & (1 << RT_DIGITAL)) != 0;

  // Apply digital trigger modes
  apply_trigger_mode_digital(remapped_buttons, LT_DIGITAL,
                             config.l_trigger_mode(), config.r_trigger_mode());
  apply_trigger_mode_digital(remapped_buttons, RT_DIGITAL,
                             config.r_trigger_mode(), config.l_trigger_mode());

  // Update state
  state.buttons = remapped_buttons;
}

void remap(uint16_t &remapped_buttons, uint16_t physical_buttons,
           uint8_t to_remap, uint8_t mapping) {
  uint16_t mask = ~(1 << mapping);
  bool pressed = (physical_buttons & (1 << to_remap)) != 0;
  remapped_buttons = (remapped_buttons & mask) | (pressed << mapping);
}

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

void check_combos(uint32_t physical_buttons) {
  // Check if the active combo is still being pressed
  if (state.active_combo != 0) {
    if (state.active_combo == physical_buttons) {
      if (absolute_time_diff_us(state.combo_trigger_timestamp,
                                get_absolute_time()) > 0) {
        execute_combo();
      }
      return;
    } else {
      state.combo_trigger_timestamp = nil_time;
      state.active_combo = 0;
    }
  }

  // Check if the combo is to toggle safe mode
  if (physical_buttons == ((1 << START) | (1 << Y) | (1 << A) | (1 << Z))) {
    state.active_combo = physical_buttons;
    state.combo_trigger_timestamp = make_timeout_time_ms(3000);
    return;
  }

  // If not in safe mode, check other combos
  if (!state.safe_mode) {
    switch (physical_buttons) {
      case (1 << START) | (1 << X) | (1 << A):
      case (1 << START) | (1 << X) | (1 << Z):
      case (1 << START) | (1 << X) | (1 << LT_DIGITAL):
      case (1 << START) | (1 << X) | (1 << RT_DIGITAL):
      case (1 << START) | (1 << Y) | (1 << Z):
        state.active_combo = physical_buttons;
        state.combo_trigger_timestamp = make_timeout_time_ms(3000);
        break;
    }
  }
}

void execute_combo() {
  controller_configuration &config = controller_configuration::get_instance();

  state.display_alert();

  // Call appropriate function for combo
  switch (state.active_combo) {
    case (1 << START) | (1 << Y) | (1 << A) | (1 << Z):
      state.toggle_safe_mode();
      break;
    case (1 << START) | (1 << X) | (1 << A):
      config.swap_mappings();
      break;
    case (1 << START) | (1 << X) | (1 << Z):
      config.configure_triggers();
      break;
    case (1 << START) | (1 << X) | (1 << LT_DIGITAL):
      config.configure_stick(config.l_stick_range, state.l_stick_coefficients,
                             config.l_stick_calibration_measurement,
                             state.analog_sticks.r_stick, get_left_stick);
      break;
    case (1 << START) | (1 << X) | (1 << RT_DIGITAL):
      config.configure_stick(config.r_stick_range, state.r_stick_coefficients,
                             config.r_stick_calibration_measurement,
                             state.analog_sticks.l_stick, get_right_stick);
      break;
    case (1 << START) | (1 << Y) | (1 << B):
      controller_configuration::factory_reset();
      break;
  }

  state.active_combo = 0;
  state.combo_trigger_timestamp = nil_time;
}

void analog_main() {
  // Enable lockout
  multicore_lockout_victim_init();

  while (true) {
    read_triggers();
    read_sticks();
  }
}

void read_triggers() {
  controller_configuration &config = controller_configuration::get_instance();

  uint8_t l_trigger, r_trigger;

  // Read trigger values
  get_triggers(l_trigger, r_trigger);

  // Adjust trigger values based on center values
  l_trigger -= std::min(l_trigger, state.l_trigger_center);
  r_trigger -= std::min(r_trigger, state.r_trigger_center);

  // Apply analog trigger modes
  triggers new_triggers;
  new_triggers.l_trigger = apply_trigger_mode_analog(
      l_trigger, config.l_trigger_threshold_value(), state.lt_pressed,
      config.mapping(LT_DIGITAL) == LT_DIGITAL, config.l_trigger_mode(),
      config.r_trigger_mode());
  new_triggers.r_trigger = apply_trigger_mode_analog(
      r_trigger, config.r_trigger_threshold_value(), state.rt_pressed,
      config.mapping(RT_DIGITAL) == RT_DIGITAL, config.r_trigger_mode(),
      config.l_trigger_mode());
  state.analog_triggers = new_triggers;
}

uint8_t apply_trigger_mode_analog(uint8_t analog_value, uint8_t threshold_value,
                                  bool digital_value, bool enable_analog,
                                  trigger_mode mode, trigger_mode other_mode) {
  uint8_t out = 0;

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
        float multiplier =
            (threshold_value * TRIGGER_MULTIPLIER_M) + TRIGGER_MULTIPLIER_B;
        float multiplied_value = analog_value * multiplier;
        if (multiplied_value > 255) {
          out = 255 * enable_analog;
        } else {
          out = static_cast<uint8_t>(multiplied_value) * enable_analog;
        }
      }
      break;
  }

  return out;
}

void read_sticks() {
  controller_configuration &config = controller_configuration::get_instance();

  uint16_t lx, ly, rx, ry;
  get_sticks(lx, ly, rx, ry);

  sticks new_sticks;
  new_sticks.l_stick = process_raw_stick(lx, ly, state.l_stick_coefficients,
                                         config.l_stick_range);
  new_sticks.r_stick = process_raw_stick(rx, ry, state.r_stick_coefficients,
                                         config.r_stick_range);
  state.analog_sticks = new_sticks;
}

stick process_raw_stick(uint16_t x_raw, uint16_t y_raw,
                        stick_coefficients coefficients, uint8_t range) {
  double linearized_x = linearize_axis(x_raw, coefficients.x_coefficients);
  double linearized_y = linearize_axis(y_raw, coefficients.y_coefficients);

  return remap_stick(linearized_x, linearized_y, range);
}

double linearize_axis(uint16_t axis_raw,
                      std::array<double, NUM_COEFFICIENTS> axis_coefficients) {
  double linearized_axis = 0;
  for (int i = 0; i < NUM_COEFFICIENTS; ++i) {
    double raised_raw = 1;
    for (int j = 0; j < i; ++j) {
      raised_raw *= axis_raw;
    }
    linearized_axis += axis_coefficients[i] * raised_raw;
  }

  return linearized_axis;
}

stick remap_stick(double linearized_x, double linearized_y, uint8_t range) {
  controller_configuration &config = controller_configuration::get_instance();
  double min_value = CENTER - range;
  double max_value = CENTER + range;

  int x = round(std::clamp(linearized_x, min_value, max_value));
  int y = round(std::clamp(linearized_y, min_value, max_value));

  return stick{x : static_cast<uint8_t>(x), y : static_cast<uint8_t>(y)};
}
