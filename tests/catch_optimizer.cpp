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
#include    <as2js/optimizer.h>

#include    <as2js/exception.h>
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/parser.h>


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
// JSON data used to test the optimizer, most of the work is in this table
// these are long JSON strings! It is actually generated using the
// json_to_string tool and the optimizer_data/*.json source files.
//
// Note: the top entries are arrays so we can execute programs in the
//       order we define them...
//
char const g_optimizer_additive[] =
#include "optimizer_data/additive.ci"
;
char const g_optimizer_assignments[] =
#include "optimizer_data/assignments.ci"
;
char const g_optimizer_bitwise[] =
#include "optimizer_data/bitwise.ci"
;
char const g_optimizer_compare[] =
#include "optimizer_data/compare.ci"
;
char const g_optimizer_conditional[] =
#include "optimizer_data/conditional.ci"
;
char const g_optimizer_equality[] =
#include "optimizer_data/equality.ci"
;
char const g_optimizer_logical[] =
#include "optimizer_data/logical.ci"
;
char const g_optimizer_match[] =
#include "optimizer_data/match.ci"
;
char const g_optimizer_multiplicative[] =
#include "optimizer_data/multiplicative.ci"
;
char const g_optimizer_relational[] =
#include "optimizer_data/relational.ci"
;
char const g_optimizer_statements[] =
#include "optimizer_data/statements.ci"
;








// This function runs all the tests defined in the
// string 'data'
void run_tests(char const * input_data, char const * filename)
{
    if(SNAP_CATCH2_NAMESPACE::g_save_parser_tests)
    {
        std::ofstream json_file;
        json_file.open(filename);
        CATCH_REQUIRE(json_file.is_open());
        json_file
            << "// To properly indent this JSON you may use http://json-indent.appspot.com/\n"
            << input_data
            << "\n";
    }

    as2js::input_stream<std::stringstream>::pointer_t in(std::make_shared<as2js::input_stream<std::stringstream>>());
    *in << input_data;
    as2js::json::pointer_t json_data(std::make_shared<as2js::json>());
    as2js::json::json_value::pointer_t json(json_data->parse(in));

    // verify that the optimizer() did not fail
    CATCH_REQUIRE(!!json);
    CATCH_REQUIRE(json->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

    std::string const name_string("name");
    std::string const program_string("program");
    std::string const verbose_string("verbose");
    std::string const slow_string("slow");
    std::string const parser_result_string("parser result");
    std::string const optimizer_result_string("optimizer result");
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

        {
            as2js::json::json_value::pointer_t program_value(prog.find(program_string)->second);
//std::cerr << "prog = [" << program_value->get_string() << "]\n";
            as2js::input_stream<std::stringstream>::pointer_t prog_text(std::make_shared<as2js::input_stream<std::stringstream>>());
            *prog_text << program_value->get_string();
            as2js::options::pointer_t options(std::make_shared<as2js::options>());
            as2js::parser::pointer_t parser(std::make_shared<as2js::parser>(prog_text, options));

            SNAP_CATCH2_NAMESPACE::test_callback tc(verbose);

            // no errors expected while parsing (if you want to test errors
            // in the parser, use the catch_parser.cpp test instead)
            //
            as2js::node::pointer_t root(parser->parse());

            // verify the parser result, that way we can make sure we are
            // testing the tree we want to test in the optimizer
            //
            SNAP_CATCH2_NAMESPACE::verify_result(parser_result_string, prog.find(parser_result_string)->second, root, verbose, false);

            // now the optimizer may end up generating messages...
            // (there are not many, mainly things like division by zero
            // and illegal operation.)
            //
            as2js::json::json_value::object_t::const_iterator expected_msg_it(prog.find(expected_messages_string));
            if(expected_msg_it != prog.end())
            {
                // the expected messages value must be an array
                //
                as2js::message_level_t message_level(as2js::message_level_t::MESSAGE_LEVEL_INFO);
                as2js::json::json_value::array_t const& msg_array(expected_msg_it->second->get_array());
                size_t const max_msgs(msg_array.size());
                for(size_t j(0); j < max_msgs; ++j)
                {
                    as2js::json::json_value::pointer_t message_value(msg_array[j]);
                    as2js::json::json_value::object_t const& message(message_value->get_object());

                    as2js::json::json_value::object_t::const_iterator const message_options_iterator(message.find("options"));
                    if(message_options_iterator != message.end())
                    {
//{
//as2js::json::json_value::object_t::const_iterator line_it(message.find("line #"));
//if(line_it != message.end())
//{
//    int64_t lines(line_it->second->get_int64().get());
//std::cerr << "_________\nLine #" << lines << "\n";
//}
//else
//std::cerr << "_________\nLine #<undefined>\n";
//}
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
                        tc.f_expected.push_back(expected);

                        message_level = std::min(message_level, expected.f_message_level);
                    }
                }

                // the default message level is INFO, don't change if we have
                // a higher level here; however, if we have a lower level,
                // change the message level in the as2js library
                //
                if(message_level < as2js::message_level_t::MESSAGE_LEVEL_INFO)
                {
                    as2js::set_message_level(message_level);
                }
            }

            // run the optimizer
            as2js::optimizer::optimize(root);

            // the result is object which can have children
            // which are represented by an array of objects
            //
            SNAP_CATCH2_NAMESPACE::verify_result(optimizer_result_string, prog.find(optimizer_result_string)->second, root, verbose, false);

            tc.got_called();
        }

        std::cout << " OK\n";
    }

    std::cout << "\n";
}


}
// no name namespace





CATCH_TEST_CASE("optimizer_invalid_nodes", "[optimizer][invalid]")
{
    // empty node does nothing, return false
    {
        as2js::node::pointer_t node;
        CATCH_REQUIRE(!as2js::optimizer::optimize(node));
    }

    // unknown node does nothing, return false
    {
        as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_UNKNOWN));
        CATCH_REQUIRE(!as2js::optimizer::optimize(node));
        CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_UNKNOWN);
        CATCH_REQUIRE(node->get_children_size() == 0);
    }

    // a special case where an optimization occurs on a node without a parent
    // (something that should not occur in a real tree)
    {
        // ADD
        //   INTEGER = 3
        //   INTEGER = 20
        as2js::node::pointer_t node_add(std::make_shared<as2js::node>(as2js::node_t::NODE_ADD));

        as2js::node::pointer_t node_three(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
        as2js::integer three;
        three.set(3);
        node_three->set_integer(three);
        node_add->append_child(node_three);

        as2js::node::pointer_t node_twenty(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
        as2js::integer twenty;
        twenty.set(20);
        node_twenty->set_integer(twenty);
        node_add->append_child(node_twenty);

        // optimization does not happen
        CATCH_REQUIRE_THROWS_MATCHES(
              as2js::optimizer::optimize(node_add)
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: somehow the optimizer is optimizing a node without a parent."));

        // verify that nothing changed
        CATCH_REQUIRE(node_add->get_type() == as2js::node_t::NODE_ADD);
        CATCH_REQUIRE(node_add->get_children_size() == 2);
        CATCH_REQUIRE(node_three->get_type() == as2js::node_t::NODE_INTEGER);
        CATCH_REQUIRE(node_three->get_children_size() == 0);
        CATCH_REQUIRE(node_three->get_integer().compare(three) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(node_twenty->get_type() == as2js::node_t::NODE_INTEGER);
        CATCH_REQUIRE(node_twenty->get_children_size() == 0);
        CATCH_REQUIRE(node_twenty->get_integer().compare(twenty) == as2js::compare_t::COMPARE_EQUAL);
    }
}


CATCH_TEST_CASE("optimizer_additive", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_additive: additive (+, -)")
    {
        run_tests(g_optimizer_additive, "optimizer/additive.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_assignments", "[optimizer]")
{
    CATCH_START_SECTION("parser_array: assignments (=, +=, -=, etc.)")
    {
        run_tests(g_optimizer_assignments, "optimizer/assignments.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_bitwise", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_bitwise: bitwise (&, |, ^)")
    {
        run_tests(g_optimizer_bitwise, "optimizer/bitwise.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_compare", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_compare: compare (<=>)")
    {
        run_tests(g_optimizer_compare, "optimizer/compare.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_conditional", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_conditional: conditional (?:, <?, >?)")
    {
        run_tests(g_optimizer_conditional, "optimizer/conditional.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_equality", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_equality: equality (==, !=)")
    {
        run_tests(g_optimizer_equality, "optimizer/equality.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_logical", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_logical: logical (&&, ||, ^^)")
    {
        run_tests(g_optimizer_logical, "optimizer/logical.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_match", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_match: match (~=)")
    {
        run_tests(g_optimizer_match, "optimizer/match.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_multiplicative", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_multiplicative: multiplicative (*, /, %)")
    {
        run_tests(g_optimizer_multiplicative, "optimizer/multiplicative.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_relational", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_relational: relational (<, <=, >, >=)")
    {
        run_tests(g_optimizer_relational, "optimizer_relational.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("optimizer_statements", "[optimizer]")
{
    CATCH_START_SECTION("optimizer_statements: statement")
    {
        run_tests(g_optimizer_statements, "optimizer_statements.json");
    }
    CATCH_END_SECTION()
}








// vim: ts=4 sw=4 et
