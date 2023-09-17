// Copyright (c) 2005-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    "as2js/node.h"

#include    "as2js/exception.h"


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Handle operator types to string and from string.
 *
 * The as2js compiler allows you to overload operators in your classes.
 * This feature requires us to know about the operator name as a string,
 * not just a type such as NODE_ADD. This file implements two functions
 * to convert operators types to and from strings.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE OPERATOR  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Internal structures and tables used to do operator conversions.
 *
 * The following namespace defines a structure and a table of node types
 * with the name of the operator as a string. For debug purposes, we also
 * include the line number.
 */
namespace
{



/** \brief Structure to define an operator.
 *
 * This structure defines one operator including a node type, the
 * name of the operator such as "!" for the logical not, and a
 * line number. The line number is only used for debug purposes
 * when a mistake is found in the conversion table.
 */
struct operator_to_string_t
{
    /** \brief The type of node.
     *
     * This parameter defines a node type such as NODE_ADD. Only
     * operators are to be defined in this table, although there
     * is nothing that prevents you from adding any type here.
     */
    node_t          f_node;

    /** \brief The name of the operator.
     *
     * This entry represents the "name" of the operator. This is
     * the ASCII representation of the operator such as "!" for
     * the logical not operator.
     */
    char const *    f_name;

    /** \brief The line on which the operator is defined.
     *
     * For debug purposes, when we make changes to the table we
     * may end up with an invalid table. This line number is used
     * to generate an error to the programmer who can then fix
     * the problem quickly instead of trying to guess what is
     * wrong in the table.
     */
    int             f_line;
};


/** \brief Table of operator types to operator names.
 *
 * This table is used to convert operators (i.e. node_t::NODE_ADD)
 * to strings, and vice versa.
 *
 * The operators are sorted numerically so we can search them using
 * a fast binary search algorithm. When compiling in debug mode,
 * the operator_to_string() function verifies that the order is
 * proper.
 *
 * \sa operator_to_string()
 */
operator_to_string_t const g_operator_to_string[] =
{
    // single character -- sorted in ASCII
    { node_t::NODE_LOGICAL_NOT,                     "!", __LINE__ },
    { node_t::NODE_MODULO,                          "%", __LINE__ },
    { node_t::NODE_BITWISE_AND,                     "&", __LINE__ },
    { node_t::NODE_MULTIPLY,                        "*", __LINE__ },
    { node_t::NODE_ADD,                             "+", __LINE__ },
    { node_t::NODE_COMMA,                           ",", __LINE__ },
    { node_t::NODE_SUBTRACT,                        "-", __LINE__ },
    { node_t::NODE_DIVIDE,                          "/", __LINE__ },
    { node_t::NODE_LESS,                            "<", __LINE__ },
    { node_t::NODE_ASSIGNMENT,                      "=", __LINE__ },
    { node_t::NODE_GREATER,                         ">", __LINE__ },
    { node_t::NODE_BITWISE_XOR,                     "^", __LINE__ },
    { node_t::NODE_BITWISE_OR,                      "|", __LINE__ },
    { node_t::NODE_BITWISE_NOT,                     "~", __LINE__ },

    // two or more characters transformed to an enum only
    { node_t::NODE_ALMOST_EQUAL,                    "\u2248", __LINE__ }, // this is just one character too, but UTF-8
    { node_t::NODE_ASSIGNMENT_ADD,                  "+=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_BITWISE_AND,          "&=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_BITWISE_OR,           "|=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_BITWISE_XOR,          "^=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_COALESCE,             "??=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_DIVIDE,               "/=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_LOGICAL_AND,          "&&=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_LOGICAL_OR,           "||=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_LOGICAL_XOR,          "^^=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_MAXIMUM,              ">?=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_MINIMUM,              "<?=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_MODULO,               "%=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_MULTIPLY,             "*=",   __LINE__ },
    { node_t::NODE_ASSIGNMENT_POWER,                "**=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_ROTATE_LEFT,          "<%=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,         ">%=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_SHIFT_LEFT,           "<<=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,          ">>=",  __LINE__ },
    { node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED, ">>>=", __LINE__ },
    { node_t::NODE_ASSIGNMENT_SUBTRACT,             "-=",   __LINE__ },
    { node_t::NODE_CALL,                            "()",   __LINE__ },
    { node_t::NODE_COALESCE,                        "??",   __LINE__ },
    { node_t::NODE_COMPARE,                         "<=>",  __LINE__ },
    { node_t::NODE_DECREMENT,                       "--x",  __LINE__ },
    { node_t::NODE_EQUAL,                           "==",   __LINE__ },
    { node_t::NODE_GREATER_EQUAL,                   ">=",   __LINE__ },
    { node_t::NODE_IN,                              "in",   __LINE__ },
    { node_t::NODE_INCREMENT,                       "++x",  __LINE__ },
    { node_t::NODE_LESS_EQUAL,                      "<=",   __LINE__ },
    { node_t::NODE_LOGICAL_AND,                     "&&",   __LINE__ },
    { node_t::NODE_LOGICAL_OR,                      "||",   __LINE__ },
    { node_t::NODE_LOGICAL_XOR,                     "^^",   __LINE__ },
    { node_t::NODE_MATCH,                           "~=",   __LINE__ },
    { node_t::NODE_MAXIMUM,                         ">?",   __LINE__ },
    { node_t::NODE_MINIMUM,                         "<?",   __LINE__ },
    { node_t::NODE_NOT_EQUAL,                       "!=",   __LINE__ },
    { node_t::NODE_NOT_MATCH,                       "~!",   __LINE__ },
    { node_t::NODE_OPTIONAL_MEMBER,                 "?.",   __LINE__ },
    { node_t::NODE_POST_DECREMENT,                  "x--",  __LINE__ },
    { node_t::NODE_POST_INCREMENT,                  "x++",  __LINE__ },
    { node_t::NODE_POWER,                           "**",   __LINE__ },
    { node_t::NODE_ROTATE_LEFT,                     "<%",   __LINE__ },
    { node_t::NODE_ROTATE_RIGHT,                    ">%",   __LINE__ },
    { node_t::NODE_SHIFT_LEFT,                      "<<",   __LINE__ },
    { node_t::NODE_SHIFT_RIGHT,                     ">>",   __LINE__ },
    { node_t::NODE_SHIFT_RIGHT_UNSIGNED,            ">>>",  __LINE__ },
    { node_t::NODE_SMART_MATCH,                     "~~",   __LINE__ },
    { node_t::NODE_STRICTLY_EQUAL,                  "===",  __LINE__ },
    { node_t::NODE_STRICTLY_NOT_EQUAL,              "!==",  __LINE__ }

    // the following does not make it in user redefinable operators
    //{ node_t::NODE_CONDITIONAL,                   "", __LINE__ },
    //{ node_t::NODE_DELETE,                        "", __LINE__ },
    //{ node_t::NODE_IN,                            "", __LINE__ },
    //{ node_t::NODE_INSTANCEOF,                    "", __LINE__ },
    //{ node_t::NODE_IS,                            "", __LINE__ },
    //{ node_t::NODE_LIST,                          "", __LINE__ },
    //{ node_t::NODE_NEW,                           "", __LINE__ },
    //{ node_t::NODE_RANGE,                         "", __LINE__ },
    //{ node_t::NODE_SCOPE,                         "", __LINE__ },
};


/** \brief The size of the g_operator_to_string table.
 *
 * This variable represents the size, number of structures, in the
 * g_operator_to_string table.
 */
size_t const g_operator_to_string_size = sizeof(g_operator_to_string) / sizeof(g_operator_to_string[0]);



}
// no name namespace



/** \brief Transform an operator to a string.
 *
 * This function transforms the specified operator (\p op) to a
 * printable string. It is generaly used to print out an error
 * message.
 *
 * If the function cannot find the operator, then it returns a
 * null pointer (be careful, we return a standard C null terminated
 * string here, not an std::string or as2js::string.)
 *
 * \param[in] op  The operator to convert to a string.
 *
 * \return A basic null terminated C string with the operator name or nullptr.
 *
 * \sa string_to_operator()
 */
char const * node::operator_to_string(node_t op)
{
#if defined(_DEBUG) || defined(DEBUG)
    {
        // make sure that the node types are properly sorted
        //
        static bool checked = false;
        if(!checked)
        {
            // check only once
            //
            checked = true;
            for(size_t idx = 1; idx < g_operator_to_string_size; ++idx)
            {
                if(g_operator_to_string[idx].f_node <= g_operator_to_string[idx - 1].f_node) [[unlikely]]
                {
                    // LCOV_EXCL_START
                    std::cerr << "INTERNAL ERROR at offset " << idx
                              << " (line #" << g_operator_to_string[idx].f_line
                              << ", node type " << static_cast<uint32_t>(g_operator_to_string[idx].f_node)
                              << " vs. " << static_cast<uint32_t>(g_operator_to_string[idx - 1].f_node)
                              << "): the g_operator_to_string table isn't sorted properly. We can't binary search it."
                              << std::endl;
                    throw internal_error("node types not properly sorted, cannot properly search for operators using a binary search.");
                    // LCOV_EXCL_STOP
                }
            }
        }
    }
#endif

    size_t i(0);
    size_t j(g_operator_to_string_size);
    while(i < j)
    {
        size_t p((j - i) / 2 + i);
        int r(static_cast<int>(g_operator_to_string[p].f_node) - static_cast<int>(op));
        if(r == 0)
        {
            return g_operator_to_string[p].f_name;
        }
        if(r < 0)
        {
            i = p + 1;
        }
        else
        {
            j = p;
        }
    }

    return nullptr;
}


/** \brief Transform a string in an operator.
 *
 * The user may declare operators in his classes. Because of that
 * the lexer returns identifiers and strings that need to later be
 * converted to an operator. This function is used for this purpose.
 *
 * If the operator is invalid, then the function returns NODE_UNKNOWN.
 *
 * \todo
 * The table is sorted by type_t::NODE_... value. It would be good
 * to create another table sorted by name. We could declare a static
 * table which gets initialized on a first call to this function.
 * Then we could also make use of the binary search here.
 *
 * \todo
 * This is a TBD, I think it is okay, but the compiler may need some
 * tweaking to work...
 * It seems that the ++x and x++ (and corresponding --) won't work
 * right. We should be able to detect that once we try to declare
 * such operators in a class. The "x" is nice when outputing the
 * result, but it is problematic when searching for a node type.
 * However, we certainly have to add it anyway depending on whether
 * the function has a parameter or not because otherwise we cannot
 * know whether it is a pre- or a post-increment or -decrement.
 *
 * \param[in] str  The string representing the operator to convert.
 *
 * \return The node type representing this operator.
 *
 * \sa operator_to_string()
 */
node_t node::string_to_operator(std::string const & str)
{
    for(size_t idx(0); idx < g_operator_to_string_size; ++idx)
    {
        // not sorted by name so we use a slow poke search...
        if(str == g_operator_to_string[idx].f_name)
        {
            return g_operator_to_string[idx].f_node;
        }
    }

    // some equivalence which right now we cannot add to the g_operator_to_string
    // (we probably could enhance that to have that and also we would need to
    // support a fast search, possibly a map)
    //
    if(str == "<>")
    {
        return node_t::NODE_NOT_EQUAL;
    }
    if(str == ":="
    || str == "\xE2\x89\x94")
    {
        return node_t::NODE_ASSIGNMENT;
    }
    if(str == "\xC3\x97")
    {
        return node_t::NODE_MULTIPLY;
    }
    if(str == "\xC3\xB7")
    {
        return node_t::NODE_DIVIDE;
    }
    if(str == "\xE2\x87\x92")
    {
        return node_t::NODE_ARROW;
    }
    if(str == "\xE2\x88\x88"
    || str == "\xE2\x88\x8A")
    {
        return node_t::NODE_IN;
    }
    if(str == "\xE2\x88\xA7")
    {
        return node_t::NODE_LOGICAL_AND;
    }
    if(str == "\xE2\x88\xA8")
    {
        return node_t::NODE_LOGICAL_OR;
    }
    if(str == "\xE2\x89\x88")
    {
        return node_t::NODE_ALMOST_EQUAL;
    }
    if(str == "\xE2\x89\xA4")
    {
        return node_t::NODE_LESS_EQUAL;
    }
    if(str == "\xE2\x89\xA5")
    {
        return node_t::NODE_GREATER_EQUAL;
    }

    return node_t::NODE_UNKNOWN;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
