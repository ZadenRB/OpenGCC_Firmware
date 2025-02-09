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

#include "main.hpp"

#include CONFIG_H

#include <algorithm>
#include <cmath>

#include "calibration.hpp"
#include "configuration.hpp"
#include "hardware/clocks.h"
#include "joybus.hpp"
#include "pico/multicore.h"
#include "state.hpp"

controller_state state;

int main() {
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
                             config.l_trigger_mode());
  apply_trigger_mode_digital(remapped_buttons, RT_DIGITAL,
                             config.r_trigger_mode());

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
                                trigger_mode mode) {
  switch (mode) {
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
  raw_triggers trigger_data = get_triggers();

  // Adjust trigger values based on center values
  l_trigger -= std::min(trigger_data.l, state.l_trigger_center);
  r_trigger -= std::min(trigger_data.r, state.r_trigger_center);

  // Apply analog trigger modes
  triggers new_triggers;
  new_triggers.l_trigger = apply_trigger_mode_analog(
      l_trigger, config.l_trigger_configured_value(), state.lt_pressed,
      config.mapping(LT_DIGITAL) == LT_DIGITAL, config.l_trigger_mode());
  new_triggers.r_trigger = apply_trigger_mode_analog(
      r_trigger, config.r_trigger_configured_value(), state.rt_pressed,
      config.mapping(RT_DIGITAL) == RT_DIGITAL, config.r_trigger_mode());
  state.analog_triggers = new_triggers;
}

uint8_t apply_trigger_mode_analog(uint8_t analog_value,
                                  uint8_t configured_value, bool digital_value,
                                  bool enable_analog, trigger_mode mode) {
  uint8_t out = 0;

  switch (mode) {
    case digital_only:
      out = 0;
      break;
    case both:
    case analog_only:
      out = analog_value * enable_analog;
      break;
    case capped_analog:
      if (analog_value > configured_value) {
        out = configured_value * enable_analog;
      } else {
        out = analog_value * enable_analog;
      }
      break;
    case analog_on_digital:
    case both_on_digital:
      out = configured_value * digital_value * enable_analog;
      break;
    case multiplied_analog:
      float multiplier =
          (configured_value * TRIGGER_MULTIPLIER_M) + TRIGGER_MULTIPLIER_B;
      float multiplied_value = analog_value * multiplier;
      if (multiplied_value > 255) {
        out = 255 * enable_analog;
      } else {
        out = static_cast<uint8_t>(multiplied_value) * enable_analog;
      }
      break;
  }

  return out;
}

void read_sticks() {
  controller_configuration &config = controller_configuration::get_instance();

  raw_sticks sticks_data = get_sticks();

  sticks new_sticks;
  new_sticks.l_stick =
      process_raw_stick(sticks_data.l_stick, state.analog_sticks.l_stick,
                        state.l_stick_coefficients,
                        state.l_stick_snapback_state, config.l_stick_range);

  new_sticks.r_stick =
      process_raw_stick(sticks_data.r_stick, state.analog_sticks.r_stick,
                        state.r_stick_coefficients,
                        state.r_stick_snapback_state, config.r_stick_range);
  state.analog_sticks = new_sticks;
}

stick process_raw_stick(raw_stick stick_data, stick previous_stick,
                        stick_coefficients coefficients,
                        stick_snapback_state &snapback_state, uint8_t range) {
  if (!stick_data.fresh) {
    return previous_stick;
  }

  double normalized_x =
      normalize_axis(stick_data.x, coefficients.x_coefficients);
  double normalized_y =
      normalize_axis(stick_data.y, coefficients.y_coefficients);

  return remap_stick(normalized_x, normalized_y, snapback_state, range);
}

double normalize_axis(uint16_t raw_axis,
                      std::array<double, NUM_COEFFICIENTS> axis_coefficients) {
  double normalized_axis = 0;
  for (int i = 0; i < NUM_COEFFICIENTS; ++i) {
    double raised_raw = 1;
    for (int j = 0; j < i; ++j) {
      raised_raw *= raw_axis;
    }
    normalized_axis += axis_coefficients[i] * raised_raw;
  }

  return normalized_axis;
}

stick remap_stick(double normalized_x, double normalized_y,
                  stick_snapback_state &snapback_state, uint8_t range) {
  controller_configuration &config = controller_configuration::get_instance();

  precise_stick unsnapped_stick =
      unsnap_stick(normalized_x, normalized_y, snapback_state);

  double min_value = CENTER - range;
  double max_value = CENTER + range;

  uint8_t x = round(std::clamp(unsnapped_stick.x, min_value, max_value));
  uint8_t y = round(std::clamp(unsnapped_stick.y, min_value, max_value));

  return stick{x, y};
}

precise_stick unsnap_stick(double normalized_x, double normalized_y,
                           stick_snapback_state &snapback_state) {
  absolute_time_t now = get_absolute_time();

  double x_displacement = normalized_x - CENTER;
  double y_displacement = normalized_y - CENTER;
  double x_distance = fabs(x_displacement);
  double y_distance = fabs(y_displacement);

  if (x_distance >= SNAPBACK_DISTANCE) {
    snapback_state.x.eligible_to_snapback = true;
    snapback_state.x.last_eligible_to_snapback = now;
  } else if (!is_nil_time(snapback_state.x.last_eligible_to_snapback) &&
             absolute_time_diff_us(snapback_state.x.last_eligible_to_snapback,
                                   now) >= SNAPBACK_ELIGIBILITY_TIMEOUT_US) {
    snapback_state.x.eligible_to_snapback = false;
    snapback_state.x.last_eligible_to_snapback = nil_time;
  }

  if (y_distance >= SNAPBACK_DISTANCE) {
    snapback_state.y.eligible_to_snapback = true;
    snapback_state.y.last_eligible_to_snapback = now;
  } else if (!is_nil_time(snapback_state.y.last_eligible_to_snapback) &&
             absolute_time_diff_us(snapback_state.y.last_eligible_to_snapback,
                                   now) >= SNAPBACK_ELIGIBILITY_TIMEOUT_US) {
    snapback_state.y.eligible_to_snapback = false;
    snapback_state.y.last_eligible_to_snapback = nil_time;
  }

  double unsnapped_x = unsnap_axis(normalized_x, x_displacement, x_distance,
                                   y_distance, now, snapback_state.x);
  double unsnapped_y = unsnap_axis(normalized_y, y_displacement, y_distance,
                                   x_distance, now, snapback_state.y);

  return {unsnapped_x, unsnapped_y};
}

bool axis_crossed_center(double displacement, double last_displacement,
                         double other_axis_distance) {
  return std::signbit(displacement) != std::signbit(last_displacement) &&
         other_axis_distance <= CROSSING_DISTANCE;
}

double unsnap_axis(double normalized_axis, double axis_displacement,
                   double axis_distance, double other_axis_distance,
                   absolute_time_t now, axis_snapback_state &snapback_state) {
  double last_distance = fabs(snapback_state.last_displacement);

  if (snapback_state.eligible_to_snapback &&
      axis_crossed_center(axis_displacement, snapback_state.last_displacement,
                          other_axis_distance)) {
    snapback_state.falling = false;
    snapback_state.falling_count = 0;
    snapback_state.wave_started_at = now;
    snapback_state.wave_expires_at =
        delayed_by_us(now, DEFAULT_WAVE_DURATION_US);
    snapback_state.in_snapback = true;
    snapback_state.eligible_to_snapback = false;
  } else if (snapback_state.in_snapback) {
    bool snapback_expired =
        absolute_time_diff_us(snapback_state.wave_expires_at, now) >= 0;

    if (snapback_expired) {
      snapback_state.in_snapback = false;
      snapback_state.eligible_to_snapback = false;
    } else if (!snapback_state.falling) {
      if (axis_distance <= last_distance &&
          axis_distance >= CENTERED_DISTANCE) {
        snapback_state.falling_count += 1;
      } else {
        snapback_state.falling_count = 0;
      }

      if (snapback_state.falling_count >= 3) {
        snapback_state.falling = true;
        snapback_state.falling_count = 0;
        uint64_t new_wave_duration =
            absolute_time_diff_us(snapback_state.wave_started_at, now) +
            SNAPBACK_WAVE_DURATION_BUFFER;
        snapback_state.wave_expires_at = delayed_by_us(now, new_wave_duration);
      }
    }
  }

  snapback_state.last_displacement = axis_displacement;

  if (snapback_state.in_snapback) {
    return CENTER;
  }

  return normalized_axis;
}
