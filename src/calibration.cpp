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
    skipped_steps.erase(current_step);
    --current_step;
}

void stick_calibration::record_measurement(double x, double y) {
    measured_x_coordinates.push_back(x);
    measured_y_coordinates.push_back(y);
    ++current_step;
}

void stick_calibration::skip_measurement() {
    measured_x_coordinates.push_back(0);
    measured_y_coordinates.push_back(0);
    skipped_steps.insert(current_step);
    ++current_step;
}

void stick_calibration::remove_skipped_coordinates(
    std::vector<double> &coordinates) {
    for (auto i = skipped_steps.rbegin(); i != skipped_steps.rend(); ++i) {
        coordinates[*i] = coordinates.back();
        coordinates.pop_back();
    }
}

void stick_calibration::generate_coefficients(stick_coefficients &out) {
    remove_skipped_coordinates(measured_x_coordinates);
    remove_skipped_coordinates(expected_x_coordinates);
    remove_skipped_coordinates(measured_y_coordinates);
    remove_skipped_coordinates(expected_y_coordinates);

    fit_curve(out.x_coefficients, measured_x_coordinates,
              expected_x_coordinates);
    fit_curve(out.y_coefficients, measured_y_coordinates,
              expected_y_coordinates);
}

bool stick_calibration::done() { return current_step == num_steps; }