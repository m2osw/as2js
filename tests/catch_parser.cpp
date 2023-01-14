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
#include    <as2js/parser.h>

#include    <as2js/exception.h>
#include    <as2js/message.h>
#include    <as2js/json.h>


// self
//
#include    "catch_main.h"


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





//
// JSON data used to test the parser, most of the work is in this table
// these are long JSON strings! It is actually generated using the
// json_to_string tool and the parser_data/*.json source files.
//
// Note: the top entries are arrays so we can execute programs in the
//       order we define them...
//
char const g_array[] =
#include "parser_data/array.ci"
;
char const g_basics[] =
#include "parser_data/basics.ci"
;
char const g_class[] =
#include "parser_data/class.ci"
;
char const g_enum[] =
#include "parser_data/enum.ci"
;
char const g_if[] =
#include "parser_data/if.ci"
;
char const g_for[] =
#include "parser_data/for.ci"
;
char const g_function[] =
#include "parser_data/function.ci"
;
char const g_pragma[] =
#include "parser_data/pragma.ci"
;
char const g_switch[] =
#include "parser_data/switch.ci"
;
char const g_synchronized[] =
#include "parser_data/synchronized.ci"
;
char const g_trycatch[] =
#include "parser_data/trycatch.ci"
;
char const g_type[] =
#include "parser_data/type.ci"
;
char const g_variable[] =
#include "parser_data/variable.ci"
;
char const g_while[] =
#include "parser_data/while.ci"
;
char const g_yield[] =
#include "parser_data/yield.ci"
;
// TODO: specialize all those parts!
char const g_data[] =
#include "parser_data/parser.ci"
;








// This function runs all the tests defined in the
// string 'data'
void run_tests(char const *data, char const *filename)
{
    std::string input_data(data);

    if(SNAP_CATCH2_NAMESPACE::g_save_parser_tests)
    {
        std::ofstream json_file;
        json_file.open(filename);
        CATCH_REQUIRE(json_file.is_open());
        json_file << "// To properly indent this JSON you may use http://json-indent.appspot.com/"
                << std::endl << data << std::endl;
    }

    as2js::input_stream<std::stringstream>::pointer_t in(std::make_shared<as2js::input_stream<std::stringstream>>());
    *in << input_data;
    as2js::json::pointer_t json_data(std::make_shared<as2js::json>());
    as2js::json::json_value::pointer_t json(json_data->parse(in));

    // verify that the parser() did not fail
    //
    CATCH_REQUIRE(!!json);
    CATCH_REQUIRE(json->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

    std::string const name_string("name");
    std::string const program_string("program");
    std::string const verbose_string("verbose");
    std::string const slow_string("slow");
    std::string const result_string("result");
    std::string const expected_messages_string("expected messages");

    as2js::json::json_value::array_t const& array(json->get_array());
    size_t const max_programs(array.size());
    for(size_t idx(0); idx < max_programs; ++idx)
    {
        as2js::json::json_value::pointer_t prog_obj(array[idx]);
        CATCH_REQUIRE(prog_obj->get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);
        as2js::json::json_value::object_t const& prog(prog_obj->get_object());

        bool verbose(false);
        as2js::json::json_value::object_t::const_iterator verbose_it(prog.find(verbose_string));
        if(verbose_it != prog.end())
        {
            verbose = verbose_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE;
        }

        bool slow(false);
        as2js::json::json_value::object_t::const_iterator slow_it(prog.find(slow_string));
        if(slow_it != prog.end())
        {
            slow = slow_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE;
        }

        // got a program, try to compile it with all the possible options
        as2js::json::json_value::pointer_t name(prog.find(name_string)->second);
        std::cout << "  -- working on \"" << name->get_string() << "\" " << (slow ? "" : "...") << std::flush;

        for(std::size_t opt(0); opt < (1ULL << SNAP_CATCH2_NAMESPACE::g_options_size); ++opt)
        {
            if(slow && ((opt + 1) % 250) == 0)
            {
                std::cout << "." << std::flush;
            }
//std::cerr << "\n***\n*** OPTIONS:";
            as2js::options::pointer_t options(std::make_shared<as2js::options>());
            for(std::size_t o(0); o < SNAP_CATCH2_NAMESPACE::g_options_size; ++o)
            {
                if((opt & (1 << o)) != 0)
                {
                    options->set_option(
                              SNAP_CATCH2_NAMESPACE::g_options[o].f_option
                            , options->get_option(SNAP_CATCH2_NAMESPACE::g_options[o].f_option)
                                | SNAP_CATCH2_NAMESPACE::g_options[o].f_value);
//std::cerr << " " << SNAP_CATCH2_NAMESPACE::g_options[o].f_name << "=" << SNAP_CATCH2_NAMESPACE::g_options[o].f_value;
                }
            }
//std::cerr << "\n***\n";

            as2js::json::json_value::pointer_t program_value(prog.find(program_string)->second);
            std::string program_source(program_value->get_string());
//std::cerr << "prog = [" << program_source << "]\n";
            as2js::input_stream<std::stringstream>::pointer_t prog_text(std::make_shared<as2js::input_stream<std::stringstream>>());
            *prog_text << program_source;
            as2js::parser::pointer_t parser(std::make_shared<as2js::parser>(prog_text, options));

            SNAP_CATCH2_NAMESPACE::test_callback tc(verbose);

            as2js::json::json_value::object_t::const_iterator expected_msg_it(prog.find(expected_messages_string));
            if(expected_msg_it != prog.end())
            {

                // the expected messages value must be an array
                //
                as2js::json::json_value::array_t const & msg_array(expected_msg_it->second->get_array());
                size_t const max_msgs(msg_array.size());
                for(size_t j(0); j < max_msgs; ++j)
                {
                    as2js::json::json_value::pointer_t message_value(msg_array[j]);
                    as2js::json::json_value::object_t const& message(message_value->get_object());

                    bool ignore_message(false);

                    as2js::json::json_value::object_t::const_iterator const message_options_iterator(message.find("options"));
                    if(message_options_iterator != message.end())
                    {
//{
//as2js::json::json_value::object_t::const_iterator line_it(message.find("line #"));
//if(line_it != message.end())
//{
//    int64_t lines(line_it->second->get_integer().get());
//std::cerr << "_________\nLine #" << lines << "\n";
//}
//else
//std::cerr << "_________\nLine #<undefined>\n";
//}
                        std::string const message_options(message_options_iterator->second->get_string());
                        for(char const * s(message_options.c_str()), *start(s);; ++s)
                        {
                            if(*s == ',' || *s == '|' || *s == '\0')
                            {
                                std::string opt_name(start, s - start);
                                for(std::size_t o(0); o < SNAP_CATCH2_NAMESPACE::g_options_size; ++o)
                                {
                                    if(SNAP_CATCH2_NAMESPACE::g_options[o].f_name == opt_name)
                                    {
                                        ignore_message = (opt & (1 << o)) != 0;
//std::cerr << "+++ pos option [" << opt_name << "] " << ignore_message << "\n";
                                        goto found_option;
                                    }
                                    else if(SNAP_CATCH2_NAMESPACE::g_options[o].f_neg_name == opt_name)
                                    {
                                        ignore_message = (opt & (1 << o)) == 0;
//std::cerr << "+++ neg option [" << opt_name << "] " << ignore_message << "\n";
                                        goto found_option;
                                    }
                                }
                                std::cerr << "error: Option \"" << opt_name << "\" not found in our list of valid options\n";
                                CATCH_REQUIRE("option name from JSON not found in SNAP_CATCH2_NAMESPACE::g_options" == nullptr);

found_option:
                                if(*s == '\0')
                                {
                                    break;
                                }
                                if(*s == '|')
                                {
                                    if(ignore_message)
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    if(!ignore_message)
                                    {
                                        break;
                                    }
                                }

                                // skip commas
                                do
                                {
                                    ++s;
                                }
                                while(*s == ',' || *s == '|');
                                start = s;
                            }
                        }
                    }

                    if(!ignore_message)
                    {
                        SNAP_CATCH2_NAMESPACE::test_callback::expected_t expected;
                        expected.f_message_level = static_cast<as2js::message_level_t>(message.find("message level")->second->get_integer().get());
                        expected.f_error_code = SNAP_CATCH2_NAMESPACE::str_to_error_code(message.find("error code")->second->get_string());
                        expected.f_pos.set_filename("unknown-file");
                        as2js::json::json_value::object_t::const_iterator func_it(message.find("function name"));
                        if(func_it == message.end())
                        {
                            expected.f_pos.set_function("unknown-func");
                        }
                        else
                        {
                            expected.f_pos.set_function(func_it->second->get_string());
                        }
                        as2js::json::json_value::object_t::const_iterator line_it(message.find("line #"));
                        if(line_it != message.end())
                        {
                            std::int64_t lines(line_it->second->get_integer().get());
                            for(std::int64_t l(1); l < lines; ++l)
                            {
                                expected.f_pos.new_line();
                            }
                        }
                        expected.f_message = message.find("message")->second->get_string();
//std::cerr << "    --- message [" << expected.f_message << "]\n";
                        tc.f_expected.push_back(expected);
                    }
                }
            }

            as2js::node::pointer_t root(parser->parse());

            tc.got_called();

            // the result is object which can have children
            // which are represented by an array of objects
            SNAP_CATCH2_NAMESPACE::verify_parser_result(result_string, prog.find(result_string)->second, root, verbose, false);
        }

        std::cout << " OK\n";
    }

    std::cout << "\n";
}


}
// no name namespace





CATCH_TEST_CASE("parser_array", "[parser][data]")
{
    CATCH_START_SECTION("parser_array: verify JavaScript arrays")
    {
        run_tests(g_array, "test_parser_array.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_basics", "[parser]")
{
    CATCH_START_SECTION("parser_basics: verify JavaScript basic elements")
    {
        run_tests(g_basics, "test_parser_basics.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_class", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_class: verify class extension")
    {
        run_tests(g_class, "test_parser_class.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_enum", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_enum: verify enum extension")
    {
        run_tests(g_enum, "test_parser_enum.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_for", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_for: verify JavaScript for loops")
    {
        run_tests(g_for, "test_parser_for.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_function", "[parser][function]")
{
    CATCH_START_SECTION("parser_function: verify JavaScript functions")
    {
        run_tests(g_function, "test_parser_function.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_if", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_if: verify JavaScript if()/else")
    {
        run_tests(g_if, "test_parser_if.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_pragma", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_pragma: verify pragma extension")
    {
        run_tests(g_pragma, "test_parser_pragma.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_synchronized", "[parser][synchronized]")
{
    CATCH_START_SECTION("parser_synchronized: verify synchronized extension")
    {
        run_tests(g_synchronized, "test_parser_synchronized.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_switch", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_switch: verify JavaScript switch")
    {
        run_tests(g_switch, "test_parser_switch.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_try_catch", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_try_catch: verify JavaScript exception handling")
    {
        run_tests(g_trycatch, "test_parser_trycatch.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_type", "[parser][type]")
{
    CATCH_START_SECTION("parser_type: verify type extensions")
    {
        run_tests(g_type, "test_parser_type.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_variable", "[parser][variable]")
{
    CATCH_START_SECTION("parser_variable: verify JavaScript variable")
    {
        run_tests(g_variable, "test_parser_variable.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_while", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_while: verify JavaScript while")
    {
        run_tests(g_while, "test_parser_while.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("parser_yield", "[parser][instruction]")
{
    CATCH_START_SECTION("parser_yield: verify JavaScript yield")
    {
        run_tests(g_yield, "test_parser_yield.json");
    }
    CATCH_END_SECTION()
}


// TODO: remove once everything is "properly" typed/moved to separate files
CATCH_TEST_CASE("parser_data", "[parser][mixed]")
{
    CATCH_START_SECTION("parser_data: verify other parser functionality (still mixed)")
    {
        run_tests(g_data, "test_parser.json");
    }
    CATCH_END_SECTION()
}





// vim: ts=4 sw=4 et
