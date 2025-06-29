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
#include    <as2js/json.h>

#include    <as2js/exception.h>
#include    <as2js/message.h>


// self
//
#include    "catch_main.h"


// libutf8
//
#include    <libutf8/libutf8.h>


// ICU
//
// See http://icu-project.org/apiref/icu4c/index.html
#include    <unicode/uchar.h>
//#include    <unicode/cuchar> // once available in Linux...


// C++
//
#include    <limits>
#include    <cstring>
#include    <algorithm>
#include    <iomanip>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


std::int32_t generate_string(std::string & str, std::string & stringified)
{
    stringified += '"';
    char32_t c(U'\0');
    int32_t used(0);
    int ctrl(rand() % 7);
    int const max_chars(rand() % 25 + 5);
    for(int j(0); j < max_chars; ++j)
    {
        do
        {
            c = rand() & 0x1FFFFF;
            if(ctrl == 0)
            {
                ctrl = rand() % 7;
                if((ctrl & 3) == 1)
                {
                    c = c & 1 ? '"' : '\'';
                }
                else
                {
                    c &= 0x1F;
                }
            }
            else
            {
                --ctrl;
            }
        }
        while(c >= 0x110000
           || (c >= 0xD800 && c <= 0xDFFF)
           || ((c & 0xFFFE) == 0xFFFE)
           || c == '\\'         // this can cause problems (i.e. if followed by say an 'f' then it becomes the '\f' character, not '\' then an 'f'
           || c == '\0');
        str += libutf8::to_u8string(c);
        switch(c)
        {
        case U'\b':
            stringified += '\\';
            stringified += 'b';
            used |= 0x01;
            break;

        case U'\f':
            stringified += '\\';
            stringified += 'f';
            used |= 0x02;
            break;

        case U'\n':
            stringified += '\\';
            stringified += 'n';
            used |= 0x04;
            break;

        case U'\r':
            stringified += '\\';
            stringified += 'r';
            used |= 0x08;
            break;

        case U'\t':
            stringified += '\\';
            stringified += 't';
            used |= 0x10;
            break;

        case U'"':
            stringified += '\\';
            stringified += '"';
            used |= 0x20;
            break;

        case U'\'':
            // JSON does not expect the apostrophe (') to be escaped
            //stringified += '\\';
            stringified += '\'';
            used |= 0x40;
            break;

        case U'\\':
            stringified += "\\\\";
            //used |= 0x100; -- this is very unlikely to happen
            break;

        default:
            if(c < 0x0020 || c == 0x007F)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                stringified += ss.str();
                used |= 0x80;
            }
            else
            {
                stringified += libutf8::to_u8string(c);
            }
            break;

        }
    }
    stringified += '"';

    return used;
}


void stringify_string(std::string const & str, std::string & stringified)
{
    stringified += '"';
    size_t const max_chars(str.length());
    for(size_t j(0); j < max_chars; ++j)
    {
        // we essentially ignore UTF-8 in this case so we can just use the
        // bytes as is
        //
        char c(str[j]);
        switch(c)
        {
        case '\b':
            stringified += '\\';
            stringified += 'b';
            break;

        case '\f':
            stringified += '\\';
            stringified += 'f';
            break;

        case '\n':
            stringified += '\\';
            stringified += 'n';
            break;

        case '\r':
            stringified += '\\';
            stringified += 'r';
            break;

        case '\t':
            stringified += '\\';
            stringified += 't';
            break;

        case '\\':
            stringified += "\\\\";
            break;

        case '"':
            stringified += '\\';
            stringified += '"';
            break;

        case '\'':
            // JSON does not escape apostrophes (')
            //stringified += '\\';
            stringified += '\'';
            break;

        default:
            if(static_cast<std::uint8_t>(c) < 0x0020 || c == 0x007F)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(static_cast<std::uint8_t>(c));
                stringified += ss.str();
            }
            else
            {
                stringified += c;
            }
            break;

        }
    }
    stringified += '"';
}


struct test_data_t
{
    as2js::position                     f_pos = as2js::position();
    as2js::json::json_value::pointer_t  f_value = as2js::json::json_value::pointer_t();
    std::uint32_t                       f_count = 0;
};


int const TYPE_NULL             = 0x00000001;
int const TYPE_INTEGER          = 0x00000002;
int const TYPE_FLOATING_POINT   = 0x00000004;
int const TYPE_NAN              = 0x00000008;
int const TYPE_PINFINITY        = 0x00000010;
int const TYPE_MINFINITY        = 0x00000020;
int const TYPE_TRUE             = 0x00000040;
int const TYPE_FALSE            = 0x00000080;
int const TYPE_STRING           = 0x00000100;
int const TYPE_ARRAY            = 0x00000200;
int const TYPE_OBJECT           = 0x00000400;

int const TYPE_ALL              = 0x000007FF;

int g_type_used = 0;


std::string float_to_string(double f)
{
    std::string s(std::to_string(f));
    while(s.back() == '0')
    {
        s.pop_back();
    }
    if(s.back() == '.')
    {
        s.pop_back();
    }
    return s;
}


void create_item(
      test_data_t & data
    , as2js::json::json_value::pointer_t parent
    , int depth)
{
    std::size_t const max_items(rand() % 8 + 2);
    for(std::size_t j(0); j < max_items; ++j)
    {
        ++data.f_count;
        as2js::json::json_value::pointer_t item;
        int const select(rand() % 8);
        switch(select)
        {
        case 0: // NULL
            g_type_used |= TYPE_NULL;
            item = std::make_shared<as2js::json::json_value>(data.f_pos);
            break;

        case 1: // INTEGER
            g_type_used |= TYPE_INTEGER;
            {
                as2js::integer::value_type int_value((rand() << 13) ^ rand());
                as2js::integer integer(int_value);
                item = std::make_shared<as2js::json::json_value>(data.f_pos, integer);
            }
            break;

        case 2: // FLOATING_POINT
            switch(rand() % 10)
            {
            case 0:
                g_type_used |= TYPE_NAN;
                {
                    as2js::floating_point flt;
                    flt.set_nan();
                    item = std::make_shared<as2js::json::json_value>(data.f_pos, flt);
                }
                break;

            case 1:
                g_type_used |= TYPE_PINFINITY;
                {
                    as2js::floating_point flt;
                    flt.set_infinity();
                    item = std::make_shared<as2js::json::json_value>(data.f_pos, flt);
                }
                break;

            case 2:
                g_type_used |= TYPE_MINFINITY;
                {
                    as2js::floating_point::value_type flt_value(-std::numeric_limits<as2js::floating_point::value_type>::infinity());
                    as2js::floating_point flt(flt_value);
                    item = std::make_shared<as2js::json::json_value>(data.f_pos, flt);
                }
                break;

            default:
                g_type_used |= TYPE_FLOATING_POINT;
                {
                    as2js::floating_point::value_type flt_value(static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()) / static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()));
                    as2js::floating_point flt(flt_value);
                    item = std::make_shared<as2js::json::json_value>(data.f_pos, flt);
                }
                break;

            }
            break;

        case 3: // TRUE
            g_type_used |= TYPE_TRUE;
            item = std::make_shared<as2js::json::json_value>(data.f_pos, true);
            break;

        case 4: // FALSE
            g_type_used |= TYPE_FALSE;
            item = std::make_shared<as2js::json::json_value>(data.f_pos, false);
            break;

        case 5: // STRING
            g_type_used |= TYPE_STRING;
            {
                std::string str;
                std::string stringified;
                generate_string(str, stringified);
                item = std::make_shared<as2js::json::json_value>(data.f_pos, str);
            }
            break;

        case 6: // empty ARRAY
            g_type_used |= TYPE_ARRAY;
            {
                as2js::json::json_value::array_t empty_array;
                item = std::make_shared<as2js::json::json_value>(data.f_pos, empty_array);
                if(depth < 5 && (rand() & 1) != 0)
                {
                    create_item(data, item, depth + 1);
                }
            }
            break;

        case 7: // empty OBJECT
            g_type_used |= TYPE_OBJECT;
            {
                as2js::json::json_value::object_t empty_object;
                item = std::make_shared<as2js::json::json_value>(data.f_pos, empty_object);
                if(depth < 5 && (rand() & 1) != 0)
                {
                    create_item(data, item, depth + 1);
                }
            }
            break;

        // more?
        default:
            throw std::logic_error("test generated an invalid # to generate an object item");

        }
        if(parent->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY)
        {
            parent->set_item(parent->get_array().size(), item);
        }
        else
        {
            std::string field_name;
            std::string stringified_value;
            generate_string(field_name, stringified_value);
            parent->set_member(field_name, item);
        }
    }
}


void create_array(test_data_t & data)
{
    as2js::json::json_value::array_t array;
    data.f_value = std::make_shared<as2js::json::json_value>(data.f_pos, array);
    create_item(data, data.f_value, 0);
}


void create_object(test_data_t & data)
{
    as2js::json::json_value::object_t object;
    data.f_value = std::make_shared<as2js::json::json_value>(data.f_pos, object);
    create_item(data, data.f_value, 0);
}


void data_to_string(as2js::json::json_value::pointer_t value, std::string & expected)
{
    switch(value->get_type())
    {
    case as2js::json::json_value::type_t::JSON_TYPE_NULL:
        expected += "null";
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_TRUE:
        expected += "true";
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_FALSE:
        expected += "false";
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_INTEGER:
        expected += std::to_string(value->get_integer().get());
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT:
        if(value->get_floating_point().is_nan())
        {
            expected += "NaN";
        }
        else if(value->get_floating_point().is_positive_infinity())
        {
            expected += "Infinity";
        }
        else if(value->get_floating_point().is_negative_infinity())
        {
            expected += "-Infinity";
        }
        else
        {
            expected += float_to_string(value->get_floating_point().get());
        }
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_STRING:
        stringify_string(value->get_string(), expected);
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_ARRAY:
        expected += '[';
        {
            bool first(true);
            for(auto it : value->get_array())
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    expected += ',';
                }
                data_to_string(it, expected); // recursive
            }
        }
        expected += ']';
        break;

    case as2js::json::json_value::type_t::JSON_TYPE_OBJECT:
        expected += '{';
        {
            bool first(true);
            for(auto it : value->get_object())
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    expected += ',';
                }
                stringify_string(it.first, expected);
                expected += ':';
                data_to_string(it.second, expected); // recursive
            }
        }
        expected += '}';
        break;

    // more?
    default:
        throw std::logic_error("test found an invalid JSONValue::type_t to stringify a value item");

    }
}


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
    virtual void output(
          as2js::message_level_t message_level
        , as2js::err_code_t error_code
        , as2js::position const & pos
        , std::string const & message)
    {
        CATCH_REQUIRE_FALSE(f_expected.empty());

//std::cerr << "filename = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr << "error_code = " << static_cast<int>(error_code) << " / " << static_cast<int>(f_expected[0].f_error_code) << "\n";

        CATCH_REQUIRE(f_expected[0].f_call);
        CATCH_REQUIRE(message_level == f_expected[0].f_message_level);
        CATCH_REQUIRE(error_code == f_expected[0].f_error_code);
        CATCH_REQUIRE(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CATCH_REQUIRE(pos.get_function() == f_expected[0].f_pos.get_function());
        CATCH_REQUIRE(pos.get_page() == f_expected[0].f_pos.get_page());
        CATCH_REQUIRE(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CATCH_REQUIRE(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CATCH_REQUIRE(pos.get_line() == f_expected[0].f_pos.get_line());
        CATCH_REQUIRE(message == f_expected[0].f_message);

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

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        if(!f_expected.empty())
        {
            std::cerr << "\n*** STILL EXPECTED: ***\n";
            std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
            std::cerr << "msg = " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "error_code = " << static_cast<int>(f_expected[0].f_error_code) << "\n";
        }
        CATCH_REQUIRE(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::position             f_pos = as2js::position();
        std::string                 f_message = std::string(); // UTF-8 string
    };

    std::vector<expected_t>     f_expected = std::vector<expected_t>();

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;


bool is_identifier_char(std::int32_t const c)
{
    // special cases in JavaScript identifiers
    if(c == 0x200C    // ZWNJ
    || c == 0x200D)   // ZWJ
    {
        return true;
    }

    switch(u_charType(static_cast<UChar32>(c)))
    {
    case U_UPPERCASE_LETTER:
    case U_LOWERCASE_LETTER:
    case U_TITLECASE_LETTER:
    case U_MODIFIER_LETTER:
    case U_OTHER_LETTER:
    case U_LETTER_NUMBER:
    case U_NON_SPACING_MARK:
    case U_COMBINING_SPACING_MARK:
    case U_DECIMAL_DIGIT_NUMBER:
    case U_CONNECTOR_PUNCTUATION:
        return true;

    default:
        return false;

    }
}


}
// no name namespace




CATCH_TEST_CASE("json_basic_values", "[json][basic]")
{
    // a null pointer value...
    as2js::json::json_value::pointer_t const nullptr_value;

    // NULL value
    CATCH_START_SECTION("json: NULL value")
    {
        as2js::position pos;
        pos.reset_counters(33);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::json::json_value::pointer_t value(new as2js::json::json_value(pos));
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_NULL);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const& p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == pos.get_filename());
        CATCH_REQUIRE(p.get_function() == pos.get_function());
        CATCH_REQUIRE(p.get_line() == 33);
        CATCH_REQUIRE(value->to_string() == "null");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_NULL);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == pos.get_filename());
        CATCH_REQUIRE(q.get_function() == pos.get_function());
        CATCH_REQUIRE(q.get_line() == 33);
        CATCH_REQUIRE(copy.to_string() == "null");
    }
    CATCH_END_SECTION()

    // TRUE value
    CATCH_START_SECTION("json: TRUE value")
    {
        as2js::position pos;
        pos.reset_counters(35);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, true));
        // modify out pos object to make sure that the one in value is not a reference
        pos.set_filename("verify.json");
        pos.set_function("bad_objects");
        pos.new_line();
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == "data.json");
        CATCH_REQUIRE(p.get_function() == "save_objects");
        CATCH_REQUIRE(p.get_line() == 35);
        CATCH_REQUIRE(value->to_string() == "true");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == "data.json");
        CATCH_REQUIRE(q.get_function() == "save_objects");
        CATCH_REQUIRE(q.get_line() == 35);
        CATCH_REQUIRE(copy.to_string() == "true");
    }
    CATCH_END_SECTION()

    // FALSE value
    CATCH_START_SECTION("json: FALSE value")
    {
        as2js::position pos;
        pos.reset_counters(53);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, false));
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FALSE);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == pos.get_filename());
        CATCH_REQUIRE(p.get_function() == pos.get_function());
        CATCH_REQUIRE(p.get_line() == 53);
        CATCH_REQUIRE(value->to_string() == "false");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_FALSE);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == pos.get_filename());
        CATCH_REQUIRE(q.get_function() == pos.get_function());
        CATCH_REQUIRE(q.get_line() == 53);
        CATCH_REQUIRE(copy.to_string() == "false");
    }
    CATCH_END_SECTION()

    // INTEGER value
    CATCH_START_SECTION("json: INTEGER value")
    {
        for(int idx(0); idx < 100; ++idx)
        {
            as2js::position pos;
            pos.reset_counters(103);
            pos.set_filename("data.json");
            pos.set_function("save_objects");
            as2js::integer::value_type int_value((rand() << 14) ^ rand());
            as2js::integer integer(int_value);
            as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, integer));
            CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_INTEGER);
            CATCH_REQUIRE(value->get_integer().get() == int_value);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & p(value->get_position());
            CATCH_REQUIRE(p.get_filename() == pos.get_filename());
            CATCH_REQUIRE(p.get_function() == pos.get_function());
            CATCH_REQUIRE(p.get_line() == 103);
            std::stringstream ss;
            ss << integer.get();
            std::string cmp(ss.str());
            CATCH_REQUIRE(value->to_string() == cmp);
            // copy operator
            as2js::json::json_value copy(*value);
            CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_INTEGER);
            CATCH_REQUIRE(copy.get_integer().get() == int_value);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & q(copy.get_position());
            CATCH_REQUIRE(q.get_filename() == pos.get_filename());
            CATCH_REQUIRE(q.get_function() == pos.get_function());
            CATCH_REQUIRE(q.get_line() == 103);
            CATCH_REQUIRE(copy.to_string() == cmp);
        }
    }
    CATCH_END_SECTION()

    // FLOATING_POINT value
    CATCH_START_SECTION("json: FLOATING_POINT NaN value")
    {
        as2js::position pos;
        pos.reset_counters(144);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::floating_point::value_type flt_value(std::numeric_limits<as2js::floating_point::value_type>::quiet_NaN());
        as2js::floating_point flt(flt_value);
        as2js::json::json_value::pointer_t value(new as2js::json::json_value(pos, flt));
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // NaN's do not compare equal
        bool const unequal_nan(value->get_floating_point().get() != flt_value);
#pragma GCC diagnostic pop
        CATCH_REQUIRE(unequal_nan);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == pos.get_filename());
        CATCH_REQUIRE(p.get_function() == pos.get_function());
        CATCH_REQUIRE(p.get_line() == 144);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CATCH_REQUIRE(value->to_string() == "NaN");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // NaN's do not compare equal
        bool const copy_unequal_nan(copy.get_floating_point().get() != flt_value);
#pragma GCC diagnostic pop
        CATCH_REQUIRE(copy_unequal_nan);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == pos.get_filename());
        CATCH_REQUIRE(q.get_function() == pos.get_function());
        CATCH_REQUIRE(q.get_line() == 144);
        CATCH_REQUIRE(copy.to_string() == "NaN");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: FLOATING_POINT value")
    {
        for(int idx(0); idx < 100; ++idx)
        {
            as2js::position pos;
            pos.reset_counters(44);
            pos.set_filename("data.json");
            pos.set_function("save_objects");
            as2js::floating_point::value_type flt_value(static_cast<as2js::floating_point::value_type>(rand()) / static_cast<as2js::floating_point::value_type>(rand()));
            as2js::floating_point flt(flt_value);
            as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, flt));
            CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_FLOATING_POINT(value->get_floating_point().get(), flt_value);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & p(value->get_position());
            CATCH_REQUIRE(p.get_filename() == pos.get_filename());
            CATCH_REQUIRE(p.get_function() == pos.get_function());
            CATCH_REQUIRE(p.get_line() == 44);
            std::string const cmp(float_to_string(flt_value));
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
            CATCH_REQUIRE(value->to_string() == cmp);
            // copy operator
            as2js::json::json_value copy(*value);
            CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_FLOATING_POINT(copy.get_floating_point().get(), flt_value);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & q(copy.get_position());
            CATCH_REQUIRE(q.get_filename() == pos.get_filename());
            CATCH_REQUIRE(q.get_function() == pos.get_function());
            CATCH_REQUIRE(q.get_line() == 44);
            CATCH_REQUIRE(copy.to_string() == cmp);
        }
    }
    CATCH_END_SECTION()

    // STRING value
    CATCH_START_SECTION("json: STRING value")
    {
        for(size_t idx(0), used(0); idx < 100 || used != 0xFF; ++idx)
        {
            as2js::position pos;
            pos.reset_counters(89);
            pos.set_filename("data.json");
            pos.set_function("save_objects");
            std::string str;
            std::string stringified;
            used |= generate_string(str, stringified);
            as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, str));
            CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE(value->get_string() == str);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & p(value->get_position());
            CATCH_REQUIRE(p.get_filename() == pos.get_filename());
            CATCH_REQUIRE(p.get_function() == pos.get_function());
            CATCH_REQUIRE(p.get_line() == 89);
#if 0
std::string r(value->to_string());
std::cerr << std::hex << " lengths " << r.length() << " / " << stringified.length() << "\n";
size_t max_chrs(std::min(r.length(), stringified.length()));
for(size_t g(0); g < max_chrs; ++g)
{
    if(static_cast<int>(r[g]) != static_cast<int>(stringified[g]))
    {
        std::cerr << " --- " << static_cast<int>(static_cast<std::uint8_t>(r[g])) << " / " << static_cast<int>(static_cast<std::uint8_t>(stringified[g])) << "\n";
    }
    else
    {
        std::cerr << " " << static_cast<int>(static_cast<std::uint8_t>(r[g])) << " / " << static_cast<int>(static_cast<std::uint8_t>(stringified[g])) << "\n";
    }
}
if(r.length() > stringified.length())
{
    for(size_t g(stringified.length()); g < r.length(); ++g)
    {
        std::cerr << " *** " << static_cast<int>(static_cast<std::uint8_t>(r[g])) << "\n";
    }
}
else
{
    for(size_t g(r.length()); g < stringified.length(); ++g)
    {
        std::cerr << " +++ " << static_cast<int>(static_cast<std::uint8_t>(stringified[g])) << "\n";
    }
}
std::cerr << std::dec;
#endif
            CATCH_REQUIRE(value->to_string() == stringified);
            // copy operator
            as2js::json::json_value copy(*value);
            CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_STRING);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE(copy.get_string() == str);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_member("name", nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_member() called with a non-object value type."));

            as2js::position const & q(copy.get_position());
            CATCH_REQUIRE(q.get_filename() == pos.get_filename());
            CATCH_REQUIRE(q.get_function() == pos.get_function());
            CATCH_REQUIRE(q.get_line() == 89);
            CATCH_REQUIRE(copy.to_string() == stringified);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_array", "[json][array]")
{
    // a null pointer value...
    as2js::json::json_value::pointer_t const nullptr_value;

    // test with an empty array
    CATCH_START_SECTION("json: empty array")
    {
        as2js::position pos;
        pos.reset_counters(109);
        pos.set_filename("array.json");
        pos.set_function("save_array");
        as2js::json::json_value::array_t initial;
        as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, initial));
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        as2js::json::json_value::array_t const & array(value->get_array());
        CATCH_REQUIRE(array.empty());
        for(int idx(-10); idx <= 10; ++idx)
        {
            if(idx == 0)
            {
                // nullptr is not valid for data
                CATCH_REQUIRE_THROWS_MATCHES(
                          value->set_item(idx, nullptr_value)
                        , as2js::invalid_data
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: json::json_value::set_item() called with a null pointer as the value."));
            }
            else
            {
                // index is invalid
                CATCH_REQUIRE_THROWS_MATCHES(
                          value->set_item(idx, nullptr_value)
                        , as2js::out_of_range
                        , Catch::Matchers::ExceptionMessage(
                                  "out_of_range: json::json_value::set_item() called with an index out of range."));
            }
        }

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const & p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == pos.get_filename());
        CATCH_REQUIRE(p.get_function() == pos.get_function());
        CATCH_REQUIRE(p.get_line() == 109);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CATCH_REQUIRE(value->to_string() == "[]");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        as2js::json::json_value::array_t const & array_copy(copy.get_array());
        CATCH_REQUIRE(array_copy.empty());
        for(int idx(-10); idx <= 10; ++idx)
        {
            if(idx == 0)
            {
                // nullptr is not valid for data
                //CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::invalid_data);

                CATCH_REQUIRE_THROWS_MATCHES(
                          copy.set_item(idx, nullptr_value)
                        , as2js::invalid_data
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: json::json_value::set_item() called with a null pointer as the value."));
            }
            else
            {
                // index is invalid
                //CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::out_of_range);

                CATCH_REQUIRE_THROWS_MATCHES(
                          copy.set_item(idx, nullptr_value)
                        , as2js::out_of_range
                        , Catch::Matchers::ExceptionMessage(
                                  "out_of_range: json::json_value::set_item() called with an index out of range."));
            }
        }

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_object()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_object() called with a non-object value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("name", nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_member() called with a non-object value type."));

        as2js::position const& q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == pos.get_filename());
        CATCH_REQUIRE(q.get_function() == pos.get_function());
        CATCH_REQUIRE(q.get_line() == 109);
        CATCH_REQUIRE(copy.to_string() == "[]");
    }
    CATCH_END_SECTION()

    // test with a few random arrays
    CATCH_START_SECTION("json: random array value")
    {
        for(int idx(0); idx < 10; ++idx)
        {
            as2js::position pos;
            pos.reset_counters(109);
            pos.set_filename("array.json");
            pos.set_function("save_array");
            as2js::json::json_value::array_t initial;

            std::string result("[");
            size_t const max_items(rand() % 100 + 20);
            for(size_t j(0); j < max_items; ++j)
            {
                if(j != 0)
                {
                    result += ",";
                }
                as2js::json::json_value::pointer_t item;
                int const select(rand() % 8);
                switch(select)
                {
                case 0: // NULL
                    item.reset(new as2js::json::json_value(pos));
                    result += "null";
                    break;

                case 1: // INTEGER
                    {
                        as2js::integer::value_type int_value((rand() << 13) ^ rand());
                        as2js::integer integer(int_value);
                        item = std::make_shared<as2js::json::json_value>(pos, integer);
                        result += std::to_string(int_value);
                    }
                    break;

                case 2: // FLOATING_POINT
                    {
                        as2js::floating_point::value_type flt_value(static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()) / static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()));
                        as2js::floating_point flt(flt_value);
                        item = std::make_shared<as2js::json::json_value>(pos, flt);
                        result += float_to_string(flt_value);
                    }
                    break;

                case 3: // TRUE
                    item.reset(new as2js::json::json_value(pos, true));
                    result += "true";
                    break;

                case 4: // FALSE
                    item.reset(new as2js::json::json_value(pos, false));
                    result += "false";
                    break;

                case 5: // STRING
                    {
                        std::string str;
                        std::string stringified;
                        generate_string(str, stringified);
                        item.reset(new as2js::json::json_value(pos, str));
                        result += stringified;
                    }
                    break;

                case 6: // empty ARRAY
                    {
                        as2js::json::json_value::array_t empty_array;
                        item.reset(new as2js::json::json_value(pos, empty_array));
                        result += "[]";
                    }
                    break;

                case 7: // empty OBJECT
                    {
                        as2js::json::json_value::object_t empty_object;
                        item.reset(new as2js::json::json_value(pos, empty_object));
                        result += "{}";
                    }
                    break;

                // more?
                default:
                    throw std::logic_error("test generated an invalid # to generate an array item");

                }
                initial.push_back(item);
            }
            result += "]";

            as2js::json::json_value::pointer_t value(std::make_shared<as2js::json::json_value>(pos, initial));
            CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            as2js::json::json_value::array_t const& array(value->get_array());
            CATCH_REQUIRE(array.size() == max_items);
            //for(int idx(-10); idx <= 10; ++idx)
            //{
            //    if(idx == 0)
            //    {
            //        // nullptr is not valid for data
            //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::invalid_data);
            //    }
            //    else
            //    {
            //        // index is invalid
            //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::out_of_range);
            //    }
            //}

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            // now setting member to nullptr deletes it from the object
            //CATCH_REQUIRE_THROWS_MATCHES(
            //          value->set_member("name", nullptr_value)
            //        , as2js::internal_error
            //        , Catch::Matchers::ExceptionMessage(
            //                  "internal_error: set_member() called with a non-object value type"));

            //CPPUNIT_ASSERT_THROW(value->get_object(), as2js::internal_error);
            //CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::internal_error);
            as2js::position const& p(value->get_position());
            CATCH_REQUIRE(p.get_filename() == pos.get_filename());
            CATCH_REQUIRE(p.get_function() == pos.get_function());
            CATCH_REQUIRE(p.get_line() == 109);
#if 0
std::string r(value->to_string());
std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
size_t max_chrs(std::min(r.length(), result.length()));
for(size_t g(0); g < max_chrs; ++g)
{
    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
    {
        std::cerr << " --- " << static_cast<int>(static_cast<std::uint8_t>(r[g])) << " / " << static_cast<int>(static_cast<std::uint8_t>(result[g])) << "\n";
    }
    else
    {
        std::cerr << " " << static_cast<int>(static_cast<std::uint8_t>(r[g])) << " / " << static_cast<int>(static_cast<std::uint8_t>(result[g])) << "\n";
    }
}
if(r.length() > result.length())
{
}
else
{
    for(size_t g(r.length()); g < result.length(); ++g)
    {
        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
    }
}
std::cerr << std::dec;
#endif
            CATCH_REQUIRE(value->to_string() == result);
            // copy operator
            as2js::json::json_value copy(*value);
            CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            as2js::json::json_value::array_t const& array_copy(copy.get_array());
            CATCH_REQUIRE(array_copy.size() == max_items);
            //for(int idx(-10); idx <= 10; ++idx)
            //{
            //    if(idx == 0)
            //    {
            //        // nullptr is not valid for data
            //        CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::invalid_data);
            //    }
            //    else
            //    {
            //        // index is invalid
            //        CATCH_REQUIRE_THROWS_MATCHES(copy.set_item(idx, nullptr_value), as2js::out_of_range);
            //    }
            //}

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_object()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_object() called with a non-object value type."));

            // this now works as "delete that element in that object"
            //CATCH_REQUIRE_THROWS_MATCHES(
            //          copy.set_member("name", nullptr_value)
            //        , as2js::internal_error
            //        , Catch::Matchers::ExceptionMessage(
            //                  "set_member() called with a non-object value type."));

            //CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::internal_error);
            //CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::internal_error);
            as2js::position const& q(copy.get_position());
            CATCH_REQUIRE(q.get_filename() == pos.get_filename());
            CATCH_REQUIRE(q.get_function() == pos.get_function());
            CATCH_REQUIRE(q.get_line() == 109);
            CATCH_REQUIRE(copy.to_string() == result);
            // the cyclic flag should have been reset, make sure of that:
            CATCH_REQUIRE(copy.to_string() == result);

            // test that we catch a direct 'array[x] = array;'
            value->set_item(max_items, value);
            // copy is not affected...
            CATCH_REQUIRE(copy.to_string() == result);
            // value to string fails because it is cyclic
            //CPPUNIT_ASSERT_THROW(value->to_string() == result, as2js::cyclical_structure);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->to_string()
                    , as2js::cyclical_structure
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: JSON cannot stringify a set of objects and arrays which are cyclical."));

            as2js::json::json_value::array_t const& cyclic_array(value->get_array());
            CATCH_REQUIRE(cyclic_array.size() == max_items + 1);

            {
                std::string str;
                std::string stringified;
                generate_string(str, stringified);
                as2js::json::json_value::pointer_t item;
                item.reset(new as2js::json::json_value(pos, str));
                // remove the existing ']' first
                result.erase(result.end() - 1);
                result += ',';
                result += stringified;
                result += ']';
                value->set_item(max_items, item);
//std::string r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
                CATCH_REQUIRE(value->to_string() == result);
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_object", "[json][object]")
{
    // a null pointer value...
    as2js::json::json_value::pointer_t const nullptr_value;

    // test with an empty object
    CATCH_START_SECTION("json: empty object")
    {
        as2js::position pos;
        pos.reset_counters(109);
        pos.set_filename("object.json");
        pos.set_function("save_object");
        as2js::json::json_value::object_t initial;
        as2js::json::json_value::pointer_t value(new as2js::json::json_value(pos, initial));
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        as2js::json::json_value::object_t const & object(value->get_object());
        CATCH_REQUIRE(object.empty());
        // name is invalid
        //CPPUNIT_ASSERT_THROW(value->set_member("", nullptr_value), as2js::invalid_index);

        CATCH_REQUIRE_THROWS_MATCHES(
                  value->set_member("", nullptr_value)
                , as2js::invalid_index
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: json::json_value::set_member() called with an empty string as the member name."));

        // nullptr is not valid for data
        //CPPUNIT_ASSERT_THROW(value->set_member("ignore", nullptr_value), as2js::invalid_data);

        // in the new version, setting to a nullptr means remove that member
        //CATCH_REQUIRE_THROWS_MATCHES(
        //          value->set_member("ignore", nullptr_value)
        //        , as2js::invalid_data
        //        , Catch::Matchers::ExceptionMessage(
        //                  "set_member() called with a non-member value type."));

        as2js::position const& p(value->get_position());
        CATCH_REQUIRE(p.get_filename() == pos.get_filename());
        CATCH_REQUIRE(p.get_function() == pos.get_function());
        CATCH_REQUIRE(p.get_line() == 109);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CATCH_REQUIRE(value->to_string() == "{}");
        // copy operator
        as2js::json::json_value copy(*value);
        CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_integer().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_integer() called with a non-integer value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_floating_point().get()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_floating_point() called with a non-floating point value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_string()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_string() called with a non-string value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.get_array()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: get_array() called with a non-array value type."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_item(rand(), nullptr_value)
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: set_item() called with a non-array value type."));

        as2js::json::json_value::object_t const& object_copy(copy.get_object());
        CATCH_REQUIRE(object_copy.empty());
        // name is invalid
        //CPPUNIT_ASSERT_THROW(copy.set_member("", nullptr_value), as2js::invalid_index);

        CATCH_REQUIRE_THROWS_MATCHES(
                  copy.set_member("", nullptr_value)
                , as2js::invalid_index
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: json::json_value::set_member() called with an empty string as the member name."));

        // nullptr is not valid for data
        //CPPUNIT_ASSERT_THROW(copy.set_member("ignore", nullptr_value), as2js::invalid_data);

        // setting a member to nullptr is now equivalent to deleting it
        //CATCH_REQUIRE_THROWS_MATCHES(
        //          copy.set_member("ignore", nullptr_value)
        //        , as2js::invalid_data
        //        , Catch::Matchers::ExceptionMessage(
        //                  "as2js: wrong."));

        as2js::position const& q(copy.get_position());
        CATCH_REQUIRE(q.get_filename() == pos.get_filename());
        CATCH_REQUIRE(q.get_function() == pos.get_function());
        CATCH_REQUIRE(q.get_line() == 109);
        CATCH_REQUIRE(copy.to_string() == "{}");
    }
    CATCH_END_SECTION()

    // test with a few random objects
    CATCH_START_SECTION("json: random objects")
    {
        typedef std::map<std::string, std::string>  sort_t;
        for(int idx(0); idx < 10; ++idx)
        {
            as2js::position pos;
            pos.reset_counters(199);
            pos.set_filename("object.json");
            pos.set_function("save_object");
            as2js::json::json_value::object_t initial;
            sort_t sorted;

            size_t const max_items(rand() % 100 + 20);
            for(size_t j(0); j < max_items; ++j)
            {
                std::string field_name;
                std::string stringified_value;
                generate_string(field_name, stringified_value);
                stringified_value += ':';
                as2js::json::json_value::pointer_t item;
                int const select(rand() % 8);
                switch(select)
                {
                case 0: // NULL
                    item.reset(new as2js::json::json_value(pos));
                    stringified_value += "null";
                    break;

                case 1: // INTEGER
                    {
                        as2js::integer::value_type int_value((rand() << 13) ^ rand());
                        as2js::integer integer(int_value);
                        item = std::make_shared<as2js::json::json_value>(pos, integer);
                        stringified_value += std::to_string(int_value);
                    }
                    break;

                case 2: // FLOATING_POINT
                    {
                        as2js::floating_point::value_type flt_value(static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()) / static_cast<as2js::floating_point::value_type>((rand() << 16) | rand()));
                        as2js::floating_point flt(flt_value);
                        item.reset(new as2js::json::json_value(pos, flt));
                        stringified_value += float_to_string(flt_value);
                    }
                    break;

                case 3: // TRUE
                    item.reset(new as2js::json::json_value(pos, true));
                    stringified_value += "true";
                    break;

                case 4: // FALSE
                    item.reset(new as2js::json::json_value(pos, false));
                    stringified_value += "false";
                    break;

                case 5: // STRING
                    {
                        std::string str;
                        std::string stringified;
                        generate_string(str, stringified);
                        item.reset(new as2js::json::json_value(pos, str));
                        stringified_value += stringified;
                    }
                    break;

                case 6: // empty ARRAY
                    {
                        as2js::json::json_value::array_t empty_array;
                        item.reset(new as2js::json::json_value(pos, empty_array));
                        stringified_value += "[]";
                    }
                    break;

                case 7: // empty OBJECT
                    {
                        as2js::json::json_value::object_t empty_object;
                        item.reset(new as2js::json::json_value(pos, empty_object));
                        stringified_value += "{}";
                    }
                    break;

                // more?
                default:
                    throw std::logic_error("test generated an invalid # to generate an object item");

                }
                initial[field_name] = item;
                sorted[field_name] = stringified_value;
            }
            std::string result("{");
            bool first(true);
            for(auto it : sorted)
            {
                if(!first)
                {
                    result += ',';
                }
                else
                {
                    first = false;
                }
                result += it.second;
            }
            result += "}";

            as2js::json::json_value::pointer_t value(new as2js::json::json_value(pos, initial));
            CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            as2js::json::json_value::object_t const& object(value->get_object());
            CATCH_REQUIRE(object.size() == max_items);
            //for(int idx(-10); idx <= 10; ++idx)
            //{
            //    if(idx == 0)
            //    {
            //        // nullptr is not valid for data
            //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::invalid_data);
            //    }
            //    else
            //    {
            //        // index is invalid
            //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::out_of_range);
            //    }
            //}
            as2js::position const& p(value->get_position());
            CATCH_REQUIRE(p.get_filename() == pos.get_filename());
            CATCH_REQUIRE(p.get_function() == pos.get_function());
            CATCH_REQUIRE(p.get_line() == 199);
//std::string r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//    for(size_t g(result.length()); g < r.length(); ++g)
//    {
//        std::cerr << " *** " << static_cast<int>(r[g]) << "\n";
//    }
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
            CATCH_REQUIRE(value->to_string() == result);
            // copy operator
            as2js::json::json_value copy(*value);
            CATCH_REQUIRE(copy.get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_integer().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_floating_point().get()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with a non-string value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.get_array()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_array() called with a non-array value type."));

            CATCH_REQUIRE_THROWS_MATCHES(
                      copy.set_item(rand(), nullptr_value)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_item() called with a non-array value type."));

            as2js::json::json_value::object_t const& object_copy(copy.get_object());
            CATCH_REQUIRE(object_copy.size() == max_items);
            //for(int idx(-10); idx <= 10; ++idx)
            //{
            //    if(idx == 0)
            //    {
            //        // nullptr is not valid for data
            //        CPPUNIT_ASSERT_THROW(copy.set_member("", nullptr_value), as2js::invalid_data);
            //    }
            //    else
            //    {
            //        // index is invalid
            //        CPPUNIT_ASSERT_THROW(copy.set_member("ingore", nullptr_value), as2js::out_of_range);
            //    }
            //}
            as2js::position const & q(copy.get_position());
            CATCH_REQUIRE(q.get_filename() == pos.get_filename());
            CATCH_REQUIRE(q.get_function() == pos.get_function());
            CATCH_REQUIRE(q.get_line() == 199);
            CATCH_REQUIRE(copy.to_string() == result);
            // the cyclic flag should have been reset, make sure of that:
            CATCH_REQUIRE(copy.to_string() == result);

            // test that we catch a direct 'object[x] = object;'
            value->set_member("random", value);
            // copy is not affected...
            CATCH_REQUIRE(copy.to_string() == result);
            // value to string fails because it is cyclic
            //CPPUNIT_ASSERT_THROW(value->to_string() == result, as2js::cyclical_structure);

            CATCH_REQUIRE_THROWS_MATCHES(
                      value->to_string()
                    , as2js::cyclical_structure
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: JSON cannot stringify a set of objects and arrays which are cyclical."));

            as2js::json::json_value::object_t const& cyclic_object(value->get_object());
            CATCH_REQUIRE(cyclic_object.size() == max_items + 1);

            {
                std::string str;
                std::string stringified("\"random\":");
                generate_string(str, stringified);
                as2js::json::json_value::pointer_t item;
                item = std::make_shared<as2js::json::json_value>(pos, str);
                sorted["random"] = stringified;
                // with objects the entire result needs to be rebuilt
                result = "{";
                first = true;
                for(auto it : sorted)
                {
                    if(!first)
                    {
                        result += ',';
                    }
                    else
                    {
                        first = false;
                    }
                    result += it.second;
                }
                result += "}";
                value->set_member("random", item);
//std::string r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//    for(size_t g(result.length()); g < r.length(); ++g)
//    {
//        std::cerr << " *** " << static_cast<int>(r[g]) << "\n";
//    }
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
                CATCH_REQUIRE(value->to_string() == result);
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_random_object", "[json][object]")
{
    CATCH_START_SECTION("json: random objects and arrays")
    {
        // test with a few random objects
        g_type_used = 0;
        typedef std::map<std::string, std::string>  sort_t;
        for(int idx(0); idx < 10 || g_type_used != TYPE_ALL; ++idx)
        {
            std::string const header(rand() & 1 ? "// we can have a C++ comment\n/* or even a C like comment in the header\n(not the rest because we do not have access...) */\n" : "");

            test_data_t data;
            data.f_pos.reset_counters(199);
            data.f_pos.set_filename("full.json");
            data.f_pos.set_function("save_full");

            if(rand() & 1)
            {
                create_object(data);
            }
            else
            {
                create_array(data);
            }
            std::string expected;
            //expected += 0xFEFF; // BOM
            expected += header;
            if(!header.empty())
            {
                expected += '\n';
            }
            data_to_string(data.f_value, expected);
//std::cerr << "created " << data.f_count << " items.\n";

            as2js::json::pointer_t json(std::make_shared<as2js::json>());
            json->set_value(data.f_value);

            as2js::output_stream<std::stringstream>::pointer_t out(std::make_shared<as2js::output_stream<std::stringstream>>());
            json->output(out, header);
            std::string const& result(out->str());
#if 0
{
std::cerr << std::hex << " lengths " << expected.length() << " / " << result.length() << "\n";
size_t max_chrs(std::min(expected.length(), result.length()));
for(size_t g(0); g < max_chrs; ++g)
{
    if(static_cast<int>(expected[g]) != static_cast<int>(result[g]))
    {
        std::cerr << " --- " << static_cast<int>(expected[g]) << " / " << static_cast<int>(result[g]) << "\n";
    }
    else
    {
        std::cerr << " " << static_cast<int>(expected[g]) << " / " << static_cast<int>(result[g]) << "\n";
    }
}
if(expected.length() > result.length())
{
    for(size_t g(result.length()); g < expected.length(); ++g)
    {
        std::cerr << " *** " << static_cast<int>(expected[g]) << "\n";
    }
}
else
{
    for(size_t g(expected.length()); g < result.length(); ++g)
    {
        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
    }
}
std::cerr << std::dec;
}
#endif
            CATCH_REQUIRE(result == expected);

            CATCH_REQUIRE(json->get_value() == data.f_value);
            // make sure the tree is also correct:
            std::string expected_tree;
            //expected_tree += 0xFEFF; // BOM
            expected_tree += header;
            if(!header.empty())
            {
                expected_tree += '\n';
            }
            data_to_string(json->get_value(), expected_tree);
            CATCH_REQUIRE(expected_tree == expected);

            // copy operator
            as2js::json copy(*json);

            // the copy gets the exact same value pointer...
            CATCH_REQUIRE(copy.get_value() == data.f_value);
            // make sure the tree is also correct:
            std::string expected_copy;
            //expected_copy += 0xFEFF; // BOM
            expected_copy += header;
            if(!header.empty())
            {
                expected_copy += '\n';
            }
            data_to_string(copy.get_value(), expected_copy);
            CATCH_REQUIRE(expected_copy == expected);

            // create an unsafe temporary file and save that JSON in there...
            int number(rand() % 1000000);
            std::stringstream ss;
            ss << SNAP_CATCH2_NAMESPACE::g_tmp_dir() << "/json_test" << std::setfill('0') << std::setw(6) << number << ".js";
//std::cerr << "filename [" << ss.str() << "]\n";
            std::string const filename(ss.str());
            json->save(filename, header);

            as2js::json::pointer_t load_json(std::make_shared<as2js::json>());
            as2js::json::json_value::pointer_t loaded_value(load_json->load(filename));
            CATCH_REQUIRE(loaded_value == load_json->get_value());

            as2js::output_stream<std::stringstream>::pointer_t lout(new as2js::output_stream<std::stringstream>());
            load_json->output(lout, header);
            std::string const & lresult(lout->str());
{
std::ofstream co;
co.open(filename + "2");
CATCH_REQUIRE(co.is_open());
co << lresult;
}

            CATCH_REQUIRE(lresult == expected);

            unlink(filename.c_str());
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_positive_numbers", "[json][number]")
{
    CATCH_START_SECTION("json: positive numbers")
    {
        std::string const content(
                "// we can have a C++ comment\n"
                "/* or even a C like comment in the header\n"
                "(not the rest because we do not have access...) */\n"
                "[\n"
                "\t+111,\n"
                "\t+1.113,\n"
                "\t+Infinity,\n"
                "\t+NaN\n"
                "]\n"
            );

        test_data_t data;
        data.f_pos.reset_counters(201);
        data.f_pos.set_filename("full.json");
        data.f_pos.set_function("save_full");

        as2js::input_stream<std::stringstream>::pointer_t in(std::make_shared<as2js::input_stream<std::stringstream>>());
        *in << content;

        as2js::json::pointer_t load_json(std::make_shared<as2js::json>());
        as2js::json::json_value::pointer_t loaded_value(load_json->parse(in));
        CATCH_REQUIRE(loaded_value == load_json->get_value());

        as2js::json::json_value::pointer_t value(load_json->get_value());
        CATCH_REQUIRE(value->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);
        as2js::json::json_value::array_t array(value->get_array());
        CATCH_REQUIRE(array.size() == 4);

        CATCH_REQUIRE(array[0]->get_type() == as2js::json::json_value::type_t::JSON_TYPE_INTEGER);
        as2js::integer integer(array[0]->get_integer());
        CATCH_REQUIRE(integer.get() == 111);

        CATCH_REQUIRE(array[1]->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);
        as2js::floating_point floating_point(array[1]->get_floating_point());
        CATCH_REQUIRE_FLOATING_POINT(floating_point.get(), 1.113);

        CATCH_REQUIRE(array[2]->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);
        floating_point = array[2]->get_floating_point();
        CATCH_REQUIRE(floating_point.is_positive_infinity());

        CATCH_REQUIRE(array[3]->get_type() == as2js::json::json_value::type_t::JSON_TYPE_FLOATING_POINT);
        floating_point = array[3]->get_floating_point();
        CATCH_REQUIRE(floating_point.is_nan());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_errors", "[json][errors]")
{
    CATCH_START_SECTION("json: cannot open input")
    {
        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_NOT_FOUND;
        expected.f_pos.set_filename("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "cannot open JSON file \"/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately\".";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::json::pointer_t load_json(new as2js::json);
        CATCH_REQUIRE(load_json->load("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately") == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: cannot open output")
    {
        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "could not open output file \"/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately\".";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::json::pointer_t save_json(new as2js::json);
        CATCH_REQUIRE(save_json->save("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately", "// unused\n") == false);
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: invalid data")
    {
        as2js::json::pointer_t json(new as2js::json);
        as2js::output_stream<std::stringstream>::pointer_t lout(new as2js::output_stream<std::stringstream>);
        std::string const header("// unused\n");
        //CPPUNIT_ASSERT_THROW(json->output(lout, header), as2js::invalid_data);

        CATCH_REQUIRE_THROWS_MATCHES(
                  json->output(lout, header)
                , as2js::invalid_data
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: this JSON has no value to output."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: EOF error")
    {
        // use an unsafe temporary file...
        int number(rand() % 1000000);
        std::stringstream ss;
        ss << SNAP_CATCH2_NAMESPACE::g_tmp_dir() << "/json_test" << std::setfill('0') << std::setw(6) << number << ".js";
        std::string filename(ss.str());
        // create an empty file
        FILE *f(fopen(filename.c_str(), "w"));
//std::cerr << "--- opened [" << filename << "] result: " << reinterpret_cast<void*>(f) << "\n";
        CATCH_REQUIRE(f != nullptr);
        fclose(f);

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_EOF;
        expected1.f_pos.set_filename(filename);
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "the end of the file was reached while reading JSON data.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename(filename);
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"" + filename + "\".";
        tc.f_expected.push_back(expected2);

//std::cerr << "filename [" << ss.str() << "]\n";
        as2js::json::pointer_t json(std::make_shared<as2js::json>());
        CATCH_REQUIRE(json->load(filename) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: string name missing")
    {
        std::string str(
            "{'valid':123,,'valid too':123}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: unquoted string")
    {
        std::string str(
            "{'valid':123,invalid:123}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: number instead of string for name")
    {
        std::string str(
            "{'valid':123,123:'invalid'}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: array instead of name")
    {
        std::string str(
            "{'valid':123,['invalid']}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: object instead of name")
    {
        std::string str(
            "{'valid':123,{'invalid':123}}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: colon missing")
    {
        std::string str(
            "{'valid':123,'colon missing'123}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COLON_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a colon (:) as the JSON object member name (colon missing) and member value separator (invalid type is INTEGER)";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: sub-list missing colon")
    {
        std::string str(
            // we use 'valid' twice but one is in a sub-object to test
            // that does not generate a problem
            "{'valid':123,'sub-member':{'valid':123,'sub-sub-member':{'sub-sub-invalid'123},'ignore':'this'}}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COLON_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a colon (:) as the JSON object member name (sub-sub-invalid) and member value separator (invalid type is INTEGER)";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: field repeated")
    {
        std::string str(
            "{'valid':123,'re-valid':{'sub-valid':123,'sub-sub-member':{'sub-sub-valid':123},'more-valid':'this'},'valid':'again'}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_OBJECT_MEMBER_DEFINED_TWICE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "the same object member \"valid\" was defined twice, which is not allowed in JSON.";
        tc.f_expected.push_back(expected1);

        as2js::json::pointer_t json(new as2js::json);
        // defined twice does not mean we get a null pointer...
        // (we should enhance this test to verify the result which is
        // that we keep the first entry with a given name.)
        CATCH_REQUIRE(json->parse(in) != as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: comma missing")
    {
        std::string str(
            "{'valid':123 'next-member':456}"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON object members.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: double comma")
    {
        std::string str(
            "['valid',-123,,'next-item',456]"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (COMMA) found in a JSON input stream.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: negative string")
    {
        std::string str(
            "['valid',-555,'bad-neg',-'123']"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (STRING) found after a \"-\" sign, a number was expected.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: positive string")
    {
        std::string str(
            "['valid',+555,'bad-pos',+'123']"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (STRING) found after a \"+\" sign, a number was expected.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: missing comma")
    {
        std::string str(
            "['valid',123 'next-item',456]"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON array items.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: missing comma in sub-array")
    {
        std::string str(
            "['valid',[123 'next-item'],456]"
        );
        as2js::input_stream<std::stringstream>::pointer_t in(new as2js::input_stream<std::stringstream>());
        *in << str;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON array items.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::json::pointer_t json(new as2js::json);
        CATCH_REQUIRE(json->parse(in) == as2js::json::json_value::pointer_t());
        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("json: unexpected token")
    {
        // skip controls to avoid problems with the lexer itself...
        for(char32_t c(0x20); c < 0x110000; ++c)
        {
            switch(c)
            {
            //case '\n':
            //case '\r':
            //case '\t':
            case ' ':
            case '{':
            case '[':
            case '\'':
            case '"':
            case '#':
            case '-':
            case '@':
            case '\\':
            case '`':
            case 0x7F:
            //case ',': -- that would generate errors because it would be in the wrong place
            case '.':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                // that looks like valid entries as is... so ignore
                continue;

            default:
                if(c >= 0xD800 && c <= 0xDFFF)
                {
                    // skip surrogate, no need to test those
                    continue;
                }
                if(!is_identifier_char(c))
                {
                    // skip "punctuation" for now...
                    continue;
                }
                break;

            }
            std::string str;
            str += libutf8::to_u8string(c);

            as2js::node::pointer_t node;
            {
                as2js::options::pointer_t options(std::make_shared<as2js::options>());
                options->set_option(as2js::option_t::OPTION_JSON, 1);
                as2js::input_stream<std::stringstream>::pointer_t input(std::make_shared<as2js::input_stream<std::stringstream>>());
                *input << str;
                as2js::lexer::pointer_t lexer(std::make_shared<as2js::lexer>(input, options));
                CATCH_REQUIRE(lexer->get_input() == input);
                node = lexer->get_next_token(false);
                CATCH_REQUIRE(node != nullptr);
            }

            as2js::input_stream<std::stringstream>::pointer_t in(std::make_shared<as2js::input_stream<std::stringstream>>());
            *in << str;

            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "unexpected token (";
            expected1.f_message += node->get_type_name();
            expected1.f_message += ") found in a JSON input stream.";
            tc.f_expected.push_back(expected1);

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
            expected2.f_pos.set_filename("unknown-file");
            expected2.f_pos.set_function("unknown-func");
            expected2.f_message = "could not interpret this JSON input \"\".";
            tc.f_expected.push_back(expected2);

            as2js::json::pointer_t json(std::make_shared<as2js::json>());
            as2js::json::json_value::pointer_t result(json->parse(in));
            CATCH_REQUIRE(result == as2js::json::json_value::pointer_t());
            tc.got_called();
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("json_canonicalization", "[json][canonical]")
{
    CATCH_START_SECTION("json: canonicalize")
    {
        char const * const json_to_canonicalize[] =
        {
            "{}",
            "{}",

            "{\"we-accept\": 'some funny things'}",
            "{\"we-accept\":\"some funny things\"}",

            "{'single_field': 11.3040}",
            "{\"single_field\":11.304}",

            "{'no_decimal': 34.00}",
            "{\"no_decimal\":34}",
        };
        std::size_t const max(std::size(json_to_canonicalize));
        for(std::size_t idx(0); idx < max; idx += 2)
        {
            std::string const result(as2js::json_canonicalize(json_to_canonicalize[idx]));
            CATCH_REQUIRE(result == json_to_canonicalize[idx + 1]);
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
