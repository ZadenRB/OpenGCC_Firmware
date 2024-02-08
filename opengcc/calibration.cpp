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

stick_calibration::stick_calibration(uint8_t range)
    : current_step{0} {
    uint16_t positive_cardinal = CENTER + range;
    uint16_t negative_cardinal = CENTER - range;
    uint16_t positive_diagonal = CENTER + (0.7 * range);
    uint16_t negative_diagonal = CENTER - (0.7 * range);

    expected_measurement.x_coordinates = {CENTER, positive_cardinal, CENTER, positive_diagonal, CENTER, CENTER, CENTER, negative_diagonal, CENTER, negative_cardinal,
                              CENTER, negative_diagonal, CENTER, CENTER, CENTER, positive_diagonal};
    expected_measurement.y_coordinates = {CENTER, CENTER, CENTER, positive_diagonal, CENTER, positive_cardinal, CENTER, positive_diagonal, CENTER, CENTER,
                            CENTER, negative_diagonal, CENTER, negative_cardinal, CENTER, negative_diagonal};
}

stick_calibration::stick_calibration(uint8_t range, stick_calibration_measurement actual_measurement)
    : current_step{0},
    actual_measurement(actual_measurement) {
    uint16_t positive_cardinal = CENTER + range;
    uint16_t negative_cardinal = CENTER - range;
    uint16_t positive_diagonal = CENTER + (0.7 * range);
    uint16_t negative_diagonal = CENTER - (0.7 * range);

    expected_measurement.x_coordinates = {CENTER, positive_cardinal, CENTER, positive_diagonal, CENTER, CENTER, CENTER, negative_diagonal, CENTER, negative_cardinal,
                              CENTER, negative_diagonal, CENTER, CENTER, CENTER, positive_diagonal};
    expected_measurement.y_coordinates = {CENTER, CENTER, CENTER, positive_diagonal, CENTER, positive_cardinal, CENTER, positive_diagonal, CENTER, CENTER,
                            CENTER, negative_diagonal, CENTER, negative_cardinal, CENTER, negative_diagonal};
}

void stick_calibration::display_step(stick &display_stick) {
    // Set display stick to expected x & y for current calibration step
    display_stick.x = expected_measurement.x_coordinates[current_step];
    display_stick.y = expected_measurement.y_coordinates[current_step];
}

void stick_calibration::undo_measurement() {
    if (current_step > 0) {
        --current_step;
    }
}

void stick_calibration::record_measurement(uint16_t x, uint16_t y) {
    if (current_step < NUM_CALIBRATION_STEPS) {
        actual_measurement.x_coordinates[current_step] = x;
        actual_measurement.y_coordinates[current_step] = y;
        actual_measurement.skipped_measurements[current_step] = false;
        ++current_step;
    }
}

void stick_calibration::skip_measurement() {
    if (current_step < NUM_CALIBRATION_STEPS) {
        actual_measurement.skipped_measurements[current_step] = true;
        ++current_step;
    }
}

stick_calibration_measurement stick_calibration::get_measurement() {
    return actual_measurement;
}

stick_coefficients stick_calibration::generate_coefficients() {
    stick_coefficients ret;

    ret.x_coefficients = fit_curve<NUM_COEFFICIENTS, NUM_CALIBRATION_STEPS>(expected_measurement.x_coordinates,
                                                     actual_measurement.x_coordinates,
                                                     actual_measurement.skipped_measurements);
    ret.y_coefficients = fit_curve<NUM_COEFFICIENTS, NUM_CALIBRATION_STEPS>(expected_measurement.y_coordinates,
                                                     actual_measurement.y_coordinates,
                                                     actual_measurement.skipped_measurements);

    return ret;
}

bool stick_calibration::done() { return current_step == NUM_CALIBRATION_STEPS; }
