// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// self
//
#include    <as2js/lexer.h>



/** \file
 * \brief Definition of internal tables of the optimizer.
 *
 * The structures defined here are used to define arrays of optimizations.
 *
 * In general we place a set of optimizations in separate files
 * based on the type of operator, statement, or other feature
 * being optimized.
 *
 * The organization is fairly simple:
 *
 * . optimization_table_t
 *
 * A large set of optimization_table_t structures each one
 * definiting one simple optimization such as:
 *
 * \code
 *     'a + b' => 'sum(a, b)'
 * \endcode
 *
 * when 'a' and 'b' are literal numbers.
 *
 * . 
 */

namespace as2js
{
namespace optimizer_details
{


#define POINTER_AND_COUNT(name)             name, sizeof(name) / sizeof(name[0])
#define NULL_POINTER_AND_COUNT()            nullptr, 0


uint32_t const OPTIMIZATION_MATCH_FLAG_CHILDREN =        0x0001;


struct optimization_match_t
{
    struct optimization_literal_t
    {
        node::node_t                        f_operator;
        char const *                        f_string;
        integer::value_type                 f_integer;
        floating_point::value_type          f_floating_point;
    };

    std::uint8_t                    f_depth;        // to build a tree
    std::uint8_t                    f_match_flags;  // zero by default

    node::node_t const *            f_node_types;
    size_t                          f_node_types_count;

    optimization_literal_t const *  f_with_value;

    node::attribute_t const *       f_attributes;   // list of attributes, NODE_ATTR_max is used to separate each list
    size_t                          f_attributes_count;

    node::flag_t const *            f_flags;        // list of flags, NODE_FLAG_max is used to seperate each list
    size_t                          f_flags_count;
};


enum class optimization_function_t
{
    OPTIMIZATION_FUNCTION_ADD,
    OPTIMIZATION_FUNCTION_BITWISE_AND,
    OPTIMIZATION_FUNCTION_BITWISE_NOT,
    OPTIMIZATION_FUNCTION_BITWISE_OR,
    OPTIMIZATION_FUNCTION_BITWISE_XOR,
    OPTIMIZATION_FUNCTION_COMPARE,
    OPTIMIZATION_FUNCTION_CONCATENATE,
    OPTIMIZATION_FUNCTION_DIVIDE,
    OPTIMIZATION_FUNCTION_EQUAL,
    OPTIMIZATION_FUNCTION_LESS,
    OPTIMIZATION_FUNCTION_LESS_EQUAL,
    OPTIMIZATION_FUNCTION_LOGICAL_NOT,
    OPTIMIZATION_FUNCTION_LOGICAL_XOR,
    OPTIMIZATION_FUNCTION_MATCH,
    OPTIMIZATION_FUNCTION_MAXIMUM,
    OPTIMIZATION_FUNCTION_MINIMUM,
    OPTIMIZATION_FUNCTION_MODULO,
    OPTIMIZATION_FUNCTION_MOVE,
    OPTIMIZATION_FUNCTION_MULTIPLY,
    OPTIMIZATION_FUNCTION_NEGATE,
    OPTIMIZATION_FUNCTION_POWER,
    OPTIMIZATION_FUNCTION_REMOVE,
    OPTIMIZATION_FUNCTION_ROTATE_LEFT,
    OPTIMIZATION_FUNCTION_ROTATE_RIGHT,
    OPTIMIZATION_FUNCTION_SET_INTEGER,
    //OPTIMIZATION_FUNCTION_SET_FLOAT,
    OPTIMIZATION_FUNCTION_SET_NODE_TYPE,
    OPTIMIZATION_FUNCTION_SHIFT_LEFT,
    OPTIMIZATION_FUNCTION_SHIFT_RIGHT,
    OPTIMIZATION_FUNCTION_SHIFT_RIGHT_UNSIGNED,
    OPTIMIZATION_FUNCTION_SMART_MATCH,
    OPTIMIZATION_FUNCTION_STRICTLY_EQUAL,
    OPTIMIZATION_FUNCTION_SUBTRACT,
    OPTIMIZATION_FUNCTION_SWAP,
    OPTIMIZATION_FUNCTION_TO_CONDITIONAL,
    //OPTIMIZATION_FUNCTION_TO_FLOATING_POINT,
    OPTIMIZATION_FUNCTION_TO_INTEGER,
    OPTIMIZATION_FUNCTION_TO_NUMBER,
    //OPTIMIZATION_FUNCTION_TO_STRING,
    OPTIMIZATION_FUNCTION_WHILE_TRUE_TO_FOREVER
};


typedef uint16_t                    index_t;

struct optimization_optimize_t
{
    optimization_function_t         f_function;
    index_t                         f_indexes[6];   // number of indices used varies depending on the function
};


uint32_t const OPTIMIZATION_ENTRY_FLAG_UNSAFE_MATH =        0x0001;
uint32_t const OPTIMIZATION_ENTRY_FLAG_UNSAFE_OBJECT =      0x0002;     // in most cases because the object may have its own operator(s)

struct optimization_entry_t
{
    char const *                    f_name;
    uint32_t                        f_flags;

    optimization_match_t const *    f_match;
    size_t                          f_match_count;

    optimization_optimize_t const * f_optimize;
    size_t                          f_optimize_count;
};


struct optimization_table_t
{
    optimization_entry_t const *    f_entry;
    size_t                          f_entry_count;
};


struct optimization_tables_t
{
    optimization_table_t const *    f_table;
    size_t                          f_table_count;
};




bool optimize_tree(node::pointer_t n);

bool match_tree(
          node::vector_of_pointers_t & node_array
        , node::pointer_t n
        , optimization_match_t const * match
        , std::size_t match_size
        , std::uint8_t depth);

void apply_functions(
          node::vector_of_pointers_t & node_array
        , optimization_optimize_t const * optimize
        , std::size_t optimize_size);


} // namespace optimizer_details
} // namespace as2js
// vim: ts=4 sw=4 et
