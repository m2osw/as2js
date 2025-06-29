// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/as2js
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma

// C++
//
#include    <cstdint>


typedef std::int64_t        external_function_t;

constexpr external_function_t const     EXTERNAL_FUNCTION_UNKNOWN = 0;
constexpr external_function_t const     EXTERNAL_FUNCTION_POW = 1;


extern "C" double           rt_fmod(double x, double y);
extern "C" std::int64_t     rt_ipow(std::int64_t n, std::int64_t p);
extern "C" double           rt_pow(double x, double y);


// vim: ts=4 sw=4 et
