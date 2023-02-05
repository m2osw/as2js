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

// C++
//
#include    <vector>
#include    <memory>
#include    <cstdint>



namespace as2js
{


// options you can tweak so the compiler reacts in a different
// manner in different situations (for instance, the \e escape
// sequence can be used to generate the escape character whenever
// the extended escape sequences is set to 1).
//
// At this time AS_OPTION_STRICT is always set to 1
class options
{
public:
    typedef std::shared_ptr<options>    pointer_t;

    enum class option_t
    {
        OPTION_UNKNOWN = 0,

        OPTION_ALLOW_WITH,          // we do NOT allow with() statements by default
        OPTION_COVERAGE,
        OPTION_DEBUG,
        OPTION_EXTENDED_ESCAPE_SEQUENCES,
        OPTION_EXTENDED_OPERATORS,  // 1 support extended, 2 or 3 support extended and prevent '=' (use ':=' instead)
        OPTION_EXTENDED_STATEMENTS, // 1 support extended, 2 or 3 support extended and prevent if()/else/for()/while() ... without the '{' ... '}'
        OPTION_JSON,
        OPTION_OCTAL,
        OPTION_STRICT,
        OPTION_TRACE,
        OPTION_UNSAFE_MATH,         // optimize even what can be considered unsafe (see https://stackoverflow.com/questions/6430448/why-doesnt-gcc-optimize-aaaaaa-to-aaaaaa)

        OPTION_max
    };

    typedef std::int64_t            option_value_t;

                                    options();

    void                            set_option(option_t option, option_value_t value);
    option_value_t                  get_option(option_t option);

private:
    std::vector<option_value_t>     f_options;
};



} // namespace as2js
// vim: ts=4 sw=4 et
