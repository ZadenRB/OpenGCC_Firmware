/*
    Copyright 2022 Zaden Ruggiero-Bouné

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

#include "q2x14.hpp"

Q2x14::Q2x14(signed short in) { val = in; }

Q2x14::Q2x14(float in) { val = (signed short)((in)*16384.0); }

Q2x14 Q2x14::operator+(Q2x14 const &obj) {
    return Q2x14((signed short)(val + obj.val));
}

Q2x14 Q2x14::operator-(Q2x14 const &obj) {
    return Q2x14((signed short)(val - obj.val));
}

Q2x14 Q2x14::operator*(Q2x14 const &obj) {
    return Q2x14((signed short)((((int)(val)) * ((int)(obj.val))) >> 14));
}

Q2x14 Q2x14::operator/(Q2x14 const &obj) {
    return Q2x14((signed short)((((signed int)val) << 14) / obj.val));
}

float Q2x14::to_float() { return ((float)val) / 16384.0; }