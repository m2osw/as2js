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

// self
//
#include    "catch_main.h"


// as2js
//
#include    <as2js/position.h>
#include    <as2js/exception.h>


// C++
//
#include    <cstring>
#include    <algorithm>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>








CATCH_TEST_CASE("position_names", "[position]")
{
    CATCH_START_SECTION("position_names: check filename")
    {
        as2js::position pos;

        // by default it is empty
        CATCH_REQUIRE(pos.get_filename() == "");

        // some long filename
        pos.set_filename("the/filename/can really/be anything.test");
        CATCH_REQUIRE(pos.get_filename() == "the/filename/can really/be anything.test");

        // reset back to empty
        pos.set_filename("");
        CATCH_REQUIRE(pos.get_filename() == "");

        // set to another value
        pos.set_filename("file.js");
        CATCH_REQUIRE(pos.get_filename() == "file.js");

        as2js::position other;
        CATCH_REQUIRE_FALSE(pos == other);
        other = pos;
        CATCH_REQUIRE(pos == other);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("position_names: function")
    {
        as2js::position pos;

        // by default it is empty
        CATCH_REQUIRE(pos.get_function() == "");

        // some long function name
        pos.set_function("as2js::super::function::name");
        CATCH_REQUIRE(pos.get_function() == "as2js::super::function::name");

        // reset back to empty
        pos.set_function("");
        CATCH_REQUIRE(pos.get_function() == "");

        // set to another value
        pos.set_function("add");
        CATCH_REQUIRE(pos.get_function() == "add");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("position_counters", "[position]")
{
    CATCH_START_SECTION("position_counters: default counters")
    {
        as2js::position pos;

        CATCH_REQUIRE(pos.get_page() == 1);
        CATCH_REQUIRE(pos.get_page_line() == 1);
        CATCH_REQUIRE(pos.get_paragraph() == 1);
        CATCH_REQUIRE(pos.get_line() == 1);
        CATCH_REQUIRE(pos.get_column() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("position_counters: increase counters")
    {
        as2js::position pos;

        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CATCH_REQUIRE(pos.get_page() == page);
                CATCH_REQUIRE(pos.get_page_line() == page_line);
                CATCH_REQUIRE(pos.get_paragraph() == paragraph);
                CATCH_REQUIRE(pos.get_line() == total_line);

                constexpr int const max_column(256);
                for(int column(1); column < max_column; ++column)
                {
                    CATCH_REQUIRE(pos.get_column() == column);
                    pos.new_column();
                }
                CATCH_REQUIRE(pos.get_column() == max_column);

                if(line % paragraphs == 0)
                {
                    pos.new_paragraph();
                    ++paragraph;
                }
                pos.new_line();
                ++total_line;
                ++page_line;
            }
            pos.new_page();
        }

        // reset counters back to 1
        //
        pos.reset_counters();
        CATCH_REQUIRE(pos.get_page() == 1);
        CATCH_REQUIRE(pos.get_page_line() == 1);
        CATCH_REQUIRE(pos.get_paragraph() == 1);
        CATCH_REQUIRE(pos.get_line() == 1);
        CATCH_REQUIRE(pos.get_column() == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("position_counters: test reseting line number")
    {
        as2js::position pos;

        // we can also define the start line
        int last_line(1);
        for(int idx(1); idx < 250; ++idx)
        {
            int line(rand() % 20000);
            if(idx % 13 == 0)
            {
                // force a negative number to test the throw
                line = -line;
            }
            if(line < 1)
            {
                // this throws because the line # is not valid
                CATCH_REQUIRE_THROWS_MATCHES(
                      pos.reset_counters(line)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: the line parameter of the position object cannot be less than 1."));

                // the counters are unchanged in that case
                CATCH_REQUIRE(pos.get_page() == 1);
                CATCH_REQUIRE(pos.get_page_line() == 1);
                CATCH_REQUIRE(pos.get_paragraph() == 1);
                CATCH_REQUIRE(pos.get_line() == last_line);
                CATCH_REQUIRE(pos.get_column() == 1);
            }
            else
            {
                pos.reset_counters(line);
                CATCH_REQUIRE(pos.get_page() == 1);
                CATCH_REQUIRE(pos.get_page_line() == 1);
                CATCH_REQUIRE(pos.get_paragraph() == 1);
                CATCH_REQUIRE(pos.get_line() == line);
                CATCH_REQUIRE(pos.get_column() == 1);
                last_line = line;
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("position_output", "[position]")
{
    CATCH_START_SECTION("position_output: output without a filename")
    {
        as2js::position pos;

        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CATCH_REQUIRE(pos.get_page() == page);
                CATCH_REQUIRE(pos.get_page_line() == page_line);
                CATCH_REQUIRE(pos.get_paragraph() == paragraph);
                CATCH_REQUIRE(pos.get_line() == total_line);

                int const max_column(rand() % 200 + 1);
                for(int column(1); column < max_column; ++column)
                {
                    std::stringstream pos_str;
                    pos_str << pos;
                    std::stringstream test_str;
                    test_str << "line " << total_line << ":";
                    if(pos.get_column() != 1)
                    {
                        test_str << pos.get_column() << ":";
                    }
                    CATCH_REQUIRE(pos_str.str() == test_str.str());

                    pos.new_column();
                }

                if(line % paragraphs == 0)
                {
                    pos.new_paragraph();
                    ++paragraph;
                }
                pos.new_line();
                ++total_line;
                ++page_line;
            }
            pos.new_page();
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("position_output: with a filename")
    {
        as2js::position pos;

        pos.set_filename("file.js");
        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CATCH_REQUIRE(pos.get_page() == page);
                CATCH_REQUIRE(pos.get_page_line() == page_line);
                CATCH_REQUIRE(pos.get_paragraph() == paragraph);
                CATCH_REQUIRE(pos.get_line() == total_line);

                std::stringstream pos_str;
                pos_str << pos;
                std::stringstream test_str;
                test_str << "file.js:" << total_line << ":";
                CATCH_REQUIRE(pos_str.str() == test_str.str());

                if(line % paragraphs == 0)
                {
                    pos.new_paragraph();
                    ++paragraph;
                }
                pos.new_line();
                ++total_line;
                ++page_line;
            }
            pos.new_page();
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
