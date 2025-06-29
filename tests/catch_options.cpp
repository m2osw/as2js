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

// as2js
//
#include    <as2js/options.h>


// self
//
#include    "catch_main.h"


// last include
//
#include    <snapdev/poison.h>






CATCH_TEST_CASE("options", "[options]")
{
    CATCH_START_SECTION("options: verify options")
    {
        as2js::options::pointer_t opt(std::make_shared<as2js::options>());

        // verify that all options are set to zero by default
        //
        for(as2js::option_t o(as2js::option_t::OPTION_UNKNOWN);
                            o < as2js::option_t::OPTION_max;
                            o = static_cast<as2js::option_t>(static_cast<int>(o) + 1))
        {
            CATCH_REQUIRE(opt->get_option(o) == 0);
        }

        for(as2js::option_t o(as2js::option_t::OPTION_UNKNOWN);
                            o < as2js::option_t::OPTION_max;
                            o = static_cast<as2js::option_t>(static_cast<int>(o) + 1))
        {
            for(int i(0); i < 100; ++i)
            {
                std::int64_t value(0);
                SNAP_CATCH2_NAMESPACE::random(value);
                opt->set_option(o, value);
                CATCH_REQUIRE(opt->get_option(o) == value);
            }
        }
    }
    CATCH_END_SECTION()
}




// vim: ts=4 sw=4 et
