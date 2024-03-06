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

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <array>
#include <vector>

#include "state.hpp"

/** \file calibration.hpp
 * \brief Controller calibration API
 */

/// \brief Number of steps in the calibration process
constexpr size_t NUM_CALIBRATION_STEPS = 16;

/// \brief Number of steps in the notch calibration process
constexpr size_t NUM_NOTCH_CALIBRATION_STEPS = 8;

constexpr uint8_t MIN_RANGE = 80;
constexpr uint8_t MAX_RANGE = 127;

/// \brief A set of calibration measurements
struct stick_calibration_measurement {
  std::array<uint16_t, NUM_CALIBRATION_STEPS> x_coordinates;
  std::array<uint16_t, NUM_CALIBRATION_STEPS> y_coordinates;
  std::array<bool, NUM_CALIBRATION_STEPS> skipped_measurements;
};

/// \brief Stick calibration implementation
class stick_calibration {
 private:
  size_t current_step;
  stick_calibration_measurement expected_measurement;
  stick_calibration_measurement actual_measurement;

 public:
  /** \brief Construct the stick calibration object
     *
     * \param range The maximum absolute value (offset from center) the calibrated stick should output
     */
  stick_calibration(uint8_t range);

  /** \brief Construct the stick calibration object with measurements
     *
     * \param range The maximum absolute value (offset from center) the calibrated stick should output
     * \param actual_measurement The measurements to use for calibration
     */
  stick_calibration(uint8_t range,
                    stick_calibration_measurement actual_measurement);

  /** \brief Display the target location for the current calibration step
     *
     * \param display_stick The stick to display calibration steps on
     */
  void display_step(stick &display_stick);

  /// \brief Go to the previous calibration step
  void undo_measurement();

  /** \brief Record a measurement for the current calibration step
     *
     * \param x X-axis value to record
     * \param y Y-axis value to record
     */
  void record_measurement(uint16_t x, uint16_t y);

  /// \brief Skip the current calibration step
  void skip_measurement();

  /** \brief Determine if calibration is complete
     * 
     * \return True if calibration is complete, otherwise false
     */
  bool done();

  /// \brief Return recorded measurement
  stick_calibration_measurement get_measurement();

  /**
     * \brief Generate coefficients based on calibration
     *
     * \note See src/curve_fitting.hpp for details on coefficient output
     *
     * \return Coefficients to linearize stick
     */
  stick_coefficients generate_coefficients();
};

#endif  // CALIBRATION_H_
