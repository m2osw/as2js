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
#include    <as2js/node.h>

#include    <as2js/message.h>



// self
//
#include    "catch_main.h"


// libutf8
//
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


// snapdev
//
#include    <snapdev/safe_stream.h>


// C++
//
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>




#include "catch_node_data.ci"




CATCH_TEST_CASE("node_display_all_types", "[node][display][type]")
{
    CATCH_START_SECTION("node_display_all_types: display all types")
    {
        // display all the different types available
        // this is "easy" so we do not have to test all
        // the potential flags, attributes, etc.
        //
        for(size_t i(0); i < g_node_types_size; ++i)
        {
            // get the type
            as2js::node_t const node_type(g_node_types[i].f_type);
            int node_type_int(static_cast<int>(node_type));

            // get the next type of node
            as2js::node::pointer_t node(std::make_shared<as2js::node>(node_type));

            // check the type
            CATCH_REQUIRE(node->get_type() == node_type);

            std::stringstream out;
            out << *node;

            // build the expected message
            std::stringstream expected;
            // indent is expected to be exactly 2 on startup and here we only have one line
            expected
                << node << ": " << std::setfill('0') << std::setw(2) << 2
                << std::setfill(' ') << '.' << std::setw(2) << "";
            if(node_type_int == -1)
            {
                // special case for negative number (EOF)
                //
                expected << "  -1";
            }
            else
            {
                expected << std::setw(4) << std::setfill('0') << node_type_int;
            }
            expected << std::setfill('\0') << ": " << g_node_types[i].f_name;

            // add the type as a character if it represents just one character
            if(node_type_int > ' ' && node_type_int < 0x7F)
            {
                expected << " = '" << static_cast<char>(node_type_int) << "'";
            }

            switch(node_type)
            {
            case as2js::node_t::NODE_BREAK:
            case as2js::node_t::NODE_CLASS:
            case as2js::node_t::NODE_CONTINUE:
            case as2js::node_t::NODE_ENUM:
            case as2js::node_t::NODE_FUNCTION:
            case as2js::node_t::NODE_GOTO:
            case as2js::node_t::NODE_IDENTIFIER:
            case as2js::node_t::NODE_IMPORT:
            case as2js::node_t::NODE_INTERFACE:
            case as2js::node_t::NODE_LABEL:
            case as2js::node_t::NODE_NAMESPACE:
            case as2js::node_t::NODE_PACKAGE:
            case as2js::node_t::NODE_REGULAR_EXPRESSION:
            case as2js::node_t::NODE_STRING:
            case as2js::node_t::NODE_TEMPLATE:
            case as2js::node_t::NODE_TEMPLATE_HEAD:
            case as2js::node_t::NODE_TEMPLATE_MIDDLE:
            case as2js::node_t::NODE_TEMPLATE_TAIL:
            case as2js::node_t::NODE_VARIABLE:
            case as2js::node_t::NODE_VAR_ATTRIBUTES:
            case as2js::node_t::NODE_VIDENTIFIER:
                output_str(expected, node->get_string());
                break;

            case as2js::node_t::NODE_INTEGER:
                expected
                    << ": " << node->get_integer().get()
                    << ", 0x"
                    << std::hex
                    << std::setw(16)
                    << std::setfill('0')
                    << node->get_integer().get()
                    << std::dec
                    << std::setw(0)
                    << std::setfill('\0');
                break;

            case as2js::node_t::NODE_FLOATING_POINT:
                expected << ": " << node->get_floating_point().get();
                break;

            case as2js::node_t::NODE_PARAM:
                output_str(expected, node->get_string());
                expected << ":";
                break;

            case as2js::node_t::NODE_CALL:
            case as2js::node_t::NODE_CATCH:
            case as2js::node_t::NODE_DIRECTIVE_LIST:
            case as2js::node_t::NODE_FOR:
            case as2js::node_t::NODE_PARAM_MATCH:
            case as2js::node_t::NODE_SWITCH:
            case as2js::node_t::NODE_TYPE:
            case as2js::node_t::NODE_VAR:
                expected << ":";
                break;

            default:
                break;

            }
            expected << " (" << node->get_position() << ")" << std::endl;

//std::cerr << "output [" << out.str() << "]\n";
//std::cerr << "expected [" << expected.str() << "]\n";

            CATCH_REQUIRE(out.str() == expected.str());
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_display_unicode_string", "[node][display][unicode][string]")
{
    CATCH_START_SECTION("node_display_unicode_string: verify Unicode")
    {
        int got_all(0);
        for(std::size_t idx(0); idx < 100 || got_all != 7; ++idx)
        {
            // get a string node
            as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_STRING));

            // generate a random string
            //
            std::string s;
            for(int j(0); j < 256; ++j)
            {
                char32_t c(U'\0');
                do
                {
                    c = ((rand() << 16) ^ rand()) & 0x1FFFFF;
                }
                while(c == U'\0'                        // skip null char
                   || c > 0x10FFFF                      // too large (not unicode)
                   || (c >= 0xD800 && c <= 0xDFFF));    // surrogate
                if(c < 0x7F)
                {
                    got_all |= 1;
                    if(c == '\'')
                    {
                        got_all |= 4;
                    }
                }
                else
                {
                    got_all |= 2;
                }
                s += libutf8::to_u8string(c);
            }

            // save random string in node
            node->set_string(s);

            // display that now
            std::stringstream out;
            out << *node;

            // build the expected message
            std::stringstream expected;
            // indent is expected to be exactly 2 on startup and here we only have one line
            expected
                << node
                << ": "
                << std::setfill('0')
                << std::setw(2)
                << 2
                << std::setfill(' ')
                << '.'
                << std::setw(2)
                << ""
                << std::setw(4)
                << std::setfill('0')
                << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_STRING))
                << std::setfill('\0')
                << ": STRING";
            output_str(expected, s);
            expected << " (" << node->get_position() << ")" << std::endl;

//std::cerr << "output [" << out.str() << "]\n";
//std::cerr << "expected [" << expected.str() << "]\n";

            CATCH_REQUIRE(out.str() == expected.str());
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_display_flags", "[node][display][flags]")
{
    CATCH_START_SECTION("node_display_flags: verify flags get printed")
    {
        // go through all the node types
        //
        for(std::size_t i(0); i < g_node_types_size; ++i)
        {
            std::size_t max_flags(0);
            for(flags_per_node_t const *flags(g_node_types[i].f_node_flags);
                                        flags->f_flag != as2js::flag_t::NODE_FLAG_max;
                                        ++flags)
            {
                ++max_flags;
            }
            if(max_flags == 0)
            {
                // ignore types without flags, they are not interesting here
                //
                continue;
            }

            as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[i].f_type));

            if((g_node_types[i].f_flags & TEST_NODE_ACCEPT_STRING) != 0)
            {
                std::string const s(SNAP_CATCH2_NAMESPACE::random_string(0, 20, SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_UNICODE));
                node->set_string(s);
            }

            CATCH_REQUIRE(max_flags < sizeof(std::size_t) * 8);
            std::size_t const possibilities_max(1 << max_flags);
            for(std::size_t j(0); j < possibilities_max; ++j)
            {
                int pos(0);
                for(flags_per_node_t const *flags(g_node_types[i].f_node_flags);
                                            flags->f_flag != as2js::flag_t::NODE_FLAG_max;
                                            ++flags, ++pos)
                {
                    node->set_flag(flags->f_flag, ((1 << pos) & j) != 0);
                }

                // display that now
                //
                std::stringstream out;
                out << *node;

                // build the expected message
                //
                std::stringstream expected;

                // indent is expected to be exactly 2 on startup and here we only have one line
                //
                expected << node << ": " << std::setfill('0') << std::setw(2) << 2 << std::setfill(' ') << '.' << std::setw(2) << ""
                         << std::setw(4) << std::setfill('0') << static_cast<int>(g_node_types[i].f_type)
                         << std::setfill('\0') << ": " << g_node_types[i].f_name;

                switch(g_node_types[i].f_type)
                {
                case as2js::node_t::NODE_BREAK:
                case as2js::node_t::NODE_CLASS:
                case as2js::node_t::NODE_CONTINUE:
                case as2js::node_t::NODE_ENUM:
                case as2js::node_t::NODE_FUNCTION:
                case as2js::node_t::NODE_GOTO:
                case as2js::node_t::NODE_IDENTIFIER:
                case as2js::node_t::NODE_IMPORT:
                case as2js::node_t::NODE_INTERFACE:
                case as2js::node_t::NODE_LABEL:
                case as2js::node_t::NODE_NAMESPACE:
                case as2js::node_t::NODE_PACKAGE:
                case as2js::node_t::NODE_STRING:
                case as2js::node_t::NODE_VARIABLE:
                case as2js::node_t::NODE_VAR_ATTRIBUTES:
                case as2js::node_t::NODE_VIDENTIFIER:
                    output_str(expected, node->get_string());
                    break;

                case as2js::node_t::NODE_INTEGER:
                    expected
                        << ": "
                        << node->get_integer().get()
                        << ", 0x"
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << node->get_integer().get()
                        << std::dec
                        << std::setw(0)
                        << std::setfill('\0');
                    break;

                case as2js::node_t::NODE_FLOATING_POINT:
                    expected << ": " << node->get_floating_point().get();
                    break;

                case as2js::node_t::NODE_PARAM:
                    output_str(expected, node->get_string());
                    expected << ':';
                    break;

                //case as2js::node_t::NODE_PARAM_MATCH:
                default:
                    expected << ':';
                    break;

                }

                pos = 0;
                for(flags_per_node_t const *flags(g_node_types[i].f_node_flags);
                                            flags->f_flag != as2js::flag_t::NODE_FLAG_max;
                                            ++flags, ++pos)
                {
                    if(((1 << pos) & j) != 0)
                    {
                        expected << ' ' << flags->f_name;
                    }
                }

                expected << " (" << node->get_position() << ")" << std::endl;

//std::cerr << "output [" << out.str() << "]\n";
//std::cerr << "expected [" << expected.str() << "]\n";

                CATCH_REQUIRE(out.str() == expected.str());
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("node_display_types_attributes", "[node][display][attributes]")
{
    CATCH_START_SECTION("node_display_types_attributes: verify attributes get displayed")
    {
        // Test all the attributes in the output
        //
        // Note that we test all the attributes, although we always test
        // exactly 2 attributes in common... we may enhance this algorithm
        // later to test all the attributes in all possible combinasons,
        // but that is a bit tricky because of the conflicts.
        //
        for(int i(0); i < 10; ++i)
        {
            // create a node that is not a NODE_PROGRAM
            // (i.e. a node that accepts all attributes)
            std::size_t idx_node;
            do
            {
                idx_node = rand() % g_node_types_size;
            }
            while(g_node_types[idx_node].f_type == as2js::node_t::NODE_PROGRAM);
            as2js::node::pointer_t node(std::make_shared<as2js::node>(g_node_types[idx_node].f_type));

            switch(g_node_types[idx_node].f_type)
            {
            case as2js::node_t::NODE_PARAM:
                node->set_string("param1");
                break;

            default:
                break;

            }

            // need to test all combinatorial cases...
            for(std::size_t j(0); j < g_groups_of_attributes_size; ++j)
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
                            //
                            continue;

                        }
                    }

                    // set that one attribute first
                    //
                    node->set_attribute(*attr_list, true);

                    // test against all the other attributes
                    //
                    for(int a(0); a < static_cast<int>(as2js::attribute_t::NODE_ATTR_max); ++a)
                    {
                        // no need to test with itself, we do that earlier
                        //
                        if(static_cast<as2js::attribute_t>(a) == *attr_list)
                        {
                            CATCH_REQUIRE(node->get_attribute(*attr_list));
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
                        //
                        if(!in_conflict(j, *attr_list, static_cast<as2js::attribute_t>(a)))
                        {
                            // if in conflict we do not care much here because the
                            // display is going to be exactly the same
                            //
                            node->set_attribute(static_cast<as2js::attribute_t>(a), true);

                            // display that now
                            //
                            std::stringstream out;
                            out << *node;

                            // build the expected message
                            //
                            std::stringstream expected;

                            // indent is expected to be exactly 2 on startup and here we only have one line
                            //
                            expected
                                << node
                                << ": "
                                << std::setfill('0')
                                << std::setw(2)
                                << 2
                                << std::setfill(' ')
                                << '.'
                                << std::setw(2)
                                << "";
                            if(g_node_types[idx_node].f_type == as2js::node_t::NODE_EOF)
                            {
                                expected
                                    << std::setw(4)
                                    << std::setfill(' ')
                                    << static_cast<int>(g_node_types[idx_node].f_type);
                            }
                            else
                            {
                                expected
                                    << std::setw(4)
                                    << std::setfill('0')
                                    << static_cast<int>(g_node_types[idx_node].f_type);
                            }
                            expected
                                << std::setfill('\0')
                                << ": "
                                << g_node_types[idx_node].f_name;

                            // add the type as a character if it represents just one character
                            //
                            if(static_cast<int>(g_node_types[idx_node].f_type) > ' '
                            && static_cast<int>(g_node_types[idx_node].f_type) < 0x7F)
                            {
                                expected << " = '" << static_cast<char>(static_cast<int>(g_node_types[idx_node].f_type)) << "'";
                            }

                            switch(g_node_types[idx_node].f_type)
                            {
                            case as2js::node_t::NODE_BREAK:
                            case as2js::node_t::NODE_CLASS:
                            case as2js::node_t::NODE_CONTINUE:
                            case as2js::node_t::NODE_ENUM:
                            case as2js::node_t::NODE_FUNCTION:
                            case as2js::node_t::NODE_GOTO:
                            case as2js::node_t::NODE_IDENTIFIER:
                            case as2js::node_t::NODE_IMPORT:
                            case as2js::node_t::NODE_INTERFACE:
                            case as2js::node_t::NODE_LABEL:
                            case as2js::node_t::NODE_NAMESPACE:
                            case as2js::node_t::NODE_PACKAGE:
                            case as2js::node_t::NODE_REGULAR_EXPRESSION:
                            case as2js::node_t::NODE_STRING:
                            case as2js::node_t::NODE_TEMPLATE:
                            case as2js::node_t::NODE_TEMPLATE_HEAD:
                            case as2js::node_t::NODE_TEMPLATE_MIDDLE:
                            case as2js::node_t::NODE_TEMPLATE_TAIL:
                            case as2js::node_t::NODE_VARIABLE:
                            case as2js::node_t::NODE_VAR_ATTRIBUTES:
                            case as2js::node_t::NODE_VIDENTIFIER:
                                output_str(expected, node->get_string());
                                break;

                            case as2js::node_t::NODE_INTEGER:
                                expected
                                    << ": " << node->get_integer().get()
                                    << ", 0x"
                                    << std::hex
                                    << std::setw(16)
                                    << std::setfill('0')
                                    << node->get_integer().get()
                                    << std::dec << std::setw(0)
                                    << std::setfill('\0');
                                break;

                            case as2js::node_t::NODE_FLOATING_POINT:
                                expected << ": " << node->get_floating_point().get();
                                break;

                            case as2js::node_t::NODE_CALL:
                            case as2js::node_t::NODE_CATCH:
                            case as2js::node_t::NODE_DIRECTIVE_LIST:
                            case as2js::node_t::NODE_FOR:
                            case as2js::node_t::NODE_PARAM_MATCH:
                            case as2js::node_t::NODE_SWITCH:
                            case as2js::node_t::NODE_TYPE:
                            case as2js::node_t::NODE_VAR:
                                expected << ":";
                                break;

                            case as2js::node_t::NODE_PARAM:
                                expected << ": 'param1':";
                                break;

                            default:
                                break;

                            }

                            int pa(a);
                            int pb(static_cast<int>(*attr_list));
                            if(pa > pb)
                            {
                                std::swap(pa, pb);
                            }
                            expected << " attrs: " << g_attribute_names[pa] << " " << g_attribute_names[pb];

                            expected << " (" << node->get_position() << ")" << std::endl;

//std::cerr << "  output [" << out.str() << "]\n";
//std::cerr << "expected [" << expected.str() << "] (for: " << static_cast<int>(*attr_list) << " / " << a << ")\n";

                            CATCH_REQUIRE(out.str() == expected.str());

                            node->set_attribute(static_cast<as2js::attribute_t>(a), false);
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


CATCH_TEST_CASE("node_display_tree", "[node][display][tree]")
{
    CATCH_START_SECTION("node_display_tree: display a tree of nodes")
    {
        // create all the nodes as the lexer would do
        as2js::node::pointer_t root(std::make_shared<as2js::node>(as2js::node_t::NODE_ROOT));
        as2js::position pos;
        pos.reset_counters(22);
        pos.set_filename("display.js");
        root->set_position(pos);
        as2js::node::pointer_t directive_list_a(std::make_shared<as2js::node>(as2js::node_t::NODE_DIRECTIVE_LIST));
        as2js::node::pointer_t directive_list_b(std::make_shared<as2js::node>(as2js::node_t::NODE_DIRECTIVE_LIST));
        directive_list_b->set_flag(as2js::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES, true);
        as2js::node::pointer_t assignment(std::make_shared<as2js::node>(as2js::node_t::NODE_ASSIGNMENT));
        as2js::node::pointer_t identifier_a(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        identifier_a->set_string("a");
        identifier_a->set_attribute(as2js::attribute_t::NODE_ATTR_TRUE, true);
        as2js::node::pointer_t power(std::make_shared<as2js::node>(as2js::node_t::NODE_POWER));
        as2js::node::pointer_t member(std::make_shared<as2js::node>(as2js::node_t::NODE_MEMBER));
        as2js::node::pointer_t identifier_math(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        identifier_math->set_string("Math");
        identifier_math->set_attribute(as2js::attribute_t::NODE_ATTR_NATIVE, true);
        as2js::node::pointer_t math_type(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        math_type->set_string("Math");
        identifier_math->set_type_node(math_type);
        as2js::node::pointer_t math_instance(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        math_instance->set_string("m");
        identifier_math->set_instance(math_instance);
        as2js::node::pointer_t identifier_e(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        identifier_e->set_string("e");
        identifier_e->set_flag(as2js::flag_t::NODE_IDENTIFIER_FLAG_TYPED, true);
        as2js::node::pointer_t e_type(std::make_shared<as2js::node>(as2js::node_t::NODE_IDENTIFIER));
        e_type->set_string("Float");
        identifier_e->set_type_node(e_type);
        as2js::node::pointer_t literal(std::make_shared<as2js::node>(as2js::node_t::NODE_FLOATING_POINT));
        as2js::floating_point f;
        f.set(1.424);
        literal->set_floating_point(f);
        as2js::node::pointer_t function(std::make_shared<as2js::node>(as2js::node_t::NODE_FUNCTION));
        function->set_string("my_func");
        as2js::node::pointer_t func_var(std::make_shared<as2js::node>(as2js::node_t::NODE_VAR));
        as2js::node::pointer_t func_variable(std::make_shared<as2js::node>(as2js::node_t::NODE_VARIABLE));
        func_variable->set_string("q");
        as2js::node::pointer_t label(std::make_shared<as2js::node>(as2js::node_t::NODE_LABEL));
        label->set_string("ignore");
        function->add_label(label);
        function->add_variable(func_variable);

        // build the tree as the parser would do
        root->append_child(directive_list_a);
        root->append_child(directive_list_b);
        directive_list_a->append_child(assignment);
        assignment->append_child(identifier_a);
        assignment->insert_child(-1, power);
        power->append_child(member);
        power->insert_child(1, literal);
        member->append_child(identifier_e);
        member->insert_child(0, identifier_math);
        directive_list_b->append_child(function);
        function->append_child(func_var);
        func_var->append_child(func_variable);
        function->append_child(label);

        // now test the output
        std::stringstream out;
        out << *root;

        // build the expected message
        std::stringstream expected;

        // ROOT
        expected << root << ": " << std::setfill('0') << std::setw(2) << 2 << std::setfill(' ') << '.' << std::setw(2) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_ROOT))
                 << std::setfill('\0') << ": ROOT"
                 << " (" << root->get_position() << ")" << std::endl;

        // DIRECTIVE_LIST A
        expected << directive_list_a << ": " << std::setfill('0') << std::setw(2) << 3 << std::setfill(' ') << '-' << std::setw(3) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_DIRECTIVE_LIST))
                 << std::setfill('\0') << ": DIRECTIVE_LIST:"
                 << " (" << directive_list_a->get_position() << ")" << std::endl;

        // ASSIGNMENT
        expected << assignment << ": " << std::setfill('0') << std::setw(2) << 4 << std::setfill(' ') << '-' << std::setw(4) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_ASSIGNMENT))
                 << std::setfill('\0') << ": ASSIGNMENT = '='"
                 << " (" << assignment->get_position() << ")" << std::endl;

        // IDENTIFIER A
        expected << identifier_a << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << '-' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_IDENTIFIER))
                 << std::setfill('\0') << ": IDENTIFIER: 'a' attrs: TRUE"
                 << " (" << identifier_a->get_position() << ")" << std::endl;

        // POWER
        expected << power << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << '-' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_POWER))
                 << std::setfill('\0') << ": POWER"
                 << " (" << power->get_position() << ")" << std::endl;

        // MEMBER
        expected << member << ": " << std::setfill('0') << std::setw(2) << 6 << std::setfill(' ') << '-' << std::setw(6) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_MEMBER))
                 << std::setfill('\0') << ": MEMBER = '.'"
                 << " (" << member->get_position() << ")" << std::endl;

        // IDENTIFIER MATH
        expected << identifier_math << ": " << std::setfill('0') << std::setw(2) << 7 << std::setfill(' ') << '-' << std::setw(7) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_IDENTIFIER))
                 << std::setfill('\0') << ": IDENTIFIER: 'Math' Instance: " << math_instance << " Type node: " << math_type << " attrs: NATIVE"
                 << " (" << identifier_math->get_position() << ")" << std::endl;

        // IDENTIFIER E
        expected << identifier_e << ": " << std::setfill('0') << std::setw(2) << 7 << std::setfill(' ') << '-' << std::setw(7) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_IDENTIFIER))
                 << std::setfill('\0') << ": IDENTIFIER: 'e' TYPED Type node: " << e_type
                 << " (" << identifier_e->get_position() << ")" << std::endl;

        // FLOATING_POINT
        expected << literal << ": " << std::setfill('0') << std::setw(2) << 6 << std::setfill(' ') << '-' << std::setw(6) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_FLOATING_POINT))
                 << std::setfill('\0') << ": FLOATING_POINT: 1.424"
                 << " (" << literal->get_position() << ")" << std::endl;

        // DIRECTIVE_LIST B
        expected << directive_list_b << ": " << std::setfill('0') << std::setw(2) << 3 << std::setfill(' ') << '-' << std::setw(3) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_DIRECTIVE_LIST))
                 << std::setfill('\0') << ": DIRECTIVE_LIST: NEW-VARIABLES"
                 << " (" << directive_list_b->get_position() << ")" << std::endl;

        // FUNCTION
        expected << function << ": " << std::setfill('0') << std::setw(2) << 4 << std::setfill(' ') << '-' << std::setw(4) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_FUNCTION))
                 << std::setfill('\0') << ": FUNCTION: 'my_func'"
                 << " (" << function->get_position() << ")" << std::endl;

        // VAR
        expected << func_var << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << '-' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_VAR))
                 << std::setfill('\0') << ": VAR:"
                 << " (" << func_var->get_position() << ")" << std::endl;

        // VARIABLE
        expected << func_variable << ": " << std::setfill('0') << std::setw(2) << 6 << std::setfill(' ') << '-' << std::setw(6) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_VARIABLE))
                 << std::setfill('\0') << ": VARIABLE: 'q'"
                 << " (" << func_variable->get_position() << ")" << std::endl;

        // LABEL
        expected << label << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << '-' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_LABEL))
                 << std::setfill('\0') << ": LABEL: 'ignore'"
                 << " (" << label->get_position() << ")" << std::endl;

        // VARIABLE
        expected << func_variable << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << '=' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_VARIABLE))
                 << std::setfill('\0') << ": VARIABLE: 'q'"
                 << " (" << func_variable->get_position() << ")" << std::endl;

        // LABEL
        expected << label << ": " << std::setfill('0') << std::setw(2) << 5 << std::setfill(' ') << ':' << std::setw(5) << ""
                 << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<as2js::node_t>(as2js::node_t::NODE_LABEL))
                 << std::setfill('\0') << ": LABEL: 'ignore'"
                 << " (" << label->get_position() << ")" << std::endl;

//std::cerr << "output [" << out.str() << "]\n";
//std::cerr << "expected [" << expected.str() << "]\n";

        CATCH_REQUIRE(out.str() == expected.str());
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
