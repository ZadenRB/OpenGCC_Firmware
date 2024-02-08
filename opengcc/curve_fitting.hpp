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
 *
 * \return Inverse of array
 */
template <uint N>
std::array<std::array<double, N>, N> convert_to_inverse(
    const std::array<std::array<double, N>, N>& in);

/** \brief Generates coefficients for an polynomial to map
 * `measured_coefficients` to `expected_coordinates` via regression
 *
 * Produces coefficients for a function
 * `coefficients[0]*x^0 + ... + coefficients[i]*x^i + ... + `
 * `coefficients[num_coefficients-1]*x^(num_coefficients-1)`.
 * Used to linearize Hall-Effect sensor output.
 *
 * \note Increasing `num_coefficients` will increase runtime of sensor
 * linearization which is run on every poll, so there is a tradeoff between
 * accuracy and performance.
 *
 * \tparam num_coefficients Number of coefficients to generate
 * \tparam num_calibration steps Number of calibration steps
 * \param coefficients Output array for coefficients
 * \param measured_coordinates The measured input coordinates
 * \param expected_coordinates The expected output coordinates
 *
 * \return Coefficients to map measured to expected
 */
template <uint num_coefficients, uint num_calibration_steps>
std::array<double, num_coefficients> fit_curve(
    const std::array<uint16_t, num_calibration_steps>& expected_coordinates,
    const std::array<uint16_t, num_calibration_steps>& actual_coordinates,
    const std::array<bool, num_calibration_steps>& skipped_coordinates);

#include "curve_fitting.tpp" 

#endif  // CURVE_FITTING_H_
