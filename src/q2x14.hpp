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

#ifndef Q2X14_H_
#define Q2X14_H_

/** \file q2x14.hpp
 * \brief Fixed point decimal library for fast decimal math without FPU
 * 
 * q2x14 is one sign bit, one integer bit, and 14 decimal bits.
 */

/// \brief Q2x14 data representation
class Q2x14 {
   private:
    signed short val;

   public:
    /** \brief Construct a new Q2x14 from a short
     * 
     * \param in Value to convert
     */
    Q2x14(signed short in);

    /** \brief Construct a new Q2x14 object from a float
     * 
     * \param in Value to convert
     */
    Q2x14(float in);

    Q2x14 operator+(Q2x14 const& obj);
    Q2x14 operator-(Q2x14 const& obj);
    Q2x14 operator*(Q2x14 const& obj);
    Q2x14 operator/(Q2x14 const& obj);

    /** \brief Convert Q2x14 to a float
     * 
     * \return Float value
     */
    float to_float();
};

#endif  // Q2X14_H_