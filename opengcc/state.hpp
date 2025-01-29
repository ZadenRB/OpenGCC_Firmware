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

#ifndef STATE_H_
#define STATE_H_

#include <array>

#include "hardware/pio.h"
#include "pico/time.h"

/** \file state.hpp
 * \brief Controller's volatile state
 *
 * Structures and global variables relevant to the controller state that do not
 * persist between power cycles.
 */

/// \brief D-pad left bit in controller state
constexpr uint DPAD_LEFT = 0;

/// \brief D-pad right bit in controller state
constexpr uint DPAD_RIGHT = 1;

/// \brief D-pad down bit in controller state
constexpr uint DPAD_DOWN = 2;

/// \brief D-pad up bit in controller state
constexpr uint DPAD_UP = 3;

/// \brief Z button bit in controller state
constexpr uint Z = 4;

/// \brief Right trigger button bit in controller state
constexpr uint RT_DIGITAL = 5;

/// \brief Left trigger button bit in controller state
constexpr uint LT_DIGITAL = 6;

/// \brief Always high bit in controller state
constexpr uint ALWAYS_HIGH = 7;

/// \brief A button bit in controller state
constexpr uint A = 8;

/// \brief B button bit in controller state
constexpr uint B = 9;

/// \brief X button bit in controller state
constexpr uint X = 10;

/// \brief Y button bit in controller state
constexpr uint Y = 11;

/// \brief Start button bit in controller state
constexpr uint START = 12;

/// \brief Origin bit in controller state
constexpr uint ORIGIN = 13;

/// \brief Center point of analog stick
constexpr uint16_t CENTER = 127;

/// \brief Number of coefficients for stick linearization
constexpr int NUM_COEFFICIENTS = 4;

/// \brief Calibration points for x & y axis of an analog stick
struct stick_coefficients {
  /// \brief Coefficients for x-axis linearization
  std::array<double, NUM_COEFFICIENTS> x_coefficients;

  /// \brief Coefficients for y axis of linearization
  std::array<double, NUM_COEFFICIENTS> y_coefficients;
};

/// \brief Distance from CENTER outside which an axis is eligible to snapback
constexpr double SNAPBACK_DISTANCE = 40;

/// \brief Distance from CENTER within which an axis is considered to be centered for snapback purposes
constexpr uint16_t CENTERED_DISTANCE = 5;

/** \brief Distance from CENTER within which the other axis can be considered to be crossing the center for snapback purposes
 * 
 * Used to differentiate between, for instance, a stick freely returning to center and a stick being spun around the outer gate
*/
constexpr uint16_t CROSSING_DISTANCE = 32;

/// \brief Default timeout for snapback wave after crossing zero
constexpr uint64_t DEFAULT_WAVE_DURATION_US = 6500;

/// \brief Timeout for snapback eligibility when close to zero
constexpr uint64_t SNAPBACK_ELIGIBILITY_TIMEOUT_US = 5000;

/// \brief Number of consecutive falling measurements required during snapback to enter the falling state
constexpr uint8_t FALLING_COUNT_THRESHOLD = 3;

/// \brief Buffer added to rise time of snapback wave to allow it to fall
constexpr uint8_t SNAPBACK_WAVE_DURATION_BUFFER = 80;

/// \brief Individual axis snapback state
struct axis_snapback_state {
  /// \brief Previous displacement of the axis
  double last_displacement;
  /// \brief `true` if the axis value is returning to zero during snapback, `false` otherwise
  bool falling;
  /// \brief Number of consecutive falling measurements
  uint8_t falling_count;
  /// \brief Timestamp at which the current snapback wave started
  absolute_time_t wave_started_at;
  /// \brief Timestamp at which the current snapback wave expires
  absolute_time_t wave_expires_at;
  /// \brief `true` if the axis is currently in snapback, `false` otherwise
  bool in_snapback;
  /// \brief `true` if the axis is eligible to enter snapback, `false` otherwise
  bool eligible_to_snapback;
  /// \brief Last timestamp at which the axis was set to be eligible to enter snapback
  absolute_time_t last_eligible_to_snapback;
};

/// \brief Grouping of axis snapback states for a single analog stick
struct stick_snapback_state {
  /// \brief X-axis snapback state
  axis_snapback_state x;
  /// \brief Y-axis snapback state
  axis_snapback_state y;
};

/// \brief Grouping of axes for a single analog stick with full precision
struct precise_stick {
  /// \brief X-axis
  double x;
  /// \brief Y-axis
  double y;
};

/// \brief Grouping of axes for a single analog stick after processing
struct stick {
  /// \brief X-axis
  uint8_t x;
  /// \brief Y-axis
  uint8_t y;
};

/// \brief Grouping of analog sticks
struct sticks {
  /// \brief Left stick
  stick l_stick;
  /// \brief Right stick
  stick r_stick;
};

/// \brief Grouping of analog triggers
struct triggers {
  /// \brief state of left trigger
  uint8_t l_trigger;
  /// \brief state of right trigger
  uint8_t r_trigger;
};

/// \brief Controller state
struct controller_state {
  /// \brief State of digital inputs
  uint16_t buttons = 0;
  /// \brief Whether left trigger digital is pressed (post remap)
  bool lt_pressed = false;
  /// \brief Whether right trigger digital is pressed (post remap)
  bool rt_pressed = false;
  /// \brief Calibration coefficients for left stick
  stick_coefficients l_stick_coefficients;
  /// \brief Calibration coefficients for right stick
  stick_coefficients r_stick_coefficients;
  /// \brief State of sticks
  sticks analog_sticks;
  /// \brief State of triggers (analog)
  triggers analog_triggers;
  /// \brief `true` if origin has not been set, `false` if it has
  bool origin = true;
  /// \brief `false` if stick and trigger centers have not been set, `true` if they have
  bool center_set = false;
  /// \brief Left trigger center value, used to offset readings
  uint8_t l_trigger_center = 0;
  /// \brief Right trigger center value, used to offset readings
  uint8_t r_trigger_center = 0;
  /// \brief `true` if safe mode is active, `false` if it is not
  bool safe_mode = true;
  /// \brief State of digital inputs of the in-progress combo
  uint16_t active_combo = 0;
  /// \brief Alarm ID for triggering a combo
  absolute_time_t combo_trigger_timestamp = nil_time;
  /// \brief Left stick snapback state
  stick_snapback_state l_stick_snapback_state;
  /// \brief Right stick snapback state
  stick_snapback_state r_stick_snapback_state;

  /// \brief Max out triggers for 1.5 seconds to indicate an alert
  void display_alert();

  /// \brief Toggle safe mode
  void toggle_safe_mode();
};

/** \brief Global state
 * \note Not inherently thread-safe, take care with any state shared between
 * cores.
 */
extern controller_state state;

#endif  // STATE_H_
