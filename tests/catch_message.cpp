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

// snapdev
//
#include    <as2js/message.h>



// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/enum_class_math.h>


// libutf8
//
#include    <libutf8/libutf8.h>


// C++
//
#include    <climits>


// last include
//
#include    <snapdev/poison.h>



namespace
{

class test_callback
    : public as2js::message_callback
{
public:
    test_callback()
    {
        as2js::set_message_callback(this);
        g_warning_count = as2js::warning_count();
        g_error_count = as2js::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::position const& pos, std::string const& message)
    {

//std::cerr<< " filename = " << pos.get_filename() << " / " << f_expected_pos.get_filename() << "\n";
//std::cerr<< " msg = [" << message << "] / [" << f_expected_message << "]\n";

        CATCH_REQUIRE(f_expected_call);
        CATCH_REQUIRE(message_level == f_expected_message_level);
        CATCH_REQUIRE(error_code == f_expected_error_code);
        CATCH_REQUIRE(pos.get_filename() == f_expected_pos.get_filename());
        CATCH_REQUIRE(pos.get_function() == f_expected_pos.get_function());
        CATCH_REQUIRE(pos.get_page() == f_expected_pos.get_page());
        CATCH_REQUIRE(pos.get_page_line() == f_expected_pos.get_page_line());
        CATCH_REQUIRE(pos.get_paragraph() == f_expected_pos.get_paragraph());
        CATCH_REQUIRE(pos.get_line() == f_expected_pos.get_line());
        CATCH_REQUIRE(message == f_expected_message);

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CATCH_REQUIRE(g_warning_count == as2js::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::error_count() << "\n";
            CATCH_REQUIRE(g_error_count == as2js::error_count());
        }

        f_got_called = true;
    }

    bool                        f_expected_call = true;
    bool                        f_got_called = false;
    as2js::message_level_t      f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
    as2js::err_code_t           f_expected_error_code = as2js::err_code_t::AS_ERR_NONE;
    as2js::position             f_expected_pos = as2js::position();
    std::string                 f_expected_message = std::string(); // UTF-8 string

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;

}
// no name namespace



CATCH_TEST_CASE("message_string", "[message]")
{
    CATCH_START_SECTION("message_string: check message outputs (use --verbose to see dots while processing)")
    {
        for(as2js::message_level_t i(as2js::message_level_t::MESSAGE_LEVEL_OFF);
                                   i <= as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                                   ++i)
        {
//i = static_cast<as2js::message_level_t>(static_cast<int>(i) + 1);
            if(SNAP_CATCH2_NAMESPACE::g_verbose())
            {
                std::cerr << "[" << static_cast<int32_t>(i) << "]";
            }

            for(as2js::err_code_t j(as2js::err_code_t::AS_ERR_NONE);
                                  j <= as2js::err_code_t::AS_ERR_max;
                                  ++j)
            {
                if(SNAP_CATCH2_NAMESPACE::g_verbose())
                {
                    std::cerr << ".";
                }

                {
                    test_callback c;
                    c.f_expected_message_level = i;
                    c.f_expected_error_code = j;
                    c.f_expected_pos.set_filename("unknown-file");
                    c.f_expected_pos.set_function("unknown-func");

                    for(as2js::message_level_t k(as2js::message_level_t::MESSAGE_LEVEL_OFF);
                                               k <= as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                                               ++k)
                    {
                        as2js::set_message_level(k);
                        as2js::message_level_t const min(std::min(k, as2js::message_level_t::MESSAGE_LEVEL_ERROR));
//std::cerr << "i: " << static_cast<int32_t>(i) << ", k: " << static_cast<int32_t>(k) << ", min: " << static_cast<int32_t>(min) << " expect: " << c.f_expected_call << "\n";
                        {
                            c.f_expected_call = false;
                            c.f_got_called = false;
                            c.f_expected_message = "";
                            as2js::message msg(i, j);
                        }
                        CATCH_REQUIRE_FALSE(c.f_got_called); // no message no call
                        {
                            char32_t unicode(random_char(SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_UNICODE));
                            c.f_expected_call = i != as2js::message_level_t::MESSAGE_LEVEL_OFF && i >= min;
                            c.f_got_called = false;
                            c.f_expected_message = "with a message: " + libutf8::to_u8string(unicode);
                            as2js::message msg(i, j);
                            msg << "with a message: " << unicode;
                        }
                        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
                    }
                }

                as2js::position pos;
                pos.set_filename("file.js");
                int total_line(1);
                for(int page(1); page < 10; ++page)
                {
                    //std::cerr << "+";

                    int paragraphs(rand() % 10 + 10);
                    int page_line(1);
                    int paragraph(1);
                    for(int line(1); line < 100; ++line)
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

                        {
                            test_callback c;
                            c.f_expected_message_level = i;
                            c.f_expected_error_code = j;
                            c.f_expected_pos = pos;
                            c.f_expected_pos.set_filename("file.js");
                            c.f_expected_pos.set_function("unknown-func");

                            for(as2js::message_level_t k(as2js::message_level_t::MESSAGE_LEVEL_OFF);
                                                       k <= as2js::message_level_t::MESSAGE_LEVEL_TRACE;
                                                       ++k)
                            {
                                as2js::set_message_level(k);
                                as2js::message_level_t const min(std::min(k, as2js::message_level_t::MESSAGE_LEVEL_ERROR));
                                {
                                    c.f_expected_call = false;
                                    c.f_got_called = false;
                                    c.f_expected_message = "";
                                    as2js::message msg(i, j, pos);
                                }
                                CATCH_REQUIRE(!c.f_got_called);
                                {
                                    c.f_expected_call = i != as2js::message_level_t::MESSAGE_LEVEL_OFF && i >= min;
                                    c.f_got_called = false;
                                    c.f_expected_message = "and a small message";
                                    as2js::message msg(i, j, pos);
                                    msg << "and a small message";
                                }
                                CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
                            }
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

            if(SNAP_CATCH2_NAMESPACE::g_verbose())
            {
                std::cerr << '\n';
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("message_operator", "[message]")
{
    CATCH_START_SECTION("message_operator: verify operators")
    {
        test_callback c;
        c.f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        c.f_expected_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        c.f_expected_pos.set_filename("operator.js");
        c.f_expected_pos.set_function("compute");
        as2js::set_message_level(as2js::message_level_t::MESSAGE_LEVEL_INFO);

        // test the copy constructor and operator
        {
            test_callback try_copy(c);
            test_callback try_assignment;
            try_assignment = c;
        }
        // this is required as the destructors called on the previous '}'
        // will otherwise clear that pointer...
        as2js::set_message_callback(&c);

        as2js::position pos;
        pos.set_filename("operator.js");
        pos.set_function("compute");
        c.f_expected_pos = pos;

        // test with nothing
        {
            c.f_expected_call = false;
            c.f_got_called = false;
            c.f_expected_message = "";
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        }
        CATCH_REQUIRE(!c.f_got_called); // no message no call

        // test with char *
        {
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = "with a message";
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << "with a message";
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with std::string
        {
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = "with an std::string message";
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            std::string str("with an std::string message");
            msg << str;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with ASCII wchar_t
        {
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = "Simple wide char string";
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            wchar_t const *str(L"Simple wide char string");
            msg << str;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with Unicode wchar_t
        {
            wchar_t const *str(L"Some: \x2028 Unicode \xA9");
            std::string unicode(as2js::convert(str));
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = unicode;
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << str;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with ASCII std::wstring
        {
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = "with an std::string message";
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            std::string str("with an std::string message");
            msg << str;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with Unicode std::wstring
        {
            std::wstring str(L"Some: \x2028 Unicode \xA9");
            std::string unicode(as2js::convert(str));
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = unicode;
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << unicode;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with std::string too
        {
            std::wstring str(L"Some: \x2028 Unicode \xA9");
            std::string unicode(as2js::convert(str));
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = unicode;
            as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << unicode;
        }
        CATCH_REQUIRE(c.f_expected_call == c.f_got_called);

        // test with char
        for(int idx(1); idx <= 255; ++idx)
        {
            char ci(static_cast<char>(idx));
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with signed char
        for(int idx(-128); idx <= 127; ++idx)
        {
            signed char ci(static_cast<signed char>(idx));
            {
                std::stringstream str;
                str << static_cast<int>(ci);
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with unsigned char
        for(int idx(-128); idx <= 127; ++idx)
        {
            unsigned char ci(static_cast<unsigned char>(idx));
            {
                std::stringstream str;
                str << static_cast<int>(ci);
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with signed short
        for(int idx(0); idx < 256; ++idx)
        {
            signed short ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << static_cast<int>(ci);
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with unsigned short
        for(int idx(0); idx < 256; ++idx)
        {
            unsigned short ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << static_cast<int>(ci);
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with signed int
        for(int idx(0); idx < 256; ++idx)
        {
            signed int ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with unsigned int
        for(int idx(0); idx < 256; ++idx)
        {
            unsigned int ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with signed long
        for(int idx(0); idx < 256; ++idx)
        {
            signed long ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with unsigned long
        for(int idx(0); idx < 256; ++idx)
        {
            unsigned long ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // if not 64 bits, then the next 2 tests should probably change a bit
        // to support the additional bits
        CATCH_REQUIRE(sizeof(unsigned long long) == 64 / 8);

        // test with signed long long
        for(int idx(0); idx < 256; ++idx)
        {
            signed long long ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with unsigned long long
        for(int idx(0); idx < 256; ++idx)
        {
            unsigned long long ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with Int64
        for(int idx(0); idx < 256; ++idx)
        {
            as2js::integer::value_type ci;
            SNAP_CATCH2_NAMESPACE::random(ci);
            as2js::integer value(ci);
            {
                std::stringstream str;
                str << ci;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << value;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with float
        for(int idx(0); idx < 256; ++idx)
        {
            float s1(rand() & 1 ? -1.0f : 1.0f);
            std::uint64_t rnd(0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            float n1(static_cast<float>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            float d1(static_cast<float>(rnd));
            float r(n1 / d1 * s1);
            {
                std::stringstream str;
                str << r;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << r;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with double
        for(int idx(0); idx < 256; ++idx)
        {
            double s1(rand() & 1 ? -1.0 : 1.0);
            std::uint64_t rnd(0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            double n1(static_cast<double>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            double d1(static_cast<double>(rnd));
            double r(n1 / d1 * s1);
            {
                std::stringstream str;
                str << r;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << r;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with Float64
        for(int idx(0); idx < 256; ++idx)
        {
            double s1(rand() & 1 ? -1.0 : 1.0);
            std::uint64_t rnd(0);
            SNAP_CATCH2_NAMESPACE::random(rnd);
            double n1(static_cast<double>(rnd));
            SNAP_CATCH2_NAMESPACE::random(rnd);
            while(rnd == 0) // denominator should not be zero
            {
                SNAP_CATCH2_NAMESPACE::random(rnd);
            }
            double d1(static_cast<double>(rnd));
            double r(n1 / d1 * s1);
            as2js::floating_point f(r);
            {
                std::stringstream str;
                str << r;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << f;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with bool
        for(int idx(0); idx <= 255; ++idx)
        {
            bool ci(static_cast<char>(idx));
            {
                std::stringstream str;
                str << static_cast<int>(ci);
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ci;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
        }

        // test with pointers
        for(int idx(0); idx <= 255; ++idx)
        {
            int *ptr(new int[5]);
            {
                std::stringstream str;
                str << ptr;
                c.f_expected_call = true;
                c.f_got_called = false;
                c.f_expected_message = str.str();
                as2js::message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
                msg << ptr;
            }
            CATCH_REQUIRE(c.f_expected_call == c.f_got_called);
            delete [] ptr;
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
