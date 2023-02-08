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

#ifndef _CURVE_FITTING_H_
#define _CURVE_FITTING_H_

#include <array>
#include <cassert>
#include <vector>

/** \file curve_fitting.hpp
 * \brief Curve fitting functionality
 */

/** \brief Calculate the inverse of a matrix
 *
 * \tparam N Dimension of array
 * \param out Output for the inverse
 * \param in Matrix to calculate the inverse of
 */
template <std::size_t N>
void convert_to_inverse(std::array<std::array<double, N>, N>& out,
                        const std::array<std::array<double, N>, N>& in) {
    // Fill inverse with input array
    std::array<std::array<double, N>, N* 2> inverse = {};
    for (std::size_t r = 0; r < N; ++r) {
        for (std::size_t c = 0; c < N; ++c) {
            inverse[c][r] = in[c][r];
        }
    }

    // Find inverse
    for (std::size_t r = 0; r < N; ++r) {
        for (std::size_t c = 0; c < N * 2; ++c) {
            if (c == (r + N)) {
                inverse[c][r] = 1;
            }
        }
    }
    for (std::size_t r = N - 1; r > 0; --r) {
        if (inverse[0][r - 1] < inverse[0][r]) {
            for (std::size_t c = 0; c < N * 2; ++c) {
                double temp = inverse[c][r];
                inverse[c][r] = inverse[c][r - 1];
                inverse[c][r - 1] = temp;
            }
        }
    }
    for (std::size_t c = 0; c < N; ++c) {
        for (std::size_t r = 0; r < N; ++r) {
            if (r != c) {
                double temp = inverse[c][r] / inverse[c][c];
                for (std::size_t i = 0; i < N * 2; ++i) {
                    inverse[i][r] -= inverse[i][c] * temp;
                }
            }
        }
    }
    for (std::size_t r = 0; r < N; ++r) {
        double temp = inverse[r][r];
        for (std::size_t c = 0; c < N * 2; ++c) {
            inverse[c][r] = inverse[c][r] / temp;
        }
    }

    // Populate out array
    for (std::size_t r = 0; r < N; ++r) {
        for (std::size_t c = 0; c < N; ++c) {
            out[c][r] = inverse[c + N][r];
        }
    }
}

/** \brief Generates coefficients for an polynomial to map
 * `measured_coefficients` to `expected_coordinates` via regression
 *
 * Produces coefficients for a function
 * `coefficients[0]*x^0 + ... + coefficients[i]*x^i + ... + `
 * `coefficients[NCoefficients-1]*x^(NCoefficients-1)`.
 * Used to linearize Hall-Effect sensor output.
 *
 * \note Increasing `NCoefficients` will increase runtime of sensor
 * linearization which is run on every poll, so there is a tradeoff between
 * accuracy and performance.
 *
 * \tparam NCoefficients Number of coefficients to generate
 * \param coefficients Output array for coefficients
 * \param measured_coordinates The measured input coordinates
 * \param expected_coordinates The expected output coordinates
 */
template <std::size_t NCoefficients>
void fit_curve(std::array<double, NCoefficients>& coefficients,
               const std::vector<double>& measured_coordinates,
               const std::vector<double>& expected_coordinates) {
    assert(measured_coordinates.size() == expected_coordinates.size());
    size_t num_coordinates = measured_coordinates.size();

    std::array<std::array<double, NCoefficients>, NCoefficients> a = {};

    for (std::size_t i = 0; i < NCoefficients * 2 - 1; ++i) {
        double sum = 0;
        for (double measured_coordinate : measured_coordinates) {
            double raised_measured_coordinate = 1;
            for (uint32_t k = 0; k < i; ++k) {
                raised_measured_coordinate *= measured_coordinate;
            }
            sum += raised_measured_coordinate;
        }

        std::size_t current_col = i;
        std::size_t current_row = 0;
        if (current_col > NCoefficients - 1) {
            current_col = NCoefficients - 1;
            current_row = i - current_col;
        }

        while (true) {
            a[current_col][current_row] = sum;
            --current_col;
            ++current_row;

            if (current_col < 0 || current_row > NCoefficients - 1) {
                break;
            }
        }
    }

    std::array<double, NCoefficients> b = {};

    for (std::size_t i = 0; i < NCoefficients; ++i) {
        double sum = 0;
        for (std::size_t j = 0; j < num_coordinates; ++j) {
            double raised_measured_coordinate = 1;
            for (uint32_t k = 0; k < i; ++k) {
                raised_measured_coordinate *= measured_coordinates[j];
            }
            sum += raised_measured_coordinate * expected_coordinates[j];
        }
        b[i] = sum;
    }

    std::array<std::array<double, NCoefficients>, NCoefficients> inverse = {};
    convert_to_inverse(inverse, a);

    for (std::size_t r = 0; r < NCoefficients; ++r) {
        double value = 0;
        for (std::size_t c = 0; c < NCoefficients; ++c) {
            value += inverse[c][r] * b[c];
        }
        coefficients[r] = value;
    }
}

#endif  // CURVE_FITTING_H_