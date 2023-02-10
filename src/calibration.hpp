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

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <array>
#include <vector>

#include "configuration.hpp"
#include "state.hpp"

/** \file calibration.hpp
 * \brief Controller calibration API
 */

/// \brief Default number of steps in the calibration process
constexpr size_t NUM_CALIBRATION_STEPS = 9;

/// \brief Stick calibration implementation
class stick_calibration {
   private:
    size_t current_step;
    size_t num_steps;
    std::vector<double> expected_x_coordinates;
    std::vector<double> expected_y_coordinates;
    std::vector<double> measured_x_coordinates;
    std::vector<double> measured_y_coordinates;
    std::vector<bool> skipped_steps;
    stick &display_stick;

   public:
    /** \brief Construct a new stick calibration object
     *
     * \param display The stick to display calibration steps on
     */
    stick_calibration(stick &display);

    /// \brief Display the target location for the current calibration step
    void display_step();

    /// \brief Go to the previous calibration step
    void undo_measurement();

    /** \brief Record a measurement for the current calibration step
     *
     * \param x X-axis value to record
     * \param y Y-axis value to record
     */
    void record_measurement(double x, double y);

    /// \brief Skip the current calibration step
    void skip_measurement();

    /// \brief Is calibration done
    bool done();

    /**
     * \brief Remove coordinates which were skipped
     *
     * \param coordinates Vector of coordinates to remove skipped indices from
     */
    void remove_skipped_coordinates(std::vector<double> &coordinates);

    /**
     * \brief Generate coefficients based on calibration
     *
     * \note See src/curve_fitting.hpp for details on coefficient output
     */
    void generate_coefficients(stick_coefficients &out);
};

#endif  // CALIBRATION_H_