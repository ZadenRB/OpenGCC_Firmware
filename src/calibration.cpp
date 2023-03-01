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
   NobGCC-SW If not, see http://www.gnu.org/licenses/.
*/

#include "calibration.hpp"

#include "curve_fitting.hpp"

stick_calibration::stick_calibration(stick &display) : display_stick(display) {
    current_step = 0;
    num_steps = NUM_CALIBRATION_STEPS;
}

void stick_calibration::display_step() {
    display_stick.x = expected_x_coordinates[current_step];
    display_stick.y = expected_y_coordinates[current_step];
}

void stick_calibration::undo_measurement() {
    measured_x_coordinates.pop_back();
    measured_y_coordinates.pop_back();
    skipped_steps.pop_back();
    if (current_step > 0) {
        --current_step;
    }
}

void stick_calibration::record_measurement(double x, double y) {
    measured_x_coordinates.push_back(x);
    measured_y_coordinates.push_back(y);
    skipped_steps.push_back(false);
    if (current_step < num_steps) {
        ++current_step;
    }
}

void stick_calibration::skip_measurement() {
    measured_x_coordinates.push_back(0);
    measured_y_coordinates.push_back(0);
    skipped_steps.push_back(true);
    if (current_step < num_steps) {
        ++current_step;
    }
}

void stick_calibration::remove_skipped_coordinates(
    std::vector<double> &coordinates) {
    for (int i = coordinates.size(); i >= 0; --i) {
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