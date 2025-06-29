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
#include    <as2js/floating_point.h>


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


#pragma GCC diagnostic ignored "-Wfloat-equal"




CATCH_TEST_CASE("floating_point", "[number][floating_point][type]")
{
    CATCH_START_SECTION("floating_point: default constructor")
    {
        // default constructor gives us exactly zero
        //
        as2js::floating_point zero;
        CATCH_REQUIRE(zero.get() == 0.0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point: basics with float")
    {
        // float constructor, copy constructor, copy assignment
        //
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 64 bit number
            //
            float s1(rand() & 1 ? -1.0f : 1.0f);
            std::int64_t rnd(0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            float n1(static_cast<float>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            float d1(static_cast<float>(rnd));
            float r(n1 / d1 * s1);
            as2js::floating_point random(r);
            CATCH_REQUIRE(random.get() == r);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.classified_infinity() == 0);

            as2js::floating_point copy(random);
            CATCH_REQUIRE(copy.get() == r);
            CATCH_REQUIRE(!copy.is_nan());
            CATCH_REQUIRE(!copy.is_infinity());
            CATCH_REQUIRE(!copy.is_positive_infinity());
            CATCH_REQUIRE(!copy.is_negative_infinity());
            CATCH_REQUIRE(copy.classified_infinity() == 0);

            float s2(rand() & 1 ? -1.0f : 1.0f);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            float n2(static_cast<float>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            float d2(static_cast<float>(rnd));
            float q(n2 / d2 * s2);

            random = q;
            CATCH_REQUIRE(random.get() == q);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.classified_infinity() == 0);

            for(int j(0); j <= 10; ++j)
            {
                // 1.0, 0.1, 0.01, ... 0.000000001
                double const epsilon(pow(10.0, static_cast<double>(-j)));

                bool nearly_equal(false);
                {
                    as2js::floating_point::value_type const diff = fabs(random.get() - copy.get());
                    if(random.get() == 0.0
                    || copy.get() == 0.0
                    || diff < std::numeric_limits<double>::min())
                    {
                        nearly_equal = diff < (epsilon * std::numeric_limits<double>::min());
                    }
                    else
                    {
                        nearly_equal = diff / (fabs(random.get()) + fabs(copy.get())) < epsilon;
                    }
                }

                CATCH_REQUIRE(as2js::compare_utils::is_ordered(random.compare(copy)));
                CATCH_REQUIRE(as2js::compare_utils::is_ordered(copy.compare(random)));
                if(q < r)
                {
                    CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
                    CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
                    CATCH_REQUIRE(!(random.nearly_equal(copy, epsilon) ^ nearly_equal));
                    CATCH_REQUIRE(!(copy.nearly_equal(random, epsilon) ^ nearly_equal));
                }
                else if(q > r)
                {
                    CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
                    CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
                    CATCH_REQUIRE(!(random.nearly_equal(copy, epsilon) ^ nearly_equal));
                    CATCH_REQUIRE(!(copy.nearly_equal(random, epsilon) ^ nearly_equal));
                }
                else
                {
                    CATCH_REQUIRE(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
                    CATCH_REQUIRE(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
                    CATCH_REQUIRE(random.nearly_equal(copy, epsilon));
                    CATCH_REQUIRE(copy.nearly_equal(random, epsilon));
                }
            }

            float s3(rand() & 1 ? -1.0f : 1.0f);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            float n3(static_cast<float>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            float d3(static_cast<float>(rnd));
            float p(n3 / d3 * s3);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.classified_infinity() == 0);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point: basics with double")
    {
        // double constructor, copy constructor, copy assignment
        for(int i(0); i < 1000; ++i)
        {
            // generate a random 64 bit number
            double s1(rand() & 1 ? -1.0 : 1.0);
            std::int64_t rnd(0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            double n1(static_cast<double>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }

            double d1(static_cast<double>(rnd));
            double r(n1 / d1 * s1);

            as2js::floating_point random(r);
            CATCH_REQUIRE(random.get() == r);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.get() != std::numeric_limits<double>::quiet_NaN());
            CATCH_REQUIRE(random.classified_infinity() == 0);

            as2js::floating_point copy(random);
            CATCH_REQUIRE(copy.get() == r);
            CATCH_REQUIRE(!copy.is_nan());
            CATCH_REQUIRE(!copy.is_infinity());
            CATCH_REQUIRE(!copy.is_positive_infinity());
            CATCH_REQUIRE(!copy.is_negative_infinity());
            CATCH_REQUIRE(copy.get() != std::numeric_limits<double>::quiet_NaN());
            CATCH_REQUIRE(copy.classified_infinity() == 0);

            double s2(rand() & 1 ? -1.0 : 1.0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            double n2(static_cast<double>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            double d2(static_cast<double>(rnd));
            double q(n2 / d2 * s2);

            random = q;
            CATCH_REQUIRE(random.get() == q);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.get() != std::numeric_limits<double>::quiet_NaN());
            CATCH_REQUIRE(random.classified_infinity() == 0);

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

            double s3(rand() & 1 ? -1.0 : 1.0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            double n3(static_cast<double>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            double d3(static_cast<double>(rnd));
            double p(n3 / d3 * s3);

            random.set(p);
            CATCH_REQUIRE(random.get() == p);
            CATCH_REQUIRE(!random.is_nan());
            CATCH_REQUIRE(!random.is_infinity());
            CATCH_REQUIRE(!random.is_positive_infinity());
            CATCH_REQUIRE(!random.is_negative_infinity());
            CATCH_REQUIRE(random.get() != std::numeric_limits<double>::quiet_NaN());
            CATCH_REQUIRE(random.classified_infinity() == 0);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("floating_point_special_numbers", "[number][floating_point][type]")
{
    CATCH_START_SECTION("floating_point: special numbers")
    {
        as2js::floating_point special;

        // start with zero
        CATCH_REQUIRE(special.get() == 0.0);
        CATCH_REQUIRE(special.nearly_equal(0.0));

        // create a random number to compare with
        double s1(rand() & 1 ? -1.0 : 1.0);
        std::uint64_t rnd;
        SNAP_CATCH2_NAMESPACE::random(rnd);
        double n1(static_cast<double>(rnd));
        SNAP_CATCH2_NAMESPACE::random(rnd);
        while(rnd == 0) // denominator should not be zero
        {
            SNAP_CATCH2_NAMESPACE::random(rnd);
        }
        double d1(static_cast<double>(rnd));
        double p(n1 / d1 * s1);
        as2js::floating_point r(p);

        // test NaN
        special.set_nan();
        CATCH_REQUIRE(special.is_nan());
        CATCH_REQUIRE_FALSE(special.is_infinity());
        CATCH_REQUIRE_FALSE(special.is_positive_infinity());
        CATCH_REQUIRE_FALSE(special.is_negative_infinity());
        CATCH_REQUIRE(special.get() != 0.0);
        CATCH_REQUIRE_FALSE(special.get() == p);
        CATCH_REQUIRE(special.get() != p);
        CATCH_REQUIRE_FALSE(special.get() > p);
        CATCH_REQUIRE_FALSE(special.get() >= p);
        CATCH_REQUIRE_FALSE(special.get() < p);
        CATCH_REQUIRE_FALSE(special.get() <= p);
        // We do not offer those yet
        //CATCH_REQUIRE(special != r);
        //CATCH_REQUIRE(!(special == r));
        //CATCH_REQUIRE(!(special > r));
        //CATCH_REQUIRE(!(special >= r));
        //CATCH_REQUIRE(!(special < r));
        //CATCH_REQUIRE(!(special <= r));
        CATCH_REQUIRE(special.get() != std::numeric_limits<double>::quiet_NaN());
        CATCH_REQUIRE(special.compare(p) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(special.compare(r) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(r.compare(special) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(special.classified_infinity() == 0);
        CATCH_REQUIRE_FALSE(special.nearly_equal(p));
        CATCH_REQUIRE_FALSE(special.nearly_equal(special));

        // test Infinity
        special.set_infinity(); // +inf
        CATCH_REQUIRE_FALSE(special.is_nan());
        CATCH_REQUIRE(special.is_infinity());
        CATCH_REQUIRE(special.is_positive_infinity());
        CATCH_REQUIRE_FALSE(special.is_negative_infinity());
        CATCH_REQUIRE(special.get() != 0.0);
        CATCH_REQUIRE(special.get() != p);
        CATCH_REQUIRE_FALSE(special.get() == p);
        CATCH_REQUIRE(special.get() > p);
        CATCH_REQUIRE(special.get() >= p);
        CATCH_REQUIRE_FALSE(special.get() < p);
        CATCH_REQUIRE_FALSE(special.get() <= p);
        // We do not offer those yet
        //CATCH_REQUIRE(special != r);
        //CATCH_REQUIRE(!(special == r));
        //CATCH_REQUIRE(!(special > r));
        //CATCH_REQUIRE(!(special >= r));
        //CATCH_REQUIRE(!(special < r));
        //CATCH_REQUIRE(!(special <= r));
        CATCH_REQUIRE(special.get() != std::numeric_limits<double>::quiet_NaN());
        CATCH_REQUIRE(special.compare(p) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(special.compare(r) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(r.compare(special) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(special.classified_infinity() == 1);
        CATCH_REQUIRE_FALSE(special.nearly_equal(p));
        CATCH_REQUIRE(special.nearly_equal(special));

        as2js::floating_point pinf;
        pinf.set_infinity();
        CATCH_REQUIRE(pinf.compare(special) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(special.compare(pinf) == as2js::compare_t::COMPARE_EQUAL);

        special.set(-special.get()); // -inf
        CATCH_REQUIRE_FALSE(special.is_nan());
        CATCH_REQUIRE(special.is_infinity());
        CATCH_REQUIRE_FALSE(special.is_positive_infinity());
        CATCH_REQUIRE(special.is_negative_infinity());
        CATCH_REQUIRE(special.get() != 0.0);
        CATCH_REQUIRE(special.get() != p);
        CATCH_REQUIRE_FALSE(special.get() == p);
        CATCH_REQUIRE_FALSE(special.get() > p);
        CATCH_REQUIRE_FALSE(special.get() >= p);
        CATCH_REQUIRE(special.get() < p);
        CATCH_REQUIRE(special.get() <= p);
        // We do not offer those yet
        //CATCH_REQUIRE(special != r);
        //CATCH_REQUIRE(!(special == r));
        //CATCH_REQUIRE(!(special > r));
        //CATCH_REQUIRE(!(special >= r));
        //CATCH_REQUIRE(!(special < r));
        //CATCH_REQUIRE(!(special <= r));
        CATCH_REQUIRE(special.get() != std::numeric_limits<double>::quiet_NaN());
        CATCH_REQUIRE(special.compare(p) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(special.compare(r) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(r.compare(special) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(special.classified_infinity() == -1);
        CATCH_REQUIRE_FALSE(special.nearly_equal(p));
        CATCH_REQUIRE(special.nearly_equal(special));

        CATCH_REQUIRE(pinf.compare(special) != as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(special.compare(pinf) != as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE_FALSE(pinf.nearly_equal(special));
        CATCH_REQUIRE_FALSE(special.nearly_equal(pinf));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("floating_point_nearly_equal", "[number][floating_point][type][compare]")
{
    CATCH_START_SECTION("floating_point_nearly_equal: exactly equal")
    {
        // exactly equal
        //
        as2js::floating_point f1(3.14159);
        as2js::floating_point f2(3.14159);
        CATCH_REQUIRE(f1.nearly_equal(f2));
        CATCH_REQUIRE(f2.nearly_equal(f1));

        // equal to self as well
        //
        CATCH_REQUIRE(f1.nearly_equal(f1));
        CATCH_REQUIRE(f2.nearly_equal(f2));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point_nearly_equal: +/-1e-5")
    {
        // nearly equal at +/-1e-5
        //
        as2js::floating_point f1(3.14159);
        as2js::floating_point f2(3.14158);
        CATCH_REQUIRE(f1.nearly_equal(f2));
        CATCH_REQUIRE(f2.nearly_equal(f1));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point_nearly_equal: +/-1e-6")
    {
        // nearly equal at +/-1e-6
        //
        as2js::floating_point f1(3.1415926);
        as2js::floating_point f2(3.1415936);
        CATCH_REQUIRE(f1.nearly_equal(f2));
        CATCH_REQUIRE(f2.nearly_equal(f1));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point_nearly_equal: +/-1e-4")
    {
        // nearly equal at +/-1e-4 -- fails
        //
        as2js::floating_point f1(3.1415926);
        as2js::floating_point f2(3.1416926);
        CATCH_REQUIRE_FALSE(f1.nearly_equal(f2));
        CATCH_REQUIRE_FALSE(f2.nearly_equal(f1));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("floating_point_nearly_equal: very different")
    {
        // nearly equal, very different
        {
            as2js::floating_point f1(3.1415926);
            as2js::floating_point f2(-3.1415926);
            CATCH_REQUIRE_FALSE(f1.nearly_equal(f2));
            CATCH_REQUIRE_FALSE(f2.nearly_equal(f1));
        }
        {
            as2js::floating_point f1(3.1415926);
            as2js::floating_point f2(0.0);
            CATCH_REQUIRE_FALSE(f1.nearly_equal(f2));
            CATCH_REQUIRE_FALSE(f2.nearly_equal(f1));
        }
        {
            as2js::floating_point f1(0.0);
            as2js::floating_point f2(3.1415926);
            CATCH_REQUIRE_FALSE(f1.nearly_equal(f2));
            CATCH_REQUIRE_FALSE(f2.nearly_equal(f1));
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
