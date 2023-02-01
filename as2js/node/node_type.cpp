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
#include    "as2js/node.h"

#include    "as2js/exception.h"


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implement node type functions.
 *
 * This file includes the implementation of various functions that
 * directly work against the type of a node.
 *
 * It also includes a function one can use to convert a node_t
 * into a string.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE  ***********************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Private definitions of the node type.
 *
 * Local declarations defining a table of node types and their name so
 * we can convert a node type to a string. The main purpose of which is
 * to generate meaningful errors (i.e. 'had a problem with node type #38',
 * or 'had a problem with node of type "ABSTRACT"'.)
 */
namespace
{

/** \brief Structure used to define the name of each node type.
 *
 * This structure includes the node type and its name. Both are
 * used to convert a numeric type to a string so one can write
 * that string in error streams.
 */
struct type_name_t
{
    /** \brief The node type.
     *
     * The node type concerned by this entry.
     */
    node_t          f_type;

    /** \brief The name of the node type.
     *
     * This pointer defines the name of the node type.
     */
    char const *    f_name;

    /** \brief The line number of the definition.
     *
     * This value defines the line number where the definition is found
     * in this file. It is useful for debug purposes.
     */
    int             f_line;
};

/** \brief Macro used to convert parameters into strings.
 *
 * \param[in] s  An identifier to be transformed in a string.
 */
#define    TO_STR_sub(s)            #s

/** \brief Macro used to add a named type to the table of node types.
 *
 * \warning
 * This macro cannot be used with the NODE_EOF type because the
 * identifier EOF is generally defined to -1 and it gets replaced
 * before the macro gets called resulting in an invalid definition.
 *
 * \warning
 * This macro cannot be used with the NODE_NULL type because the
 * identifier NULL is generally defined to 0 and it gets replaced
 * before the macro gets called resulting in an invalid definition.
 *
 * \param[in] n  The name of the node type as an identifier.
 */
#define    NODE_TYPE_NAME(n)     { node_t::NODE_##n, TO_STR_sub(n), __LINE__ }

/** \brief List of node types with their name.
 *
 * This table defines a list of node types with their corresponding name
 * defined as a string. The definitions make use of the NODE_TYPE_NAME()
 * macro to better ensure validity of each entry (i.e. the identifier
 * used with the NODE_TYPE_NAME() is transformed to a NODE_... name and
 * the corresponding string make it impossible to have either
 * non-synchronized.)
 *
 * The table is sorted by type (node_t::NODE_...). In debug mode, the
 * type_to_string() function verifies that the order remains valid.
 */
type_name_t const g_node_type_name[] =
{
    // EOF is -1 on most C/C++ computers... so we have to do this one by hand
    { node_t::NODE_EOF, "EOF", __LINE__ },
    NODE_TYPE_NAME(UNKNOWN),

    // the one character types have to be ordered by their character
    // which means it does not match the alphabetical order we
    // generally use
    NODE_TYPE_NAME(LOGICAL_NOT),                      // 0x21
    NODE_TYPE_NAME(MODULO),                           // 0x25
    NODE_TYPE_NAME(BITWISE_AND),                      // 0x26
    NODE_TYPE_NAME(OPEN_PARENTHESIS),                 // 0x28
    NODE_TYPE_NAME(CLOSE_PARENTHESIS),                // 0x29
    NODE_TYPE_NAME(MULTIPLY),                         // 0x2A
    NODE_TYPE_NAME(ADD),                              // 0x2B
    NODE_TYPE_NAME(COMMA),                            // 0x2C
    NODE_TYPE_NAME(SUBTRACT),                         // 0x2D
    NODE_TYPE_NAME(MEMBER),                           // 0x2E
    NODE_TYPE_NAME(DIVIDE),                           // 0x2F
    NODE_TYPE_NAME(COLON),                            // 0x3A
    NODE_TYPE_NAME(SEMICOLON),                        // 0x3B
    NODE_TYPE_NAME(LESS),                             // 0x3C
    NODE_TYPE_NAME(ASSIGNMENT),                       // 0x3D
    NODE_TYPE_NAME(GREATER),                          // 0x3E
    NODE_TYPE_NAME(CONDITIONAL),                      // 0x3F
    NODE_TYPE_NAME(OPEN_SQUARE_BRACKET),              // 0x5B
    NODE_TYPE_NAME(CLOSE_SQUARE_BRACKET),             // 0x5D
    NODE_TYPE_NAME(BITWISE_XOR),                      // 0x5E
    NODE_TYPE_NAME(OPEN_CURVLY_BRACKET),              // 0x7B
    NODE_TYPE_NAME(BITWISE_OR),                       // 0x7C
    NODE_TYPE_NAME(CLOSE_CURVLY_BRACKET),             // 0x7D
    NODE_TYPE_NAME(BITWISE_NOT),                      // 0x7E

    NODE_TYPE_NAME(ABSTRACT),
    NODE_TYPE_NAME(ALMOST_EQUAL),
    NODE_TYPE_NAME(ARRAY),
    NODE_TYPE_NAME(ARRAY_LITERAL),
    NODE_TYPE_NAME(ARROW),
    NODE_TYPE_NAME(AS),
    NODE_TYPE_NAME(ASSIGNMENT_ADD),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_AND),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_OR),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_COALESCE),
    NODE_TYPE_NAME(ASSIGNMENT_DIVIDE),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_AND),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_OR),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_MAXIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MINIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MODULO),
    NODE_TYPE_NAME(ASSIGNMENT_MULTIPLY),
    NODE_TYPE_NAME(ASSIGNMENT_POWER),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(ASSIGNMENT_SUBTRACT),
    NODE_TYPE_NAME(ASYNC),
    NODE_TYPE_NAME(ATTRIBUTES),
    NODE_TYPE_NAME(AUTO),
    NODE_TYPE_NAME(AWAIT),
    NODE_TYPE_NAME(BOOLEAN),
    NODE_TYPE_NAME(BREAK),
    NODE_TYPE_NAME(BYTE),
    NODE_TYPE_NAME(CALL),
    NODE_TYPE_NAME(CASE),
    NODE_TYPE_NAME(CATCH),
    NODE_TYPE_NAME(CHAR),
    NODE_TYPE_NAME(CLASS),
    NODE_TYPE_NAME(COALESCE),
    NODE_TYPE_NAME(COMPARE),
    NODE_TYPE_NAME(CONST),
    NODE_TYPE_NAME(CONTINUE),
    NODE_TYPE_NAME(DEBUGGER),
    NODE_TYPE_NAME(DECREMENT),
    NODE_TYPE_NAME(DEFAULT),
    NODE_TYPE_NAME(DELETE),
    NODE_TYPE_NAME(DIRECTIVE_LIST),
    NODE_TYPE_NAME(DO),
    NODE_TYPE_NAME(DOUBLE),
    NODE_TYPE_NAME(ELSE),
    NODE_TYPE_NAME(EMPTY),
    NODE_TYPE_NAME(ENSURE),
    NODE_TYPE_NAME(ENUM),
    NODE_TYPE_NAME(EQUAL),
    NODE_TYPE_NAME(EXCLUDE),
    NODE_TYPE_NAME(EXTENDS),
    NODE_TYPE_NAME(EXPORT),
    NODE_TYPE_NAME(FALSE),
    NODE_TYPE_NAME(FINAL),
    NODE_TYPE_NAME(FINALLY),
    NODE_TYPE_NAME(FLOAT),
    NODE_TYPE_NAME(FLOATING_POINT),
    NODE_TYPE_NAME(FOR),
    NODE_TYPE_NAME(FUNCTION),
    NODE_TYPE_NAME(GOTO),
    NODE_TYPE_NAME(GREATER_EQUAL),
    NODE_TYPE_NAME(IDENTIFIER),
    NODE_TYPE_NAME(IF),
    NODE_TYPE_NAME(IMPLEMENTS),
    NODE_TYPE_NAME(IMPORT),
    NODE_TYPE_NAME(IN),
    NODE_TYPE_NAME(INCLUDE),
    NODE_TYPE_NAME(INCREMENT),
    NODE_TYPE_NAME(INLINE),
    NODE_TYPE_NAME(INSTANCEOF),
    NODE_TYPE_NAME(INTEGER),
    NODE_TYPE_NAME(INTERFACE),
    NODE_TYPE_NAME(INVARIANT),
    NODE_TYPE_NAME(IS),
    NODE_TYPE_NAME(LABEL),
    NODE_TYPE_NAME(LESS_EQUAL),
    NODE_TYPE_NAME(LIST),
    NODE_TYPE_NAME(LOGICAL_AND),
    NODE_TYPE_NAME(LOGICAL_OR),
    NODE_TYPE_NAME(LOGICAL_XOR),
    NODE_TYPE_NAME(LONG),
    NODE_TYPE_NAME(MATCH),
    NODE_TYPE_NAME(MAXIMUM),
    NODE_TYPE_NAME(MINIMUM),
    NODE_TYPE_NAME(NAME),
    NODE_TYPE_NAME(NAMESPACE),
    NODE_TYPE_NAME(NATIVE),
    NODE_TYPE_NAME(NEW),
    NODE_TYPE_NAME(NOT_EQUAL),
    NODE_TYPE_NAME(NOT_MATCH),
    //NODE_TYPE_NAME(NULL), -- macro does not work in this case
    { node_t::NODE_NULL, "NULL", __LINE__ },
    NODE_TYPE_NAME(OBJECT_LITERAL),
    NODE_TYPE_NAME(OPTIONAL_MEMBER),
    NODE_TYPE_NAME(PACKAGE),
    NODE_TYPE_NAME(PARAM),
    NODE_TYPE_NAME(PARAMETERS),
    NODE_TYPE_NAME(PARAM_MATCH),
    NODE_TYPE_NAME(POST_DECREMENT),
    NODE_TYPE_NAME(POST_INCREMENT),
    NODE_TYPE_NAME(POWER),
    NODE_TYPE_NAME(PRIVATE),
    NODE_TYPE_NAME(PROGRAM),
    NODE_TYPE_NAME(PROTECTED),
    NODE_TYPE_NAME(PUBLIC),
    NODE_TYPE_NAME(RANGE),
    NODE_TYPE_NAME(REGULAR_EXPRESSION),
    NODE_TYPE_NAME(REQUIRE),
    NODE_TYPE_NAME(REST),
    NODE_TYPE_NAME(RETURN),
    NODE_TYPE_NAME(ROOT),
    NODE_TYPE_NAME(ROTATE_LEFT),
    NODE_TYPE_NAME(ROTATE_RIGHT),
    NODE_TYPE_NAME(SCOPE),
    NODE_TYPE_NAME(SET),
    NODE_TYPE_NAME(SHIFT_LEFT),
    NODE_TYPE_NAME(SHIFT_RIGHT),
    NODE_TYPE_NAME(SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(SHORT),
    NODE_TYPE_NAME(SMART_MATCH),
    NODE_TYPE_NAME(STATIC),
    NODE_TYPE_NAME(STRICTLY_EQUAL),
    NODE_TYPE_NAME(STRICTLY_NOT_EQUAL),
    NODE_TYPE_NAME(STRING),
    NODE_TYPE_NAME(SUPER),
    NODE_TYPE_NAME(SWITCH),
    NODE_TYPE_NAME(SYNCHRONIZED),
    NODE_TYPE_NAME(TEMPLATE),
    NODE_TYPE_NAME(TEMPLATE_HEAD),
    NODE_TYPE_NAME(TEMPLATE_MIDDLE),
    NODE_TYPE_NAME(TEMPLATE_TAIL),
    NODE_TYPE_NAME(THEN),
    NODE_TYPE_NAME(THIS),
    NODE_TYPE_NAME(THROW),
    NODE_TYPE_NAME(THROWS),
    NODE_TYPE_NAME(TRANSIENT),
    NODE_TYPE_NAME(TRUE),
    NODE_TYPE_NAME(TRY),
    NODE_TYPE_NAME(TYPE),
    NODE_TYPE_NAME(TYPEOF),
    NODE_TYPE_NAME(UNDEFINED),
    NODE_TYPE_NAME(USE),
    NODE_TYPE_NAME(VAR),
    NODE_TYPE_NAME(VARIABLE),
    NODE_TYPE_NAME(VAR_ATTRIBUTES),
    NODE_TYPE_NAME(VIDENTIFIER),
    NODE_TYPE_NAME(VOID),
    NODE_TYPE_NAME(VOLATILE),
    NODE_TYPE_NAME(WHILE),
    NODE_TYPE_NAME(WITH),
    NODE_TYPE_NAME(YIELD),
};

/** \brief Define the size of the node type table.
 *
 * This parameter represents the number of node type structures. 
 */
size_t const g_node_type_name_size = sizeof(g_node_type_name) / sizeof(g_node_type_name[0]);


}
// no name namespace


/** \brief Retrieve the type of the node.
 *
 * This function gets the type of the node and returns it. The type
 * is one of the node_t::NODE_... values.
 *
 * Note the value of the node types are not all sequencial. The lower
 * portion used one to one with characters has many sparse places.
 * However, the node constructor ensures that only valid types get
 * used.
 *
 * There are some functions available to convert a certain number of
 * node types. These are used by the compiler and optimizer to
 * implement their functionality.
 *
 * \li to_unknown() -- change any node to NODE_UNKNOWN
 * \li to_as() -- change a NODE_CALL to a NODE_AS
 * \li to_boolean_type_only() -- check whether a node represents NODE_TRUE
 *                               or NODE_FALSE
 * \li to_boolean() -- change to a NODE_TRUE or NODE_FALSE if possible
 * \li to_call() -- change a getter or setter to a NODE_CALL
 * \li to_identifier() -- force various keywords to an identifier
 * \li to_integer() -- force a number to a NODE_INTEGER
 * \li to_floating_point() -- force a number to a NODE_FLOATING_POINT
 * \li to_number() -- change a string to a NODE_FLOATING_POINT
 * \li to_string() -- change a number to a NODE_STRING
 * \li to_videntifier() -- change a NODE_IDENTIFIER to a NODE_VIDENTIFIER
 * \li to_var_attributes() -- change a NODE_VARIABLE to a NODE_VAR_ATTRIBUTES
 *
 * \return The current type of the node.
 *
 * \sa to_unknown()
 * \sa to_as()
 * \sa to_boolean()
 * \sa to_call()
 * \sa to_identifier()
 * \sa to_integer()
 * \sa to_floating_point()
 * \sa to_number()
 * \sa to_string()
 * \sa to_videntifier()
 * \sa to_var_attributes()
 * \sa set_boolean()
 */
node_t node::get_type() const
{
    return f_type;
}


/** \brief Convert the specified type to a string.
 *
 * The type of a node (node_t::NODE_...) can be retrieved as
 * a string using this function. In pretty much all cases this
 * is done whenever an error occurs and not in normal circumstances.
 * It is also used to debug the node tree.
 *
 * Note that if you have a node, you probably want to call
 * get_type_name() instead.
 *
 * \exception incompatible_node_type
 * If the table of node type to name is invalid, then we raise
 * this exception. Also, if the \p type parameter is not a valid
 * type (i.e. NODE_max, or an undefined number such as 999) then
 * this exception is also generated. Calling this function with
 * an invalid should not happen when you use the get_type_name()
 * function since the node constructor prevents the use of invalid
 * node types when creating nodes.
 *
 * \return A null terminated C-like string with the node name.
 *
 * \sa get_type_name()
 */
char const * node::type_to_string(node_t type)
{
#if defined(_DEBUG) || defined(DEBUG)
    {
        // make sure that the node types are properly sorted
        static bool checked = false;
        if(!checked)
        {
            // check only once
            checked = true;
            for(size_t idx = 1; idx < g_node_type_name_size; ++idx)
            {
                if(g_node_type_name[idx].f_type <= g_node_type_name[idx - 1].f_type)
                {
                    // LCOV_EXCL_START
                    // if the table is properly defined then we cannot reach
                    // these lines
                    //
                    std::cerr << "INTERNAL ERROR at offset " << idx
                              << " (line #" << g_node_type_name[idx].f_line
                              << ", node type " << static_cast<uint32_t>(g_node_type_name[idx].f_type)
                              << " vs. " << static_cast<uint32_t>(g_node_type_name[idx - 1].f_type)
                              << "): the g_node_type_name table is not sorted properly. We cannot binary search it."
                              << std::endl;
                    throw internal_error("node type names not properly sorted, cannot properly search for names using a binary search.");
                    // LCOV_EXCL_STOP
                }
            }
        }
    }
#endif

    size_t i(0);
    size_t j(g_node_type_name_size);
    while(i < j)
    {
        size_t p((j - i) / 2 + i);
        int r(static_cast<int>(g_node_type_name[p].f_type) - static_cast<int>(static_cast<node_t>(type)));
        if(r == 0)
        {
            return g_node_type_name[p].f_name;
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

    throw incompatible_node_type(
          "name for node type number "
        + std::to_string(static_cast<int>(type))
        + " not found.");
}


void node::set_type_node(node::pointer_t n)
{
    f_type_node = n;
}


node::pointer_t node::get_type_node() const
{
    return f_type_node.lock();
}


/** \brief Retrieve the type of this node.
 *
 * This function retrieves the type of this node.
 *
 * This function is equivalent to:
 *
 * \code
 *      char const *name(node::type_to_string(node->get_type()));
 * \endcode
 *
 * \return The type of this node as a string.
 *
 * \sa type_to_string()
 */
char const * node::get_type_name() const
{
    return type_to_string(f_type);
}


/** \brief Return true if node represents a number.
 *
 * This function returns true if the node is an integer or a
 * floating point value. This is tested using the node type
 * which should either be NODE_INTEGER or NODE_FLOATING_POINT.
 *
 * Note that means this function returns false on a string that
 * represents a valid number.
 *
 * Note that JavaScript also considered Boolean values and null as
 * valid numbers. To test such, use is_nan() instead.
 *
 * \return true if this node represents a number
 *
 * \sa is_nan()
 * \sa is_integer()
 * \sa is_boolean()
 * \sa is_floating_point()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_number() const
{
    return f_type == node_t::NODE_INTEGER
        || f_type == node_t::NODE_FLOATING_POINT;
}


/** \brief Check whether this node represents a NaN if converted to a number.
 *
 * When converting a node to a number (to_number() function) we accept a
 * certain number of parameters as numbers:
 *
 * \li integers (unchanged)
 * \li float points (unchanged)
 * \li true (1) or false (0)
 * \li null (0)
 * \li strings that represent valid numbers as a whole
 * \li undefined (NaN)
 *
 * \return true if the value could not be converted to a number other than NaN.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_boolean()
 * \sa is_floating_point()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_nan() const
{
    if(f_type == node_t::NODE_STRING)
    {
        return as2js::is_number(f_str);
    }

    return f_type != node_t::NODE_INTEGER
        && f_type != node_t::NODE_FLOATING_POINT
        && f_type != node_t::NODE_TRUE
        && f_type != node_t::NODE_FALSE
        && f_type != node_t::NODE_NULL;
}


/** \brief Check whether a node is an integer.
 *
 * This function checks whether the type of the node is NODE_INTEGER.
 *
 * \return true if the node type is NODE_INTEGER.
 *
 * \sa is_number()
 * \sa is_boolean()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_integer() const
{
    return f_type == node_t::NODE_INTEGER;
}


/** \brief Check whether a node is a floating point.
 *
 * This function checks whether the type of the node is NODE_FLOATING_POINT.
 *
 * \return true if the node type is NODE_FLOATING_POINT.
 *
 * \sa is_number()
 * \sa is_boolean()
 * \sa is_integer()
 * \sa is_nan()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_floating_point() const
{
    return f_type == node_t::NODE_FLOATING_POINT;
}


/** \brief Check whether a node is a Boolean value.
 *
 * This function checks whether the type of the node is NODE_TRUE or
 * NODE_FALSE.
 *
 * \return true if the node type represents a boolean value.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_boolean() const
{
    return f_type == node_t::NODE_TRUE || f_type == node_t::NODE_FALSE;
}


/** \brief Check whether a node represents the true Boolean value.
 *
 * This function checks whether the type of the node is NODE_TRUE.
 *
 * \return true if the node type represents true.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_true() const
{
    return f_type == node_t::NODE_TRUE;
}


/** \brief Check whether a node represents the false Boolean value.
 *
 * This function checks whether the type of the node is NODE_FALSE.
 *
 * \return true if the node type represents false.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_false() const
{
    return f_type == node_t::NODE_FALSE;
}


/** \brief Check whether a node is a string.
 *
 * This function checks whether the type of the node is NODE_STRING.
 *
 * \return true if the node type represents a string value.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_false()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_string() const
{
    return f_type == node_t::NODE_STRING;
}


/** \brief Check whether a node is the special value undefined.
 *
 * This function checks whether the type of the node is NODE_UNDEFINED.
 *
 * \return true if the node type represents the undefined value.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_null()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_undefined() const
{
    return f_type == node_t::NODE_UNDEFINED;
}


/** \brief Check whether a node is the special value null.
 *
 * This function checks whether the type of the node is NODE_NULL.
 *
 * \return true if the node type represents the null value.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_identifier()
 * \sa is_literal()
 */
bool node::is_null() const
{
    return f_type == node_t::NODE_NULL;
}


/** \brief Check whether a node is an identifier.
 *
 * This function checks whether the type of the node is NODE_IDENTIFIER
 * or NODE_VIDENTIFIER.
 *
 * \return true if the node type represents an identifier value.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_literal()
 */
bool node::is_identifier() const
{
    return f_type == node_t::NODE_IDENTIFIER || f_type == node_t::NODE_VIDENTIFIER;
}


/** \brief Check whether this node represents a literal.
 *
 * Literals are:
 *
 * \li true or false
 * \li floating point
 * \li integer
 * \li null
 * \li string
 * \li undefined
 *
 * If this node represents any one of those types, this function
 * returns true.
 *
 * \return true if the node is a literal.
 *
 * \sa is_number()
 * \sa is_integer()
 * \sa is_floating_point()
 * \sa is_nan()
 * \sa is_boolean()
 * \sa is_true()
 * \sa is_false()
 * \sa is_string()
 * \sa is_undefined()
 * \sa is_null()
 * \sa is_identifier()
 */
bool node::is_literal() const
{
    switch(f_type)
    {
    case node_t::NODE_FALSE:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_INTEGER:
    case node_t::NODE_NULL:
    case node_t::NODE_STRING:
    case node_t::NODE_TRUE:
    case node_t::NODE_UNDEFINED:
        return true;

    default:
        return false;

    }
}


/** \brief Check whether a node has side effects.
 *
 * This function checks whether a node, or any of its children, has a
 * side effect.
 *
 * Having a side effect means that the function of the node is to modify
 * something. For example an assignment modifies its destination which
 * is an obvious side effect. The following node types are viewed as
 * having a side effects:
 *
 * \li NODE_ASSIGNMENT[_...] -- all the assignment
 * \li NODE_CALL -- a function call
 * \li NODE_DECREMENT -- the '--' operator
 * \li NODE_DELETE -- the 'delete' operator
 * \li NODE_INCREMENT -- the '++' operator
 * \li NODE_NEW -- the 'new' operator
 * \li NODE_POST_DECREMENT -- the '--' operator
 * \li NODE_POST_INCREMENT -- the '++' operator
 *
 * The test is run against this node and all of its children because if
 * any one node implies a modification, the tree as a whole implies a
 * modification and thus the function must return true.
 *
 * For optimizations, we will still be able to remove nodes wrapping
 * nodes that have side effects. For example the following optimization
 * is perfectly valid:
 *
 * \code
 *      a + (b = 3);
 *      // can be optimized to
 *      b = 3;
 * \endcode
 *
 * The one reason the previous statement may not be optimizable is if
 * 'a' represents an object which has the '+' (addition) operator
 * defined. Anyway, in that case the optimizer sees the following code
 * which cannot be optimized:
 *
 * \code
 *      a.operator_add(b = 3);
 * \endcode
 *
 * \return true if this node has a side effect.
 */
bool node::has_side_effects() const
{
    //
    // Well... I'm wondering if we can really
    // trust this current version.
    //
    // Problem I:
    //    some identifiers can be getters and
    //    they can have side effects; though
    //    a getter should be considered constant
    //    toward the object being read and thus
    //    it should be fine in 99% of cases
    //    [imagine a serial number generator
    //    though...]
    //
    // Problem II:
    //    some operators may not have been
    //    compiled yet and they could have
    //    side effects too; now this is much
    //    less likely a problem because then
    //    the programmer is most certainly
    //    creating a really weird program
    //    with all sorts of side effects that
    //    he wants no one else to know about,
    //    etc. etc. etc.
    //
    // Problem III:
    //    Note that we do not memorize whether
    //    a node has side effects because its
    //    children may change and then side
    //    effects may appear and disappear.
    //

    switch(f_type)
    {
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_ASSIGNMENT_ADD:
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node_t::NODE_ASSIGNMENT_DIVIDE:
    case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node_t::NODE_ASSIGNMENT_MINIMUM:
    case node_t::NODE_ASSIGNMENT_MODULO:
    case node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node_t::NODE_ASSIGNMENT_POWER:
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_ASSIGNMENT_SUBTRACT:
    case node_t::NODE_CALL:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_NEW:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
        return true;

    //case NODE_IDENTIFIER:
    //
    // TODO: Test whether this is a reference to a [sg]etter
    //       function (needs to be compiled already...)
    //    
    //    break;

    default:
        break;

    }

    for(std::size_t idx(0); idx < f_children.size(); ++idx)
    {
        if(f_children[idx] && f_children[idx]->has_side_effects())
        {
            return true;
        }
    }

    return false;
}


//
// This is not needed, plus it is wrong. We instead use the get_type_node()
// and compare those pointers. I keep the function for now, but ti is most
// certainly totally useless.
//
// /** \brief Transform a node of type NODE_TYPE to a string.
//  *
//  * This function transforms a type definition into a string. This can be
//  * used to determine whether two different types are equal without having
//  * to test a complicated set of nodes.
//  *
//  * \note
//  * The definition of this function may need to be in a different
//  * file instead of the node_type.cpp file. It is not linked to
//  * the type of a node, it concerns a high level type definition.
//  *
//  * \exception internal_error
//  * This exception is raised if the input node is not a NODE_TYPE or
//  * the node does not have 0 or 1 child.
//  *
//  * \return A string representing the type of "*" if it cannot be converted.
//  */
// std::string node::type_node_to_string() const
// {
//     if(f_type != node_t::NODE_TYPE)
//     {
//         throw internal_error("node_type.cpp: node::type_node_to_string(): called with a node which is not a NODE_TYPE.");
//     }
// 
//     // any children? (we should have exactly one)
//     switch(get_children_size())
//     {
//     case 0:
//         return "";
// 
//     case 1:
//         break;
// 
//     default:
//         throw internal_error("node_type.cpp: node::type_node_to_string(): called with a NODE_TYPE that has more than one child.");
// 
//     }
// 
//     // we want to use a recursive function
//     class t2s
//     {
//     public:
//         static String convert(node::pointer_t node, bool & unknown)
//         {
//             switch(node->get_type())
//             {
//             case node_t::NODE_IDENTIFIER:
//             case node_t::NODE_VIDENTIFIER:
//             case node_t::NODE_STRING:
//                 return node->get_string();
// 
//             case node_t::NODE_MEMBER:
//                 if(node->get_children_size() != 2)
//                 {
//                     unknown = true;
//                     return "";
//                 }
//                 return convert(node->get_child(0), unknown) + "." + convert(node->get_child(1), unknown);
// 
//             default:
//                 unknown = true;
//                 return "";
// 
//             }
//             /*NOTREACHED*/
//         }
//     };
// 
//     bool unknown(false);
//     String const result(t2s::convert(get_child(0), unknown));
//     if(unknown)
//     {
//         return "*";
//     }
// 
//     return result;
// }


} // namespace as2js
// vim: ts=4 sw=4 et
