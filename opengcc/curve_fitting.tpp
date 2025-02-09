/*
    Copyright 2023-2025 Zaden Ruggiero-Bouné

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

#include <pico/types.h>

#include <array>
#include <vector>

template <uint dimension>
std::array<std::array<double, dimension>, dimension> convert_to_inverse(
    const std::array<std::array<double, dimension>, dimension>& in) {
  std::array<std::array<double, dimension>, dimension> ret = {};

  // Fill inverse with input array
  std::array<std::array<double, dimension>, 2 * dimension> inverse = {};
  for (int r = 0; r < dimension; ++r) {
    for (int c = 0; c < dimension; ++c) {
      inverse[c][r] = in[c][r];
    }
  }

  // Find inverse
  for (int r = 0; r < dimension; ++r) {
    for (int c = 0; c < 2 * dimension; ++c) {
      if (c == (r + dimension)) {
        inverse[c][r] = 1;
      }
    }
  }
  for (int r = dimension - 1; r > 0; --r) {
    if (inverse[0][r - 1] < inverse[0][r]) {
      for (int c = 0; c < 2 * dimension; ++c) {
        double temp = inverse[c][r];
        inverse[c][r] = inverse[c][r - 1];
        inverse[c][r - 1] = temp;
      }
    }
  }
  for (int c = 0; c < dimension; ++c) {
    for (int r = 0; r < dimension; ++r) {
      if (r != c) {
        double temp = inverse[c][r] / inverse[c][c];
        for (int i = 0; i < 2 * dimension; ++i) {
          inverse[i][r] -= inverse[i][c] * temp;
        }
      }
    }
  }
  for (int r = 0; r < dimension; ++r) {
    double temp = inverse[r][r];
    for (int c = 0; c < 2 * dimension; ++c) {
      inverse[c][r] = inverse[c][r] / temp;
    }
  }

  // Populate out array
  for (int r = 0; r < dimension; ++r) {
    for (int c = 0; c < dimension; ++c) {
      ret[c][r] = inverse[c + dimension][r];
    }
  }

  return ret;
}

template <uint num_coefficients, uint num_calibration_steps>
std::array<double, num_coefficients> fit_curve(
    const std::array<uint16_t, num_calibration_steps>& expected_coordinates,
    const std::array<uint16_t, num_calibration_steps>& actual_coordinates,
    const std::array<bool, num_calibration_steps>& skipped_coordinates) {
  std::array<double, num_coefficients> ret = {};

  std::array<std::array<double, num_coefficients>, num_coefficients> a = {};

  for (int i = 0; i < num_coefficients * 2 - 1; ++i) {
    double sum = 0;
    for (int j = 0; j < num_calibration_steps; ++j) {
      if (!skipped_coordinates[j]) {
        double actual_coordinate = actual_coordinates[j];
        double raised_actual_coordinate = 1;
        for (int k = 0; k < i; ++k) {
          raised_actual_coordinate *= actual_coordinate;
        }
        sum += raised_actual_coordinate;
      }
    }

    int current_col = i;
    int current_row = 0;
    if (current_col > num_coefficients - 1) {
      current_col = num_coefficients - 1;
      current_row = i - current_col;
    }

    while (true) {
      a[current_col][current_row] = sum;
      --current_col;
      ++current_row;

      if (current_col < 0 || current_row > num_coefficients - 1) {
        break;
      }
    }
  }

  std::array<double, num_coefficients> b = {};

  for (int i = 0; i < num_coefficients; ++i) {
    double sum = 0;
    for (int j = 0; j < num_calibration_steps; ++j) {
      if (!skipped_coordinates[j]) {
        double raised_actual_coordinate = 1;
        for (int k = 0; k < i; ++k) {
          raised_actual_coordinate *= actual_coordinates[j];
        }
        sum += raised_actual_coordinate * expected_coordinates[j];
      }
    }
    b[i] = sum;
  }

  std::array<std::array<double, num_coefficients>, num_coefficients> inverse =
      convert_to_inverse<num_coefficients>(a);

  for (int r = 0; r < num_coefficients; ++r) {
    double value = 0;
    for (int c = 0; c < num_coefficients; ++c) {
      value += inverse[c][r] * b[c];
    }
    ret[r] = value;
  }

  return ret;
}
