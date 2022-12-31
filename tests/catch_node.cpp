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
#include    <as2js/exception.h>
#include    <as2js/message.h>
#include    <as2js/node.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/ostream_to_buf.h>
#include    <snapdev/safe_stream.h>
#include    <snapdev/tokenize_string.h>


// libutf8
//
#include    <libutf8/iterator.h>


// C++
//
#include    <cstring>
#include    <algorithm>
#include    <iomanip>


// C
//
#include    <signal.h>
#include    <sys/wait.h>


// last include
//
#include    <snapdev/poison.h>




#include    "catch_node_data.ci"



namespace
{


int quick_exec(std::string const & cmd)
{
    int const child_pid(fork());
    if(child_pid < 0)
    {
        int const e(errno);
        std::cerr << "error: fork() failed: "
            << e
            << ", "
            << strerror(e)
            << "\n";
        return -1;
    }

    if(child_pid != 0)
    {
        // parent just waits on the child
        //
        int status(0);
        pid_t const pid(waitpid(child_pid, &status, 0));
        if(pid != child_pid)
        {
            std::cerr << "error: waitpid() returned "
                << pid
                << ", expected: "
                << child_pid
                << " instead.\n";
            return 128;
        }

        if(!WIFEXITED(status))
        {
            std::cerr << "error: waitpid() returned with a status other than \"exited\".\n";
            return 128;
        }
        else if(WIFSIGNALED(status))
        {
            std::cerr << "error: child was signaled.\n";
            return 128;
        }
        else
        {
            return WEXITSTATUS(status);
        }
    }

    std::vector<std::string> arg_strings;
    snapdev::tokenize_string(arg_strings, cmd, {" "}, true);

    std::vector<char *> args(arg_strings.size() + 1);
    for(std::size_t idx(0); idx < arg_strings.size(); ++idx)
    {
        args[idx] = const_cast<char *>(arg_strings[idx].c_str());
    }

    execvp(args[0], args.data());

    // it should never return
    //
    snapdev::NOT_REACHED();
}



}







CATCH_TEST_CASE("node_types", "[node][type]")
{
    CATCH_START_SECTION("node_types: all types (defined in catch_node.ci)")
    {
        std::vector<bool> valid_types(static_cast<std::size_t>(as2js::node_t::NODE_max) + 1);
        for(std::size_t i(0); i < g_node_types_size; ++i)
        {
//std::cerr << "--- working on node type: [" << g_node_types[i].f_name << "] (" << static_cast<std::size_t>(g_node_types[i].f_type) << ")\n";
            if(static_cast<std::size_t>(g_node_types[i].f_type) < static_cast<std::size_t>(as2js::node_t::NODE_max))
            {
                valid_types[static_cast<std::size_t>(g_node_types[i].f_type)] = true;
            }

            // define the type
            as2js::node_t const node_type(g_node_types[i].f_type);

            CATCH_REQUIRE(strcmp(as2js::node::type_to_string(node_type), g_node_types[i].f_name) == 0);

            if(static_cast<std::size_t>(node_type) > static_cast<std::size_t>(as2js::node_t::NODE_max)
            && node_type != as2js::node_t::NODE_EOF)
            {
                std::cerr << "Somehow a node type (" << static_cast<int>(node_type)
                          << ") is larger than the maximum allowed ("
                          << (static_cast<int>(as2js::node_t::NODE_max) - 1) << ")" << std::endl;
                CATCH_REQUIRE(node_type == as2js::node_t::NODE_EOF);
            }

            // get the next type of node
            as2js::node::pointer_t node(std::make_shared<as2js::node>(node_type));

            // check the type
            CATCH_REQUIRE(node->get_type() == node_type);

            // get the name
            char const *name(node->get_type_name());
//std::cerr << "type = " << static_cast<int>(node_type) << " / " << name << "\n";
            CATCH_REQUIRE(strcmp(name, g_node_types[i].f_name) == 0);

            // test functions determining general types
            CATCH_REQUIRE((node->is_number() == false || node->is_number() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_number() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NUMBER) == 0));

            // This NaN test is not sufficient for strings
            CATCH_REQUIRE((node->is_nan() == false || node->is_nan() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_nan() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NAN) == 0));

            CATCH_REQUIRE((node->is_integer() == false || node->is_integer() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_integer() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_INTEGER) == 0));

            CATCH_REQUIRE((node->is_floating_point() == false || node->is_floating_point() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_floating_point() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FLOATING_POINT) == 0));

            CATCH_REQUIRE((node->is_boolean() == false || node->is_boolean() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_boolean() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0));

            CATCH_REQUIRE((node->is_true() == false || node->is_true() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_true() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) == 0));

            CATCH_REQUIRE((node->is_false() == false || node->is_false() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_false() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FALSE) == 0));

            CATCH_REQUIRE((node->is_string() == false || node->is_string() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_string() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_STRING) == 0));

            CATCH_REQUIRE((node->is_undefined() == false || node->is_undefined() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_undefined() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_UNDEFINED) == 0));

            CATCH_REQUIRE((node->is_null() == false || node->is_null() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_null() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NULL) == 0));

            CATCH_REQUIRE((node->is_identifier() == false || node->is_identifier() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_identifier() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_IDENTIFIER) == 0));

            CATCH_REQUIRE((node->is_literal() == false || node->is_literal() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->is_literal() ^ ((g_node_types[i].f_flags & (
                                                                                              TEST_NODE_IS_INTEGER
                                                                                            | TEST_NODE_IS_FLOATING_POINT
                                                                                            | TEST_NODE_IS_TRUE
                                                                                            | TEST_NODE_IS_FALSE
                                                                                            | TEST_NODE_IS_STRING
                                                                                            | TEST_NODE_IS_UNDEFINED
                                                                                            | TEST_NODE_IS_NULL)) == 0));

            if(!node->is_literal())
            {
                as2js::node::pointer_t literal(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
                CATCH_REQUIRE(as2js::node::compare(node, literal, as2js::compare_mode_t::COMPARE_STRICT)  == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(node, literal, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(node, literal, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(literal, node, as2js::compare_mode_t::COMPARE_STRICT)  == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(literal, node, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(literal, node, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
                CATCH_REQUIRE(as2js::node::compare(literal, node, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_ERROR);
            }

            CATCH_REQUIRE((node->has_side_effects() == false || node->has_side_effects() == true));
            CATCH_REQUIRE(static_cast<as2js::node const *>(node.get())->has_side_effects() ^ ((g_node_types[i].f_flags & TEST_NODE_HAS_SIDE_EFFECTS) == 0));

            if(g_node_types[i].f_operator != nullptr)
            {
                char const *op(as2js::node::operator_to_string(g_node_types[i].f_type));
                CATCH_REQUIRE(op != nullptr);
                CATCH_REQUIRE(strcmp(g_node_types[i].f_operator, op) == 0);
                //std::cerr << " testing " << node->get_type_name() << " from " << op << std::endl;
                CATCH_REQUIRE(as2js::node::string_to_operator(op) == g_node_types[i].f_type);

                // check the special case for not equal
                if(strcmp(g_node_types[i].f_operator, "!=") == 0)
                {
                    CATCH_REQUIRE(as2js::node::string_to_operator("<>") == g_node_types[i].f_type);
                }

                // check the special case for assignment
                if(strcmp(g_node_types[i].f_operator, "=") == 0)
                {
                    CATCH_REQUIRE(as2js::node::string_to_operator(":=") == g_node_types[i].f_type);
                }
            }
            else
            {
                // static function can also be called from the node pointer
                //std::cerr << " testing " << node->get_type_name() << std::endl;
                CATCH_REQUIRE(node->operator_to_string(g_node_types[i].f_type) == nullptr);
                CATCH_REQUIRE(as2js::node::string_to_operator(node->get_type_name()) == as2js::node_t::NODE_UNKNOWN);
            }

            if((g_node_types[i].f_flags & TEST_NODE_IS_SWITCH_OPERATOR) == 0)
            {
                // only NODE_PARAM_MATCH accepts this call
                as2js::node::pointer_t node_switch(std::make_shared<as2js::node>(as2js::node_t::NODE_SWITCH));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node_switch->set_switch_operator(node_type)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_switch_operator() called with an operator which is not valid for switch."));
            }
            else
            {
                as2js::node::pointer_t node_switch(std::make_shared<as2js::node>(as2js::node_t::NODE_SWITCH));
                node_switch->set_switch_operator(node_type);
                CATCH_REQUIRE(node_switch->get_switch_operator() == node_type);
            }
            if(node_type != as2js::node_t::NODE_SWITCH)
            {
                // a valid operator, but not a valid node to set
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_switch_operator(as2js::node_t::NODE_STRICTLY_EQUAL)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_switch_operator() called on a node which is not a switch node."));
                // not a valid node to get
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_switch_operator()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_switch_operator() called on a node which is not a switch node."));
            }

            if((g_node_types[i].f_flags & TEST_NODE_IS_PARAM_MATCH) == 0)
            {
                // only NODE_PARAM_MATCH accepts this call
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_param_size(10)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_param_size() called with a node other than a \"NODE_PARAM_MATCH\"."));
            }
            else
            {
                // zero is not acceptable
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_param_size(0)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_param_size() was called with a size of zero."));
                // this one is accepted
                node->set_param_size(10);
                // cannot change the size once set
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_param_size(10)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_param_size() called twice."));
            }

            if((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_boolean()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_boolean() called with a non-Boolean node type."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_boolean(rand() & 1)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_boolean() called with a non-Boolean node type."));
            }
            else if((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) != 0)
            {
                CATCH_REQUIRE(node->get_boolean());
            }
            else
            {
                CATCH_REQUIRE(!node->get_boolean());
            }

            if((g_node_types[i].f_flags & TEST_NODE_IS_INTEGER) == 0)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_integer()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_integer() called with a non-integer node type."));
                as2js::integer random(rand());
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_integer(random)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_integer() called with a non-integer node type."));
            }

            if((g_node_types[i].f_flags & TEST_NODE_IS_FLOATING_POINT) == 0)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_floating_point()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_floating_point() called with a non-floating point node type."));

                as2js::floating_point random(rand());
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_floating_point(random)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_floating_point() called with a non-floating point node type."));
            }

            // here we have a special case as "many" different nodes accept
            // a string to represent one thing or another
            //
            if((g_node_types[i].f_flags & TEST_NODE_ACCEPT_STRING) == 0)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_string()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: get_string() called with non-string node type: \""
                            + std::string(as2js::node::type_to_string(node_type))
                            + "\"."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_string("test")
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: set_string() called with a non-string node type."));
            }
            else
            {
                node->set_string("random test");
                CATCH_REQUIRE(node->get_string() == "random test");
            }

            // first test the flags that this type of node accepts
            std::bitset<static_cast<int>(as2js::flag_t::NODE_FLAG_max)> valid_flags;
            for(flags_per_node_t const *node_flags(g_node_types[i].f_node_flags);
                                        node_flags->f_flag != as2js::flag_t::NODE_FLAG_max;
                                        ++node_flags)
            {
                // mark this specific flag as valid
                valid_flags[static_cast<int>(node_flags->f_flag)] = true;

                as2js::flag_set_t set;
                CATCH_REQUIRE(node->compare_all_flags(set));


                // before we set it, always false
                CATCH_REQUIRE(!node->get_flag(node_flags->f_flag));
                node->set_flag(node_flags->f_flag, true);
                CATCH_REQUIRE(node->get_flag(node_flags->f_flag));

                CATCH_REQUIRE(!node->compare_all_flags(set));
                set[static_cast<int>(node_flags->f_flag)] = true;
                CATCH_REQUIRE(node->compare_all_flags(set));

                node->set_flag(node_flags->f_flag, false);
                CATCH_REQUIRE(!node->get_flag(node_flags->f_flag));
            }

            // now test all the other flags
            for(int j(-5); j <= static_cast<int>(as2js::flag_t::NODE_FLAG_max) + 5; ++j)
            {
                if(j < 0
                || j >= static_cast<int>(as2js::flag_t::NODE_FLAG_max)
                || !valid_flags[j])
                {
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->get_flag(static_cast<as2js::flag_t>(j))
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node_flag.cpp: node::verify_flag(): flag ("
                                + std::to_string(j)
                                + ") / type missmatch ("
                                + std::to_string(static_cast<int>(node->get_type()))
                                + ")."));
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->set_flag(static_cast<as2js::flag_t>(j), true)
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node_flag.cpp: node::verify_flag(): flag ("
                                + std::to_string(j)
                                + ") / type missmatch ("
                                + std::to_string(static_cast<int>(node->get_type()))
                                + ")."));
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->set_flag(static_cast<as2js::flag_t>(j), false)
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node_flag.cpp: node::verify_flag(): flag ("
                                + std::to_string(j)
                                + ") / type missmatch ("
                                + std::to_string(static_cast<int>(node->get_type()))
                                + ")."));
                }
            }

            // test completely invalid attribute indices
            for(int j(-5); j < 0; ++j)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_attribute(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_attribute(static_cast<as2js::attribute_t>(j), true)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_attribute(static_cast<as2js::attribute_t>(j), false)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->attribute_to_string(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
            }
            for(int j(static_cast<int>(as2js::attribute_t::NODE_ATTR_max));
                    j <= static_cast<int>(as2js::attribute_t::NODE_ATTR_max) + 5;
                    ++j)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_attribute(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_attribute(static_cast<as2js::attribute_t>(j), true)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->set_attribute(static_cast<as2js::attribute_t>(j), false)
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->attribute_to_string(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j))
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: unknown attribute number in node::attribute_to_string()."));
            }

            // attributes can be assigned to all types except NODE_PROGRAM
            // which only accepts NODE_DEFINED
            for(int j(0); j < static_cast<int>(as2js::attribute_t::NODE_ATTR_max); ++j)
            {
                bool valid(true);
                switch(node_type)
                {
                case as2js::node_t::NODE_PROGRAM:
                    valid = j == static_cast<int>(as2js::attribute_t::NODE_ATTR_DEFINED);
                    break;

                case as2js::node_t::NODE_ADD:
                case as2js::node_t::NODE_ARRAY:
                case as2js::node_t::NODE_ARRAY_LITERAL:
                case as2js::node_t::NODE_AS:
                case as2js::node_t::NODE_ASSIGNMENT:
                case as2js::node_t::NODE_ASSIGNMENT_ADD:
                case as2js::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                case as2js::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                case as2js::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                case as2js::node_t::NODE_ASSIGNMENT_DIVIDE:
                case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                case as2js::node_t::NODE_ASSIGNMENT_MAXIMUM:
                case as2js::node_t::NODE_ASSIGNMENT_MINIMUM:
                case as2js::node_t::NODE_ASSIGNMENT_MODULO:
                case as2js::node_t::NODE_ASSIGNMENT_MULTIPLY:
                case as2js::node_t::NODE_ASSIGNMENT_POWER:
                case as2js::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                case as2js::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                case as2js::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                case as2js::node_t::NODE_ASSIGNMENT_SUBTRACT:
                case as2js::node_t::NODE_BITWISE_AND:
                case as2js::node_t::NODE_BITWISE_NOT:
                case as2js::node_t::NODE_BITWISE_OR:
                case as2js::node_t::NODE_BITWISE_XOR:
                case as2js::node_t::NODE_CALL:
                case as2js::node_t::NODE_CONDITIONAL:
                case as2js::node_t::NODE_DECREMENT:
                case as2js::node_t::NODE_DELETE:
                case as2js::node_t::NODE_DIVIDE:
                case as2js::node_t::NODE_EQUAL:
                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_FLOATING_POINT:
                case as2js::node_t::NODE_FUNCTION:
                case as2js::node_t::NODE_GREATER:
                case as2js::node_t::NODE_GREATER_EQUAL:
                case as2js::node_t::NODE_IDENTIFIER:
                case as2js::node_t::NODE_IN:
                case as2js::node_t::NODE_INCREMENT:
                case as2js::node_t::NODE_INSTANCEOF:
                case as2js::node_t::NODE_INTEGER:
                case as2js::node_t::NODE_IS:
                case as2js::node_t::NODE_LESS:
                case as2js::node_t::NODE_LESS_EQUAL:
                case as2js::node_t::NODE_LIST:
                case as2js::node_t::NODE_LOGICAL_AND:
                case as2js::node_t::NODE_LOGICAL_NOT:
                case as2js::node_t::NODE_LOGICAL_OR:
                case as2js::node_t::NODE_LOGICAL_XOR:
                case as2js::node_t::NODE_MATCH:
                case as2js::node_t::NODE_MAXIMUM:
                case as2js::node_t::NODE_MEMBER:
                case as2js::node_t::NODE_MINIMUM:
                case as2js::node_t::NODE_MODULO:
                case as2js::node_t::NODE_MULTIPLY:
                case as2js::node_t::NODE_NAME:
                case as2js::node_t::NODE_NEW:
                case as2js::node_t::NODE_NOT_EQUAL:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_OBJECT_LITERAL:
                case as2js::node_t::NODE_POST_DECREMENT:
                case as2js::node_t::NODE_POST_INCREMENT:
                case as2js::node_t::NODE_POWER:
                case as2js::node_t::NODE_PRIVATE:
                case as2js::node_t::NODE_PUBLIC:
                case as2js::node_t::NODE_RANGE:
                case as2js::node_t::NODE_ROTATE_LEFT:
                case as2js::node_t::NODE_ROTATE_RIGHT:
                case as2js::node_t::NODE_SCOPE:
                case as2js::node_t::NODE_SHIFT_LEFT:
                case as2js::node_t::NODE_SHIFT_RIGHT:
                case as2js::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                case as2js::node_t::NODE_STRICTLY_EQUAL:
                case as2js::node_t::NODE_STRICTLY_NOT_EQUAL:
                case as2js::node_t::NODE_STRING:
                case as2js::node_t::NODE_SUBTRACT:
                case as2js::node_t::NODE_SUPER:
                case as2js::node_t::NODE_THIS:
                case as2js::node_t::NODE_TRUE:
                case as2js::node_t::NODE_TYPEOF:
                case as2js::node_t::NODE_UNDEFINED:
                case as2js::node_t::NODE_VIDENTIFIER:
                case as2js::node_t::NODE_VOID:
                    break;

                default:
                    // any other type and you get an exception
                    valid = j != static_cast<int>(as2js::attribute_t::NODE_ATTR_TYPE);
                    break;

                }

                //if(node_type == as2js::node_t::NODE_PROGRAM
                //&& j != static_cast<int>(as2js::attribute_t::NODE_ATTR_DEFINED))
                if(!valid)
                {
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->get_attribute(static_cast<as2js::attribute_t>(j))
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node \""
                                + std::string(as2js::node::type_to_string(node->get_type()))
                                + "\" does not like attribute \""
                                + as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j))
                                + "\" in node::verify_attribute()."));
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->set_attribute(static_cast<as2js::attribute_t>(j), true)
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node \""
                                + std::string(as2js::node::type_to_string(node->get_type()))
                                + "\" does not like attribute \""
                                + as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j))
                                + "\" in node::verify_attribute()."));
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->set_attribute(static_cast<as2js::attribute_t>(j), false)
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: node \""
                                + std::string(as2js::node::type_to_string(node->get_type()))
                                + "\" does not like attribute \""
                                + as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j))
                                + "\" in node::verify_attribute()."));
                }
                else
                {
                    // before we set it, always false
                    CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(j)));
                    node->set_attribute(static_cast<as2js::attribute_t>(j), true);
                    CATCH_REQUIRE(node->get_attribute(static_cast<as2js::attribute_t>(j)));
                    // since we reset them all we won't have a problem with conflicts in this loop
                    node->set_attribute(static_cast<as2js::attribute_t>(j), false);
                    CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(j)));
                }
                char const *attr_name1(node->attribute_to_string(static_cast<as2js::attribute_t>(j)));
                CATCH_REQUIRE(attr_name1 != nullptr);
                char const *attr_name2(as2js::node::attribute_to_string(static_cast<as2js::attribute_t>(j)));
                CATCH_REQUIRE(attr_name2 != nullptr);
                CATCH_REQUIRE(strcmp(attr_name1, attr_name2) == 0);

                switch(static_cast<as2js::attribute_t>(j))
                {
                case as2js::attribute_t::NODE_ATTR_PUBLIC:       CATCH_REQUIRE(strcmp(attr_name1, "PUBLIC")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_PRIVATE:      CATCH_REQUIRE(strcmp(attr_name1, "PRIVATE")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_PROTECTED:    CATCH_REQUIRE(strcmp(attr_name1, "PROTECTED")      == 0); break;
                case as2js::attribute_t::NODE_ATTR_INTERNAL:     CATCH_REQUIRE(strcmp(attr_name1, "INTERNAL")       == 0); break;
                case as2js::attribute_t::NODE_ATTR_TRANSIENT:    CATCH_REQUIRE(strcmp(attr_name1, "TRANSIENT")      == 0); break;
                case as2js::attribute_t::NODE_ATTR_VOLATILE:     CATCH_REQUIRE(strcmp(attr_name1, "VOLATILE")       == 0); break;
                case as2js::attribute_t::NODE_ATTR_STATIC:       CATCH_REQUIRE(strcmp(attr_name1, "STATIC")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_ABSTRACT:     CATCH_REQUIRE(strcmp(attr_name1, "ABSTRACT")       == 0); break;
                case as2js::attribute_t::NODE_ATTR_VIRTUAL:      CATCH_REQUIRE(strcmp(attr_name1, "VIRTUAL")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_ARRAY:        CATCH_REQUIRE(strcmp(attr_name1, "ARRAY")          == 0); break;
                case as2js::attribute_t::NODE_ATTR_INLINE:       CATCH_REQUIRE(strcmp(attr_name1, "INLINE")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_REQUIRE_ELSE: CATCH_REQUIRE(strcmp(attr_name1, "REQUIRE_ELSE")   == 0); break;
                case as2js::attribute_t::NODE_ATTR_ENSURE_THEN:  CATCH_REQUIRE(strcmp(attr_name1, "ENSURE_THEN")    == 0); break;
                case as2js::attribute_t::NODE_ATTR_NATIVE:       CATCH_REQUIRE(strcmp(attr_name1, "NATIVE")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_DEPRECATED:   CATCH_REQUIRE(strcmp(attr_name1, "DEPRECATED")     == 0); break;
                case as2js::attribute_t::NODE_ATTR_UNSAFE:       CATCH_REQUIRE(strcmp(attr_name1, "UNSAFE")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_CONSTRUCTOR:  CATCH_REQUIRE(strcmp(attr_name1, "CONSTRUCTOR")    == 0); break;
                case as2js::attribute_t::NODE_ATTR_FINAL:        CATCH_REQUIRE(strcmp(attr_name1, "FINAL")          == 0); break;
                case as2js::attribute_t::NODE_ATTR_ENUMERABLE:   CATCH_REQUIRE(strcmp(attr_name1, "ENUMERABLE")     == 0); break;
                case as2js::attribute_t::NODE_ATTR_TRUE:         CATCH_REQUIRE(strcmp(attr_name1, "TRUE")           == 0); break;
                case as2js::attribute_t::NODE_ATTR_FALSE:        CATCH_REQUIRE(strcmp(attr_name1, "FALSE")          == 0); break;
                case as2js::attribute_t::NODE_ATTR_UNUSED:       CATCH_REQUIRE(strcmp(attr_name1, "UNUSED")         == 0); break;
                case as2js::attribute_t::NODE_ATTR_DYNAMIC:      CATCH_REQUIRE(strcmp(attr_name1, "DYNAMIC")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_FOREACH:      CATCH_REQUIRE(strcmp(attr_name1, "FOREACH")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_NOBREAK:      CATCH_REQUIRE(strcmp(attr_name1, "NOBREAK")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_AUTOBREAK:    CATCH_REQUIRE(strcmp(attr_name1, "AUTOBREAK")      == 0); break;
                case as2js::attribute_t::NODE_ATTR_TYPE:         CATCH_REQUIRE(strcmp(attr_name1, "TYPE")           == 0); break;
                case as2js::attribute_t::NODE_ATTR_DEFINED:      CATCH_REQUIRE(strcmp(attr_name1, "DEFINED")        == 0); break;
                case as2js::attribute_t::NODE_ATTR_max:          CATCH_REQUIRE(!"attribute max should not be checked in this test"); break;
                }
            }

            // cloning is available for basic nodes
            //
            if((g_node_types[i].f_flags & TEST_NODE_IS_BASIC) == 0)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->clone_basic_node()
                    , as2js::internal_error
                    , Catch::Matchers::ExceptionMessage(
                              "internal_error: node.cpp: clone_basic_node(): called with a node which is not considered to be a basic node."));
            }
            else
            {
                // TODO: better test what is copied and what is not copied
                //
                as2js::node::pointer_t clone(node->clone_basic_node());
                CATCH_REQUIRE(node->get_type() == clone->get_type());
                //CATCH_REQUIRE(node->get_flag(...) == clone->get_flag(...));
                CATCH_REQUIRE(node->get_attribute_node() == clone->get_attribute_node());
                //CATCH_REQUIRE(node->get_attribute(...) == clone->get_attribute(...));
                //CATCH_REQUIRE(node->get_switch_operator() == clone->get_switch_operator()); -- none of these nodes are NODE_SWITCH
                CATCH_REQUIRE(node->is_locked() == clone->is_locked());
                CATCH_REQUIRE(node->get_position() == clone->get_position());
                CATCH_REQUIRE(node->get_instance() == clone->get_instance());
                CATCH_REQUIRE(node->get_goto_enter() == clone->get_goto_enter());
                CATCH_REQUIRE(node->get_goto_exit() == clone->get_goto_exit());
                //CATCH_REQUIRE(node->get_variable(...) == clone->get_variable(...));
                //CATCH_REQUIRE(node->find_label(...) == clone->find_label(...));
            }
        }

        // as we may be adding new node types without updating the tests,
        // here we verify that all node types that were not checked are
        // indeed invalid
        //
        // the vector is important because the node type numbers are not
        // incremental; some make use of the input character (i.e. '=' and
        // '!' are node types for the assignment and logical not) then we
        // jump to number 1001
        //
        for(std::size_t i(0); i <= static_cast<std::size_t>(as2js::node_t::NODE_max); ++i)
        {
            if(!valid_types[i])
            {
                // WARNING: if an errror occurs here it is likely because
                //          the corresponding node type is new and the test
                //          was not yet updated; i.e. if the node is now
                //          considered valid, then we are expected to
                //          return from the make_shared<>() instead of
                //          throwing as expected by the test for invalid
                //          nodes
                //
                //          also... this does not work if you also cannot
                //          create the node (i.e. I added a type, added the
                //          parsing in the lexer, did not add the type to
                //          the node::node() constructor)
                //
//std::cerr << "--- creating node with 'invalid' type: " << i << "\n";
                as2js::node_t const node_type(static_cast<as2js::node_t>(i));
                CATCH_REQUIRE_THROWS_MATCHES(
                      std::make_shared<as2js::node>(node_type)
                    , as2js::incompatible_node_type
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: unknown node type number, "
                            + std::to_string(i)
                            + ", used to create a node."));
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("node_types: node types outside the defined range")
    {
        // test with completely random numbers too (outside of the
        // standard range of node types)
        //
        for(size_t i(0); i < 100; ++i)
        {
            int32_t j((rand() << 16) ^ rand());
            if(j < -1 || j >= static_cast<ssize_t>(as2js::node_t::NODE_max))
            {
                as2js::node_t node_type(static_cast<as2js::node_t>(j));
                CATCH_REQUIRE_THROWS_MATCHES(
                      std::make_shared<as2js::node>(node_type)
                    , as2js::incompatible_node_type
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: unknown node type number, "
                            + std::to_string(j)
                            + ", used to create a node."));
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_compare", "[node][compare]")
{
    CATCH_START_SECTION("node_compare: NULL value")
    {
        as2js::node::pointer_t node1_true(std::make_shared<as2js::node>(as2js::node_t::NODE_TRUE));
        as2js::node::pointer_t node2_false(std::make_shared<as2js::node>(as2js::node_t::NODE_FALSE));
        as2js::node::pointer_t node3_true(std::make_shared<as2js::node>(as2js::node_t::NODE_TRUE));
        as2js::node::pointer_t node4_false(std::make_shared<as2js::node>(as2js::node_t::NODE_FALSE));

        as2js::node::pointer_t node5_33(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
        as2js::integer i33;
        i33.set(33);
        node5_33->set_integer(i33);

        as2js::node::pointer_t node6_101(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
        as2js::integer i101;
        i101.set(101);
        node6_101->set_integer(i101);

        as2js::node::pointer_t node7_33(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
        as2js::floating_point f33;
        f33.set(3.3);
        node7_33->set_floating_point(f33);

        as2js::node::pointer_t node7_nearly33(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
        as2js::floating_point fnearly33;
        fnearly33.set(3.300001);
        node7_nearly33->set_floating_point(fnearly33);

        as2js::node::pointer_t node8_101(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
        as2js::floating_point f101;
        f101.set(1.01);
        node8_101->set_floating_point(f101);

        as2js::node::pointer_t node9_null(std::make_shared<as2js::node>(as2js::node_t::NODE_NULL));
        as2js::node::pointer_t node10_null(std::make_shared<as2js::node>(as2js::node_t::NODE_NULL));

        as2js::node::pointer_t node11_undefined(std::make_shared<as2js::node>(as2js::node_t::NODE_UNDEFINED));
        as2js::node::pointer_t node12_undefined(std::make_shared<as2js::node>(as2js::node_t::NODE_UNDEFINED));

        as2js::node::pointer_t node13_empty_string(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
        as2js::node::pointer_t node14_blah(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
        node14_blah->set_string("blah");
        as2js::node::pointer_t node15_foo(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
        node15_foo->set_string("foo");
        as2js::node::pointer_t node16_07(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
        node16_07->set_string("0.7");
        as2js::node::pointer_t node17_nearly33(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));
        node17_nearly33->set_string("3.300001");

        // BOOLEAN
        CATCH_REQUIRE(as2js::node::compare(node1_true, node1_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node3_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node1_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node3_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node1_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node3_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node1_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node3_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node1_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node3_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node1_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node3_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node2_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node4_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node2_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node4_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node2_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node4_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node2_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node4_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node2_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node4_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node2_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node4_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node2_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node2_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node4_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node4_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node2_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node2_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node4_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node4_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node2_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node2_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node1_true, node4_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node3_true, node4_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node1_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node3_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node1_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node3_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node1_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node3_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node1_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node3_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);

        CATCH_REQUIRE(as2js::node::compare(node2_false, node1_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node3_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node1_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node4_false, node3_true, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);

        // FLOAT
        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_nearly33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node7_nearly33, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node17_nearly33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node17_nearly33, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node8_101, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node8_101, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_nearly33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node7_nearly33, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node17_nearly33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node17_nearly33, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node8_101, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node8_101, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node7_nearly33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_nearly33, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node17_nearly33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node17_nearly33, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node7_33, node8_101, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node8_101, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // INTEGER
        CATCH_REQUIRE(as2js::node::compare(node5_33, node5_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node6_101, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node5_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node6_101, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node5_33, node5_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node6_101, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node5_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node6_101, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node5_33, node5_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node6_101, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node5_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node6_101, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // NULL
        CATCH_REQUIRE(as2js::node::compare(node9_null, node9_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node10_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node9_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node10_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node9_null, node9_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node10_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node9_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node10_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node9_null, node9_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node10_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node9_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node10_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // UNDEFINED
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node11_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node12_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // STRING
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node13_empty_string, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node14_blah, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node15_foo, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node13_empty_string, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node14_blah, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node15_foo, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node13_empty_string, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node14_blah, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node15_foo, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node13_empty_string, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node14_blah, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node15_foo, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node13_empty_string, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node14_blah, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node15_foo, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node13_empty_string, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node14_blah, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node15_foo, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node13_empty_string, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node14_blah, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node15_foo, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node13_empty_string, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node14_blah, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node15_foo, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node13_empty_string, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node14_blah, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node15_foo, node15_foo, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // NULL vs UNDEFINED
        CATCH_REQUIRE(as2js::node::compare(node9_null, node11_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node12_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node11_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node12_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node9_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node9_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node10_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node10_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

        CATCH_REQUIRE(as2js::node::compare(node9_null, node11_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node12_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node11_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node12_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node9_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node9_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node10_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node10_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

        CATCH_REQUIRE(as2js::node::compare(node9_null, node11_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node12_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node11_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node10_null, node12_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node9_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node9_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node10_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
        CATCH_REQUIRE(as2js::node::compare(node12_undefined, node10_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

        // <any> against FLOATING_POINT
        CATCH_REQUIRE(as2js::node::compare(node1_true, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node16_07, node7_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node16_07, node7_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);

        CATCH_REQUIRE(as2js::node::compare(node1_true, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node2_false, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node5_33, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node6_101, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node9_null, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node11_undefined, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node13_empty_string, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node14_blah, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node16_07, node7_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);

        // FLOATING_POINT against <any>
        CATCH_REQUIRE(as2js::node::compare(node8_101, node1_true, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node2_false, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node5_33, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node6_101, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node9_null, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node11_undefined, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node13_empty_string, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node14_blah, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node16_07, as2js::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

        CATCH_REQUIRE(as2js::node::compare(node8_101, node1_true, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node2_false, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node5_33, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node6_101, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node9_null, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node11_undefined, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node13_empty_string, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node14_blah, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node16_07, as2js::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);

        CATCH_REQUIRE(as2js::node::compare(node8_101, node2_false, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node5_33, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node6_101, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node9_null, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node11_undefined, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node13_empty_string, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node14_blah, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
        CATCH_REQUIRE(as2js::node::compare(node8_101, node16_07, as2js::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_conversions", "[node][conversion]")
{
    CATCH_START_SECTION("node_conversions: simple")
    {
        // first test simple conversions
        for(size_t i(0); i < g_node_types_size; ++i)
        {
            // original type
            as2js::node_t original_type(g_node_types[i].f_type);

            // all nodes can be converted to UNKNOWN
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_unknown()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                node->to_unknown();
                CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_UNKNOWN);
            }

            // CALL can be convert to AS
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_as()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                if(original_type == as2js::node_t::NODE_CALL)
                {
                    // in this case it works
                    CATCH_REQUIRE(node->to_as());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_AS);
                }
                else
                {
                    // in this case it fails
                    CATCH_REQUIRE(!node->to_as());
                    CATCH_REQUIRE(node->get_type() == original_type);
                }
            }

            // test what would happen if we were to call to_boolean()
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    as2js::node_lock lock(node);
                    node->to_boolean_type_only();
                    CATCH_REQUIRE(node->get_type() == original_type);
                }
                as2js::node_t new_type(node->to_boolean_type_only());
                switch(original_type)
                {
                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(new_type == as2js::node_t::NODE_TRUE);
                    break;

                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_UNDEFINED:
                case as2js::node_t::NODE_INTEGER: // by default integers are set to zero
                case as2js::node_t::NODE_FLOATING_POINT: // by default floating points are set to zero
                case as2js::node_t::NODE_STRING: // by default strings are empty
                    CATCH_REQUIRE(new_type == as2js::node_t::NODE_FALSE);
                    break;

                default:
                    CATCH_REQUIRE(new_type == as2js::node_t::NODE_UNDEFINED);
                    break;

                }
            }

            // a few nodes can be converted to a boolean value
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_boolean()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(node->to_boolean());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_TRUE);
                    break;

                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_UNDEFINED:
                case as2js::node_t::NODE_INTEGER: // by default integers are set to zero
                case as2js::node_t::NODE_FLOATING_POINT: // by default floating points are set to zero
                case as2js::node_t::NODE_STRING: // by default strings are empty
                    CATCH_REQUIRE(node->to_boolean());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FALSE);
                    break;

                default:
                    CATCH_REQUIRE(!node->to_boolean());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // a couple types of nodes can be converted to a CALL
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_call()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_ASSIGNMENT:
                case as2js::node_t::NODE_MEMBER:
                    CATCH_REQUIRE(node->to_call());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_CALL);
                    break;

                default:
                    CATCH_REQUIRE(!node->to_call());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // a few types of nodes can be converted to an INTEGER
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_integer()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_INTEGER: // no change
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    break;

                case as2js::node_t::NODE_FLOATING_POINT:
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    break;

                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_UNDEFINED:
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == 0);
                    break;

                case as2js::node_t::NODE_STRING: // empty string to start with...
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == 0);

                    // if not empty...
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("34");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == 34);
                    }
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("+84");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == 84);
                    }
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("-37");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == -37);
                    }
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("3.4");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == 3);
                    }
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("34e+5");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == 3400000);
                    }
                    {
                        as2js::node::pointer_t node_str(std::make_shared<as2js::node>(original_type));
                        node_str->set_string("some NaN");
                        CATCH_REQUIRE(node_str->to_integer());
                        CATCH_REQUIRE(node_str->get_type() == as2js::node_t::NODE_INTEGER);
                        CATCH_REQUIRE(node_str->get_integer().get() == 0);
                    }
                    break;

                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == 1);
                    break;

                default:
                    CATCH_REQUIRE(!node->to_integer());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // a few types of nodes can be converted to a FLOATING_POINT
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_floating_point()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_INTEGER: // no change
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    break;

                case as2js::node_t::NODE_FLOATING_POINT:
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    break;

                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_STRING:
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        bool const is_zero(node->get_floating_point().get() == 0.0);
#pragma GCC diagnostic pop
                        CATCH_REQUIRE(is_zero);
                    }
                    break;

                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        bool const is_one(node->get_floating_point().get() == 1.0);
#pragma GCC diagnostic pop
                        CATCH_REQUIRE(is_one);
                    }
                    break;

                case as2js::node_t::NODE_UNDEFINED:
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    CATCH_REQUIRE(node->get_floating_point().is_nan());
                    break;

                default:
                    CATCH_REQUIRE(!node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // IDENTIFIER can be converted to LABEL
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_label()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                if(original_type == as2js::node_t::NODE_IDENTIFIER)
                {
                    // in this case it works
                    node->to_label();
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_LABEL);
                }
                else
                {
                    // this one fails with a soft error (returns false)
                    CATCH_REQUIRE(!node->to_label());
                    CATCH_REQUIRE(node->get_type() == original_type);
                }
            }

            // a few types of nodes can be converted to a Number
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_number()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_INTEGER: // no change!
                case as2js::node_t::NODE_FLOATING_POINT: // no change!
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_NULL:
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == 0);
                    break;

                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == 1);
                    break;

                case as2js::node_t::NODE_STRING: // empty strings represent 0 here
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        bool const is_zero(node->get_floating_point().get() == 0.0);
#pragma GCC diagnostic pop
                        CATCH_REQUIRE(is_zero);
                    }
                    break;

                case as2js::node_t::NODE_UNDEFINED:
//std::cerr << " . type = " << static_cast<int>(original_type) << " / " << node->get_type_name() << "\n";
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    CATCH_REQUIRE(node->get_floating_point().is_nan());
                    break;

                default:
                    CATCH_REQUIRE(!node->to_number());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // a few types of nodes can be converted to a STRING
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_string()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_STRING:
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(node->get_string() == "");
                    break;

                case as2js::node_t::NODE_FLOATING_POINT:
                case as2js::node_t::NODE_INTEGER:
                    // by default numbers are zero; we have other tests
                    // to verify the conversion
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == "0");
                    break;

                case as2js::node_t::NODE_FALSE:
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == "false");
                    break;

                case as2js::node_t::NODE_TRUE:
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == "true");
                    break;

                case as2js::node_t::NODE_NULL:
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == "null");
                    break;

                case as2js::node_t::NODE_UNDEFINED:
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == "undefined");
                    break;

                case as2js::node_t::NODE_IDENTIFIER: // the string remains the same
                //case as2js::node_t::NODE_VIDENTIFIER: // should the VIDENTIFIER be supported too?
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    break;

                default:
                    CATCH_REQUIRE(!node->to_string());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // a few types of nodes can be converted to an IDENTIFIER
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_identifier()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                switch(original_type)
                {
                case as2js::node_t::NODE_IDENTIFIER:
                    CATCH_REQUIRE(node->to_identifier());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(node->get_string() == "");
                    break;

                case as2js::node_t::NODE_PRIVATE:
                    CATCH_REQUIRE(node->to_identifier());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_IDENTIFIER);
                    CATCH_REQUIRE(node->get_string() == "private");
                    break;

                case as2js::node_t::NODE_PROTECTED:
                    CATCH_REQUIRE(node->to_identifier());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_IDENTIFIER);
                    CATCH_REQUIRE(node->get_string() == "protected");
                    break;

                case as2js::node_t::NODE_PUBLIC:
                    CATCH_REQUIRE(node->to_identifier());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_IDENTIFIER);
                    CATCH_REQUIRE(node->get_string() == "public");
                    break;

                default:
                    CATCH_REQUIRE(!node->to_identifier());
                    CATCH_REQUIRE(node->get_type() == original_type);
                    break;

                }
            }

            // IDENTIFIER can be converted to VIDENTIFIER
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_videntifier()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                if(original_type == as2js::node_t::NODE_IDENTIFIER)
                {
                    // in this case it works
                    node->to_videntifier();
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_VIDENTIFIER);
                }
                else
                {
                    // this one fails dramatically
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_videntifier()
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: to_videntifier() called with a node other than a \"NODE_IDENTIFIER\" node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                }
            }

            // VARIABLE can be converted to VAR_ATTRIBUTES
            {
                as2js::node::pointer_t node(std::make_shared<as2js::node>(original_type));
                {
                    snapdev::ostream_to_buf<char> out(std::cerr);
                    as2js::node_lock lock(node);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_var_attributes()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                    CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
                }
                if(original_type == as2js::node_t::NODE_VARIABLE)
                {
                    // in this case it works
                    node->to_var_attributes();
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_VAR_ATTRIBUTES);
                }
                else
                {
                    // in this case it fails
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->to_var_attributes()
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: to_var_attribute() called with a node other than a \"NODE_VARIABLE\" node."));
                    CATCH_REQUIRE(node->get_type() == original_type);
                }
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("node_conversions: NULL value")
    {
        bool got_dot(false);
        for(int i(0); i < 100; ++i)
        {
            // Integer to other types
            {
                as2js::integer j((static_cast<int64_t>(rand()) << 48)
                               ^ (static_cast<int64_t>(rand()) << 32)
                               ^ (static_cast<int64_t>(rand()) << 16)
                               ^ (static_cast<int64_t>(rand()) <<  0));

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    as2js::floating_point invalid;
                    CATCH_REQUIRE_THROWS_MATCHES(
                          node->set_floating_point(invalid)
                        , as2js::internal_error
                        , Catch::Matchers::ExceptionMessage(
                                  "internal_error: set_floating_point() called with a non-floating point node type."));
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->to_integer());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(node->get_integer().get() == j.get());
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    CATCH_REQUIRE(node->to_number());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
                    CATCH_REQUIRE(node->get_integer().get() == j.get());
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    as2js::node_t bool_type(node->to_boolean_type_only());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(bool_type == (j.get() ? as2js::node_t::NODE_TRUE : as2js::node_t::NODE_FALSE));
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    CATCH_REQUIRE(node->to_boolean());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(node->get_type() == (j.get() ? as2js::node_t::NODE_TRUE : as2js::node_t::NODE_FALSE));
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    CATCH_REQUIRE(node->to_floating_point());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    as2js::floating_point flt(j.get());
                    CATCH_REQUIRE(node->get_floating_point().nearly_equal(flt, 0.0001));
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
                    node->set_integer(j);
                    CATCH_REQUIRE(node->to_string());
                    // probably always true here; we had false in the loop prior
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    CATCH_REQUIRE(node->get_string() == std::string(std::to_string(j.get())));
                }
            }

            // Floating point to other values
            bool first(true);
            do
            {
                // generate a random 64 bit number
                //
                double s1(rand() & 1 ? -1 : 1);
                double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                            ^ (static_cast<int64_t>(rand()) << 32)
                                            ^ (static_cast<int64_t>(rand()) << 16)
                                            ^ (static_cast<int64_t>(rand()) <<  0)));
                double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                            ^ (static_cast<int64_t>(rand()) << 32)
                                            ^ (static_cast<int64_t>(rand()) << 16)
                                            ^ (static_cast<int64_t>(rand()) <<  0)));
                if(!first && n1 >= d1)
                {
                    // the dot is easier to reach with very small numbers
                    // so create a small number immediately
                    std::swap(n1, d1);
                    d1 *= 1e+4;
                }
                double r(n1 / d1 * s1);
                as2js::floating_point j(r);

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    CATCH_REQUIRE(node->to_integer());
                    CATCH_REQUIRE(node->get_integer().get() == static_cast<as2js::integer::value_type>(j.get()));
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    CATCH_REQUIRE(node->to_number());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        bool const is_equal(node->get_floating_point().get() == j.get());
#pragma GCC diagnostic pop
                        CATCH_REQUIRE(is_equal);
                    }
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    as2js::node_t bool_type(node->to_boolean_type_only());
                    // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    bool const is_zero(j.get() == 0.0);
#pragma GCC diagnostic pop
                    CATCH_REQUIRE(bool_type == (is_zero ? as2js::node_t::NODE_FALSE : as2js::node_t::NODE_TRUE));
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    CATCH_REQUIRE(node->to_boolean());
                    // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    bool const is_zero(j.get() == 0.0);
#pragma GCC diagnostic pop
                    CATCH_REQUIRE(node->get_type() == (is_zero ? as2js::node_t::NODE_FALSE : as2js::node_t::NODE_TRUE));

                    // also test the set_boolean() with valid values
                    node->set_boolean(true);
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_TRUE);
                    node->set_boolean(false);
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FALSE);
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    CATCH_REQUIRE(node->to_floating_point());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_FLOATING_POINT);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    bool const is_equal(node->get_floating_point().get() == j.get());
#pragma GCC diagnostic pop
                    CATCH_REQUIRE(is_equal);
                }

                {
                    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
                    node->set_floating_point(j);
                    CATCH_REQUIRE(node->to_string());
                    CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
                    std::string str(std::string(std::to_string(j.get())));
                    if(str.find('.') != std::string::npos)
                    {
                        // remove all least significant zeroes if any
                        while(str.back() == '0')
                        {
                            str.pop_back();
                        }
                        // make sure the number does not end with a period
                        if(str.back() == '.')
                        {
                            str.pop_back();
                            got_dot = true;
                        }
                    }
                    CATCH_REQUIRE(node->get_string() == str);
                }
                first = false;
            }
            while(!got_dot);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("node_conversions: special floating point values")
    {
        // verify special floating point values
        //
        { // NaN -> string
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_nan();
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_string());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
            CATCH_REQUIRE(node->get_string() == "NaN");
        }
        { // NaN -> integer
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_nan();
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_integer());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
            CATCH_REQUIRE(node->get_integer().get() == 0);
        }
        { // +Infinity
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_infinity();
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_string());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
            CATCH_REQUIRE(node->get_string() == "Infinity");
        }
        { // +Infinity
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_infinity();
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_integer());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
            CATCH_REQUIRE(node->get_integer().get() == 0);
        }
        { // -Infinity
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_infinity();
            j.set(-j.get());
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_string());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_STRING);
            CATCH_REQUIRE(node->get_string() == "-Infinity");
        }
        { // +Infinity
            as2js::floating_point j;
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
            j.set_infinity();
            j.set(-j.get());
            node->set_floating_point(j);
            CATCH_REQUIRE(node->to_integer());
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_INTEGER);
            CATCH_REQUIRE(node->get_integer().get() == 0);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_tree", "[node][tree]")
{
    class tracked_node
        : public as2js::node
    {
    public:
        tracked_node(as2js::node_t type, int & count)
            : node(type)
            , f_count(count)
        {
            ++f_count;
        }

        virtual ~tracked_node()
        {
            --f_count;
        }

    private:
        int &       f_count;
    };

    // a few basic tests
    CATCH_START_SECTION("node_tree: basics")
    {
        // counter to know how many nodes we currently have allocated
        //
        int counter(0);

        {
            as2js::node::pointer_t parent(std::make_shared<tracked_node>(as2js::node_t::NODE_DIRECTIVE_LIST, counter));

            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->get_child(-1)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: get_child(): index is too large for the number of children available."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->get_child(0)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: get_child(): index is too large for the number of children available."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->get_child(1)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: get_child(): index is too large for the number of children available."));

            // now we properly test whether the append_child(),
            // insert_child(), and set_child() functions are used
            // with a null pointer (which is considered illegal)
            as2js::node::pointer_t null_pointer;
            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->append_child(null_pointer)
                , as2js::invalid_data
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot append a child if its pointer is null."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->insert_child(123, null_pointer)
                , as2js::invalid_data
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot insert a child if its pointer is null."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  parent->set_child(9, null_pointer)
                , as2js::invalid_data
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot set a child if its pointer is null."));

            for(int i(0); i < 20; ++i)
            {
                as2js::node::pointer_t child(std::make_shared<tracked_node>(as2js::node_t::NODE_DIRECTIVE_LIST, counter));
                parent->append_child(child);

                CATCH_REQUIRE_THROWS_MATCHES(
                      parent->get_child(-1)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_child(): index is too large for the number of children available."));
                for(int j(0); j <= i; ++j)
                {
                    as2js::node::pointer_t c(parent->get_child(j));
                    CATCH_REQUIRE(c != nullptr);
                    if(i == j)
                    {
                        CATCH_REQUIRE(c == child);
                    }

                    // set_parent() with -1 does nothing when the parent
                    // of the child is the same
                    //
                    child->set_parent(parent, -1);
                }
                CATCH_REQUIRE_THROWS_MATCHES(
                      parent->get_child(i + 1)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_child(): index is too large for the number of children available."));
                CATCH_REQUIRE_THROWS_MATCHES(
                      parent->get_child(i + 2)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_child(): index is too large for the number of children available."));
            }
        }

        // did we deleted as many nodes as we created?
        //
        CATCH_REQUIRE(counter == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("node_tree: parent/child of any type")
    {
        // counter to know how many nodes we currently have allocated
        //
        int counter(0);

        // first test: try with all types as the parent and children
        for(size_t i(0); i < g_node_types_size; ++i)
        {
            // type
            as2js::node_t parent_type(g_node_types[i].f_type);

            as2js::node::pointer_t parent(std::make_shared<tracked_node>(parent_type, counter));
            CATCH_REQUIRE(parent->get_children_size() == 0);

            size_t valid_children(0);
            for(size_t j(0); j < g_node_types_size; ++j)
            {
                as2js::node_t child_type(g_node_types[j].f_type);

                as2js::node::pointer_t child(std::make_shared<tracked_node>(child_type, counter));

    //std::cerr << "parent " << parent->get_type_name() << " child " << child->get_type_name() << "\n";
                // some nodes cannot be parents...
                switch(parent_type)
                {
                case as2js::node_t::NODE_ABSTRACT:
                case as2js::node_t::NODE_ASYNC:
                case as2js::node_t::NODE_AUTO:
                case as2js::node_t::NODE_AWAIT:
                case as2js::node_t::NODE_BOOLEAN:
                case as2js::node_t::NODE_BREAK:
                case as2js::node_t::NODE_BYTE:
                case as2js::node_t::NODE_CLOSE_CURVLY_BRACKET:
                case as2js::node_t::NODE_CLOSE_PARENTHESIS:
                case as2js::node_t::NODE_CLOSE_SQUARE_BRACKET:
                case as2js::node_t::NODE_CHAR:
                case as2js::node_t::NODE_COLON:
                case as2js::node_t::NODE_COMMA:
                case as2js::node_t::NODE_CONST:
                case as2js::node_t::NODE_CONTINUE:
                case as2js::node_t::NODE_DEFAULT:
                case as2js::node_t::NODE_DOUBLE:
                case as2js::node_t::NODE_ELSE:
                case as2js::node_t::NODE_THEN:
                case as2js::node_t::NODE_EMPTY:
                case as2js::node_t::NODE_EOF:
                case as2js::node_t::NODE_IDENTIFIER:
                case as2js::node_t::NODE_INLINE:
                case as2js::node_t::NODE_INTEGER:
                case as2js::node_t::NODE_FALSE:
                case as2js::node_t::NODE_FINAL:
                case as2js::node_t::NODE_FLOAT:
                case as2js::node_t::NODE_FLOATING_POINT:
                case as2js::node_t::NODE_GOTO:
                case as2js::node_t::NODE_LONG:
                case as2js::node_t::NODE_NATIVE:
                case as2js::node_t::NODE_NULL:
                case as2js::node_t::NODE_OPEN_CURVLY_BRACKET:
                case as2js::node_t::NODE_OPEN_PARENTHESIS:
                case as2js::node_t::NODE_OPEN_SQUARE_BRACKET:
                case as2js::node_t::NODE_PRIVATE:
                case as2js::node_t::NODE_PROTECTED:
                case as2js::node_t::NODE_PUBLIC:
                case as2js::node_t::NODE_REGULAR_EXPRESSION:
                case as2js::node_t::NODE_REST:
                case as2js::node_t::NODE_SEMICOLON:
                case as2js::node_t::NODE_SHORT:
                case as2js::node_t::NODE_STRING:
                case as2js::node_t::NODE_STATIC:
                case as2js::node_t::NODE_THIS:
                case as2js::node_t::NODE_TRANSIENT:
                case as2js::node_t::NODE_TRUE:
                case as2js::node_t::NODE_UNDEFINED:
                case as2js::node_t::NODE_VIDENTIFIER:
                case as2js::node_t::NODE_VOID:
                case as2js::node_t::NODE_VOLATILE:
                    // append child to parent must fail
                    if(rand() & 1)
                    {
                        CATCH_REQUIRE_THROWS_MATCHES(
                              parent->append_child(child)
                            , as2js::incompatible_node_type
                            , Catch::Matchers::ExceptionMessage(
                                      "as2js_exception: invalid type: \""
                                    + std::string(parent->get_type_name())
                                    + "\" used as a parent node of child with type: \""
                                    + std::string(child->get_type_name())
                                    + "\"."));
                    }
                    else
                    {
                        CATCH_REQUIRE_THROWS_MATCHES(
                              child->set_parent(parent)
                            , as2js::incompatible_node_type
                            , Catch::Matchers::ExceptionMessage(
                                      "as2js_exception: invalid type: \""
                                    + std::string(parent->get_type_name())
                                    + "\" used as a parent node of child with type: \""
                                    + std::string(child->get_type_name())
                                    + "\"."));
                    }
                    break;

                default:
                    switch(child_type)
                    {
                    case as2js::node_t::NODE_CLOSE_CURVLY_BRACKET:
                    case as2js::node_t::NODE_CLOSE_PARENTHESIS:
                    case as2js::node_t::NODE_CLOSE_SQUARE_BRACKET:
                    case as2js::node_t::NODE_COLON:
                    case as2js::node_t::NODE_COMMA:
                    case as2js::node_t::NODE_ELSE:
                    case as2js::node_t::NODE_THEN:
                    case as2js::node_t::NODE_EOF:
                    case as2js::node_t::NODE_OPEN_CURVLY_BRACKET:
                    case as2js::node_t::NODE_OPEN_PARENTHESIS:
                    case as2js::node_t::NODE_OPEN_SQUARE_BRACKET:
                    case as2js::node_t::NODE_ROOT:
                    case as2js::node_t::NODE_SEMICOLON:
                        // append child to parent must fail
                        if(rand() & 1)
                        {
                            CATCH_REQUIRE_THROWS_MATCHES(
                                  parent->append_child(child)
                                , as2js::incompatible_node_type
                                , Catch::Matchers::ExceptionMessage(
                                          "as2js_exception: invalid type: \""
                                        + std::string(child->get_type_name())
                                        + "\" used as a child node."));
                        }
                        else
                        {
                            CATCH_REQUIRE_THROWS_MATCHES(
                                  child->set_parent(parent)
                                , as2js::incompatible_node_type
                                , Catch::Matchers::ExceptionMessage(
                                          "as2js_exception: invalid type: \""
                                        + std::string(child->get_type_name())
                                        + "\" used as a child node."));
                        }
                        break;

                    default:
                        // append child to parent
                        if(rand() & 1)
                        {
                            parent->append_child(child);
                        }
                        else
                        {
                            child->set_parent(parent);
                        }

                        CATCH_REQUIRE(parent->get_children_size() == valid_children + 1);
                        CATCH_REQUIRE(child->get_parent() == parent);
                        CATCH_REQUIRE(child->get_offset() == valid_children);
                        CATCH_REQUIRE(parent->get_child(valid_children) == child);
                        CATCH_REQUIRE(parent->find_first_child(child_type) == child);
                        CATCH_REQUIRE(!parent->find_next_child(child, child_type));

                        ++valid_children;
                        break;

                    }
                    break;

                }
            }
        }

        // did we deleted as many nodes as we created?
        //
        CATCH_REQUIRE(counter == 0);
    }
    CATCH_END_SECTION()

    // Test a more realistic tree with a few nodes and make sure we
    // can apply certain function and that the tree exactly results
    // in what we expect
    CATCH_START_SECTION("node_tree: realistic tree")
    {
        // counter to know how many nodes we currently have allocated
        //
        int counter(0);

        {
            // 1. Create the following in directive a:
            //
            //  // first block (directive_a)
            //  {
            //      a = Math.e ** 1.424;
            //  }
            //  // second block (directive_b)
            //  {
            //  }
            //
            // 2. Move it to directive b
            //
            //  // first block (directive_a)
            //  {
            //  }
            //  // second block (directive_b)
            //  {
            //      a = Math.e ** 1.424;
            //  }
            //
            // 3. Verify that it worked
            //

            // create all the nodes as the lexer would do
            as2js::node::pointer_t root(std::make_shared<tracked_node>(as2js::node_t::NODE_ROOT, counter));
            as2js::position pos;
            pos.reset_counters(22);
            pos.set_filename("test.js");
            root->set_position(pos);
            as2js::node::pointer_t directive_list_a(std::make_shared<tracked_node>(as2js::node_t::NODE_DIRECTIVE_LIST, counter));
            as2js::node::pointer_t directive_list_b(std::make_shared<tracked_node>(as2js::node_t::NODE_DIRECTIVE_LIST, counter));
            as2js::node::pointer_t assignment(std::make_shared<tracked_node>(as2js::node_t::NODE_ASSIGNMENT, counter));
            as2js::node::pointer_t identifier_a(std::make_shared<tracked_node>(as2js::node_t::NODE_IDENTIFIER, counter));
            identifier_a->set_string("a");
            as2js::node::pointer_t power(std::make_shared<tracked_node>(as2js::node_t::NODE_POWER, counter));
            as2js::node::pointer_t member(std::make_shared<tracked_node>(as2js::node_t::NODE_MEMBER, counter));
            as2js::node::pointer_t identifier_math(std::make_shared<tracked_node>(as2js::node_t::NODE_IDENTIFIER, counter));
            identifier_math->set_string("Math");
            as2js::node::pointer_t identifier_e(std::make_shared<tracked_node>(as2js::node_t::NODE_IDENTIFIER, counter));
            identifier_e->set_string("e");
            as2js::node::pointer_t literal(std::make_shared<tracked_node>(as2js::node_t::NODE_FLOATING_POINT, counter));
            as2js::floating_point f;
            f.set(1.424);
            literal->set_floating_point(f);

            // build the tree as the parser would do
            root->append_child(directive_list_a);
            root->append_child(directive_list_b);
            directive_list_a->append_child(assignment);
            assignment->append_child(identifier_a);
            assignment->insert_child(-1, power);
            power->append_child(member);
            CATCH_REQUIRE_THROWS_MATCHES(
                  power->insert_child(10, literal)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: trying to insert a node at the wrong position."));
            power->insert_child(1, literal);
            member->append_child(identifier_e);
            member->insert_child(0, identifier_math);

            // verify we can unlock mid-way
            as2js::node_lock temp_lock(member);
            CATCH_REQUIRE(member->is_locked());
            temp_lock.unlock();
            CATCH_REQUIRE(!member->is_locked());

            // as a complement to testing the lock, make sure that emptiness
            // (i.e. null pointer) is properly handled all the way
            {
                as2js::node::pointer_t empty;
                as2js::node_lock empty_lock(empty);
            }
            {
                as2js::node::pointer_t empty;
                as2js::node_lock empty_lock(empty);
                empty_lock.unlock();
            }

            // apply some tests
            CATCH_REQUIRE(root->get_children_size() == 2);
            CATCH_REQUIRE(directive_list_a->get_children_size() == 1);
            CATCH_REQUIRE(directive_list_a->get_child(0) == assignment);
            CATCH_REQUIRE(directive_list_b->get_children_size() == 0);
            CATCH_REQUIRE(assignment->get_children_size() == 2);
            CATCH_REQUIRE(assignment->get_child(0) == identifier_a);
            CATCH_REQUIRE(assignment->get_child(1) == power);
            CATCH_REQUIRE(identifier_a->get_children_size() == 0);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal);
            CATCH_REQUIRE(member->get_children_size() == 2);
            CATCH_REQUIRE(member->get_child(0) == identifier_math);
            CATCH_REQUIRE(member->get_child(1) == identifier_e);
            CATCH_REQUIRE(identifier_math->get_children_size() == 0);
            CATCH_REQUIRE(identifier_e->get_children_size() == 0);
            CATCH_REQUIRE(literal->get_children_size() == 0);

            CATCH_REQUIRE(root->has_side_effects());
            CATCH_REQUIRE(directive_list_a->has_side_effects());
            CATCH_REQUIRE(!directive_list_b->has_side_effects());
            CATCH_REQUIRE(!power->has_side_effects());

            // now move the assignment from a to b
            assignment->set_parent(directive_list_b);

            CATCH_REQUIRE(root->get_children_size() == 2);
            CATCH_REQUIRE(directive_list_a->get_children_size() == 0);
            CATCH_REQUIRE(directive_list_b->get_children_size() == 1);
            CATCH_REQUIRE(directive_list_b->get_child(0) == assignment);
            CATCH_REQUIRE(assignment->get_children_size() == 2);
            CATCH_REQUIRE(assignment->get_child(0) == identifier_a);
            CATCH_REQUIRE(assignment->get_child(1) == power);
            CATCH_REQUIRE(identifier_a->get_children_size() == 0);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal);
            CATCH_REQUIRE(member->get_children_size() == 2);
            CATCH_REQUIRE(member->get_child(0) == identifier_math);
            CATCH_REQUIRE(member->get_child(1) == identifier_e);
            CATCH_REQUIRE(identifier_math->get_children_size() == 0);
            CATCH_REQUIRE(identifier_e->get_children_size() == 0);
            CATCH_REQUIRE(literal->get_children_size() == 0);

            power->delete_child(0);
            CATCH_REQUIRE(power->get_children_size() == 1);
            CATCH_REQUIRE(power->get_child(0) == literal);

            power->insert_child(0, member);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal);

            CATCH_REQUIRE(root->has_side_effects());
            CATCH_REQUIRE(!directive_list_a->has_side_effects());
            CATCH_REQUIRE(directive_list_b->has_side_effects());
            CATCH_REQUIRE(!power->has_side_effects());

            // create a new literal
            as2js::node::pointer_t literal_seven(std::make_shared<tracked_node>(as2js::node_t::NODE_FLOATING_POINT, counter));
            as2js::floating_point f7;
            f7.set(-7.33312);
            literal_seven->set_floating_point(f7);
            directive_list_a->append_child(literal_seven);
            CATCH_REQUIRE(directive_list_a->get_children_size() == 1);
            CATCH_REQUIRE(directive_list_a->get_child(0) == literal_seven);

            // now replace the old literal with the new one (i.e. a full move actually)
            power->set_child(1, literal_seven);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal_seven);

            // replace with itself should work just fine
            power->set_child(0, member);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal_seven);

            // verify that a replace fails if the node pointer is null
            as2js::node::pointer_t null_pointer;
            CATCH_REQUIRE_THROWS_MATCHES(
                  literal_seven->replace_with(null_pointer)
                , as2js::invalid_data
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot replace with a node if its pointer is null."));

            // replace with the old literal
            literal_seven->replace_with(literal);
            CATCH_REQUIRE(power->get_children_size() == 2);
            CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(1) == literal);

            // verify that a node without a parent generates an exception
            CATCH_REQUIRE_THROWS_MATCHES(
                  root->replace_with(literal_seven)
                , as2js::no_parent
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: trying to replace a node which has no parent."));

            // verify that we cannot get an offset on a node without a parent
            CATCH_REQUIRE_THROWS_MATCHES(
                  root->get_offset()
                , as2js::no_parent
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: get_offset() only works against nodes that have a parent."));

            // check out our tree textually
            //std::cout << std::endl << *root << std::endl;

            // finally mark a node as unknown and call clean_tree()
            CATCH_REQUIRE(!member->is_locked());
            {
                snapdev::ostream_to_buf<char> out(std::cerr);
                as2js::node_lock lock(member);
                CATCH_REQUIRE(member->is_locked());
                CATCH_REQUIRE_THROWS_MATCHES(
                      member->to_unknown()
                    , as2js::locked_node
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: trying to modify a locked node."));
                CATCH_REQUIRE(member->get_type() == as2js::node_t::NODE_MEMBER);
                CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
            }
            CATCH_REQUIRE(!member->is_locked());
            // try too many unlock!
            CATCH_REQUIRE_THROWS_MATCHES(
                  member->unlock()
                , as2js::internal_error
                , Catch::Matchers::ExceptionMessage(
                          "internal_error: somehow the node::unlock() function was called when the lock counter is zero."));
            member->to_unknown();
            CATCH_REQUIRE(member->get_type() == as2js::node_t::NODE_UNKNOWN);
            {
                snapdev::ostream_to_buf<char> out(std::cerr);
                as2js::node_lock lock(member);
                    CATCH_REQUIRE_THROWS_MATCHES(
                          root->clean_tree()
                        , as2js::locked_node
                        , Catch::Matchers::ExceptionMessage(
                                  "as2js_exception: trying to modify a locked node."));
                CATCH_REQUIRE(member->get_type() == as2js::node_t::NODE_UNKNOWN);
                CATCH_REQUIRE(member->get_parent());
                CATCH_REQUIRE(out.str().substr(0, 65) == "error: The following node is locked and thus cannot be modified:\n");
            }
            root->clean_tree();

            // check that the tree looks as expected
            CATCH_REQUIRE(root->get_children_size() == 2);
            CATCH_REQUIRE(directive_list_a->get_children_size() == 0);
            CATCH_REQUIRE(directive_list_b->get_children_size() == 1);
            CATCH_REQUIRE(directive_list_b->get_child(0) == assignment);
            CATCH_REQUIRE(assignment->get_children_size() == 2);
            CATCH_REQUIRE(assignment->get_child(0) == identifier_a);
            CATCH_REQUIRE(assignment->get_child(1) == power);
            CATCH_REQUIRE(identifier_a->get_children_size() == 0);
            CATCH_REQUIRE(power->get_children_size() == 1);
            // Although member is not in the tree anymore, its children
            // are still there as expected (because we hold a smart pointers
            // to all of that)
            //CATCH_REQUIRE(power->get_child(0) == member);
            CATCH_REQUIRE(power->get_child(0) == literal);
            CATCH_REQUIRE(!member->get_parent());
            CATCH_REQUIRE(member->get_children_size() == 2);
            CATCH_REQUIRE(member->get_child(0) == identifier_math);
            CATCH_REQUIRE(member->get_child(1) == identifier_e);
            CATCH_REQUIRE(identifier_math->get_children_size() == 0);
            CATCH_REQUIRE(identifier_math->get_parent() == member);
            CATCH_REQUIRE(identifier_e->get_children_size() == 0);
            CATCH_REQUIRE(identifier_e->get_parent() == member);
            CATCH_REQUIRE(literal->get_children_size() == 0);
        }

        // did we deleted as many nodes as we created?
        //
        CATCH_REQUIRE(counter == 0);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_lock", "[node][lock]")
{
    CATCH_START_SECTION("node_lock: verify lock counter (proper lock/unlock)")
    {
        as2js::node::pointer_t n(std::make_shared<as2js::node>(as2js::node_t::NODE_CLASS));
        CATCH_REQUIRE_FALSE(n->is_locked());
        as2js::node_lock lock(n);
        CATCH_REQUIRE(n->is_locked());

        // we get a double unlock error in the ~node_lock() function
        // but have a catch() which ignores the fact...
        //
        // i.e. the correct way would be to instead do:
        //
        //        lock.unlock();
        //
        n->unlock();
        CATCH_REQUIRE_FALSE(n->is_locked());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("node_lock: verify lock counter (missing unlock)")
    {
        // manual lock, no unlock before deletion...
        // that generates an std::terminate so we use an external test
        // and verify that it fails with an abort() when we do not have
        // the unlock
        //
        std::string cmd(SNAP_CATCH2_NAMESPACE::g_binary_dir());
        cmd += "/tests/locked-node";
std::cerr << "--- system(\"" << cmd << "\"); ..." << std::endl;
        //int r(system(cmd.c_str()));
        int r(quick_exec(cmd));
        CATCH_REQUIRE(r == 0);
        cmd += " -u";
std::cerr << "--- system(\"" << cmd << "\"); ..." << std::endl;
        //r = system(cmd.c_str());
        r = quick_exec(cmd);
        CATCH_REQUIRE(r == 1);
        //if(!WIFEXITED(r))
        //{
        //    CATCH_REQUIRE("not exited?" == nullptr);
        //}
        //else if(WIFSIGNALED(r))
        //{
        //    CATCH_REQUIRE("signaled?" == nullptr);
        //}
        //else
        //{
        //    int const exit_code(WEXITSTATUS(r));
        //    if(exit_code != SIGABRT + 128)
        //    {
        //        CATCH_REQUIRE(exit_code != SIGABRT + 128);
        //    }
        //}
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_parameter", "[node][parameter]")
{
    CATCH_START_SECTION("node_parameter: verify node parameters")
    {
        as2js::node::pointer_t match(std::make_shared<as2js::node>(as2js::node_t::NODE_PARAM_MATCH));

        CATCH_REQUIRE(match->get_param_size() == 0);

        // zero is not acceptable
        CATCH_REQUIRE_THROWS_MATCHES(
              match->set_param_size(0)
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: set_param_size() was called with a size of zero."));

        match->set_param_size(5);
        CATCH_REQUIRE(match->get_param_size() == 5);

        // cannot change the size once set
        CATCH_REQUIRE_THROWS_MATCHES(
              match->set_param_size(10)
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: set_param_size() called twice."));

        CATCH_REQUIRE(match->get_param_size() == 5);

        // first set the depth, try with an out of range index too
        for(int i(-5); i < 0; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_depth(i, rand())
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_depth() called with an index out of range."));
        }
        ssize_t depths[5];
        for(int i(0); i < 5; ++i)
        {
            depths[i] = rand();
            match->set_param_depth(i, depths[i]);
        }
        for(int i(5); i <= 10; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_depth(i, rand())
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_depth() called with an index out of range."));
        }

        // now test that what we saved can be read back, also with some out of range
        for(int i(-5); i < 0; ++i)
        {
                CATCH_REQUIRE_THROWS_MATCHES(
                      match->get_param_depth(i)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_param_depth() called with an out of range index."));
        }
        for(int i(0); i < 5; ++i)
        {
            CATCH_REQUIRE(match->get_param_depth(i) == depths[i]);
        }
        for(int i(5); i < 10; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->get_param_depth(i)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: get_param_depth() called with an out of range index."));
        }

        // second set the index, try with an out of range index too
        for(int i(-5); i < 0; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_index(i, rand() % 5)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with one or both indexes out of range."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_index(i, rand())
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with one or both indexes out of range."));
        }
        size_t index[5];
        for(int i(0); i < 5; ++i)
        {
            index[i] = rand() % 5;
            match->set_param_index(i, index[i]);

            // if 'j' is invalid, then just throw
            // and do not change the valid value
            for(int k(0); k < 10; ++k)
            {
                int j(0);
                do
                {
                    j = rand();
                }
                while(j >= 0 && j <= 5);
                CATCH_REQUIRE_THROWS_MATCHES(
                      match->set_param_index(i, j)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: set_param_index() called with one or both indexes out of range."));
            }
        }
        for(int i(5); i <= 10; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_index(i, rand() % 5)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with one or both indexes out of range."));
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->set_param_index(i, rand())
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with one or both indexes out of range."));
        }

        // now test that what we saved can be read back, also with some out of range
        for(int i(-5); i < 0; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->get_param_index(i)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with an index out of range."));
        }
        for(int i(0); i < 5; ++i)
        {
            CATCH_REQUIRE(match->get_param_index(i) == index[i]);
        }
        for(int i(5); i < 10; ++i)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  match->get_param_index(i)
                , as2js::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: set_param_index() called with an index out of range."));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_position", "[node][position]")
{
    CATCH_START_SECTION("node_position: verify position computation")
    {
        as2js::position pos;
        pos.set_filename("file.js");
        int total_line(1);
        for(int page(1); page < 10; ++page)
        {
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

                // create any valid type of node
                size_t const idx(rand() % g_node_types_size);
                as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[idx].f_type));

                // set our current position in there
                node->set_position(pos);

                // verify that the node position is equal to ours
                as2js::position const& node_pos(node->get_position());
                CATCH_REQUIRE(node_pos.get_page() == page);
                CATCH_REQUIRE(node_pos.get_page_line() == page_line);
                CATCH_REQUIRE(node_pos.get_paragraph() == paragraph);
                CATCH_REQUIRE(node_pos.get_line() == total_line);

                std::stringstream node_pos_str;
                node_pos_str << node_pos;
                std::stringstream node_test_str;
                node_test_str << "file.js:" << total_line << ":";
                CATCH_REQUIRE(node_pos_str.str() == node_test_str.str());

                // create a replacement now
                size_t const idx_replacement(rand() % g_node_types_size);
                as2js::node::pointer_t replacement(node->create_replacement(g_node_types[idx_replacement].f_type));

                // verify that the replacement position is equal to ours
                // (and thus the node's)
                as2js::position const& replacement_pos(node->get_position());
                CATCH_REQUIRE(replacement_pos.get_page() == page);
                CATCH_REQUIRE(replacement_pos.get_page_line() == page_line);
                CATCH_REQUIRE(replacement_pos.get_paragraph() == paragraph);
                CATCH_REQUIRE(replacement_pos.get_line() == total_line);

                std::stringstream replacement_pos_str;
                replacement_pos_str << replacement_pos;
                std::stringstream replacement_test_str;
                replacement_test_str << "file.js:" << total_line << ":";
                CATCH_REQUIRE(replacement_pos_str.str() == replacement_test_str.str());

                // verify that the node position has not changed
                as2js::position const& node_pos2(node->get_position());
                CATCH_REQUIRE(node_pos2.get_page() == page);
                CATCH_REQUIRE(node_pos2.get_page_line() == page_line);
                CATCH_REQUIRE(node_pos2.get_paragraph() == paragraph);
                CATCH_REQUIRE(node_pos2.get_line() == total_line);

                std::stringstream node_pos2_str;
                node_pos2_str << node_pos2;
                std::stringstream node_test2_str;
                node_test2_str << "file.js:" << total_line << ":";
                CATCH_REQUIRE(node_pos2_str.str() == node_test2_str.str());

                // go to the next line, paragraph, etc.
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


CATCH_TEST_CASE("node_links", "[node][link]")
{
    CATCH_START_SECTION("node_links: verify node linking")
    {
        for(int i(0); i < 10; ++i)
        {
            // create any valid type of node
            size_t const idx_node(rand() % g_node_types_size);
            as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[idx_node].f_type));

            size_t const idx_bad_link(rand() % g_node_types_size);
            as2js::node::pointer_t bad_link(std::make_shared<as2js::node>(g_node_types[idx_bad_link].f_type));

            // check various links

            { // instance
                as2js::node::pointer_t link(std::make_shared<as2js::node>(as2js::node_t::NODE_CLASS));
                node->set_instance(link);
                CATCH_REQUIRE(node->get_instance() == link);

                as2js::node::pointer_t other_link(std::make_shared<as2js::node>(as2js::node_t::NODE_CLASS));
                node->set_instance(other_link);
                CATCH_REQUIRE(node->get_instance() == other_link);
            }
            CATCH_REQUIRE(!node->get_instance()); // weak pointer, reset to null

            { // type
                as2js::node::pointer_t link(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
                node->set_type_node(link);
                CATCH_REQUIRE(node->get_type_node() == link);

                as2js::node::pointer_t other_link(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
                node->set_type_node(other_link);
                CATCH_REQUIRE(node->get_type_node() == other_link);
            }
            CATCH_REQUIRE(!node->get_type_node()); // weak pointer, reset to null

            { // attributes
                as2js::node::pointer_t link(std::make_shared<as2js::node>(as2js::node_t::NODE_ATTRIBUTES));
                node->set_attribute_node(link);
                CATCH_REQUIRE(node->get_attribute_node() == link);

                as2js::node::pointer_t other_link(std::make_shared<as2js::node>(as2js::node_t::NODE_ATTRIBUTES));
                node->set_attribute_node(other_link);
                CATCH_REQUIRE(node->get_attribute_node() == other_link);
            }
            CATCH_REQUIRE(node->get_attribute_node()); // NOT a weak pointer for attributes

            { // goto exit
                as2js::node::pointer_t link(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));
                node->set_goto_exit(link);
                CATCH_REQUIRE(node->get_goto_exit() == link);

                as2js::node::pointer_t other_link(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));
                node->set_goto_exit(other_link);
                CATCH_REQUIRE(node->get_goto_exit() == other_link);
            }
            CATCH_REQUIRE(!node->get_goto_exit()); // weak pointer, reset to null

            { // goto enter
                as2js::node::pointer_t link(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));
                node->set_goto_enter(link);
                CATCH_REQUIRE(node->get_goto_enter() == link);

                as2js::node::pointer_t other_link(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));
                node->set_goto_enter(other_link);
                CATCH_REQUIRE(node->get_goto_enter() == other_link);
            }
            CATCH_REQUIRE(!node->get_goto_enter()); // weak pointer, reset to null
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_variable", "[node][variable]")
{
    CATCH_START_SECTION("node_variable: verify variables")
    {
        for(int i(0); i < 10; ++i)
        {
            // create any valid type of node
            size_t const idx_node(rand() % g_node_types_size);
            as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[idx_node].f_type));

            // create a node that is not a NODE_VARIABLE
            size_t idx_bad_link;
            do
            {
                idx_bad_link = rand() % g_node_types_size;
            }
            while(g_node_types[idx_bad_link].f_type == as2js::node_t::NODE_VARIABLE);
            as2js::node::pointer_t not_variable(std::make_shared<as2js::node>(g_node_types[idx_bad_link].f_type));
            CATCH_REQUIRE_THROWS_MATCHES(
                  node->add_variable(not_variable)
                , as2js::incompatible_node_type
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: the variable parameter of the add_variable() function must be a \"NODE_VARIABLE\"."));

            // add 10 valid variables
            as2js::node::pointer_t variables[10];
            for(size_t j(0); j < 10; ++j)
            {
                CATCH_REQUIRE(node->get_variable_size() == j);

                variables[j].reset(new as2js::node(as2js::node_t::NODE_VARIABLE));
                node->add_variable(variables[j]);
            }
            CATCH_REQUIRE(node->get_variable_size() == 10);

            // try with offsets that are too small
            for(int j(-10); j < 0; ++j)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_variable(j)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_variable() called with an out of range index."));
            }

            // then verify that the variables are indeed valid
            for(int j(0); j < 10; ++j)
            {
                CATCH_REQUIRE(node->get_variable(j) == variables[j]);
            }

            // try with offsets that are too large
            for(int j(10); j <= 20; ++j)
            {
                CATCH_REQUIRE_THROWS_MATCHES(
                      node->get_variable(j)
                    , as2js::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: get_variable() called with an out of range index."));
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_label", "[node][label]")
{
    CATCH_START_SECTION("node_label: verify labelling")
    {
        for(int i(0); i < 10; ++i)
        {
            // create a NODE_FUNCTION
            as2js::node::pointer_t function(std::make_shared<as2js::node>(as2js::node_t::NODE_FUNCTION));

            // create a node that is not a NODE_LABEL
            size_t idx_bad_label;
            do
            {
                idx_bad_label = rand() % g_node_types_size;
            }
            while(g_node_types[idx_bad_label].f_type == as2js::node_t::NODE_LABEL);
            as2js::node::pointer_t not_label(std::make_shared<as2js::node>(g_node_types[idx_bad_label].f_type));
            CATCH_REQUIRE_THROWS_MATCHES(
                  function->add_label(not_label)
                , as2js::incompatible_node_type
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: invalid type of node to call add_label() with."));

            for(int j(0); j < 10; ++j)
            {
                // create a node that is not a NODE_LABEL
                as2js::node::pointer_t label(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));

                // create a node that is not a NODE_FUNCTION
                size_t idx_bad_function;
                do
                {
                    idx_bad_function = rand() % g_node_types_size;
                }
                while(g_node_types[idx_bad_function].f_type == as2js::node_t::NODE_FUNCTION);
                as2js::node::pointer_t not_function(std::make_shared<as2js::node>(g_node_types[idx_bad_function].f_type));
                CATCH_REQUIRE_THROWS_MATCHES(
                      not_function->add_label(label)
                    , as2js::incompatible_node_type
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: invalid type of node to call add_label() with."));

                // labels need to have a name
                CATCH_REQUIRE_THROWS_MATCHES(
                      function->add_label(label)
                    , as2js::incompatible_node_data
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a label without a valid name cannot be added to a function."));

                // save the label with a name
                std::string label_name("label" + std::to_string(j));
                label->set_string(label_name);
                function->add_label(label);

                // trying to add two labels (or the same) with the same name err
                CATCH_REQUIRE_THROWS_MATCHES(
                      function->add_label(label)
                    , as2js::already_defined
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a label with the same name is already defined in this function."));

                // verify that we can find that label
                CATCH_REQUIRE(function->find_label(label_name) == label);
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_attribute", "[node][attribute]")
{
    CATCH_START_SECTION("node_attribute: verify setting attributes on nodes")
    {
        for(int i(0); i < 10; ++i)
        {
            // create a node that is not a NODE_PROGRAM
            // (i.e. a node that accepts all attributes)
            size_t idx_node;
            do
            {
                idx_node = rand() % g_node_types_size;
            }
            while(g_node_types[idx_node].f_type == as2js::node_t::NODE_PROGRAM);
            as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[idx_node].f_type));

            // need to test all combinatorial cases...
            for(size_t j(0); j < g_groups_of_attributes_size; ++j)
            {
                // go through the list of attributes that generate conflicts
                for(as2js::attribute_t const *attr_list(g_groups_of_attributes[j].f_attributes);
                                             *attr_list != as2js::attribute_t::NODE_ATTR_max;
                                             ++attr_list)
                {
                    if(*attr_list == as2js::attribute_t::NODE_ATTR_TYPE)
                    {
                        switch(node->get_type())
                        {
                        case as2js::node_t::NODE_ADD:
                        case as2js::node_t::NODE_ARRAY:
                        case as2js::node_t::NODE_ARRAY_LITERAL:
                        case as2js::node_t::NODE_AS:
                        case as2js::node_t::NODE_ASSIGNMENT:
                        case as2js::node_t::NODE_ASSIGNMENT_ADD:
                        case as2js::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                        case as2js::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                        case as2js::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                        case as2js::node_t::NODE_ASSIGNMENT_DIVIDE:
                        case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                        case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                        case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                        case as2js::node_t::NODE_ASSIGNMENT_MAXIMUM:
                        case as2js::node_t::NODE_ASSIGNMENT_MINIMUM:
                        case as2js::node_t::NODE_ASSIGNMENT_MODULO:
                        case as2js::node_t::NODE_ASSIGNMENT_MULTIPLY:
                        case as2js::node_t::NODE_ASSIGNMENT_POWER:
                        case as2js::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                        case as2js::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                        case as2js::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                        case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                        case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                        case as2js::node_t::NODE_ASSIGNMENT_SUBTRACT:
                        case as2js::node_t::NODE_BITWISE_AND:
                        case as2js::node_t::NODE_BITWISE_NOT:
                        case as2js::node_t::NODE_BITWISE_OR:
                        case as2js::node_t::NODE_BITWISE_XOR:
                        case as2js::node_t::NODE_CALL:
                        case as2js::node_t::NODE_CONDITIONAL:
                        case as2js::node_t::NODE_DECREMENT:
                        case as2js::node_t::NODE_DELETE:
                        case as2js::node_t::NODE_DIVIDE:
                        case as2js::node_t::NODE_EQUAL:
                        case as2js::node_t::NODE_FALSE:
                        case as2js::node_t::NODE_FLOATING_POINT:
                        case as2js::node_t::NODE_FUNCTION:
                        case as2js::node_t::NODE_GREATER:
                        case as2js::node_t::NODE_GREATER_EQUAL:
                        case as2js::node_t::NODE_IDENTIFIER:
                        case as2js::node_t::NODE_IN:
                        case as2js::node_t::NODE_INCREMENT:
                        case as2js::node_t::NODE_INSTANCEOF:
                        case as2js::node_t::NODE_INTEGER:
                        case as2js::node_t::NODE_IS:
                        case as2js::node_t::NODE_LESS:
                        case as2js::node_t::NODE_LESS_EQUAL:
                        case as2js::node_t::NODE_LIST:
                        case as2js::node_t::NODE_LOGICAL_AND:
                        case as2js::node_t::NODE_LOGICAL_NOT:
                        case as2js::node_t::NODE_LOGICAL_OR:
                        case as2js::node_t::NODE_LOGICAL_XOR:
                        case as2js::node_t::NODE_MATCH:
                        case as2js::node_t::NODE_MAXIMUM:
                        case as2js::node_t::NODE_MEMBER:
                        case as2js::node_t::NODE_MINIMUM:
                        case as2js::node_t::NODE_MODULO:
                        case as2js::node_t::NODE_MULTIPLY:
                        case as2js::node_t::NODE_NAME:
                        case as2js::node_t::NODE_NEW:
                        case as2js::node_t::NODE_NOT_EQUAL:
                        case as2js::node_t::NODE_NULL:
                        case as2js::node_t::NODE_OBJECT_LITERAL:
                        case as2js::node_t::NODE_POST_DECREMENT:
                        case as2js::node_t::NODE_POST_INCREMENT:
                        case as2js::node_t::NODE_POWER:
                        case as2js::node_t::NODE_PRIVATE:
                        case as2js::node_t::NODE_PUBLIC:
                        case as2js::node_t::NODE_RANGE:
                        case as2js::node_t::NODE_ROTATE_LEFT:
                        case as2js::node_t::NODE_ROTATE_RIGHT:
                        case as2js::node_t::NODE_SCOPE:
                        case as2js::node_t::NODE_SHIFT_LEFT:
                        case as2js::node_t::NODE_SHIFT_RIGHT:
                        case as2js::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                        case as2js::node_t::NODE_STRICTLY_EQUAL:
                        case as2js::node_t::NODE_STRICTLY_NOT_EQUAL:
                        case as2js::node_t::NODE_STRING:
                        case as2js::node_t::NODE_SUBTRACT:
                        case as2js::node_t::NODE_SUPER:
                        case as2js::node_t::NODE_THIS:
                        case as2js::node_t::NODE_TRUE:
                        case as2js::node_t::NODE_TYPEOF:
                        case as2js::node_t::NODE_UNDEFINED:
                        case as2js::node_t::NODE_VIDENTIFIER:
                        case as2js::node_t::NODE_VOID:
                            break;;

                        default:
                            // with any other types we would get an error
                            continue;

                        }
                    }

                    as2js::attribute_set_t set;
                    CATCH_REQUIRE(node->compare_all_attributes(set));

                    // set that one attribute first
                    node->set_attribute(*attr_list, true);

                    CATCH_REQUIRE(!node->compare_all_attributes(set));
                    set[static_cast<int>(*attr_list)] = true;
                    CATCH_REQUIRE(node->compare_all_attributes(set));

                    std::string str(g_attribute_names[static_cast<int>(*attr_list)]);

                    // test against all the other attributes
                    for(int a(0); a < static_cast<int>(as2js::attribute_t::NODE_ATTR_max); ++a)
                    {
                        // no need to test with itself, we do that earlier
                        if(static_cast<as2js::attribute_t>(a) == *attr_list)
                        {
                            continue;
                        }

                        if(static_cast<as2js::attribute_t>(a) == as2js::attribute_t::NODE_ATTR_TYPE)
                        {
                            switch(node->get_type())
                            {
                            case as2js::node_t::NODE_ADD:
                            case as2js::node_t::NODE_ARRAY:
                            case as2js::node_t::NODE_ARRAY_LITERAL:
                            case as2js::node_t::NODE_AS:
                            case as2js::node_t::NODE_ASSIGNMENT:
                            case as2js::node_t::NODE_ASSIGNMENT_ADD:
                            case as2js::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                            case as2js::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                            case as2js::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                            case as2js::node_t::NODE_ASSIGNMENT_DIVIDE:
                            case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                            case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                            case as2js::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                            case as2js::node_t::NODE_ASSIGNMENT_MAXIMUM:
                            case as2js::node_t::NODE_ASSIGNMENT_MINIMUM:
                            case as2js::node_t::NODE_ASSIGNMENT_MODULO:
                            case as2js::node_t::NODE_ASSIGNMENT_MULTIPLY:
                            case as2js::node_t::NODE_ASSIGNMENT_POWER:
                            case as2js::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                            case as2js::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                            case as2js::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                            case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                            case as2js::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                            case as2js::node_t::NODE_ASSIGNMENT_SUBTRACT:
                            case as2js::node_t::NODE_BITWISE_AND:
                            case as2js::node_t::NODE_BITWISE_NOT:
                            case as2js::node_t::NODE_BITWISE_OR:
                            case as2js::node_t::NODE_BITWISE_XOR:
                            case as2js::node_t::NODE_CALL:
                            case as2js::node_t::NODE_CONDITIONAL:
                            case as2js::node_t::NODE_DECREMENT:
                            case as2js::node_t::NODE_DELETE:
                            case as2js::node_t::NODE_DIVIDE:
                            case as2js::node_t::NODE_EQUAL:
                            case as2js::node_t::NODE_FALSE:
                            case as2js::node_t::NODE_FLOATING_POINT:
                            case as2js::node_t::NODE_FUNCTION:
                            case as2js::node_t::NODE_GREATER:
                            case as2js::node_t::NODE_GREATER_EQUAL:
                            case as2js::node_t::NODE_IDENTIFIER:
                            case as2js::node_t::NODE_IN:
                            case as2js::node_t::NODE_INCREMENT:
                            case as2js::node_t::NODE_INSTANCEOF:
                            case as2js::node_t::NODE_INTEGER:
                            case as2js::node_t::NODE_IS:
                            case as2js::node_t::NODE_LESS:
                            case as2js::node_t::NODE_LESS_EQUAL:
                            case as2js::node_t::NODE_LIST:
                            case as2js::node_t::NODE_LOGICAL_AND:
                            case as2js::node_t::NODE_LOGICAL_NOT:
                            case as2js::node_t::NODE_LOGICAL_OR:
                            case as2js::node_t::NODE_LOGICAL_XOR:
                            case as2js::node_t::NODE_MATCH:
                            case as2js::node_t::NODE_MAXIMUM:
                            case as2js::node_t::NODE_MEMBER:
                            case as2js::node_t::NODE_MINIMUM:
                            case as2js::node_t::NODE_MODULO:
                            case as2js::node_t::NODE_MULTIPLY:
                            case as2js::node_t::NODE_NAME:
                            case as2js::node_t::NODE_NEW:
                            case as2js::node_t::NODE_NOT_EQUAL:
                            case as2js::node_t::NODE_NULL:
                            case as2js::node_t::NODE_OBJECT_LITERAL:
                            case as2js::node_t::NODE_POST_DECREMENT:
                            case as2js::node_t::NODE_POST_INCREMENT:
                            case as2js::node_t::NODE_POWER:
                            case as2js::node_t::NODE_PRIVATE:
                            case as2js::node_t::NODE_PUBLIC:
                            case as2js::node_t::NODE_RANGE:
                            case as2js::node_t::NODE_ROTATE_LEFT:
                            case as2js::node_t::NODE_ROTATE_RIGHT:
                            case as2js::node_t::NODE_SCOPE:
                            case as2js::node_t::NODE_SHIFT_LEFT:
                            case as2js::node_t::NODE_SHIFT_RIGHT:
                            case as2js::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                            case as2js::node_t::NODE_STRICTLY_EQUAL:
                            case as2js::node_t::NODE_STRICTLY_NOT_EQUAL:
                            case as2js::node_t::NODE_STRING:
                            case as2js::node_t::NODE_SUBTRACT:
                            case as2js::node_t::NODE_SUPER:
                            case as2js::node_t::NODE_THIS:
                            case as2js::node_t::NODE_TRUE:
                            case as2js::node_t::NODE_TYPEOF:
                            case as2js::node_t::NODE_UNDEFINED:
                            case as2js::node_t::NODE_VIDENTIFIER:
                            case as2js::node_t::NODE_VOID:
                                break;;

                            default:
                                // with any other types we would get an error
                                continue;

                            }
                        }

                        // is attribute 'a' in conflict with attribute '*attr_list'?
                        if(in_conflict(j, *attr_list, static_cast<as2js::attribute_t>(a)))
                        {
                            test_callback c;
                            c.f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
                            c.f_expected_error_code = as2js::err_code_t::AS_ERR_INVALID_ATTRIBUTES;
                            c.f_expected_pos.set_filename("unknown-file");
                            c.f_expected_pos.set_function("unknown-func");
                            c.f_expected_message = "Attributes " + std::string(g_groups_of_attributes[j].f_names) + " are mutually exclusive. Only one of them can be used.";

    //std::cerr << "next conflict: " << c.f_expected_message << "\n";
                            // if in conflict, trying to set the flag generates
                            // an error
                            CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(a)));
                            node->set_attribute(static_cast<as2js::attribute_t>(a), true);
                            // the set_attribute() did not change the attribute because it is
                            // in conflict with another attribute which is set at this time...
                            CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(a)));
                        }
                        else
                        {
                            // before we set it, always false
    //std::cerr << "next valid attr: " << static_cast<int>(*attr_list) << " against " << a << "\n";
                            CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(a)));
                            node->set_attribute(static_cast<as2js::attribute_t>(a), true);
                            CATCH_REQUIRE(node->get_attribute(static_cast<as2js::attribute_t>(a)));
                            node->set_attribute(static_cast<as2js::attribute_t>(a), false);
                            CATCH_REQUIRE(!node->get_attribute(static_cast<as2js::attribute_t>(a)));
                        }
                    }

                    // we are done with that loop, restore the attribute to the default
                    node->set_attribute(*attr_list, false);
                }
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_attribute_tree", "[node][attribute][tree]")
{
    CATCH_START_SECTION("node_attribute_tree: check attributes in a tree")
    {
        // here we create a tree of nodes that we can then test with various
        // attributes using the set_attribute_tree() function
        //
        // the tree is very specific to make it easier to handle the test; there
        // is no need to test every single case (every attribute) since we do that
        // in other tests; this test is to make sure the tree is followed as
        // expected (all leaves are hit)
        //
        as2js::node::pointer_t root(std::make_shared<as2js::node>(as2js::node_t::NODE_ROOT));

        // block
        as2js::node::pointer_t directive_list(std::make_shared<as2js::node>(as2js::node_t::NODE_DIRECTIVE_LIST));
        root->append_child(directive_list);

        // { for( ...
        as2js::node::pointer_t for_loop(std::make_shared<as2js::node>(as2js::node_t::NODE_FOR));
        directive_list->append_child(for_loop);

        // { for( ... , ...
        as2js::node::pointer_t init(std::make_shared<as2js::node>(as2js::node_t::NODE_LIST));
        for_loop->append_child(init);

        as2js::node::pointer_t var1(std::make_shared<as2js::node>(as2js::node_t::NODE_VAR));
        init->append_child(var1);

        as2js::node::pointer_t variable1(std::make_shared<as2js::node>(as2js::node_t::NODE_VARIABLE));
        var1->append_child(variable1);

        // { for(i
        as2js::node::pointer_t variable_name1(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        variable_name1->set_string("i");
        variable1->append_child(variable_name1);

        // { for(i := 
        as2js::node::pointer_t value1(std::make_shared<as2js::node>(as2js::node_t::NODE_SET));
        variable1->append_child(value1);

        // { for(i := ... + ...
        as2js::node::pointer_t add1(std::make_shared<as2js::node>(as2js::node_t::NODE_ADD));
        value1->append_child(add1);

        // { for(i := a + ...
        as2js::node::pointer_t var_a1(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_a1->set_string("a");
        add1->append_child(var_a1);

        // { for(i := a + b
        as2js::node::pointer_t var_b1(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_b1->set_string("b");
        add1->append_child(var_b1);

        // { for(i := a + b, 
        as2js::node::pointer_t var2(std::make_shared<as2js::node>(as2js::node_t::NODE_VAR));
        init->append_child(var2);

        as2js::node::pointer_t variable2(std::make_shared<as2js::node>(as2js::node_t::NODE_VARIABLE));
        var2->append_child(variable2);

        // { for(i := a + b, j
        as2js::node::pointer_t variable_name2(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        variable_name2->set_string("j");
        variable2->append_child(variable_name2);

        // { for(i := a + b, j := 
        as2js::node::pointer_t value2(std::make_shared<as2js::node>(as2js::node_t::NODE_SET));
        variable2->append_child(value2);

        // { for(i := a + b, j := ... / ...
        as2js::node::pointer_t divide2(std::make_shared<as2js::node>(as2js::node_t::NODE_DIVIDE));
        value2->append_child(divide2);

        // { for(i := a + b, j := c / ...
        as2js::node::pointer_t var_a2(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_a2->set_string("c");
        divide2->append_child(var_a2);

        // { for(i := a + b, j := c / d
        as2js::node::pointer_t var_b2(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_b2->set_string("d");
        divide2->append_child(var_b2);

        // { for(i := a + b, j := c / d; ... < ...
        as2js::node::pointer_t less(std::make_shared<as2js::node>(as2js::node_t::NODE_LESS));
        for_loop->append_child(less);

        // { for(i := a + b, j := c / d; i < ...
        as2js::node::pointer_t var_i2(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_i2->set_string("i");
        less->append_child(var_i2);

        // { for(i := a + b, j := c / d; i < 100;
        as2js::node::pointer_t one_hunder(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
        one_hunder->set_integer(100);
        less->append_child(one_hunder);

        // { for(i := a + b, j := c / d; i < 100; ++...)
        as2js::node::pointer_t increment(std::make_shared<as2js::node>(as2js::node_t::NODE_INCREMENT));
        for_loop->append_child(increment);

        // { for(i := a + b, j := c / d; i < 100; ++i)
        as2js::node::pointer_t var_i3(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_i3->set_string("i");
        increment->append_child(var_i3);

        // { for(i := a + b, j := c / d; i < 100; ++i) { ... } }
        as2js::node::pointer_t block_list(std::make_shared<as2js::node>(as2js::node_t::NODE_DIRECTIVE_LIST));
        for_loop->append_child(block_list);

        // { for(i := a + b, j := c / d; i < 100; ++i) { ...(...); } }
        as2js::node::pointer_t func(std::make_shared<as2js::node>(as2js::node_t::NODE_CALL));
        block_list->append_child(func);

        // { for(i := a + b, j := c / d; i < 100; ++i) { func(...); } }
        as2js::node::pointer_t var_i4(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_i4->set_string("func");
        func->append_child(var_i4);

        // { for(i := a + b, j := c / d; i < 100; ++i) { func(...); } }
        as2js::node::pointer_t param_list(std::make_shared<as2js::node>(as2js::node_t::NODE_LIST));
        func->append_child(param_list);

        // { for(i := a + b, j := c / d; i < 100; ++i) { func(i, ...); } }
        as2js::node::pointer_t var_i5(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_i5->set_string("i");
        param_list->append_child(var_i5);

        // { for(i := a + b, j := c / d; i < 100; ++i) { func(i, j); } }
        as2js::node::pointer_t var_i6(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        var_i6->set_string("j");
        param_list->append_child(var_i6);

        // since we have a tree with parents we can test an invalid parent
        // which itself has a parent and get an error including the parent
        // information
        as2js::node::pointer_t test_list(std::make_shared<as2js::node>(as2js::node_t::NODE_DIRECTIVE_LIST));
        CATCH_REQUIRE_THROWS_MATCHES(
              test_list->set_parent(var_i5, 0)
            , as2js::incompatible_node_type
            , Catch::Matchers::ExceptionMessage(
                      "as2js_exception: invalid type: \"IDENTIFIER\" used as a parent node of child with type: \"DIRECTIVE_LIST\"."));

        // the DEFINED attribute applies to all types of nodes so it is easy to
        // use... (would the test benefit from testing other attributes?)
        root->set_attribute_tree(as2js::attribute_t::NODE_ATTR_DEFINED, true);
        CATCH_REQUIRE(root->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(directive_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(for_loop->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(init->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(variable1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(variable_name1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(value1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(add1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_a1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_b1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(variable2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(variable_name2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(value2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(divide2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_a2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_b2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(less->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_i2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(one_hunder->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(increment->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_i3->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(block_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(func->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_i4->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(param_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_i5->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(var_i6->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));

        // now test the clearing of the attribute
        root->set_attribute_tree(as2js::attribute_t::NODE_ATTR_DEFINED, false);
        CATCH_REQUIRE(!root->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!directive_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!for_loop->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!init->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!variable1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!variable_name1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!value1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!add1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_a1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_b1->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!variable2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!variable_name2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!value2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!divide2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_a2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_b2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!less->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_i2->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!one_hunder->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!increment->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_i3->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!block_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!func->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_i4->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!param_list->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_i5->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
        CATCH_REQUIRE(!var_i6->get_attribute(as2js::attribute_t::NODE_ATTR_DEFINED));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
