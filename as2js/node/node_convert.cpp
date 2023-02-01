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
 * \brief Convert a node object to another type.
 *
 * The conversion functions allow one to convert a certain number of
 * node objects from their current type to a different type.
 *
 * Most node cannot be converted to anything else than the UNKNOWN
 * node type, which is used to <em>delete</em> a node. The various
 * conversion functions defined below let you know what types are
 * accepted by each function.
 *
 * In most cases the conversion functions will return a Boolean
 * value. If false, then the conversion did not happen. You are
 * responsible for checking the result and act on it appropriately.
 *
 * Although a conversion function, the set_boolean() function is
 * actually defined in the node_value.cpp file. It is done that way
 * because it looks very similar to the set_integer(), set_floating_point(),
 * and set_string() functions.
 */


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Transform any node to NODE_UNKNOWN
 *
 * This function marks the node as unknown. Absolutely any node can be
 * marked as unknown. It is specifically used by the compiler and
 * optimizer to cancel nodes that cannot otherwise be deleted at
 * the time they are working on the tree.
 *
 * All the children of an unknown node are ignored too (considered
 * as NODE_UNKNOWN, although they do not all get converted.)
 *
 * To remove all the unknown nodes once the compiler is finished,
 * one can call the clean_tree() function.
 *
 * \note
 * The node must not be locked.
 */
void node::to_unknown()
{
    modifying();

    // whatever the type of node we can always convert it to an unknown
    // node since that's similar to "deleting" the node
    f_type = node_t::NODE_UNKNOWN;
    // clear the node's data to avoid other problems?
}


/** \brief Transform a call in a NODE_AS node.
 *
 * This function transforms a node defined as NODE_CALL into a NODE_AS.
 * The special casting syntax looks exactly like a function call. For
 * this reason the parser returns it as such. The compiler, however,
 * can determine whether the function name is really a function name
 * or if it is a type name. If it is a type, then the tree is changed
 * to represent an AS instruction instead:
 *
 * \code
 *     type ( expression )
 *     expression AS type
 * \endcode
 *
 * \note
 * The node must not be locked.
 *
 * \todo
 * We will need to verify that this is correct and does not introduce
 * other problems. However, remember that we do not use prototypes in
 * our world. We have well defined classes so it should work just fine.
 *
 * \return true if the conversion happens.
 */
bool node::to_as()
{
    modifying();

    // "a call to a getter" may be transformed from CALL to AS
    // because a getter can very much look like a cast (false positive)
    if(node_t::NODE_CALL == f_type)
    {
        f_type = node_t::NODE_AS;
        return true;
    }

    return false;
}


/** \brief Check whether a node can be converted to Boolean.
 *
 * This function is constant and can be used to see whether a node
 * represents true or false without actually converting the node.
 *
 * \li NODE_TRUE -- returned as is
 * \li NODE_FALSE -- returned as is
 * \li NODE_NULL -- returns NODE_FALSE
 * \li NODE_UNDEFINED -- returns NODE_FALSE
 * \li NODE_INTEGER -- returns NODE_TRUE unless the interger is zero
 *                   in which case NODE_FALSE is returned
 * \li NODE_FLOATING_POINT -- returns NODE_TRUE unless the floating point is
 *                     exactly zero in which case NODE_FALSE is returned
 * \li NODE_STRING -- returns NODE_TRUE unless the string is empty in
 *                    which case NODE_FALSE is returned
 * \li Any other node type -- returns NODE_UNDEFINED
 *
 * Note that in this case we completely ignore the content of a string.
 * The strings "false", "0.0", and "0" all represent Boolean 'true'.
 *
 * \return NODE_TRUE, NODE_FALSE, or NODE_UNDEFINED depending on 'this' node
 *
 * \sa to_boolean()
 * \sa set_boolean()
 */
node_t node::to_boolean_type_only() const
{
    switch(f_type)
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        // already a boolean
        return f_type;

    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
        return node_t::NODE_FALSE;

    case node_t::NODE_INTEGER:
        return f_int.get() != 0 ? node_t::NODE_TRUE : node_t::NODE_FALSE;

    case node_t::NODE_FLOATING_POINT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return f_float.get() != 0.0 && !f_float.is_nan() ? node_t::NODE_TRUE : node_t::NODE_FALSE;
#pragma GCC diagnostic pop

    case node_t::NODE_STRING:
        return as2js::is_true(f_str) ? node_t::NODE_TRUE : node_t::NODE_FALSE;

    default:
        // failure (cannot convert)
        return node_t::NODE_UNDEFINED;

    }
    /*NOTREACHED*/
}


/** \brief Convert this node to a Boolean node.
 *
 * This function converts 'this' node to a Boolean node:
 *
 * \li NODE_TRUE -- no conversion
 * \li NODE_FALSE -- no conversion
 * \li NODE_NULL -- converted to NODE_FALSE
 * \li NODE_UNDEFINED -- converted to NODE_FALSE
 * \li NODE_INTEGER -- converted to NODE_TRUE unless it is 0
 *                   in which case it gets converted to NODE_FALSE
 * \li NODE_FLOATING_POINT -- converted to NODE_TRUE unless it is 0.0
 *                     in which case it gets converted to NODE_FALSE
 * \li NODE_STRING -- converted to NODE_TRUE unless the string is empty
 *                    in which case it gets converted to NODE_FALSE
 *
 * Other input types do not get converted and the function returns false.
 *
 * To just test the Boolean value of a node without converting it, call
 * to_boolean_type_only() instead.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeds.
 *
 * \sa to_boolean_type_only()
 * \sa set_boolean()
 */
bool node::to_boolean()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        // already a boolean
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
        f_type = node_t::NODE_FALSE;
        break;

    case node_t::NODE_INTEGER:
        f_type = f_int.get() != 0 ? node_t::NODE_TRUE : node_t::NODE_FALSE;
        break;

    case node_t::NODE_FLOATING_POINT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        f_type = f_float.get() != 0.0 && !f_float.is_nan() ? node_t::NODE_TRUE : node_t::NODE_FALSE;
#pragma GCC diagnostic pop
        break;

    case node_t::NODE_STRING:
        f_type = as2js::is_true(f_str) ? node_t::NODE_TRUE : node_t::NODE_FALSE;
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    return true;
}


/** \brief Convert a getter or setter to a function call.
 *
 * This function is used to convert a getter ot a setter to
 * a function call.
 *
 * A read from a member variable is a getter if the name of
 * the field was actually defined as a 'get' function.
 *
 * A write to a member variable is a setter if the name of
 * the field was actually defined as a 'set' function.
 *
 * \code
 *     class foo_class
 *     {
 *         function get field() { ... }
 *         function set field() { ... }
 *     };
 *
 *     // Convert a getter to a function call
 *     a = foo.field;
 *     a = foo.field_getter();
 *
 *     // Convert a setter to a function call
 *     foo.field = a;
 *     foo.field_setter(a);
 * \endcode
 *
 * The function returns false if 'this' node is not a NODE_MEMBER or
 * a NODE_ASSIGNMENT.
 *
 * \note
 * This function has no way of knowing what's what.
 * It just changes the f_type parameter of this node.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_call()
{
    modifying();

    // getters are transformed from MEMBER to CALL
    // setters are transformed from ASSIGNMENT to CALL
    // binary and unary operators are transformed to CALL
    //
    switch(f_type)
    {
    case node_t::NODE_ADD:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_ASSIGNMENT:   // assignment setter
    case node_t::NODE_MEMBER:       // member getter
        f_type = node_t::NODE_CALL;
        return true;

    default:
        return false;

    }
}


/** \brief Convert this node to a NODE_IDENTIFIER.
 *
 * This function converts the node to an identifier. This is used to
 * transform some keywords back to an identifier.
 *
 * \li NODE_PRIVATE -- "private"
 * \li NODE_PROTECTED -- "protected"
 * \li NODE_PUBLIC -- "public"
 *
 * At this point this is used to transform these keywords in labels.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_identifier()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
        // already an identifier
        return true;

    case node_t::NODE_STRING:
        f_type = node_t::NODE_IDENTIFIER;
        return true;

    case node_t::NODE_DELETE:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("delete");
        return true;

    case node_t::NODE_PRIVATE:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("private");
        return true;

    case node_t::NODE_PROTECTED:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("protected");
        return true;

    case node_t::NODE_PUBLIC:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("public");
        return true;

    case node_t::NODE_ADD:
    case node_t::NODE_ALMOST_EQUAL:
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
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_COMPARE:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_NOT_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SMART_MATCH:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_SUBTRACT:
        {
            std::string const name(operator_to_string(get_type()));
            f_type = node_t::NODE_IDENTIFIER;
            set_string(name);
        }
        return true;

    default:
        // failure (cannot convert)
        return false;

    }
    /*NOTREACHED*/
}


/** \brief Convert this node to a NODE_INTEGER.
 *
 * This function converts the node to an integer number,
 * just like JavaScript would do (outside of the fact that
 * JavaScript only supports floating points...) This means
 * converting the following type of nodes as specified:
 *
 * \li NODE_INTEGER -- no conversion
 * \li NODE_FLOATING_POINT -- convert to integer
 * \li NODE_TRUE -- convert to 1
 * \li NODE_FALSE -- convert to 0
 * \li NODE_NULL -- convert to 0
 * \li NODE_STRING -- convert to integer if valid, zero otherwise (NaN is
 *                    not possible in an integer)
 * \li NODE_UNDEFINED -- convert to 0 (NaN is not possible in an integer)
 *
 * This function converts strings. If the string represents a
 * valid integer, convert to that integer. In this case the full 64 bits
 * are supported. If the string represents a floating point number, then
 * the number is first converted to a floating point, then cast to an
 * integer using the floor() function. If the floating point is too large
 * for the integer, then the maximum or minimum number are used as the
 * result. String that do not represent a number (integer or floating
 * point) are transformed to zero (0). This is a similar behavior to
 * the 'undefined' conversion.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_integer()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_INTEGER:
        return true;

    case node_t::NODE_FLOATING_POINT:
        if(f_float.is_nan() || f_float.is_infinity())
        {
            // the C-like cast would use 0x800...000
            // JavaScript expects zero instead
            f_int.set(0);
        }
        else
        {
            f_int.set(f_float.get()); // C-like cast to integer with a floor() (no rounding)
        }
        break;

    case node_t::NODE_TRUE:
        f_int.set(1);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
    case node_t::NODE_UNDEFINED: // should return NaN, not possible with an integer...
        f_int.set(0);
        break;

    case node_t::NODE_STRING:
        if(as2js::is_integer(f_str))
        {
            f_int.set(as2js::to_integer(f_str));
        }
        else if(as2js::is_floating_point(f_str))
        {
            f_int.set(as2js::to_floating_point(f_str)); // C-like cast to integer with a floor() (no rounding)
        }
        else
        {
            f_int.set(0); // should return NaN, not possible with an integer...
        }
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    f_type = node_t::NODE_INTEGER;
    return true;
}


/** \brief Convert this node to a NODE_FLOATING_POINT.
 *
 * This function converts the node to a floating point number,
 * just like JavaScript would do. This means converting the following
 * type of nodes:
 *
 * \li NODE_INTEGER -- convert to a float
 * \li NODE_FLOATING_POINT -- no conversion
 * \li NODE_TRUE -- convert to 1.0
 * \li NODE_FALSE -- convert to 0.0
 * \li NODE_NULL -- convert to 0.0
 * \li NODE_STRING -- convert to float if valid, otherwise NaN
 * \li NODE_UNDEFINED -- convert to NaN
 *
 * This function converts strings. If the string represents an integer,
 * it will be converted to the nearest floating point number. If the
 * string does not represent a number (including an empty string),
 * then the float is set to NaN.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_floating_point()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_INTEGER:
        f_float.set(f_int.get());
        break;

    case node_t::NODE_FLOATING_POINT:
        return true;

    case node_t::NODE_TRUE:
        f_float.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        f_float.set(0.0);
        break;

    case node_t::NODE_STRING:
        f_float.set(as2js::to_floating_point(f_str));
        break;

    case node_t::NODE_UNDEFINED:
        f_float.set_nan();
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    f_type = node_t::NODE_FLOATING_POINT;
    return true;
}


/** \brief Convert this node to a label.
 *
 * This function converts a NODE_IDENTIFIER node to a NODE_LABEL node.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_label()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
        f_type = node_t::NODE_LABEL;
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    return true;
}


/** \brief Convert this node to a number.
 *
 * This function converts the node to a number pretty much
 * like JavaScript would do, except that literals that represent
 * an exact integers are converted to an integer instead of a
 * floating point.
 *
 * If the node already is an integer or a floating point, then
 * no conversion takes place, but it is considered valid and
 * thus the function returns true.
 *
 * This means converting the following type of nodes:
 *
 * \li NODE_INTEGER -- no conversion
 * \li NODE_FLOATING_POINT -- no conversion
 * \li NODE_TRUE -- convert to 1 (INTEGER)
 * \li NODE_FALSE -- convert to 0 (INTEGER)
 * \li NODE_NULL -- convert to 0 (INTEGER)
 * \li NODE_UNDEFINED -- convert to NaN (FLOATING_POINT)
 * \li NODE_STRING -- converted to a float, NaN if not a valid float,
 *                    however, zero if empty.
 *
 * This function converts strings to a floating point, even if the
 * value represents an integer. It is done that way because JavaScript
 * expects a 'number' and that is expected to be a floating point.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool node::to_number()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_INTEGER:
    case node_t::NODE_FLOATING_POINT:
        break;

    case node_t::NODE_TRUE:
        f_type = node_t::NODE_INTEGER;
        f_int.set(1);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        f_type = node_t::NODE_INTEGER;
        f_int.set(0);
        break;

    case node_t::NODE_UNDEFINED:
        f_type = node_t::NODE_FLOATING_POINT;
        f_float.set_nan();
        break;

    case node_t::NODE_STRING:
        // JavaScript tends to force conversions from stings to numbers
        // when possible (actually it nearly always is, and strings
        // often become NaN as a result... the '+' and '+=' operators
        // are an exception; also relational operators do not convert
        // strings if both the left hand side and the right hand side
        // are strings.)
        f_type = node_t::NODE_FLOATING_POINT;
        f_float.set(as2js::to_floating_point(f_str));
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    return true;
}


/** \brief Transform a node to a string.
 *
 * This function transforms a node from what it is to a string. If the
 * transformation is successful, the function returns true. Note that
 * the function does not throw if the type of 'this' cannot be
 * converted to a string.
 *
 * The nodes that can be converted to a string are:
 *
 * \li NODE_STRING -- unchanged
 * \li NODE_IDENTIFIER -- the identifier is now a string
 * \li NODE_UNDEFINED -- changed to "undefined"
 * \li NODE_NULL -- changed to "null"
 * \li NODE_TRUE -- changed to "true"
 * \li NODE_FALSE -- changed to "false"
 * \li NODE_INTEGER -- changed to a string representation
 * \li NODE_FLOATING_POINT -- changed to a string representation
 *
 * The conversion of a floating point is not one to one compatible with
 * what a JavaScript implementation would otherwise do. This is due to
 * the fact that Java tends to convert floating points in a slightly
 * different way than C/C++. None the less, the results are generally
 * very close (to the 4th decimal digit.)
 *
 * The NaN floating point is converted to the string "NaN".
 *
 * The floating point +0.0 and -0.0 numbers are converted to exactly "0".
 *
 * The floating point +Infinity is converted to the string "Infinity".
 *
 * The floating point -Infinity is converted to the string "-Infinity".
 *
 * Other numbers are converted as floating points with a decimal point,
 * although floating points that represent an integer may be output as
 * an integer.
 *
 * \note
 * The node must not be locked.
 *
 * \return true if the conversion succeeded, false otherwise.
 */
bool node::to_string()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_STRING:
        return true;

    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_TEMPLATE:
    case node_t::NODE_TEMPLATE_HEAD:
    case node_t::NODE_TEMPLATE_MIDDLE:
    case node_t::NODE_TEMPLATE_TAIL:
        // this happens with special identifiers that are strings in the end
        break;

    case node_t::NODE_UNDEFINED:
        f_str = "undefined";
        break;

    case node_t::NODE_NULL:
        f_str = "null";
        break;

    case node_t::NODE_TRUE:
        f_str = "true";
        break;

    case node_t::NODE_FALSE:
        f_str = "false";
        break;

    case node_t::NODE_INTEGER:
        f_str = std::to_string(f_int.get());
        break;

    case node_t::NODE_FLOATING_POINT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    {
        floating_point::value_type const value(f_float.get());
        if(f_float.is_nan())
        {
            f_str = "NaN";
        }
        else if(value == 0.0)
        {
            // make sure it does not become "0.0"
            f_str = "0";
        }
        else if(f_float.is_negative_infinity())
        {
            f_str = "-Infinity";
        }
        else if(f_float.is_positive_infinity())
        {
            f_str = "Infinity";
        }
        else
        {
            f_str = std::to_string(value);
            if(f_str.find('.') != f_str.npos)
            {
                while(f_str.back() == '0')
                {
                    f_str.pop_back();
                }
                if(f_str.back() == '.')
                {
                    f_str.pop_back();
                }
            }
        }
    }
#pragma GCC diagnostic pop
        break;

    default:
        // failure (cannot convert)
        return false;

    }
    f_type = node_t::NODE_STRING;

    return true;
}


/** \brief Transform an identifier into a NODE_VIDENTIFIER.
 *
 * This function is used to transform an identifier in a variable
 * identifier. By default identifiers may represent object names.
 * However, when written between parenthesis, they always represent
 * a variable. This can be important as certain syntax are not
 * at all equivalent:
 *
 * \code
 *    (a).field      // a becomes a NODE_VIDENTIFIER
 *    a.field
 * \endcode
 *
 * In the first case, (a) is transform with the content of variable
 * 'a' and that resulting object is used to access 'field'.
 *
 * In the second case, 'a' itself represents an object and we are accessing
 * that object's 'field' directly.
 *
 * \note
 * Why do we need this distinction? Parenthesis used for grouping are
 * not saved in the resulting tree of nodes. For that reason, at the time
 * we parse that result, we could not distinguish between both
 * expressions. With the NODE_VIDENTIFIER, we can correct that problem.
 *
 * \note
 * The node must not be locked.
 *
 * \exception exception_internal_error
 * This exception is raised if the input node is not a NODE_IDENTIFIER.
 */
void node::to_videntifier()
{
    modifying();

    if(node_t::NODE_IDENTIFIER != f_type)
    {
        throw internal_error("to_videntifier() called with a node other than a \"NODE_IDENTIFIER\" node.");
    }

    f_type = node_t::NODE_VIDENTIFIER;
}


/** \brief Transform a variable into a variable of attributes.
 *
 * When compiling the tree, the code in compiler_variable.cpp may detect
 * that a variable is specifically used to represent a list of attributes.
 * When that happens, the compiler transforms the variable calling
 * this function.
 *
 * The distinction makes it a lot easier to deal with the variable later.
 *
 * \note
 * The node must not be locked.
 *
 * \exception exception_internal_error
 * This exception is raised if 'this' node is not a NODE_VARIABLE.
 */
void node::to_var_attributes()
{
    modifying();

    if(node_t::NODE_VARIABLE != f_type)
    {
        throw internal_error("to_var_attribute() called with a node other than a \"NODE_VARIABLE\" node.");
    }

    f_type = node_t::NODE_VAR_ATTRIBUTES;
}


}
// namespace as2js
// vim: ts=4 sw=4 et
