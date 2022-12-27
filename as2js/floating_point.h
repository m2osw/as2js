// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    <limits>
#include    <cmath>



namespace as2js
{

class floating_point
{
public:
    typedef double  value_type;

                    floating_point()
                    {
                    }

                    floating_point(value_type const rhs)
                    {
                        f_float = rhs;
                    }

                    floating_point(floating_point const & rhs)
                    {
                        f_float = rhs.f_float;
                    }

    floating_point & operator = (floating_point const & rhs)
                    {
                        f_float = rhs.f_float;
                        return *this;
                    }

    value_type      get() const
                    {
                        return f_float;
                    }

    void            set(value_type const new_float)
                    {
                        f_float = new_float;
                    }

    void            set_nan()
                    {
                        f_float = std::numeric_limits<floating_point::value_type>::quiet_NaN();
                    }

    void            set_infinity()
                    {
                        f_float = std::numeric_limits<floating_point::value_type>::infinity();
                    }

    bool            is_nan() const
                    {
                        return std::isnan(f_float);
                    }

    bool            is_infinity() const
                    {
                        return std::isinf(f_float);
                    }

    bool            is_positive_infinity() const
                    {
                        return std::isinf(f_float) && !std::signbit(f_float);
                    }

    bool            is_negative_infinity() const
                    {
                        return std::isinf(f_float) && std::signbit(f_float);
                    }

    int             classified_infinity() const
                    {
                        // if infinity, return -1 or +1
                        // if not infinity, return 0
                        return std::isinf(f_float)
                                ? (std::signbit(f_float) ? -1 : 1)
                                : 0;
                    }

    compare_t       compare(floating_point const& rhs) const
                    {
                        // if we got a NaN, it's not ordered
                        if(is_nan() || rhs.is_nan())
                        {
                            return compare_t::COMPARE_UNORDERED;
                        }

                        // comparing two floats properly handles infinity
                        // (at least in g++ on Intel processors)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        return f_float == rhs.f_float ? compare_t::COMPARE_EQUAL
                             : (f_float < rhs.f_float ? compare_t::COMPARE_LESS
                                                      : compare_t::COMPARE_GREATER);
#pragma GCC diagnostic pop
                    }

    static value_type default_epsilon()
                    {
                        return 0.00001;
                    }

    bool            nearly_equal(floating_point const & rhs, value_type epsilon = default_epsilon()) const
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        // already equal?
                        if(f_float == rhs.f_float)
                        {
                            return true;
                        }

                        value_type const diff = fabs(f_float - rhs.f_float);
                        if(f_float == 0.0
                        || rhs.f_float == 0.0
                        || diff < std::numeric_limits<value_type>::min())
                        {
                            return diff < (epsilon * std::numeric_limits<value_type>::min());
                        }
#pragma GCC diagnostic pop

                        return diff / (fabs(f_float) + fabs(rhs.f_float)) < epsilon;
                    }
 

private:
    value_type      f_float = 0.0;
};


} // namespace as2js
// vim: ts=4 sw=4 et
