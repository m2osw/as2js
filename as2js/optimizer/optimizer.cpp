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

// self
//
#include    "as2js/optimizer.h"

#include    "as2js/message.h"

// private classes
//
#include    "optimizer_tables.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{
namespace optimizer
{


/** \brief The as2js optimizer.
 *
 * This function goes through all the available optimizations and
 * processes them whenever they apply to your code.
 *
 * Errors may be generated whenever a problem is found.
 *
 * Also some potential errors such as a division or modulo by
 * zero can legally occur in your input program so in that case the
 * optimizer generates a warning to let you know that such a division
 * was found, but no error to speak of.
 *
 * The function reports the total number of errors that were generated
 * while optimizing.
 *
 * At any point after parsing, the program can be passed through
 * the optimizer. This means removing all the possible expressions and
 * statements which can be removed to make the code smaller in the end.
 * The optimizations applied can be tweaked using options ('use ...;').
 *
 * In most cases the compiler already takes care of calling the optimizer
 * at appropriate times. Since it is a static function, it can directly
 * be called as in:
 *
 * \code
 *    optimizer::optimize(root);
 * \endcode
 *
 * Where root is a Node representing the root of the optimization (anything
 * outside of the root does not get optimized.)
 *
 * The optimize() function tries to remove all possible expressions
 * and statements which will have no effect in the final output
 * (by default, certain things such as x + 0, may not be removed since
 * such may have an effect... if x is a string, then x + 0 concatenates
 * zero to that string.)
 *
 * The root parameter may be what was returned by the Parser::parse()
 * function of the. However, in most cases, the compiler only optimizes
 * part of the tree as required (because many parts cannot be optimized
 * and it will make things generally faster.)
 *
 * The optimizations are organized in C++ tables that get linked
 * in the compiler as read-only static data. These are organized
 * in many separate files because of the large amount of possible
 * optimizations. There is a list of the various files in the
 * optimizer:
 *
 * \li optimizer.cpp -- the main Optimizer object implementation;
 * all the other files are considered private.
 *
 * \li optimizer_matches.ci and optimizer_matches.cpp -- the tables
 * (.ci) and the functions (.cpp) used to match a tree of nodes
 * and thus determine whether an optimization can be applied or not.
 *
 * \li optimizer_tables.cpp and optimizer_tables.h -- the top level
 * tables of the optimizer. These are used to search for optimizations
 * that can be applied against your tree of nodes. The header defines
 * private classes, structures, etc.
 *
 * \li optimizer_values.ci -- a set of tables representing literal
 * values as in some cases an optimization can be applied if a
 * literal has a specific value.
 *
 * \li optimizer_optimize.ci and optimizer_optimize.cpp -- a set
 * of optimizations defined using tables and corresponding functions
 * to actually apply the optimizations to a tree of nodes.
 *
 * \li optimizer_additive.ci -- optimizations for '+' and '-', including
 * string concatenations.
 *
 * \li optimizer_assignments.ci -- optimizations for all assignments, '=',
 * '+=', '-=', '*=', etc.
 *
 * \li optimizer_bitwise.ci -- optimizations for '~', '&', '|', '^', '<<', '>>',
 * '>>>', '<!', and '>!'.
 *
 * \li optimizer_compare.ci -- optimizations for '<=>'.
 *
 * \li optimizer_conditional.ci -- optimizations for 'a ? b : c'.
 *
 * \li optimizer_equality.ci -- optimizations for '==', '!=', '===', '!==',
 * and '~~'.
 *
 * \li optimizer_logical.ci -- optimizations for '!', '&&', '||', and '^^'.
 *
 * \li optimizer_match.ci -- optimizations for '~=' and '!~'.
 *
 * \li optimizer_multiplicative.ci -- optimizations for '*', '/', '%',
 * and '**'.
 *
 * \li optimizer_relational.ci -- optimizations for '<', '<=', '>',
 * and '>='.
 *
 * \li optimizer_statments.ci -- optimizations for 'if', 'while', 'do',
 * and "directives" (blocks).
 *
 * \attention
 * It is important to note that this function is not unlikely going
 * to modify your tree (even if you do not think there is a possible
 * optimization). This means the caller should not expect the node to
 * still be the same pointer and possibly not at the same location in
 * the parent node (many nodes get deleted.)
 *
 * \param[in] node  The node to optimize.
 *
 * \return The number of errors generated while optimizing.
 */
int optimize(node::pointer_t & node)
{
    int const errcnt(message::error_count());

    optimizer_details::optimize_tree(node);

    // This may not be at the right place because the caller may be
    // looping through a list of children too... (although we have
    // an important not in the documentation... that does not mean
    // much, does it?)
    //
    if(node)
    {
        node->clean_tree();
    }

    return message::error_count() - errcnt;
}



} // namespace optimizer
} // namespace as2js
// vim: ts=4 sw=4 et
