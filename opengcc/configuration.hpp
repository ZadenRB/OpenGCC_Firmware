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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <array>
#include <functional>

#include "calibration.hpp"
#include "controller.hpp"
#include "hardware/flash.h"
#include "state.hpp"

/** \file configuration.hpp
 * \brief Controller configuration API
 *
 * Controller settings that persist between controller reboots. Handles
 * loading, modifying, and persisting the settings. This includes:
 * * Controller profiles
 * * Button mappings
 * * Trigger modes
 * * Calibration
 */

/// \brief Enumeration of trigger modes
enum trigger_mode : uint {
  both,          ///< Analog and digital output (OEM behavior)
  digital_only,  ///< Digital output only
  analog_only,   ///< Analog output only
  capped_analog,  ///< Analog output limited to configured value and digital output
  analog_on_digital,  ///< Analog output only on digital press
  both_on_digital,    ///< Analog and digital output on digital press
  multiplied_analog,  ///< Analog and digital output, analog value scaled up
  last_trigger_mode = multiplied_analog,  ///< Set to last value of enumeration
  first_trigger_mode = both               ///< Set to first value of enumeration
};

/// \brief Minimum for trigger configured value, set to Melee Z shield value
constexpr uint8_t TRIGGER_CONFIGURED_VALUE_MIN = 49;

/// \brief Maximum for trigger configured value
constexpr uint8_t TRIGGER_CONFIGURED_VALUE_MAX = 209;

/// \brief Multiplied by configured value for analog multiplication trigger mode
constexpr float TRIGGER_MULTIPLIER_M = .0125f;

/// \brief Added to configured value for analog multiplication trigger mode
constexpr float TRIGGER_MULTIPLIER_B = 0.3875f;

/** \brief Settings which a player might change when playing different games
 *
 * Essentially stores non-calibration settings, as sticks should always be
 * calibrated the same for a given controller.
 */
struct configuration_profile {
  /// \brief Button mappings
  std::array<uint8_t, 13> mappings;

  /// \brief Left trigger mode
  trigger_mode l_trigger_mode;

  /// \brief Left trigger configured value
  uint8_t l_trigger_configured_value;

  /// \brief Right trigger mode
  trigger_mode r_trigger_mode;

  /// \brief Right trigger configured value
  uint8_t r_trigger_configured_value;
};

/** \brief Current controller configuration
 *
 * \note Implemented as a singleton
 */
class controller_configuration {
 private:
  controller_configuration();
  controller_configuration &operator=(controller_configuration &&) = default;

  static int read_page();
  static int write_page();

 public:
  /// \brief Profiles
  std::array<configuration_profile, 2> profiles;

  /// \brief Current profile
  size_t current_profile;

  /// \brief Left stick calibration measurement
  stick_calibration_measurement l_stick_calibration_measurement;

  /// \brief Left stick output range
  uint8_t l_stick_range;

  /// \brief Right stick calibration measurement
  stick_calibration_measurement r_stick_calibration_measurement;

  /// \brief Right stick output range
  uint8_t r_stick_range;

  /** \brief Get the configuration instance
     *
     * \return The controller's configuration
     */
  static controller_configuration &get_instance();

  /** \brief Reload the configuration from flash/defaults
     *
     * \note Should only be used if the last sector of flash, which contains
     * configurations, is written to without updating the configuration in
     * memory accordingly.
     */
  static void reload_instance();

  controller_configuration(const controller_configuration &) = delete;
  controller_configuration(const controller_configuration &&) = delete;
  controller_configuration &operator=(const controller_configuration &) =
      delete;

  /// \brief Persist the current configuration to flash
  void persist();

  /** \brief Check buttons, determine whether to quit and save if needed
     * 
     * \param physical_buttons Physical button states
     * 
     * \return True if configuration mode should be quit, false otherwise
     */
  bool check_persist_and_quit(uint16_t physical_buttons);

  /** \brief Get a mapping by index for current profile
     *
     * \param index index of mapping to get
     *
     * \return Mapping for given index in the current profile
     */
  uint8_t mapping(size_t index);

  /** \brief Left trigger mode for current profile
     *
     * \return The left trigger mode
     */
  trigger_mode l_trigger_mode();

  /** \brief Left trigger configured value for current profile
     *
     * \return The left trigger configured value
     */
  uint8_t l_trigger_configured_value();

  /** \brief Right trigger mode for current profile
     *
     * \return The right trigger mode
     */
  trigger_mode r_trigger_mode();

  /** \brief Right trigger configured value for current profile
     *
     * \return The right trigger configured value
     */
  uint8_t r_trigger_configured_value();

  /// \brief Set the current profile to the given one
  void select_profile(size_t profile);

  /// \brief Enter remap mode
  void swap_mappings();

  /// \brief Enter trigger configuration mode
  void configure_triggers();

  /** \brief Enter stick configuration mode
     *
     * \param range_out Output for range
     * \param coefficients_out Output for coefficients
     * \param measurement_out Output for measurement
     * \param display_stick Stick to display calibration (stick not being
     * calibrated)
     * \param get_stick Function to get stick
     */
  void configure_stick(uint8_t &range_out, stick_coefficients &coefficients_out,
                       stick_calibration_measurement &measurement_out,
                       stick &display_stick,
                       std::function<raw_stick()> get_stick);

  /// \brief Erase all stored configurations
  static void factory_reset();
};

/// \brief Flash address of first possible configuration
constexpr uint32_t CONFIG_FLASH_BASE =
    PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

/// \brief Memory-mapped address of first possible configuration
constexpr uint32_t CONFIG_SRAM_BASE =
    XIP_NOCACHE_NOALLOC_BASE + CONFIG_FLASH_BASE;

/// \brief Number of flash pages per flash sector
constexpr uint32_t PAGES_PER_SECTOR = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;

/// \brief Index of last page in a sector
constexpr uint32_t LAST_PAGE = PAGES_PER_SECTOR - 1;

/// \brief The number of bytes the controller configuration occupies
constexpr size_t CONFIG_SIZE = sizeof(controller_configuration);

/** \brief How many milliseconds to debounce on button releases to prevent
 * double presses when configuring
 */
constexpr uint DEBOUNCE_TIME = 50;
#endif  // CONFIGURATION_H_
