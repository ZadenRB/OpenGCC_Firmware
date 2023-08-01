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

#include "calibration.hpp"

#include "curve_fitting.hpp"

stick_calibration::stick_calibration(stick &display)
    : current_step{0},
      num_steps{NUM_CALIBRATION_STEPS},
      display_stick(display),
      expected_x_coordinates({127, 227, 127, 197, 127, 127, 127, 57, 127, 27,
                              127, 57, 127, 127, 127, 197}),
      expected_y_coordinates({127, 127, 127, 197, 127, 227, 127, 197, 127, 127,
                              127, 57, 127, 27, 127, 57}) {}

void stick_calibration::display_step() {
    // Set display stick to expected x & y for current calibration step
    display_stick.x = expected_x_coordinates[current_step];
    display_stick.y = expected_y_coordinates[current_step];
}

void stick_calibration::undo_measurement() {
    if (current_step > 0) {
        measured_x_coordinates.pop_back();
        measured_y_coordinates.pop_back();
        skipped_steps.pop_back();
        --current_step;
    }
}

void stick_calibration::record_measurement(uint16_t x, uint16_t y) {
    if (current_step < num_steps) {
        measured_x_coordinates.push_back(x);
        measured_y_coordinates.push_back(y);
        skipped_steps.push_back(false);
        ++current_step;
    }
}

void stick_calibration::skip_measurement() {
    if (current_step < num_steps) {
        measured_x_coordinates.push_back(0);
        measured_y_coordinates.push_back(0);
        skipped_steps.push_back(true);
        ++current_step;
    }
}

void stick_calibration::remove_skipped_coordinates(
    std::vector<double> &coordinates) {
    for (int i = coordinates.size() - 1; i >= 0; --i) {
        if (skipped_steps[i]) {
            coordinates[i] = coordinates.back();
            coordinates.pop_back();
        }
    }
}

stick_coefficients stick_calibration::generate_coefficients() {
    stick_coefficients ret;

    remove_skipped_coordinates(measured_x_coordinates);
    remove_skipped_coordinates(expected_x_coordinates);
    remove_skipped_coordinates(measured_y_coordinates);
    remove_skipped_coordinates(expected_y_coordinates);

    ret.x_coefficients = fit_curve<NUM_COEFFICIENTS>(measured_x_coordinates,
                                                     expected_x_coordinates);
    ret.y_coefficients = fit_curve<NUM_COEFFICIENTS>(measured_y_coordinates,
                                                     expected_y_coordinates);

    return ret;
}

bool stick_calibration::done() { return current_step == num_steps; }
