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

#include "curve_fitting.hpp"

void fit_curve(double coefficients[4], double measured_coordinates[],
               double expected_coordinates[], uint num_calibration_points) {
    double a[4][4] = {};

    for (int i = 0; i < 4 * 2 - 1; ++i) {
        double sum = 0;
        for (int j = 0; j < num_calibration_points; ++j) {
            double raised_measured_coordinate = 1;
            for (int k = 0; k < i; ++k) {
                raised_measured_coordinate *= measured_coordinates[j];
            }
            sum += raised_measured_coordinate;
        }

        int current_col = i;
        int current_row = 0;
        if (current_col > 4 - 1) {
            current_col = 4 - 1;
            current_row = i - current_col;
        }

        while (true) {
            a[current_col][current_row] = sum;
            --current_col;
            ++current_row;

            if (current_col < 0 || current_row > 4 - 1) {
                break;
            }
        }
    }

    double b[4] = {};

    for (int i = 0; i < 4; ++i) {
        double sum = 0;
        for (int j = 0; j < num_calibration_points; ++j) {
            double raised_measured_coordinate = 1;
            for (int k = 0; k < i; ++k) {
                raised_measured_coordinate *= measured_coordinates[j];
            }
            sum += raised_measured_coordinate * expected_coordinates[j];
        }
        b[i] = sum;
    }

    double inverse[4][4] = {};
    convert_to_inverse_4x4(inverse, a);

    for (int r = 0; r < 4; ++r) {
        double value = 0;
        for (int c = 0; c < 4; ++c) {
            value += inverse[c][r] * b[c];
        }
        coefficients[r] = value;
    }
}

void convert_to_inverse_4x4(double out[4][4], double in[4][4]) {
    // Fill inverse with input array
    double inverse[8][4] = {};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            inverse[c][r] = in[c][r];
        }
    }

    // Find inverse
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4 * 2; ++c) {
            if (c == (r + 4)) {
                inverse[c][r] = 1;
            }
        }
    }
    for (int r = 4 - 1; r > 0; --r) {
        if (inverse[0][r - 1] < inverse[0][r]) {
            for (int c = 0; c < 4 * 2; ++c) {
                double temp = inverse[c][r];
                inverse[c][r] = inverse[c][r - 1];
                inverse[c][r - 1] = temp;
            }
        }
    }
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (r != c) {
                double temp = inverse[c][r] / inverse[c][c];
                for (int i = 0; i < 4 * 2; ++i) {
                    inverse[i][r] -= inverse[i][c] * temp;
                }
            }
        }
    }
    for (int r = 0; r < 4; ++r) {
        double temp = inverse[r][r];
        for (int c = 0; c < 4 * 2; ++c) {
            inverse[c][r] = inverse[c][r] / temp;
        }
    }

    // Populate out array
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out[c][r] = inverse[c + 4][r];
        }
    }
}