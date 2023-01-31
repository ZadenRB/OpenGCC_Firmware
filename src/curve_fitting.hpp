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

#include <stdint.h>

#include <array>

/** \file curve_fitting.hpp
 * \brief Curve fitting functionality
 */

/** \brief Generates coefficients for an polynomial to map
 * `measured_coefficients` to `expected_coordinates` via regression
 *
 * Produces coefficients for a function
 * `coefficients[0]*x^0 + ... + coefficients[i]*x^i + ... + `
 * `coefficients[NCoefficients-1]*x^(NCoefficients-1)`.
 * Used to linearize Hall-Effect sensor output.
 *
 * \note Increasing `NCoefficients` will affect runtime of `fit_curve`, but it
 * is a once-per-calibration operation. However, increasing `NCoefficients` will
 * increase runtime of sensor linearization which is run on every poll, so there
 * is a tradeoff between accuracy and performance.
 *
 * \tparam NCoordinates Number of measured/expected coordinates
 * \tparam NCoefficients Number of coefficients to generate
 * \param coefficients Output array for coefficients
 * \param measured_coordinates The measured input coordinates
 * \param expected_coordinates The expected output coordinates
 */
template <std::size_t NCoordinates, std::size_t NCoefficients>
void fit_curve(std::array<double, NCoefficients>& coefficients,
               std::array<double, NCoordinates> const& measured_coordinates,
               std::array<double, NCoordinates> const& expected_coordinates);

#endif  // _CURVE_FITTING_H_