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
#include    <as2js/integer.h>


// self
//
#include    "catch_main.h"


// C
//
#include    <cstring>
#include    <algorithm>


// last include
//
#include    <snapdev/poison.h>







CATCH_TEST_CASE("integer", "[number][integer][type]")
{
    CATCH_START_SECTION("integer: default constructor")
    {
        // default constructor gives us zero
        //
        as2js::integer zero;
        CATCH_REQUIRE(zero.get() == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: int8_t")
    {
        // int8_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 8 bit number
            //
            std::int8_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            //
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            //
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::int8_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::int8_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: uint8_t")
    {
        // uint8_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 8 bit number
            //
            std::uint8_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            //
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            //
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::uint8_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::uint8_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: int16_t")
    {
        // int16_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 16 bit number
            //
            std::int16_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            //
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            //
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::int16_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::int16_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: uint16_t")
    {
        // uint16_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 16 bit number
            //
            std::uint16_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            //
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            //
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::uint16_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::uint16_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: int32_t")
    {
        // int32_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 32 bit number
            //
            std::int32_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::int32_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            int32_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: uint32_t")
    {
        // uint32_t constructor, copy constructor, copy assignment
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 32 bit number
            std::uint32_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);

            // sign extends properly?
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            // copy works as expected?
            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::uint32_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::uint32_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: int64_t")
    {
        // int64_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 64 bit number
            //
            std::int64_t r;
            SNAP_CATCH2_NAMESPACE::random(r);
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == r);

            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == r);

            std::int64_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == q);

            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(q < r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(q > r)
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::int64_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("integer: uint64_t")
    {
        // uint64_t constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 64 bit number
            //
            std::uint64_t r(0);
            SNAP_CATCH2_NAMESPACE::random(r);
            as2js::integer random(r);
            CATCH_REQUIRE(random.get() == static_cast<decltype(random)::value_type>(r));

            as2js::integer copy(random);
            CATCH_REQUIRE(copy.get() == static_cast<decltype(copy)::value_type>(r));

            std::uint64_t q(0);
            SNAP_CATCH2_NAMESPACE::random(q);

            random = q;
            CATCH_REQUIRE(random.get() == static_cast<decltype(random)::value_type>(q));

            // Here the compare expects the signed int64_t...
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
            CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
            if(static_cast<as2js::integer::value_type>(q) < static_cast<as2js::integer::value_type>(r))
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
            }
            else if(static_cast<as2js::integer::value_type>(q) > static_cast<as2js::integer::value_type>(r))
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
            }
            else
            {
                CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
            }

            std::uint64_t p(0);
            SNAP_CATCH2_NAMESPACE::random(p);

            random.set(p);
            CATCH_REQUIRE(random.get() == static_cast<decltype(random)::value_type>(p));
        }
    }
    CATCH_END_SECTION()
}




// vim: ts=4 sw=4 et
