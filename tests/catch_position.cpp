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
#include    "test_as2js_main.h"


// as2js
//
#include    <as2js/position.h>
#include    <as2js/exceptions.h>


// C++
//
#include    <cstring>
#include    <algorithm>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>








void As2JsPositionUnitTests::test_names()
{
    as2js::Position pos;

    // check the filename
    {
        // by default it is empty
        CPPUNIT_ASSERT(pos.get_filename() == "");

        // some long filename
        pos.set_filename("the/filename/can really/be anything.test");
        CPPUNIT_ASSERT(pos.get_filename() == "the/filename/can really/be anything.test");

        // reset back to empty
        pos.set_filename("");
        CPPUNIT_ASSERT(pos.get_filename() == "");

        // reset back to empty
        pos.set_filename("file.js");
        CPPUNIT_ASSERT(pos.get_filename() == "file.js");
    }

    // check the function name
    {
        // by default it is empty
        CPPUNIT_ASSERT(pos.get_function() == "");

        // some long filename
        pos.set_function("as2js::super::function::name");
        CPPUNIT_ASSERT(pos.get_function() == "as2js::super::function::name");

        // reset back to empty
        pos.set_function("");
        CPPUNIT_ASSERT(pos.get_function() == "");

        // reset back to empty
        pos.set_function("add");
        CPPUNIT_ASSERT(pos.get_function() == "add");
    }
}


void As2JsPositionUnitTests::test_counters()
{
    as2js::Position pos;

    // frist verify the default
    {
        CPPUNIT_ASSERT(pos.get_page() == 1);
        CPPUNIT_ASSERT(pos.get_page_line() == 1);
        CPPUNIT_ASSERT(pos.get_paragraph() == 1);
        CPPUNIT_ASSERT(pos.get_line() == 1);
    }

    int total_line(1);
    for(int page(1); page < 100; ++page)
    {
        int paragraphs(rand() % 10 + 10);
        int page_line(1);
        int paragraph(1);
        for(int line(1); line < 1000; ++line)
        {
            CPPUNIT_ASSERT(pos.get_page() == page);
            CPPUNIT_ASSERT(pos.get_page_line() == page_line);
            CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(pos.get_line() == total_line);

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

    // by default, the reset counters resets everything back to 1
    {
        pos.reset_counters();
        CPPUNIT_ASSERT(pos.get_page() == 1);
        CPPUNIT_ASSERT(pos.get_page_line() == 1);
        CPPUNIT_ASSERT(pos.get_paragraph() == 1);
        CPPUNIT_ASSERT(pos.get_line() == 1);
    }

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
            CPPUNIT_ASSERT_THROW(pos.reset_counters(line), as2js::exception_internal_error);

            // the counters are unchanged in that case
            CPPUNIT_ASSERT(pos.get_page() == 1);
            CPPUNIT_ASSERT(pos.get_page_line() == 1);
            CPPUNIT_ASSERT(pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(pos.get_line() == last_line);
        }
        else
        {
            pos.reset_counters(line);
            CPPUNIT_ASSERT(pos.get_page() == 1);
            CPPUNIT_ASSERT(pos.get_page_line() == 1);
            CPPUNIT_ASSERT(pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(pos.get_line() == line);
            last_line = line;
        }
    }
}


void As2JsPositionUnitTests::test_output()
{
    // no filename
    {
        as2js::Position pos;
        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CPPUNIT_ASSERT(pos.get_page() == page);
                CPPUNIT_ASSERT(pos.get_page_line() == page_line);
                CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
                CPPUNIT_ASSERT(pos.get_line() == total_line);

                std::stringstream pos_str;
                pos_str << pos;
                std::stringstream test_str;
                test_str << "line " << total_line << ":";
                CPPUNIT_ASSERT(pos_str.str() == test_str.str());

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

    {
        as2js::Position pos;
        pos.set_filename("file.js");
        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CPPUNIT_ASSERT(pos.get_page() == page);
                CPPUNIT_ASSERT(pos.get_page_line() == page_line);
                CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
                CPPUNIT_ASSERT(pos.get_line() == total_line);

                std::stringstream pos_str;
                pos_str << pos;
                std::stringstream test_str;
                test_str << "file.js:" << total_line << ":";
                CPPUNIT_ASSERT(pos_str.str() == test_str.str());

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
}


// vim: ts=4 sw=4 et
