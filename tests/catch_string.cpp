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
#include    <as2js/string.h>
#include    <as2js/exception.h>


// self
//
#include    "catch_main.h"


// C++
//
#include    <algorithm>
#include    <cstring>
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>



namespace
{



bool close_double(double a, double b, double epsilon)
{
    return a >= b - epsilon && a <= b + epsilon;
}



}
// no name namespace




CATCH_TEST_CASE("string_empty", "[string][type]")
{
    CATCH_START_SECTION("string: empty string validity")
    {
        // a little extra test, make sure a string is empty on
        // creation without anything
        //
        std::string str1;
        CATCH_REQUIRE(as2js::valid(str1));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("string_bad_utf8", "[string][type]")
{
    CATCH_START_SECTION("string: bad UTF-8 sequences")
    {
        // UTF-8 starts with 0xC0 to 0xF4 and those must be followed by
        // 0x80 to 0xBF bytes anything else is incorrect
        //
        for(int i(0xC0); i <= 0xF4; ++i)
        {
            // too small a number just after 0xA0 to 0xF4
            //
            for(int j(0x01); j <= 0x7F; ++j)
            {
                char const buf[3]{
                    static_cast<char>(i),
                    static_cast<char>(j),
                    '\0'
                };
                std::string const bad_utf8(buf);
                CATCH_REQUIRE_FALSE(as2js::valid(bad_utf8));

                std::string const start_string(SNAP_CATCH2_NAMESPACE::random_string(1, 100, SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));
                std::string const end_string(SNAP_CATCH2_NAMESPACE::random_string(1, 100, SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));
                CATCH_REQUIRE(as2js::valid(start_string));
                CATCH_REQUIRE(as2js::valid(end_string));

                // make sure it gets caught anywhere in a string
                //
                std::string const complete_string(start_string + bad_utf8 + end_string);
                CATCH_REQUIRE_FALSE(as2js::valid(complete_string));
            }

            // too large a number just after 0xA0 to 0xFF
            //
            for(int j(0xC0); j <= 0xFF; ++j)
            {
                char buf[3]{
                    static_cast<char>(i),
                    static_cast<char>(j),
                    '\0'
                };
                std::string const bad_utf8(buf);
                CATCH_REQUIRE_FALSE(as2js::valid(bad_utf8));

                std::string const start_string(SNAP_CATCH2_NAMESPACE::random_string(1, 100, SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));
                std::string const end_string(SNAP_CATCH2_NAMESPACE::random_string(1, 100, SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));
                CATCH_REQUIRE(as2js::valid(start_string));
                CATCH_REQUIRE(as2js::valid(end_string));

                // make sure it gets caught anywhere in a string
                //
                std::string const complete_string(start_string + bad_utf8 + end_string);
                CATCH_REQUIRE_FALSE(as2js::valid(complete_string));
            }

            // note: the libutf8 already has many tests so I won't check
            //       every single possibility of invalid UTF-8; if there is
            //       an issue with such, we should verify with libutf8 instead
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("string", "[string][type]")
{
    CATCH_START_SECTION("string: check valid characters")
    {
        for(char32_t c(0); c < 0x110000; ++c)
        {
            // skip the surrogates at once
            //
            if(c == 0xD800)
            {
                c = 0xE000;
            }
            CATCH_REQUIRE(as2js::valid_character(c));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string: check surrogates (not valid UTF-32)")
    {
        for(char32_t c(0xD800); c < 0xE000; ++c)
        {
            CATCH_REQUIRE_FALSE(as2js::valid_character(c));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string: check outside range (not valid UTF-32)")
    {
        for(int i(0); i < 1000; ++i)
        {
            char32_t c(0);
            do
            {
                SNAP_CATCH2_NAMESPACE::random(c);
            }
            while(c < 0x110000);
            CATCH_REQUIRE_FALSE(as2js::valid_character(c));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("string_number", "[string][type][number]")
{
    CATCH_START_SECTION("string_number: empty string is 0, 0.0, and false")
    {
        std::string str;
        CATCH_REQUIRE(as2js::is_integer(str));
        CATCH_REQUIRE(as2js::is_floating_point(str));
        CATCH_REQUIRE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::to_integer(str) == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        bool const is_equal(as2js::to_floating_point(str) == 0.0);
#pragma GCC diagnostic pop
        CATCH_REQUIRE(is_equal);
        CATCH_REQUIRE_FALSE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: a lone sign (+ or -)")
    {
        std::string str("+");
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        str = "-";
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: a period alone is not a floating point number")
    {
        std::string str(".");
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        str = "+.";
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        str = "-.";
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        str = "!.5";
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: just one letter, even a hexadecimal letter, fails")
    {
        for(int c('a'); c <= 'f'; ++c)
        {
            std::string lower(1, c);
            CATCH_REQUIRE_FALSE(as2js::is_integer(lower));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(lower));
            CATCH_REQUIRE_FALSE(as2js::is_number(lower));
            CATCH_REQUIRE(as2js::is_true(lower));

            std::string upper(1, c & ~0x20);
            CATCH_REQUIRE_FALSE(as2js::is_integer(upper));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(upper));
            CATCH_REQUIRE_FALSE(as2js::is_number(upper));
            CATCH_REQUIRE(as2js::is_true(upper));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: no integral part means not a number (lowercase)")
    {
        std::string str("xyz");
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: no integral part means not a number (uppercase)")
    {
        std::string str("XYZ");
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: an exponent must be followed by a number")
    {
        std::string str("31.4159e");
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        // adding a sign is not enough
        //
        str += '+';
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));

        str[str.length() - 1] = '-';
        CATCH_REQUIRE_FALSE(as2js::is_integer(str));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
        CATCH_REQUIRE_FALSE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: 0x and 0X are not hexadecimal numbers")
    {
        {
            std::string str("0x");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }

        {
            std::string str("0X");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: strings with a <utf-8 char: a+aron> are not numbers")
    {
        { // straight UTF-8 char not a digit at all
            std::string str("\xC3\xA5");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }

        {
            std::string str("0xABC\xC3\xA5");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: 0g/0z and 0G/0Z represents nothing useful")
    {
        {
            std::string str("0g");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }

        {
            std::string str("0z");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }

        {
            std::string str("0G");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }

        {
            std::string str("0Z");
            CATCH_REQUIRE_FALSE(as2js::is_integer(str));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
            CATCH_REQUIRE_FALSE(as2js::is_number(str));
            CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
            CATCH_REQUIRE(as2js::is_true(str));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: octal is not detected; we have only decimal and hexadecimal")
    {
        // octal is not supported here, show that the string is
        // seen as a decimal number
        //
        std::string str("071");
        CATCH_REQUIRE(as2js::is_integer(str));
        CATCH_REQUIRE(as2js::is_floating_point(str));
        CATCH_REQUIRE(as2js::is_number(str));
        CATCH_REQUIRE(as2js::to_integer(str) == 71);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        bool const is_equal(as2js::to_floating_point(str) == 71.0);
#pragma GCC diagnostic pop
        CATCH_REQUIRE(is_equal);
        CATCH_REQUIRE(as2js::is_true(str));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: integers -100,000 to +100,000")
    {
        for(int64_t i(-100'000); i <= 100'000; ++i)
        {
            // decimal
            {
                std::stringstream ss;
                ss << (i >= 0 && (rand() & 1) ? "+" : "") << i;
                std::string str(ss.str());
                CATCH_REQUIRE(as2js::is_integer(str));
                CATCH_REQUIRE(as2js::is_floating_point(str));
                CATCH_REQUIRE(as2js::is_number(str));
                CATCH_REQUIRE(as2js::to_integer(str) == i);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                bool const is_equal(as2js::to_floating_point(str) == static_cast<double>(i));
#pragma GCC diagnostic pop
                CATCH_REQUIRE(is_equal);
                CATCH_REQUIRE(as2js::is_true(str));

                // no letter can follow an integer
                //
                ss << (rand() % 26 + 'a');
                CATCH_REQUIRE(as2js::is_integer(ss.str()));
            }

            // hexadecimal
            {
                // not that in C/C++ hexadecimal numbers cannot really be
                // negative; in JavaScript it's fine
                //
                std::stringstream ss;
                ss << (i < 0 ? "-" : (rand() & 1 ? "+" : "")) << "0" << (rand() & 1 ? "x" : "X") << std::hex << labs(i);
                std::string const str(ss.str());
                CATCH_REQUIRE(as2js::is_integer(str));
                CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
                CATCH_REQUIRE(as2js::is_number(str));
                CATCH_REQUIRE(as2js::to_integer(str) == i);
                CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str)));
                CATCH_REQUIRE(as2js::is_true(str));

                // add an 'h' at the end and it fails the integer test
                //
                ss << 'h';
                CATCH_REQUIRE_FALSE(as2js::is_integer(ss.str()));
            }

            if(i >= 0)
            {
                // some characters at the start mean this is not a number
                //
                std::stringstream ss;
                ss << ',' << i;
                std::string str(ss.str());
                CATCH_REQUIRE_FALSE(as2js::is_integer(str));
                CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
                CATCH_REQUIRE_FALSE(as2js::is_number(str));

                str[0] = '/';
                CATCH_REQUIRE_FALSE(as2js::is_integer(str));
                CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
                CATCH_REQUIRE_FALSE(as2js::is_number(str));

                str[0] = '|';
                CATCH_REQUIRE_FALSE(as2js::is_integer(str));
                CATCH_REQUIRE_FALSE(as2js::is_floating_point(str));
                CATCH_REQUIRE_FALSE(as2js::is_number(str));
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: floating points")
    {
        for(double i(-1000.00); i <= 1000.00; i += (rand() % 120) / 100.0)
        {
            std::stringstream ss;
            ss << i;
            if(ss.str().find('e') != std::string::npos
            || ss.str().find('E') != std::string::npos)
            {
                // this happens with numbers very close to zero and the
                // system decides to write them as '1e-12' for example
                //
                // Note: does not matter if it does not happen
                //
                continue;
            }
            std::string const str1(ss.str());
            std::int64_t const integer1(lrint(i));
            bool const is_integer1(std::find(str1.cbegin(), str1.cend(), '.') == str1.cend());
            std::string str1_without0;

            CATCH_REQUIRE(as2js::is_integer(str1) == is_integer1);
            CATCH_REQUIRE(as2js::is_floating_point(str1));
            if(str1.length() >= 3
            && str1[0] == '0'
            && str1[1] == '.')
            {
                // test without the starting '0' and it works too
                //
                str1_without0 = str1.substr(1);
                CATCH_REQUIRE(as2js::is_floating_point(str1_without0));
            }
            CATCH_REQUIRE(as2js::is_number(str1));
            CATCH_REQUIRE(as2js::is_true(str1));
            if(is_integer1)
            {
                CATCH_REQUIRE(as2js::to_integer(str1) == integer1);
            }
            else
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::to_integer(str1)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            }
            CATCH_REQUIRE(close_double(as2js::to_floating_point(str1), i, 0.01));
            if(!str1_without0.empty())
            {
                CATCH_REQUIRE(close_double(as2js::to_floating_point(str1_without0), i, 0.01));
            }

            // add x 1000 as an exponent
            ss << "e" << ((rand() & 1) != 0 ? "+" : "") << "3";
            std::string const str2(ss.str());
            // the 'e' "breaks" the integer test in JavaScript
            CATCH_REQUIRE_FALSE(as2js::is_integer(str2));
            CATCH_REQUIRE(as2js::is_floating_point(str2));
            CATCH_REQUIRE(as2js::is_number(str2));
            CATCH_REQUIRE(as2js::is_true(str2));
            CATCH_REQUIRE_THROWS_MATCHES(
                  as2js::to_integer(str2)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(close_double(as2js::to_floating_point(str2), i * 1000.0, 0.01));

            // add / 1000 as an exponent
            ss.str(""); // reset the string
            ss << i << "e-3";
            std::string const str3(ss.str());
            // the 'e' "breaks" the integer test in JavaScript
            CATCH_REQUIRE_FALSE(as2js::is_integer(str3));
            CATCH_REQUIRE(as2js::is_floating_point(str3));
            CATCH_REQUIRE(as2js::is_number(str3));
            CATCH_REQUIRE(as2js::is_true(str3));
            CATCH_REQUIRE_THROWS_MATCHES(
                  as2js::to_integer(str3)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(close_double(as2js::to_floating_point(str3), i / 1000.0, 0.00001));
        }

        // the exponent must start with e and + or -, other characters are
        // not valid
        //
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("3.5e,7"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-7.02E|9"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("3.5e!7"));
        CATCH_REQUIRE(as2js::is_floating_point("3.5e09")); // this one is valid, the exponent can start with a '0'!
        CATCH_REQUIRE(as2js::is_floating_point("3.5e90")); // this one is an edge case, number starting with '9'
        CATCH_REQUIRE(as2js::is_floating_point("3.5e0123456789")); // another edge case

        // without at least one digit, it's not a valid floating point
        //
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-."));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+."));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-e"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+e"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-E"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+E"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-.e"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+.e"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("-.E"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("+.E"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("e-3"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("e+4"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("E-5"));
        CATCH_REQUIRE_FALSE(as2js::is_floating_point("E+6"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: random 64 bits integers")
    {
        // a few more using random
        for(int i(0); i < 100000; ++i)
        {
            std::int64_t value(0);
            SNAP_CATCH2_NAMESPACE::random(value);
            std::stringstream ss;
            ss << value;
            std::string const str(ss.str());
            CATCH_REQUIRE(as2js::is_integer(str));
            CATCH_REQUIRE(as2js::is_floating_point(str));
            CATCH_REQUIRE(as2js::is_number(str));
            CATCH_REQUIRE(as2js::is_true(str));
            CATCH_REQUIRE(as2js::to_integer(str) == value);

            // this is important since double mantissa is only 52 bits
            // and here our integral numbers are 64 bits
            //
            as2js::floating_point const flt1(as2js::to_floating_point(str));
            as2js::floating_point const flt2(static_cast<as2js::floating_point::value_type>(value));
            CATCH_REQUIRE(flt1.nearly_equal(flt2, 0.0001));
            CATCH_REQUIRE(flt2.nearly_equal(flt1, 0.0001));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_number: NULL value")
    {
        // test a few non-hexadecimal numbers
        //
        for(int i(0); i < 100; ++i)
        {
            // get a character which is not a valid hex digit and not '\0'
            // and not 0x7F (Del)
            //
            char c;
            do
            {
                c = static_cast<char>(rand() % 0x7D + 1);
            }
            while((c >= '0' && c <= '9')
               || (c >= 'a' && c <= 'f')
               || (c >= 'A' && c <= 'F'));

            // bad character is right at the beginning of the hex number
            std::stringstream ss1;
            ss1 << "0" << (rand() & 1 ? "x" : "X") << c << "123ABC";
            std::string str1(ss1.str());
            CATCH_REQUIRE_FALSE(as2js::is_integer(str1));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str1));
            CATCH_REQUIRE_FALSE(as2js::is_number(str1));
            CATCH_REQUIRE(as2js::is_true(str1));
            CATCH_REQUIRE_THROWS_MATCHES(
                  as2js::to_integer(str1)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str1)));

            // invalid character is in the middle of the hex number
            //
            std::stringstream ss2;
            ss2 << "0" << (rand() & 1 ? "x" : "X") << "123" << c << "ABC";
            std::string str2(ss2.str());
            CATCH_REQUIRE_FALSE(as2js::is_integer(str2));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str2));
            CATCH_REQUIRE_FALSE(as2js::is_number(str2));
            CATCH_REQUIRE(as2js::is_true(str2));
            CATCH_REQUIRE_THROWS_MATCHES(
                  as2js::to_integer(str2)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str2)));

            // invalid character is at the very end of the hex number
            //
            std::stringstream ss3;
            ss3 << "0" << (rand() & 1 ? "x" : "X") << "123ABC" << c;
            std::string str3(ss3.str());
            CATCH_REQUIRE_FALSE(as2js::is_integer(str3));
            CATCH_REQUIRE_FALSE(as2js::is_floating_point(str3));
            CATCH_REQUIRE_FALSE(as2js::is_number(str3));
            CATCH_REQUIRE(as2js::is_true(str3));
            CATCH_REQUIRE_THROWS_MATCHES(
                  as2js::to_integer(str3)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: to_integer(std::string const & s) called with an invalid integer."));
            CATCH_REQUIRE(std::isnan(as2js::to_floating_point(str3)));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("string_simplify", "[string][type]")
{
    CATCH_START_SECTION("string_simplify: only spaces")
    {
        std::string const str("        ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: starting spaces")
    {
        std::string const str("    blah");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: ending spaces")
    {
        std::string const str("blah    ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: starting & ending spaces")
    {
        std::string const str("    blah    ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: inside spaces")
    {
        std::string const str("blah    foo");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah foo");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify starting, inside, and ending spaces")
    {
        std::string const str("    blah    foo    ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah foo");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify spaces including newlines")
    {
        std::string const str("blah  \n  foo");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "blah foo");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: empty string becomes zero")
    {
        std::string const str;
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: spaces only string becomes zero")
    {
        std::string const str("     ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify number with spaces around")
    {
        std::string const str("  3.14159  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "3.14159");
        CATCH_REQUIRE_FALSE(as2js::is_integer(simplified));
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(3.14159, 1.0e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify number with left over")
    {
        std::string const str("  3.14159 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "3.14159");
        CATCH_REQUIRE_FALSE(as2js::is_integer(simplified));
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(3.14159, 1.0e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify positive number with left over")
    {
        std::string const str("  +3.14159 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "+3.14159");
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(3.14159, 1.0e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify negative integer with left over")
    {
        std::string const str("  -314159 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "-314159");
        CATCH_REQUIRE(as2js::is_integer(simplified));
        CATCH_REQUIRE(as2js::to_integer(simplified) == -314159);
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(-314159.0, 1.0e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify the positive number with exponent and left over")
    {
        std::string const str("  +0.00314159e3 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "+0.00314159e3");
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(3.14159, 1e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify the positive number with positive exponent and left over")
    {
        std::string const str("  +0.00314159e+3 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "+0.00314159e+3");
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(3.14159, 1e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify the negative number with negative exponent and left over")
    {
        std::string const str("  -314159e-5 ignore that part  ");
        std::string const simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "-314159");
        CATCH_REQUIRE(as2js::is_integer(simplified));
        CATCH_REQUIRE(as2js::to_integer(simplified) == -314159);
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(-314159, 1e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify negative number with negative exponent and left over")
    {
        std::string str("  -314159.e-5 ignore that part  ");
        std::string simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "-314159.e-5");
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(-3.14159, 1e-8));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("string_simplify: simplify negative number with large negative exponent and left over")
    {
        std::string str("  -314159.0e-105ignorethatpart");
        std::string simplified(as2js::simplify(str));
        CATCH_REQUIRE(simplified == "-314159.0e-105");
        CATCH_REQUIRE(as2js::is_floating_point(simplified));
        CATCH_REQUIRE(as2js::is_number(simplified));
        CATCH_REQUIRE(as2js::floating_point(as2js::to_floating_point(simplified)).nearly_equal(-3.14159e-100, 1e-8));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
