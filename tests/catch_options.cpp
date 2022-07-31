// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "catch_main.h"


// as2js
//
#include    <as2js/options.h>
#include    <as2js/exceptions.h>


// C++
//
#include    <cstring>
#include    <algorithm>


// last include
//
#include    <snapdev/poison.h>






void As2JsOptionsUnitTests::test_options()
{
    as2js::Options::pointer_t opt(new as2js::Options);

    // verify that all options are set to zero by default
    for(as2js::Options::option_t o(as2js::Options::option_t::OPTION_UNKNOWN); o < as2js::Options::option_t::OPTION_max; o = static_cast<as2js::Options::option_t>(static_cast<int>(o) + 1))
    {
        CPPUNIT_ASSERT(opt->get_option(o) == 0);
    }

    for(as2js::Options::option_t o(as2js::Options::option_t::OPTION_UNKNOWN); o < as2js::Options::option_t::OPTION_max; o = static_cast<as2js::Options::option_t>(static_cast<int>(o) + 1))
    {
        for(int i(0); i < 100; ++i)
        {
            int64_t value((static_cast<int64_t>(rand()) << 48)
                        ^ (static_cast<int64_t>(rand()) << 32)
                        ^ (static_cast<int64_t>(rand()) << 16)
                        ^ (static_cast<int64_t>(rand()) <<  0));
            opt->set_option(o, value);
            CPPUNIT_ASSERT(opt->get_option(o) == value);
        }
    }
}




// vim: ts=4 sw=4 et
