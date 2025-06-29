// Copyright (c) 2005-2025  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// self
//
#include    <as2js/compare.h>


// C++
//
#include    <cstdint>



namespace as2js
{



enum class integer_size_t
{
    INTEGER_SIZE_UNKNOWN,

    INTEGER_SIZE_1BIT,
    INTEGER_SIZE_8BITS_SIGNED,
    INTEGER_SIZE_8BITS_UNSIGNED,
    INTEGER_SIZE_16BITS_SIGNED,
    INTEGER_SIZE_16BITS_UNSIGNED,
    INTEGER_SIZE_32BITS_SIGNED,
    INTEGER_SIZE_32BITS_UNSIGNED,
    INTEGER_SIZE_64BITS,

    INTEGER_SIZE_FLOATING_POINT, // practical to switch directly to such
};


inline integer_size_t get_smallest_size(std::int64_t value)
{
    if(value == 0 || value == 1)
    {
        return integer_size_t::INTEGER_SIZE_1BIT;
    }

    if(value >= -128 && value <= 127)
    {
        return integer_size_t::INTEGER_SIZE_8BITS_SIGNED;
    }

    if(value >= 0 && value <= 255)
    {
        return integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED;
    }

    if(value >= -32'768 && value <= 32'767)
    {
        return integer_size_t::INTEGER_SIZE_16BITS_SIGNED;
    }

    if(value >= 0 && value <= 65'535)
    {
        return integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED;
    }

    if(value >= -2'147'483'648 && value <= 2'147'483'647)
    {
        return integer_size_t::INTEGER_SIZE_32BITS_SIGNED;
    }

    if(value >= 0 && value <= 4'294'967'295)
    {
        return integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED;
    }

    return integer_size_t::INTEGER_SIZE_64BITS;
}


class integer
{
public:
    typedef std::int64_t value_type;

                    integer()
                    {
                    }

                    integer(value_type const rhs)
                    {
                        f_int = rhs;
                    }

                    integer(integer const & rhs)
                    {
                        f_int = rhs.f_int;
                    }

    integer &       operator = (integer const & rhs)
                    {
                        f_int = rhs.f_int;
                        return *this;
                    }

    value_type      get() const
                    {
                        return f_int;
                    }

    void            set(value_type const new_int)
                    {
                        f_int = new_int;
                    }

    compare_t       compare(integer const & rhs) const
                    {
                        return f_int == rhs.f_int ? compare_t::COMPARE_EQUAL
                             : (f_int < rhs.f_int ? compare_t::COMPARE_LESS
                                                  : compare_t::COMPARE_GREATER);
                    }

    integer_size_t  get_smallest_size() const
                    {
                        return as2js::get_smallest_size(f_int);
                    }

private:
    value_type      f_int = 0;
};


} // namespace as2js
// vim: ts=4 sw=4 et
