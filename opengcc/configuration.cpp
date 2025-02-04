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

#include "configuration.hpp"

#include "calibration.hpp"
#include "controller.hpp"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "state.hpp"

controller_configuration::controller_configuration() {
  int read_page = controller_configuration::read_page();
  if (read_page == -1) {
    // If there's no stored configuration, load defaults & persist
    // Set up default profile
    configuration_profile default_profile;
    default_profile.mappings[0] = 0b0000;
    default_profile.mappings[1] = 0b0001;
    default_profile.mappings[2] = 0b0010;
    default_profile.mappings[3] = 0b0011;
    default_profile.mappings[4] = 0b0100;
    default_profile.mappings[5] = 0b0101;
    default_profile.mappings[6] = 0b0110;
    default_profile.mappings[7] = 0b0000;
    default_profile.mappings[8] = 0b1000;
    default_profile.mappings[9] = 0b1001;
    default_profile.mappings[10] = 0b1010;
    default_profile.mappings[11] = 0b1011;
    default_profile.mappings[12] = 0b1100;
    default_profile.l_trigger_mode = both;
    default_profile.l_trigger_configured_value = TRIGGER_CONFIGURED_VALUE_MIN;
    default_profile.r_trigger_mode = both;
    default_profile.r_trigger_configured_value = TRIGGER_CONFIGURED_VALUE_MIN;

    // Set all profiles to default
    for (int i = 0; i < profiles.size(); ++i) {
      profiles[i] = default_profile;
    }
    current_profile = 0;

    // Set coefficients and range to default
    l_stick_calibration_measurement.x_coordinates = {};
    l_stick_calibration_measurement.y_coordinates = {};
    l_stick_calibration_measurement.skipped_measurements = {};
    l_stick_range = 106;

    r_stick_calibration_measurement.x_coordinates = {};
    r_stick_calibration_measurement.y_coordinates = {};
    r_stick_calibration_measurement.skipped_measurements = {};
    r_stick_range = 106;

    // Persist
    persist();
  }

  // Load stored configuration
  controller_configuration *config_in_flash =
      reinterpret_cast<controller_configuration *>(
          CONFIG_SRAM_BASE + (read_page * FLASH_PAGE_SIZE));
  for (int i = 0; i < profiles.size(); ++i) {
    profiles[i] = config_in_flash->profiles[i];
  }
  current_profile = config_in_flash->current_profile;
  l_stick_calibration_measurement =
      config_in_flash->l_stick_calibration_measurement;
  l_stick_range = config_in_flash->l_stick_range;
  r_stick_calibration_measurement =
      config_in_flash->r_stick_calibration_measurement;
  r_stick_range = config_in_flash->r_stick_range;
}

controller_configuration &controller_configuration::get_instance() {
  static controller_configuration instance;
  return instance;
}

void controller_configuration::reload_instance() {
  get_instance() = controller_configuration();
}

int controller_configuration::read_page() {
  for (int page = 0; page < PAGES_PER_SECTOR; ++page) {
    uint32_t read_address = CONFIG_SRAM_BASE + (page * FLASH_PAGE_SIZE);
    if (*reinterpret_cast<uint8_t *>(read_address) == 0xFF) {
      // Return last initialized flash (-1 if no flash is initialized)
      --page;
      return page;
    }
  }

  // If we never find uninitialized flash, return the last page
  return LAST_PAGE;
}

int controller_configuration::write_page() {
  return (controller_configuration::read_page() + 1) % PAGES_PER_SECTOR;
}

void controller_configuration::persist() {
  int to_write = write_page();
  if (to_write == 0 && read_page() != -1) {
    flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
  }

  // Get current configuration as bytes
  uint8_t *config_bytes = reinterpret_cast<uint8_t *>(this);
  std::array<uint8_t, FLASH_PAGE_SIZE> buf = {0};
  // Fill buffer with the configuration bytes and pad with 0xFF
  for (int i = 0; i < FLASH_PAGE_SIZE; ++i) {
    if (i < CONFIG_SIZE) {
      buf[i] = config_bytes[i];
    } else {
      buf[i] = 0xFF;
    }
  }

  // Write to flash
  flash_range_program(CONFIG_FLASH_BASE + (to_write * FLASH_PAGE_SIZE),
                      buf.data(), FLASH_PAGE_SIZE);
}

uint8_t controller_configuration::mapping(size_t index) {
  return profiles[current_profile].mappings[index];
}

trigger_mode controller_configuration::l_trigger_mode() {
  return profiles[current_profile].l_trigger_mode;
}

uint8_t controller_configuration::l_trigger_configured_value() {
  return profiles[current_profile].l_trigger_configured_value;
}

trigger_mode controller_configuration::r_trigger_mode() {
  return profiles[current_profile].r_trigger_mode;
}

uint8_t controller_configuration::r_trigger_configured_value() {
  return profiles[current_profile].r_trigger_configured_value;
}

void controller_configuration::select_profile(size_t profile) {
  current_profile = profile;
  persist();
}

void wait_until_buttons_released(uint16_t buttons_mask = 0xFFFF) {
  absolute_time_t debounce_timeout_time = nil_time;

  while (is_nil_time(debounce_timeout_time) ||
         !time_reached(debounce_timeout_time)) {
    uint16_t physical_buttons = get_buttons() & buttons_mask;
    state.buttons =
        (physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN));

    // If no buttons are pressed, start the debounce timer
    if (physical_buttons == 0 && is_nil_time(debounce_timeout_time)) {
      debounce_timeout_time = make_timeout_time_ms(DEBOUNCE_TIME);
    }

    // If any buttons are pressed, stop the debounce timer
    if (physical_buttons != 0 && !is_nil_time(debounce_timeout_time)) {
      debounce_timeout_time = nil_time;
    }
  }
}

bool controller_configuration::check_persist_and_quit(
    uint16_t physical_buttons) {
  if (physical_buttons == (1 << START)) {
    persist();
    return true;
  }

  if (physical_buttons == (1 << X)) {
    reload_instance();
    return true;
  }

  return false;
}

void controller_configuration::swap_mappings() {
  uint32_t first_button = 0;
  uint32_t second_button = 0;

  wait_until_buttons_released();

  // Wait for first button to be set
  while (first_button == 0) {
    uint16_t physical_buttons = get_buttons();
    state.buttons =
        physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // If one button is pressed, set as first button
    if ((physical_buttons & (physical_buttons - 1)) == 0) {
      first_button = physical_buttons;
    }
  }

  wait_until_buttons_released();

  // Wait for second button to be set
  while (second_button == 0) {
    uint16_t physical_buttons = get_buttons();
    state.buttons =
        physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // If one button is pressed, set as second button
    if ((physical_buttons & (physical_buttons - 1)) == 0) {
      second_button = physical_buttons;
    }
  }

  // Get first button's mapping
  uint8_t first_mapping_index = 0;
  while ((first_button >> first_mapping_index) > 1) {
    ++first_mapping_index;
  }
  uint8_t first_mapping =
      profiles[current_profile].mappings[first_mapping_index];

  // Get second button's mapping
  uint8_t second_mapping_index = 0;
  while ((second_button >> second_mapping_index) > 1) {
    ++second_mapping_index;
  }
  uint8_t second_mapping =
      profiles[current_profile].mappings[second_mapping_index];

  // Swap mappings
  profiles[current_profile].mappings[first_mapping_index] = second_mapping;
  profiles[current_profile].mappings[second_mapping_index] = first_mapping;

  persist();
  state.display_alert();
}

void controller_configuration::configure_triggers() {
  // Lock core 1 to prevent analog trigger output from being displayed
  multicore_lockout_start_blocking();

  wait_until_buttons_released();

  state.analog_triggers.l_trigger = 0;
  state.analog_triggers.r_trigger = 0;

  while (true) {
    uint16_t physical_buttons = get_buttons();
    state.buttons =
        (physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN)) &
        ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL));

    // Quit if needed
    bool quit = check_persist_and_quit(physical_buttons);
    if (quit) {
      multicore_lockout_end_blocking();
      state.display_alert();
      return;
    }

    // Mask out non-trigger buttons
    uint32_t trigger_pressed =
        physical_buttons & ((1 << LT_DIGITAL) | (1 << RT_DIGITAL));

    if (trigger_pressed != 0 &&
        (trigger_pressed & (trigger_pressed - 1)) == 0) {
      // If only one trigger is pressed, set it as the one to modify
      trigger_mode *mode;
      uint8_t *configured_value;
      if (trigger_pressed == (1 << LT_DIGITAL)) {
        mode = &(profiles[current_profile].l_trigger_mode);
        configured_value =
            &(profiles[current_profile].l_trigger_configured_value);
      } else if (trigger_pressed == (1 << RT_DIGITAL)) {
        mode = &(profiles[current_profile].r_trigger_mode);
        configured_value =
            &(profiles[current_profile].r_trigger_configured_value);
      }

      // Mask out trigger buttons
      uint32_t combo =
          physical_buttons & ~((1 << LT_DIGITAL) | (1 << RT_DIGITAL));

      int new_configured_value = *configured_value;
      int new_mode = *mode;

      // Update mode/configured value based on combo
      switch (combo) {
        case (1 << A):
          new_mode += 1U;
          break;
        case (1 << B):
          new_mode -= 1U;
          break;
        case (1 << DPAD_UP):
          new_configured_value += 1U;
          break;
        case (1 << DPAD_RIGHT):
          new_configured_value += 10U;
          break;
        case (1 << DPAD_DOWN):
          new_configured_value -= 1U;
          break;
        case (1 << DPAD_LEFT):
          new_configured_value -= 10U;
          break;
      }

      // Wrap trigger mode
      if (new_mode < first_trigger_mode) {
        *mode = last_trigger_mode;
      } else if (new_mode > last_trigger_mode) {
        *mode = first_trigger_mode;
      } else {
        *mode = static_cast<trigger_mode>(new_mode);
      }

      // Wrap configured value
      if (new_configured_value < TRIGGER_CONFIGURED_VALUE_MIN) {
        *configured_value =
            (TRIGGER_CONFIGURED_VALUE_MAX + 1) -
            (TRIGGER_CONFIGURED_VALUE_MIN - new_configured_value);
      } else if (new_configured_value > TRIGGER_CONFIGURED_VALUE_MAX) {
        *configured_value =
            (TRIGGER_CONFIGURED_VALUE_MIN - 1) +
            (new_configured_value - TRIGGER_CONFIGURED_VALUE_MAX);
      } else {
        *configured_value = new_configured_value;
      }

      // Display mode on left trigger & offset on right trigger
      state.analog_triggers.l_trigger = *mode;
      state.analog_triggers.r_trigger = *configured_value;

      // Wait for buttons to be released when a combo is pressed
      if ((physical_buttons &
           ((1 << A) | (1 << B) | (1 << DPAD_UP) | (1 << DPAD_RIGHT) |
            (1 << DPAD_DOWN) | (1 << DPAD_LEFT))) != 0) {
        wait_until_buttons_released(~((1 << LT_DIGITAL) | (1 << RT_DIGITAL)));
      }
    } else {
      // Otherwise display nothing
      state.analog_triggers.l_trigger = 0;
      state.analog_triggers.r_trigger = 0;
    }
  }
}

void controller_configuration::configure_stick(
    uint8_t &range_out, stick_coefficients &coefficients_out,
    stick_calibration_measurement &measurement_out, stick &display_stick,
    std::function<raw_stick()> get_stick) {
  // Lock core 1 to prevent stick output from being displayed
  multicore_lockout_start_blocking();

  wait_until_buttons_released();

  while (true) {
    uint16_t physical_buttons = get_buttons();
    state.buttons =
        physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // Quit if needed
    bool quit = check_persist_and_quit(physical_buttons);
    if (quit) {
      multicore_lockout_end_blocking();
      state.display_alert();
      return;
    }

    // Move onto calibration when Z is pressed
    if (physical_buttons == (1 << Z)) {
      state.analog_triggers.l_trigger = 0;
      break;
    }

    int new_range = range_out;

    // Update range based on combo
    switch (physical_buttons) {
      case (1 << DPAD_UP):
        new_range += 1;
        break;
      case (1 << DPAD_RIGHT):
        new_range += 10;
        break;
      case (1 << DPAD_DOWN):
        new_range -= 1;
        break;
      case (1 << DPAD_LEFT):
        new_range -= 10;
        break;
    }

    // Wrap range
    if (new_range < MIN_RANGE) {
      range_out = (MAX_RANGE + 1) - (MIN_RANGE - new_range);
    } else if (new_range > MAX_RANGE) {
      range_out = (MIN_RANGE - 1) + (new_range - MAX_RANGE);
    } else {
      range_out = new_range;
    }

    // Display range on left trigger
    state.analog_triggers.l_trigger = range_out;

    // Wait for buttons to be released when a combo is pressed
    if ((physical_buttons & ((1 << DPAD_UP) | (1 << DPAD_RIGHT) |
                             (1 << DPAD_DOWN) | (1 << DPAD_LEFT))) != 0) {
      wait_until_buttons_released();
    }
  }

  stick_calibration calibration(range_out);

  while (!calibration.done()) {
    // Show current step on display stick
    calibration.display_step(display_stick);

    uint16_t physical_buttons = get_buttons();
    state.buttons =
        physical_buttons | (1 << ALWAYS_HIGH) | (state.origin << ORIGIN);

    // Quit if needed
    bool quit = check_persist_and_quit(physical_buttons);
    if (quit) {
      multicore_lockout_end_blocking();
      state.display_alert();
      return;
    }

    // Handle combos
    switch (physical_buttons) {
      case (1 << B):
        calibration.undo_measurement();
        break;
      case (1 << Z): {
        raw_stick stick_data = get_stick();
        calibration.record_measurement(stick_data.x, stick_data.y);
        break;
      }
      case (1 << A):
        calibration.skip_measurement();
        break;
    }

    // Wait for buttons to be released when a combo is pressed
    if ((physical_buttons & ((1 << B) | (1 << Z) | (1 << A))) != 0) {
      wait_until_buttons_released();
    }
  }

  coefficients_out = calibration.generate_coefficients();
  measurement_out = calibration.get_measurement();

  persist();
  multicore_lockout_end_blocking();
  state.display_alert();
}

void controller_configuration::factory_reset() {
  flash_range_erase(CONFIG_FLASH_BASE, FLASH_SECTOR_SIZE);
  reload_instance();
  state = controller_state();
}
