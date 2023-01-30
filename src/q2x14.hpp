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

class Q2x14 {
   private:
    signed short val;

   public:
    Q2x14(signed short in);
    Q2x14(float in);

    Q2x14 operator+(Q2x14 const& obj);
    Q2x14 operator-(Q2x14 const& obj);
    Q2x14 operator*(Q2x14 const& obj);
    Q2x14 operator/(Q2x14 const& obj);

    float to_float();
};