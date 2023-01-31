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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <array>

#include "hardware/flash.h"
#include "state.hpp"

/** \file configuration.hpp
 * \brief Controller configuration API
 *
 * Controller settings that persist between controller reboots. Handles
 * loading, modifying, and persisting the settings. This includes:
 * * Button mappings
 * * Trigger modes
 */

/// \brief Enumeration of trigger modes
enum trigger_mode {
    both,          ///< Analog and digital output (OEM behavior)
    digital_only,  ///< Digital output only
    analog_only,   ///< Analog output only
    trigger_plug,  ///< Analog output to threshold, then digital output only
    analog_on_digital,  ///< Analog output only on digital press
    both_on_digital,    ///< Analog and digital output on digital press
    analog_multiplied,  ///< Analog and digital output, analog value scaled up
    last_trigger_mode =
        analog_multiplied,  ///< Set to last real value of enumeration
};

/// \brief Minimum value for trigger threshold, set to Melee Z shield value
constexpr uint8_t TRIGGER_THRESHOLD_MIN = 49;

/// \brief Maximum value for trigger threshold
constexpr uint8_t TRIGGER_THRESHOLD_MAX = 227;

/// \brief Number of coefficients for stick linearization
constexpr size_t NUM_COEFFICIENTS = 4;

/// \brief Number of calibration points
constexpr size_t NUM_CALIBRATION_POINTS = 9;

/// \brief Expected value for each calibration point on x axis
constexpr std::array<double, NUM_CALIBRATION_POINTS> EXPECTED_X = {
    0.00, 1.00, 0.71, 0.00, -0.71, -1.00, -0.71, 0.00, 0.71};

/// \brief Expected value for each calibration point on y axis
constexpr std::array<double, NUM_CALIBRATION_POINTS> EXPECTED_Y = {
    0.00, 0, 0.71, 1.00, 0.71, 0, -0.71, -1.00, -0.71};

/// \brief Calibration points for x & y axis of a stick.
struct stick_coefficients {
    /// \brief Coefficients for x axis linearization
    std::array<double, NUM_COEFFICIENTS> x_coefficients;

    /// \brief Coefficients for y axis of linearization
    std::array<double, NUM_COEFFICIENTS> y_coefficients;
};

/** \brief Current controller configuration
 *
 * \note Implemented as a singleton
 */
struct controller_configuration {
    /// \brief Button mappings
    std::array<uint8_t, 13> mappings;

    /// \brief Left trigger mode
    trigger_mode l_trigger_mode;

    /// \brief Left trigger threshold
    uint8_t l_trigger_threshold_value;

    /// \brief Right trigger mode
    trigger_mode r_trigger_mode;

    /// \brief Right trigger threshold
    uint8_t r_trigger_threshold_value;

    /// \brief A-stick linearization coefficients
    stick_coefficients a_stick;

    /// \brief C-stick linearization coefficients
    stick_coefficients c_stick;

    /** \brief Get the configuration instance
     * \return controller_configuration&
     */
    static controller_configuration &get_instance();

    controller_configuration(controller_configuration const &) = delete;
    controller_configuration(controller_configuration &&) = delete;
    controller_configuration &operator=(const controller_configuration &) =
        delete;
    controller_configuration &operator=(controller_configuration &&) = delete;

    /// \brief Persist the current configuration to flash
    void persist();

    /// \brief Enter remap mode
    void swap_mappings();

    /// \brief Enter trigger configuration mode
    void configure_triggers();

    /** \brief Enter stick configuration mode
     *
     * \param to_calibrate Output for coefficients
     * \param display_stick Stick to display calibration step to
     * \param x_raw Address to read x values from
     * \param y_raw Address to read y values from
     */
    void calibrate_stick(stick_coefficients &to_calibrate, stick &display_stick,
                         std::array<uint32_t, 2> const &x_raw,
                         std::array<uint32_t, 2> const &y_raw);

   private:
    controller_configuration();

    static uint32_t read_page();
    static uint32_t write_page();
};

/// \brief Flash address of first possible configuration
constexpr uint32_t CONFIG_FLASH_BASE =
    PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

/// \brief Memory address of first possible configuration
constexpr uint32_t CONFIG_SRAM_BASE =
    XIP_NOCACHE_NOALLOC_BASE + CONFIG_FLASH_BASE;

/// \brief Number of flash pages per flash sector
constexpr uint32_t PAGES_PER_SECTOR = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;

/// \brief Index of last page in a sector
constexpr uint32_t LAST_PAGE = PAGES_PER_SECTOR - 1;

/// \brief The number of bytes the controller configuration occupies
constexpr size_t CONFIG_SIZE = sizeof(controller_configuration);

#endif  // CONFIGURATION_H_