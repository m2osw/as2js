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
#include    <as2js/int64.h>
#include    <as2js/exceptions.h>


// C
//
#include    <cstring>
#include    <algorithm>


// last include
//
#include    <snapdev/poison.h>







void As2JsInt64UnitTests::test_int64()
{
    // default constructor gives us zero
    {
        as2js::Int64   zero;
        CPPUNIT_ASSERT(zero.get() == 0);
    }

    // int8_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        int8_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int8_t q;
        q = static_cast<int8_t>(rand());

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int8_t p;
        p = static_cast<int8_t>(rand());

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint8_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        uint8_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint8_t q;
        q = static_cast<uint8_t>(rand());

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint8_t p;
        p = static_cast<uint8_t>(rand());

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int16_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        int16_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int16_t q;
        q = (static_cast<int16_t>(rand()) << 16)
          ^ (static_cast<int16_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int16_t p;
        p = (static_cast<int16_t>(rand()) << 16)
          ^ (static_cast<int16_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint16_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        uint16_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint16_t q;
        q = (static_cast<uint16_t>(rand()) << 16)
          ^ (static_cast<uint16_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint16_t p;
        p = (static_cast<uint16_t>(rand()) << 16)
          ^ (static_cast<uint16_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int32_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 32 bit number
        int32_t r;
        r = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int32_t q;
        q = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int32_t p;
        p = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint32_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 32 bit number
        uint32_t r;
        r = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint32_t q;
        q = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint32_t p;
        p = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int64_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        int64_t r;
        r = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int64_t q;
        q = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int64_t p;
        p = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint64_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        uint64_t r;
        r = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(r));

        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == static_cast<int64_t>(r));

        uint64_t q;
        q = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(q));

        // Here the compare expects the signed int64_t...
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(static_cast<int64_t>(q) < static_cast<int64_t>(r))
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(static_cast<int64_t>(q) > static_cast<int64_t>(r))
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint64_t p;
        p = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(p));
    }
}




// vim: ts=4 sw=4 et
