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

template <std::size_t NCoordinates, std::size_t NCoefficients>
void fit_curve(std::array<double, NCoefficients>& coefficients,
               std::array<double, NCoordinates> const& measured_coordinates,
               std::array<double, NCoordinates> const& expected_coordinates);
template <std::size_t N>
void convert_to_inverse(std::array<std::array<double, N>, N>& out,
                        std::array<std::array<double, N>, N> const& in);

#endif  // _CURVE_FITTING_H_