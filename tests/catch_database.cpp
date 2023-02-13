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

// as2js
//
#include    <as2js/file/database.h>

#include    <as2js/exception.h>
#include    <as2js/message.h>


// self
//
#include    "catch_main.h"


// libutf8
//
#include    <libutf8/libutf8.h>


// C++
//
#include    <cstring>
#include    <algorithm>
#include    <iomanip>


// C
//
#include    <unistd.h>
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


int32_t generate_string(std::string & str)
{
    char32_t c;
    int32_t used(0);
    int ctrl(rand() % 7);
    int const max_chars(rand() % 25 + 20);
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
           || c == '\0');
        str += libutf8::to_u8string(c);
        switch(c)
        {
        case '\b':
            used |= 0x01;
            break;

        case '\f':
            used |= 0x02;
            break;

        case '\n':
            used |= 0x04;
            break;

        case '\r':
            used |= 0x08;
            break;

        case '\t':
            used |= 0x10;
            break;

        case '"':
            used |= 0x20;
            break;

        case '\'':
            used |= 0x40;
            break;

        default:
            if(c < 0x0020)
            {
                // other controls must be escaped using Unicode
                used |= 0x80;
            }
            break;

        }
    }

    return used;
}


class test_callback : public as2js::message_callback
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
        CATCH_REQUIRE(!f_expected.empty());

//std::cerr << "filename = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << "\n";
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
            std::cerr << "\n*** STILL " << f_expected.size() << " EXPECTED ***\n";
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
        std::string                 f_message= std::string(); // UTF-8 string
    };

    std::vector<expected_t>     f_expected = std::vector<expected_t>();

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;


}
// no name namespace




namespace SNAP_CATCH2_NAMESPACE
{



int catch_db_init()
{
    // we do not want a test.db or it would conflict with this test
    //
    if(access("test.db", F_OK) == 0)
    {
        return 1;
    }

    return 0;
}



} // namespace SNAP_CATCH2_NAMESPACE


CATCH_TEST_CASE("db_match", "[db][match]")
{
    CATCH_START_SECTION("db_match: match strings")
    {
        for(size_t idx(0); idx < 100; ++idx)
        {
            std::string start;
            generate_string(start);
            std::string middle;
            generate_string(middle);
            std::string end;
            generate_string(end);

            std::string name;
            name = start + middle + end;
            CATCH_REQUIRE(as2js::database::match_pattern(name, "*"));

            std::string p1(start);
            p1 += '*';
            CATCH_REQUIRE(as2js::database::match_pattern(name, p1));

            std::string p2(start);
            p2 += '*';
            p2 += middle;
            p2 += '*';
            CATCH_REQUIRE(as2js::database::match_pattern(name, p2));

            std::string p3(start);
            p3 += '*';
            p3 += end;
            CATCH_REQUIRE(as2js::database::match_pattern(name, p3));

            std::string p4;
            p4 += '*';
            p4 += middle;
            p4 += '*';
            CATCH_REQUIRE(as2js::database::match_pattern(name, p4));

            std::string p5;
            p5 += '*';
            p5 += middle;
            p5 += '*';
            p5 += end;
            CATCH_REQUIRE(as2js::database::match_pattern(name, p5));

            std::string p6(start);
            p6 += '*';
            p6 += middle;
            p6 += '*';
            p6 += end;
            CATCH_REQUIRE(as2js::database::match_pattern(name, p6));

            std::string p7;
            p7 += '*';
            p7 += end;
            CATCH_REQUIRE(as2js::database::match_pattern(name, p7));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("db_element", "[db][element]")
{
    CATCH_START_SECTION("db_element: type/filename")
    {
        std::int32_t used_type(0);
        std::int32_t used_filename(0);
        for(std::size_t idx(0); idx < 100 || used_type != 0xFF || used_filename != 0xFF; ++idx)
        {
            as2js::position pos;

            std::string raw_type;
            used_type |= generate_string(raw_type);
            as2js::json::json_value::pointer_t type(new as2js::json::json_value(pos, raw_type));

            std::string raw_filename;
            used_filename |= generate_string(raw_filename);
            as2js::json::json_value::pointer_t filename(new as2js::json::json_value(pos, raw_filename));

            // generate a line number
            std::int32_t raw_line((rand() & 0xFFFFFF) + 1);
            as2js::integer line_integer(raw_line);
            as2js::json::json_value::pointer_t line(new as2js::json::json_value(pos, line_integer));

            as2js::json::json_value::object_t obj;
            obj["filename"] = filename;
            obj["type"] = type;
            obj["line"] = line;
            as2js::json::json_value::pointer_t element(new as2js::json::json_value(pos, obj));

            as2js::database::element::pointer_t db_element(new as2js::database::element("this.is.an.element.name", element));

            CATCH_REQUIRE(db_element->get_element_name() == "this.is.an.element.name");
            CATCH_REQUIRE(db_element->get_type() == raw_type);
            CATCH_REQUIRE(db_element->get_filename() == raw_filename);
            CATCH_REQUIRE(db_element->get_line() == raw_line);

            generate_string(raw_type);
            db_element->set_type(raw_type);
            CATCH_REQUIRE(db_element->get_type() == raw_type);

            generate_string(raw_filename);
            db_element->set_filename(raw_filename);
            CATCH_REQUIRE(db_element->get_filename() == raw_filename);

            raw_line = (rand() & 0xFFFFFF) + 1;
            db_element->set_line(raw_line);
            CATCH_REQUIRE(db_element->get_line() == raw_line);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_element: errorneous data")
    {
        // now check for erroneous data
        //
        as2js::position pos;

        std::string not_obj;
        generate_string(not_obj);
        as2js::json::json_value::pointer_t bad_element(new as2js::json::json_value(pos, not_obj));

        CATCH_REQUIRE_THROWS_MATCHES(
              new as2js::database::element("expect.a.throw", bad_element)
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: an element cannot be created with a json value which has a type other than object."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_element: position")
    {
        as2js::position pos;

        int32_t bad_raw_type((rand() & 0xFFFFFF) + 1);
        as2js::integer bad_type_integer(bad_raw_type);
        as2js::json::json_value::pointer_t bad_type(new as2js::json::json_value(pos, bad_type_integer));

        double bad_raw_filename(static_cast<double>((rand() << 16) ^ rand()) / static_cast<double>((rand() << 16) ^ rand()));
        as2js::floating_point bad_filename_floating_point(bad_raw_filename);
        as2js::json::json_value::pointer_t bad_filename(new as2js::json::json_value(pos, bad_filename_floating_point));

        // generate a line number
        std::string bad_raw_line;
        generate_string(bad_raw_line);
        as2js::json::json_value::pointer_t bad_line(new as2js::json::json_value(pos, bad_raw_line));

        as2js::json::json_value::object_t bad_obj;
        bad_obj["filename"] = bad_filename;
        bad_obj["type"] = bad_type;
        bad_obj["line"] = bad_line;
        as2js::json::json_value::pointer_t element(new as2js::json::json_value(pos, bad_obj));

        // WARNING: errors should be generated in the order the elements
        //          appear in the map
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "The filename of an element in the database has to be a string.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "The line of an element in the database has to be an integer.";
        tc.f_expected.push_back(expected2);

        test_callback::expected_t expected3;
        expected3.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected3.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected3.f_pos.set_filename("unknown-file");
        expected3.f_pos.set_function("unknown-func");
        expected3.f_message = "The type of an element in the database has to be a string.";
        tc.f_expected.push_back(expected3);

        as2js::database::element::pointer_t db_element(new as2js::database::element("this.is.a.bad.element.name", element));
        tc.got_called();

        CATCH_REQUIRE(db_element->get_element_name() == "this.is.a.bad.element.name");
        CATCH_REQUIRE(db_element->get_type() == "");
        CATCH_REQUIRE(db_element->get_filename() == "");
        CATCH_REQUIRE(db_element->get_line() == 1);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("db_package", "[db][package]")
{
    CATCH_START_SECTION("db_package: add & find packages")
    {
        for(size_t idx(0); idx < 100; ++idx)
        {
            as2js::position pos;

            // one package of 10 elements
            as2js::json::json_value::object_t package_obj;

            struct data_t
            {
                std::string     f_element_name = std::string();
                std::string     f_type = std::string();
                std::string     f_filename = std::string();
                std::int32_t    f_line = 0;
            };
            std::vector<data_t> elements;

            for(std::size_t j(0); j < 10; ++j)
            {
                data_t data;

                generate_string(data.f_type);
                as2js::json::json_value::pointer_t type(new as2js::json::json_value(pos, data.f_type));

                generate_string(data.f_filename);
                as2js::json::json_value::pointer_t filename(new as2js::json::json_value(pos, data.f_filename));

                // generate a line number
                data.f_line = (rand() & 0xFFFFFF) + 1;
                as2js::integer line_integer(data.f_line);
                as2js::json::json_value::pointer_t line(new as2js::json::json_value(pos, line_integer));

                as2js::json::json_value::object_t obj;
                obj["type"] = type;
                obj["filename"] = filename;
                obj["line"] = line;
                as2js::json::json_value::pointer_t element(new as2js::json::json_value(pos, obj));

                generate_string(data.f_element_name);
                package_obj[data.f_element_name] = element;

                elements.push_back(data);

                // as we're here, make sure we can create such a db element
                as2js::database::element::pointer_t db_element(new as2js::database::element(data.f_element_name, element));

                CATCH_REQUIRE(db_element->get_element_name() == data.f_element_name);
                CATCH_REQUIRE(db_element->get_type() == data.f_type);
                CATCH_REQUIRE(db_element->get_filename() == data.f_filename);
                CATCH_REQUIRE(db_element->get_line() == data.f_line);
            }

            as2js::json::json_value::pointer_t package(new as2js::json::json_value(pos, package_obj));
            std::string package_name;
            generate_string(package_name);
            as2js::database::package::pointer_t db_package(new as2js::database::package(package_name, package));

            CATCH_REQUIRE(db_package->get_package_name() == package_name);

            for(size_t j(0); j < 10; ++j)
            {
                as2js::database::element::pointer_t e(db_package->get_element(elements[j].f_element_name));

                CATCH_REQUIRE(e->get_element_name() == elements[j].f_element_name);
                CATCH_REQUIRE(e->get_type()         == elements[j].f_type);
                CATCH_REQUIRE(e->get_filename()     == elements[j].f_filename);
                CATCH_REQUIRE(e->get_line()         == elements[j].f_line);

                // the add_element() does nothing if we add an element with the
                // same name
                as2js::database::element::pointer_t n(db_package->add_element(elements[j].f_element_name));
                CATCH_REQUIRE(n == e);
            }

            // attempts a find as well
            for(size_t j(0); j < 10; ++j)
            {
                {
                    // pattern "starts with"
                    int len(rand() % 5 + 1);
                    std::string pattern(elements[j].f_element_name.substr(0, len));
                    int const max_asterisk(rand() % 3 + 1);
                    for(int a(0); a < max_asterisk; ++a)
                    {
                        pattern += '*';
                    }
                    as2js::database::element::vector_t list(db_package->find_elements(pattern));

                    // check that the name of the elements found this way are valid
                    // matches
                    size_t const max_elements(list.size());
                    CATCH_REQUIRE(max_elements >= 1);
                    for(size_t k(0); k < max_elements; ++k)
                    {
                        std::string name(list[k]->get_element_name());
                        std::string match(name.substr(0, len));
                        for(int a(0); a < max_asterisk; ++a)
                        {
                            match += '*';
                        }
                        CATCH_REQUIRE(pattern == match);
                    }

                    // now verify that we found them all
                    for(std::size_t q(0); q < 10; ++q)
                    {
                        std::string name(elements[q].f_element_name);
                        std::string start_with(name.substr(0, len));
                        start_with += '*';
                        if(start_with == pattern.substr(0, len + 1))
                        {
                            // find that entry in the list
                            bool good(false);
                            for(std::size_t k(0); k < max_elements; ++k)
                            {
                                if(list[k]->get_element_name() == name)
                                {
                                    good = true;
                                    break;
                                }
                            }
                            CATCH_REQUIRE(good);
                        }
                    }
                }

                {
                    // pattern "ends with"
                    int len(rand() % 5 + 1);
                    std::string pattern;
                    pattern += '*';
                    pattern += elements[j].f_element_name.substr(elements[j].f_element_name.length() - len, len);
                    as2js::database::element::vector_t list(db_package->find_elements(pattern));

                    // check that the name of the elements found this way are valid
                    // matches
                    std::size_t const max_elements(list.size());
                    CATCH_REQUIRE(max_elements >= 1);
                    for(std::size_t k(0); k < max_elements; ++k)
                    {
                        std::string name(list[k]->get_element_name());
                        std::string match;
                        match += '*';
                        match += name.substr(name.length() - len, len);
                        CATCH_REQUIRE(pattern == match);
                    }

                    // now verify that we found them all
                    for(std::size_t q(0); q < 10; ++q)
                    {
                        std::string name(elements[q].f_element_name);
                        std::string end_with;
                        end_with += '*';
                        end_with += name.substr(name.length() - len, len);
                        if(end_with == pattern)
                        {
                            // find that entry in the list
                            bool good(false);
                            for(std::size_t k(0); k < max_elements; ++k)
                            {
                                if(list[k]->get_element_name() == name)
                                {
                                    good = true;
                                    break;
                                }
                            }
                            CATCH_REQUIRE(good);
                        }
                    }
                }

                {
                    // pattern "starts/ends with"
                    // names are generated by the generate_string() so they are
                    // at least 20 characters long which is enough here
                    int slen(rand() % 5 + 1);
                    int elen(rand() % 5 + 1);
                    std::string pattern;
                    pattern += elements[j].f_element_name.substr(0, slen);
                    pattern += '*';
                    pattern += elements[j].f_element_name.substr(elements[j].f_element_name.length() - elen, elen);
                    as2js::database::element::vector_t list(db_package->find_elements(pattern));

                    // check that the name of the elements found this way are valid
                    // matches
                    std::size_t const max_elements(list.size());
                    CATCH_REQUIRE(max_elements >= 1);
                    for(std::size_t k(0); k < max_elements; ++k)
                    {
                        std::string name(list[k]->get_element_name());
                        std::string match;
                        match += name.substr(0, slen);
                        match += '*';
                        match += name.substr(name.length() - elen, elen);
                        CATCH_REQUIRE(pattern == match);
                    }

                    // now verify that we found them all
                    for(std::size_t q(0); q < 10; ++q)
                    {
                        std::string name(elements[q].f_element_name);
                        std::string end_with;
                        end_with += name.substr(0, slen);
                        end_with += '*';
                        end_with += name.substr(name.length() - elen, elen);
                        if(end_with == pattern)
                        {
                            // find that entry in the list
                            bool good(false);
                            for(size_t k(0); k < max_elements; ++k)
                            {
                                if(list[k]->get_element_name() == name)
                                {
                                    good = true;
                                    break;
                                }
                            }
                            CATCH_REQUIRE(good);
                        }
                    }
                }
            }

            // add a few more elements
            for(size_t j(0); j < 10; ++j)
            {
                // at this point the name of an element is not verified because
                // all the internal code expects valid identifiers for those
                // names so any random name will do in this test
                std::string name;
                generate_string(name);
                as2js::database::element::pointer_t e(db_package->add_element(name));

                // it creates an empty element in this case
                CATCH_REQUIRE(e->get_element_name() == name);
                CATCH_REQUIRE(e->get_type() == "");
                CATCH_REQUIRE(e->get_filename() == "");
                CATCH_REQUIRE(e->get_line() == 1);
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_package: erroneous packages")
    {
        // now check for erroneous data
        //
        as2js::position pos;

        std::string not_obj;
        generate_string(not_obj);
        as2js::json::json_value::pointer_t bad_package(new as2js::json::json_value(pos, not_obj));

        CATCH_REQUIRE_THROWS_MATCHES(
              new as2js::database::package("expect.a.throw", bad_package)
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: a package cannot be created with a json value which has a type other than object."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_package: more bad data")
    {
        as2js::position pos;

        int32_t bad_int((rand() & 0xFFFFFF) + 1);
        as2js::integer bad_integer(bad_int);
        as2js::json::json_value::pointer_t bad_a(new as2js::json::json_value(pos, bad_integer));

        double bad_float(static_cast<double>((rand() << 16) ^ rand()) / static_cast<double>((rand() << 16) ^ rand()));
        as2js::floating_point bad_floating_point(bad_float);
        as2js::json::json_value::pointer_t bad_b(new as2js::json::json_value(pos, bad_floating_point));

        std::string bad_string;
        generate_string(bad_string);
        as2js::json::json_value::pointer_t bad_c(new as2js::json::json_value(pos, bad_string));

        //as2js::json::json_value::object_t bad_obj;
        //std::string n1;
        //generate_string(n1);
        //bad_obj[n1] = bad_a;
        //std::string n2;
        //generate_string(n2);
        //bad_obj[n2] = bad_b;
        //std::string n3;
        //generate_string(n3);
        //bad_obj[n3] = bad_c;
        //as2js::json::json_value::pointer_t element(new as2js::json::json_value(pos, bad_obj));

        as2js::json::json_value::object_t package_obj;
        std::string e1_name;
        generate_string(e1_name);
        package_obj[e1_name] = bad_a;

        std::string e2_name;
        generate_string(e2_name);
        package_obj[e2_name] = bad_b;

        std::string e3_name;
        generate_string(e3_name);
        package_obj[e3_name] = bad_c;

        // WARNING: errors should be generated in the order the elements
        //          appear in the map
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected2);

        test_callback::expected_t expected3;
        expected3.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected3.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected3.f_pos.set_filename("unknown-file");
        expected3.f_pos.set_function("unknown-func");
        expected3.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected3);

        as2js::json::json_value::pointer_t package(new as2js::json::json_value(pos, package_obj));

        std::string package_name;
        generate_string(package_name);
        as2js::database::package::pointer_t db_package(new as2js::database::package(package_name, package));
        tc.got_called();
        CATCH_REQUIRE(!!db_package);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("db_database", "[db]")
{
    CATCH_START_SECTION("db_database: database")
    {
        as2js::database::pointer_t db(new as2js::database);

        // saving without a load does nothing
        db->save();

        // whatever the package name, it does not exist...
        CATCH_REQUIRE(!db->get_package("name"));

        // adding a package fails with a throw
        CATCH_REQUIRE_THROWS_MATCHES(
              db->add_package("name")
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: attempting to add a package to the database before the database was loaded."));

        // the find_packages() function returns nothing
        as2js::database::package::vector_t v(db->find_packages("name"));
        CATCH_REQUIRE(v.empty());

        // now test a load()
        CATCH_REQUIRE(db->load("test.db"));

        // a second time returns true also
        CATCH_REQUIRE(db->load("test.db"));

        as2js::database::package::pointer_t p1(db->add_package("p1"));
        as2js::database::element::pointer_t e1(p1->add_element("e1"));
        e1->set_type("type-e1");
        e1->set_filename("e1.as");
        e1->set_line(33);
        as2js::database::element::pointer_t e2(p1->add_element("e2"));
        e2->set_type("type-e2");
        e2->set_filename("e2.as");
        e2->set_line(66);
        as2js::database::element::pointer_t e3(p1->add_element("e3"));
        e3->set_type("type-e3");
        e3->set_filename("e3.as");
        e3->set_line(99);

        as2js::database::package::pointer_t p2(db->add_package("p2"));
        as2js::database::element::pointer_t e4(p2->add_element("e4"));
        e4->set_type("type-e4");
        e4->set_filename("e4.as");
        e4->set_line(44);
        as2js::database::element::pointer_t e5(p2->add_element("e5"));
        e5->set_type("type-e5");
        e5->set_filename("e5.as");
        e5->set_line(88);
        as2js::database::element::pointer_t e6(p2->add_element("e6"));
        e6->set_type("type-e6");
        e6->set_filename("e6.as");
        e6->set_line(11);

        db->save();

        CATCH_REQUIRE(db->get_package("p1") == p1);
        CATCH_REQUIRE(db->get_package("p2") == p2);

        as2js::database::package::pointer_t q(db->get_package("p1"));
        CATCH_REQUIRE(q == p1);
        as2js::database::package::pointer_t r(db->get_package("p2"));
        CATCH_REQUIRE(r == p2);

        as2js::database::pointer_t qdb(new as2js::database);
        CATCH_REQUIRE(qdb->load("test.db"));

        as2js::database::package::pointer_t np1(qdb->get_package("p1"));
        as2js::database::element::pointer_t ne1(np1->get_element("e1"));
        CATCH_REQUIRE(ne1->get_type() == "type-e1");
        CATCH_REQUIRE(ne1->get_filename() == "e1.as");
        CATCH_REQUIRE(ne1->get_line() == 33);
        as2js::database::element::pointer_t ne2(np1->get_element("e2"));
        CATCH_REQUIRE(ne2->get_type() == "type-e2");
        CATCH_REQUIRE(ne2->get_filename() == "e2.as");
        CATCH_REQUIRE(ne2->get_line() == 66);
        as2js::database::element::pointer_t ne3(np1->get_element("e3"));
        CATCH_REQUIRE(ne3->get_type() == "type-e3");
        CATCH_REQUIRE(ne3->get_filename() == "e3.as");
        CATCH_REQUIRE(ne3->get_line() == 99);
        as2js::database::package::pointer_t np2(qdb->get_package("p2"));
        as2js::database::element::pointer_t ne4(np2->get_element("e4"));
        CATCH_REQUIRE(ne4->get_type() == "type-e4");
        CATCH_REQUIRE(ne4->get_filename() == "e4.as");
        CATCH_REQUIRE(ne4->get_line() == 44);
        as2js::database::element::pointer_t ne5(np2->get_element("e5"));
        CATCH_REQUIRE(ne5->get_type() == "type-e5");
        CATCH_REQUIRE(ne5->get_filename() == "e5.as");
        CATCH_REQUIRE(ne5->get_line() == 88);
        as2js::database::element::pointer_t ne6(np2->get_element("e6"));
        CATCH_REQUIRE(ne6->get_type() == "type-e6");
        CATCH_REQUIRE(ne6->get_filename() == "e6.as");
        CATCH_REQUIRE(ne6->get_line() == 11);

        as2js::database::package::vector_t np1a(qdb->find_packages("p1"));
        CATCH_REQUIRE(np1a.size() == 1);
        CATCH_REQUIRE(np1a[0] == np1);
        as2js::database::package::vector_t np2a(qdb->find_packages("p2"));
        CATCH_REQUIRE(np2a.size() == 1);
        CATCH_REQUIRE(np2a[0] == np2);
        as2js::database::package::vector_t np3a(qdb->find_packages("p*"));
        CATCH_REQUIRE(np3a.size() == 2);
        CATCH_REQUIRE(np3a[0] == np1);
        CATCH_REQUIRE(np3a[1] == np2);

        // done with that one
        unlink("test.db");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_database: invalid file")
    {
        {
            std::ofstream db_file;
            db_file.open("t1.db");
            CATCH_REQUIRE(db_file.is_open());
            db_file << "// db file\n"
                    << "an invalid file\n";
        }

        as2js::database::pointer_t pdb(new as2js::database);
        CATCH_REQUIRE(!pdb->load("t1.db"));
        // make sure we can still create a package (because here f_value
        // is null)
        as2js::database::package::pointer_t tp(pdb->add_package("another"));
        CATCH_REQUIRE(!!tp);

        unlink("t1.db");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_database: NULL db")
    {
        {
            std::ofstream db_file;
            db_file.open("t2.db");
            CATCH_REQUIRE(db_file.is_open());
            db_file << "// db file\n"
                    << "null\n";
        }

        as2js::database::pointer_t pdb(new as2js::database);
        CATCH_REQUIRE(pdb->load("t2.db"));
        as2js::database::package::vector_t np(pdb->find_packages("*"));
        CATCH_REQUIRE(np.empty());

        unlink("t2.db");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_database: unexpected string")
    {
        {
            std::ofstream db_file;
            db_file.open("t3.db");
            CATCH_REQUIRE(db_file.is_open());
            db_file << "// db file\n"
                    << "\"unexpected string\"\n";
        }

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("t3.db");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database must be defined as a json object, or set to \"null\".";
        tc.f_expected.push_back(expected1);

        as2js::database::pointer_t sdb(new as2js::database);
        CATCH_REQUIRE(!sdb->load("t3.db"));
        tc.got_called();

        as2js::database::package::vector_t np(sdb->find_packages("*"));
        CATCH_REQUIRE(np.empty());

        unlink("t3.db");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("db_database: invalid object")
    {
        {
            std::ofstream db_file;
            db_file.open("t4.db");
            CATCH_REQUIRE(db_file.is_open());
            db_file << "// db file\n"
                    << "{\"invalid\":\"object-here\"}\n";
        }

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("t4.db");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database is expected to be an object of object packages composed of elements.";
        tc.f_expected.push_back(expected1);

        as2js::database::pointer_t sdb(new as2js::database);
        CATCH_REQUIRE(!sdb->load("t4.db"));
        tc.got_called();

        as2js::database::package::vector_t np(sdb->find_packages("*"));
        CATCH_REQUIRE(np.empty());

        unlink("t4.db");
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
