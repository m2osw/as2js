// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <rt/rt.h>



// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/math.h>


// last include
//
#include    <snapdev/poison.h>







CATCH_TEST_CASE("rt_ipow", "[rt][math]")
{
    CATCH_START_SECTION("rt_ipow: n^0 = 1")
    {
        for(std::int64_t count(0); count < 100; ++count)
        {
            std::int64_t number(0);
            SNAP_CATCH2_NAMESPACE::random(number);
            CATCH_REQUIRE(rt_ipow(number, 0) == 1);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("rt_ipow: n^1 = n")
    {
        for(std::int64_t count(0); count < 100; ++count)
        {
            std::int64_t number(0);
            SNAP_CATCH2_NAMESPACE::random(number);
            CATCH_REQUIRE(rt_ipow(number, 1) == number);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("rt_ipow: n^-p = 0 (unless n = 1 or n = -1)")
    {
        for(std::int64_t p(-100); p < 0; ++p)
        {
            CATCH_REQUIRE(rt_ipow(1, p) == 1);
            if((p & 1) == 0)
            {
                CATCH_REQUIRE(rt_ipow(-1, p) == 1);
            }
            else
            {
                CATCH_REQUIRE(rt_ipow(-1, p) == -1);
            }
        }

        for(std::int64_t count(0); count < 100; ++count)
        {
            std::int64_t n(0);
            SNAP_CATCH2_NAMESPACE::random(n);
            std::int64_t p(0);
            do
            {
                SNAP_CATCH2_NAMESPACE::random(p);
                p |= 1ULL << 63;
            }
            while(p == -1 || p == 0);
            CATCH_REQUIRE(rt_ipow(n, p) == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("rt_ipow: 2^p")
    {
        std::int64_t n(2);
        for(std::int64_t p(1); p < 63; ++p)
        {
            CATCH_REQUIRE(rt_ipow(2, p) == n);
            n *= 2;
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("rt_ipow: n^p")
    {
        for(std::int64_t count(0); count < 100; ++count)
        {
            std::int64_t n(0);
            SNAP_CATCH2_NAMESPACE::random(n);

            // snapdev::pow() expects an `int` so we use such here, it will
            // still be a very strong test
            //
            int p(0);
            do
            {
                SNAP_CATCH2_NAMESPACE::random(p);
            }
            while(p < 2);

//std::cerr << "n ^ p = " << n << " ^ " << p << "\n";
            CATCH_REQUIRE(rt_ipow(n, p) == snapdev::pow(n, p));
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
