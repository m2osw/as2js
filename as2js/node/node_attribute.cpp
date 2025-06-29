// Copyright (c) 2005-2025  Made to Order Software Corp.  All Rights Reserved
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
#include    "as2js/message.h"

// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implementation of the node class attributes.
 *
 * node objects support a large set of attributes. Attributes can be added
 * and removed from a node. Some attributes are mutually exclusive.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE ATTRIBUTE  *************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Anonymous namespace for attribute internal definitions.
 *
 * The node attributes are organized in groups. In most cases, only one
 * attribute from the same group can be set at a time. Trying to set
 * another attribute from the same group generates an error.
 *
 * The tables defined here are used to determine whether attributes are
 * mutually exclusive.
 *
 * Note that such errors are considered to be bugs in the compiler. The
 * implementation needs to be fixed if such errors are detected.
 */
namespace
{


#define ATTRIBUTE_NAME(name)    [static_cast<int>(attribute_t::NODE_ATTR_##name)] = #name


/** \brief Array of attribute names.
 *
 * This array is used to convert an attribute to a string. It can also
 * be used to convert a string to an attribute.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
char const * g_attribute_names[static_cast<int>(attribute_t::NODE_ATTR_max)] =
{
    ATTRIBUTE_NAME(PUBLIC),
    ATTRIBUTE_NAME(PRIVATE),
    ATTRIBUTE_NAME(PROTECTED),
    ATTRIBUTE_NAME(INTERNAL),
    ATTRIBUTE_NAME(TRANSIENT), // variables only, skip when serializing a class
    ATTRIBUTE_NAME(VOLATILE), // variable only
    ATTRIBUTE_NAME(STATIC),
    ATTRIBUTE_NAME(ABSTRACT),
    ATTRIBUTE_NAME(VIRTUAL),
    ATTRIBUTE_NAME(ARRAY),
    ATTRIBUTE_NAME(INLINE),
    ATTRIBUTE_NAME(REQUIRE_ELSE),
    ATTRIBUTE_NAME(ENSURE_THEN),
    ATTRIBUTE_NAME(NATIVE),
    ATTRIBUTE_NAME(UNIMPLEMENTED),
    ATTRIBUTE_NAME(DEPRECATED),
    ATTRIBUTE_NAME(UNSAFE),
    ATTRIBUTE_NAME(EXTERN),
    ATTRIBUTE_NAME(CONSTRUCTOR),
    ATTRIBUTE_NAME(FINAL),
    ATTRIBUTE_NAME(ENUMERABLE),
    ATTRIBUTE_NAME(TRUE),
    ATTRIBUTE_NAME(FALSE),
    ATTRIBUTE_NAME(UNUSED),
    ATTRIBUTE_NAME(DYNAMIC),
    ATTRIBUTE_NAME(FOREACH),
    ATTRIBUTE_NAME(NOBREAK),
    ATTRIBUTE_NAME(AUTOBREAK),
    ATTRIBUTE_NAME(TYPE),
    ATTRIBUTE_NAME(DEFINED),
};
#pragma GCC diagnostic pop



/** \brief List of attribute groups.
 *
 * The following enumeration defines a set of group attributes. These
 * are used internally to declare the list of attribute groups.
 *
 * \todo
 * Add a class name to this enumeration, even if private, it still
 * makes it a lot safer.
 */
enum
{
    /** \brief Conditional Compilation Group.
     *
     * This group includes the TRUE and FALSE attributes. A statement
     * can be marked as TRUE (compiled in) or FALSE (left out). A
     * statement cannot at the same time be TRUE and FALSE.
     */
    ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION,

    /** \brief Function Type Group.
     *
     * Functions can be marked as ABSTRACT, CONSTRUCTOR, INLINE, NATIVE,
     * STATIC, and VIRTUAL. This group is used to detect whether a function
     * is marked by more than one of these attributes.
     *
     * Note that this group has exceptions:
     *
     * \li A NATIVE CONSTRUCTOR is considered valid.
     * \li A NATIVE VIRTUAL is considered valid.
     * \li A NATIVE STATIC is considered valid.
     * \li A STATIC INLINE is considered valid.
     */
    ATTRIBUTES_GROUP_FUNCTION_TYPE,

    /** \brief Function Contract Group.
     *
     * The function contract includes the REQUIRE ELSE and the
     * ENSURE THEN, both of which cannot be assigned to one
     * function simultaneously.
     *
     * Contracts are taken from the Effel language.
     */
    ATTRIBUTES_GROUP_FUNCTION_CONTRACT,

    /** \brief Switch Type Group.
     *
     * A 'switch' statement can be given a type: FOREACH, NOBREAK,
     * or AUTOBREAK. Only one type can be specified.
     *
     * The AUTOBREAK idea comes from languages such as Ada and
     * Visual BASIC which always break at the end of a case.
     */
    ATTRIBUTES_GROUP_SWITCH_TYPE,

    /** \brief Member Visibility Group.
     *
     * Variable and function members defined in a class can be given a
     * specific visibility of PUBLIC, PRIVATE, or PROTECTED.
     *
     * All the visibilities are mutually exclusive.
     *
     * Note that the visibility capability can either use a direct
     * attribute definition or a 'label' definition (as in C++).
     * The 'label' definition is ignored when a direct attribute is
     * used, in other words, the visibility can be contradictory in
     * that case and the compiler still accepts the entry (TBD.)
     */
    ATTRIBUTES_GROUP_MEMBER_VISIBILITY
};


/** \brief Table of group names.
 *
 * This table defines a set of names for the attribute groups. These
 * are used whenever an error is generated in link with that given
 * group.
 *
 * The index makes use of the group enumeration values.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
char const *g_attribute_groups[] =
{
    [ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION] = "true and false",
    [ATTRIBUTES_GROUP_FUNCTION_TYPE] = "abstract, constructor, inline, native, static, and virtual",
    [ATTRIBUTES_GROUP_FUNCTION_CONTRACT] = "require else and ensure then",
    [ATTRIBUTES_GROUP_SWITCH_TYPE] = "foreach, nobreak, and autobreak",
    [ATTRIBUTES_GROUP_MEMBER_VISIBILITY] = "public, private, and protected"
};
#pragma GCC diagnostic pop

}
// no name namesoace


void node::set_attribute_node(pointer_t n)
{
    f_attribute_node = n;
}


node::pointer_t node::get_attribute_node() const
{
    return f_attribute_node;
}


/** \brief Get the current status of an attribute.
 *
 * This function returns true or false depending on the current status
 * of the specified attribute.
 *
 * The function verifies that the specified attribute (\p a) corresponds to
 * the type of data you are dealing with. If not, an exception is raised.
 *
 * If the attribute was never set, this function returns false.
 *
 * \note
 * All attributes are always considered false by default.
 *
 * \param[in] a  The attribute to retrieve.
 *
 * \return true if the attribute was set, false otherwise.
 *
 * \sa set_attribute()
 * \sa verify_attribute()
 */
bool node::get_attribute(attribute_t const a) const
{
    verify_attribute(a);
    return f_attributes[static_cast<size_t>(a)];
}


/** \brief Set an attribute.
 *
 * This function sets the specified attribute \p a to the specified value
 * \p v in this node object.
 *
 * The function verifies that the specified attribute (\p a) corresponds to
 * the type of data you are dealing with.
 *
 * \param[in] a  The flag to set.
 * \param[in] v  The new value for the flag.
 *
 * \sa set_attribute_tree()
 * \sa get_attribute()
 * \sa verify_attribute()
 * \sa verify_exclusive_attributes()
 */
void node::set_attribute(attribute_t const a, bool const v)
{
    verify_attribute(a);
    if(v)
    {
        // exclusive attributes do not generate an exception, instead
        // we test the return value and if two exclusive attribute flags
        // were to be set simultaneously, we prevent the second one from
        // being set
        //
        if(!verify_exclusive_attributes(a))
        {
            return;
        }
    }
    f_attributes[static_cast<std::size_t>(a)] = v;
}


/** \brief Set an attribute in a whole tree.
 *
 * This function sets the specified attribute \p a to the specified value
 * \p v in this node object and all of its children.
 *
 * The function verifies that the specified attribute (\p a) corresponds to
 * the type of data you are dealing with.
 *
 * \param[in] a  The flag to set.
 * \param[in] v  The new value for the flag.
 *
 * \sa get_attribute()
 * \sa set_attribute()
 * \sa verify_attribute()
 * \sa verify_exclusive_attributes()
 */
void node::set_attribute_tree(attribute_t const a, bool const v)
{
    verify_attribute(a);
    if(!v || verify_exclusive_attributes(a))
    {
        // exclusive attributes do not generate an exception, instead
        // we test the return value and if two exclusive attribute flags
        // were to be set simultaneously, we prevent the second one from
        // being set
        //
        f_attributes[static_cast<std::size_t>(a)] = v;
    }

    // repeat on the children
    //
    for(std::size_t idx(0); idx < f_children.size(); ++idx)
    {
        f_children[idx]->set_attribute_tree(a, v);
    }
}


/** \brief Verify that \p a corresponds to the node type.
 *
 * This function verifies that \p a corresponds to a valid attribute
 * according to the type of this node object.
 *
 * \note
 * At this point attributes can be assigned to any type of node
 * exception a NODE_PROGRAM which only accepts the NODE_ATTR_DEFINED
 * attribute.
 *
 * \exception internal_error
 * If the attribute is not valid for this node type,
 * then this exception is raised.
 *
 * \param[in] a  The attribute to check.
 *
 * \sa set_attribute()
 * \sa get_attribute()
 */
void node::verify_attribute(attribute_t a) const
{
    switch(a)
    {
    // member visibility
    //
    case attribute_t::NODE_ATTR_PUBLIC:
    case attribute_t::NODE_ATTR_PRIVATE:
    case attribute_t::NODE_ATTR_PROTECTED:
    case attribute_t::NODE_ATTR_INTERNAL:
    case attribute_t::NODE_ATTR_TRANSIENT:
    case attribute_t::NODE_ATTR_VOLATILE:

    // function member type
    //
    case attribute_t::NODE_ATTR_STATIC:
    case attribute_t::NODE_ATTR_ABSTRACT:
    case attribute_t::NODE_ATTR_VIRTUAL:
    case attribute_t::NODE_ATTR_ARRAY:
    case attribute_t::NODE_ATTR_INLINE:

    // function contracts
    //
    case attribute_t::NODE_ATTR_REQUIRE_ELSE:
    case attribute_t::NODE_ATTR_ENSURE_THEN:

    // functions/variables accessible from the outside
    //
    case attribute_t::NODE_ATTR_EXTERN:

    // function/variable is defined in your system (execution env.)
    //
    case attribute_t::NODE_ATTR_NATIVE:
    case attribute_t::NODE_ATTR_UNIMPLEMENTED:

    // function/variable will be removed in future releases, do not use
    //
    case attribute_t::NODE_ATTR_DEPRECATED:
    case attribute_t::NODE_ATTR_UNSAFE:

    // operator overload (function member)
    //
    case attribute_t::NODE_ATTR_CONSTRUCTOR:

    // function & member constrains
    //
    case attribute_t::NODE_ATTR_FINAL:
    case attribute_t::NODE_ATTR_ENUMERABLE:

    // conditional compilation
    //
    case attribute_t::NODE_ATTR_TRUE:
    case attribute_t::NODE_ATTR_FALSE:
    case attribute_t::NODE_ATTR_UNUSED:                      // if definition is used, error!

    // class attribute (whether a class can be enlarged at run time)
    //
    case attribute_t::NODE_ATTR_DYNAMIC:

    // switch attributes
    //
    case attribute_t::NODE_ATTR_FOREACH:
    case attribute_t::NODE_ATTR_NOBREAK:
    case attribute_t::NODE_ATTR_AUTOBREAK:
        // TBD -- we will need to see whether we want to limit the attributes
        //        on a per node type basis and how we can do that properly
        if(f_type != node_t::NODE_PROGRAM)
        {
            return;
        }
        break;

    // attributes were defined
    //
    case attribute_t::NODE_ATTR_DEFINED:
        // all nodes can receive this flag
        return;

    case attribute_t::NODE_ATTR_TYPE:
        // the type attribute is limited to nodes that can appear in
        // an expression so we limit to such nodes for now
        //
        switch(f_type)
        {
        case node_t::NODE_ADD:
        case node_t::NODE_ARRAY:
        case node_t::NODE_ARRAY_LITERAL:
        case node_t::NODE_AS:
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
        case node_t::NODE_CALL:
        case node_t::NODE_CONDITIONAL:
        case node_t::NODE_DECREMENT:
        case node_t::NODE_DELETE:
        case node_t::NODE_DIVIDE:
        case node_t::NODE_EQUAL:
        case node_t::NODE_FALSE:
        case node_t::NODE_FLOATING_POINT:
        case node_t::NODE_FUNCTION:
        case node_t::NODE_GREATER:
        case node_t::NODE_GREATER_EQUAL:
        case node_t::NODE_IDENTIFIER:
        case node_t::NODE_IN:
        case node_t::NODE_INCREMENT:
        case node_t::NODE_INSTANCEOF:
        case node_t::NODE_INTEGER:
        case node_t::NODE_IS:
        case node_t::NODE_LESS:
        case node_t::NODE_LESS_EQUAL:
        case node_t::NODE_LIST:
        case node_t::NODE_LOGICAL_AND:
        case node_t::NODE_LOGICAL_NOT:
        case node_t::NODE_LOGICAL_OR:
        case node_t::NODE_LOGICAL_XOR:
        case node_t::NODE_MATCH:
        case node_t::NODE_MAXIMUM:
        case node_t::NODE_MEMBER:
        case node_t::NODE_MINIMUM:
        case node_t::NODE_MODULO:
        case node_t::NODE_MULTIPLY:
        case node_t::NODE_NAME:
        case node_t::NODE_NEW:
        case node_t::NODE_NOT_EQUAL:
        case node_t::NODE_NULL:
        case node_t::NODE_OBJECT_LITERAL:
        case node_t::NODE_POST_DECREMENT:
        case node_t::NODE_POST_INCREMENT:
        case node_t::NODE_POWER:
        case node_t::NODE_PRIVATE:
        case node_t::NODE_PUBLIC:
        case node_t::NODE_RANGE:
        case node_t::NODE_ROTATE_LEFT:
        case node_t::NODE_ROTATE_RIGHT:
        case node_t::NODE_SCOPE:
        case node_t::NODE_SHIFT_LEFT:
        case node_t::NODE_SHIFT_RIGHT:
        case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
        case node_t::NODE_STRICTLY_EQUAL:
        case node_t::NODE_STRICTLY_NOT_EQUAL:
        case node_t::NODE_STRING:
        case node_t::NODE_SUBTRACT:
        case node_t::NODE_SUPER:
        case node_t::NODE_THIS:
        case node_t::NODE_TRUE:
        case node_t::NODE_TYPEOF:
        case node_t::NODE_UNDEFINED:
        case node_t::NODE_VIDENTIFIER:
        case node_t::NODE_VOID:
            return;

        [[unlikely]] default:
            // anything else is an error
            break;

        }
        break;

    [[unlikely]] case attribute_t::NODE_ATTR_max:
        break;

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }

    std::stringstream ss;
    ss << "node \""
       << get_type_name()
       << "\" does not like attribute \""
       << attribute_to_string(a)
       << "\" in node::verify_attribute().";
    throw internal_error(ss.str());
}


/** \brief Verify that we can indeed set an attribute.
 *
 * Many attributes are mutually exclusive. This function checks that
 * only one of a group of attributes gets set.
 *
 * This function is not called if you clear an attribute since in that
 * case the default applies.
 *
 * When attributes are found to be in conflict, it is not an internal
 * error, so instead the function generates an error message and the
 * function returns false. This means the compiler may end up generating
 * more errors than one might want to get.
 *
 * \exception internal_error
 * This exception is raised whenever the parameter \p a is invalid.
 *
 * \param[in] a  The attribute being set.
 *
 * \return true if the attributes are not in conflict.
 *
 * \sa set_attribute()
 */
bool node::verify_exclusive_attributes(attribute_t const a) const
{
    bool conflict(false);
    char const *names;
    switch(a)
    {
    case attribute_t::NODE_ATTR_ARRAY:
    case attribute_t::NODE_ATTR_DEFINED:
    case attribute_t::NODE_ATTR_DEPRECATED:
    case attribute_t::NODE_ATTR_DYNAMIC:
    case attribute_t::NODE_ATTR_ENUMERABLE:
    case attribute_t::NODE_ATTR_EXTERN:
    case attribute_t::NODE_ATTR_FINAL:
    case attribute_t::NODE_ATTR_INTERNAL:
    case attribute_t::NODE_ATTR_TRANSIENT:
    case attribute_t::NODE_ATTR_TYPE:
    case attribute_t::NODE_ATTR_UNSAFE:
    case attribute_t::NODE_ATTR_UNUSED:
    case attribute_t::NODE_ATTR_VOLATILE:
        // these attributes have no conflicts
        return true;

    // function contract
    case attribute_t::NODE_ATTR_REQUIRE_ELSE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ENSURE_THEN)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_CONTRACT];
        break;

    case attribute_t::NODE_ATTR_ENSURE_THEN:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_REQUIRE_ELSE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_CONTRACT];
        break;

    // member visibility
    case attribute_t::NODE_ATTR_PUBLIC:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PRIVATE)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PROTECTED)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    case attribute_t::NODE_ATTR_PRIVATE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PUBLIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PROTECTED)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    case attribute_t::NODE_ATTR_PROTECTED:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PUBLIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PRIVATE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    // function type group
    case attribute_t::NODE_ATTR_ABSTRACT:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NATIVE)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_INLINE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_CONSTRUCTOR:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_INLINE)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_INLINE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NATIVE)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_NATIVE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_INLINE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_UNIMPLEMENTED:
        // at this point, the NATIVE flag may not yet be set (i.e. it can be
        // inherited)
        //if(!f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NATIVE)])
        //{
        //    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_position);
        //    msg << "Attribute unimplemented can only be used with native functions.";
        //    return false;
        //}
        return true;

    case attribute_t::NODE_ATTR_STATIC:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_VIRTUAL:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_INLINE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    // switch type group
    case attribute_t::NODE_ATTR_FOREACH:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NOBREAK)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_AUTOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    case attribute_t::NODE_ATTR_NOBREAK:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FOREACH)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_AUTOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    case attribute_t::NODE_ATTR_AUTOBREAK:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FOREACH)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    // conditional compilation group
    case attribute_t::NODE_ATTR_TRUE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FALSE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION];
        break;

    case attribute_t::NODE_ATTR_FALSE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_TRUE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION];
        break;

    [[unlikely]] case attribute_t::NODE_ATTR_max:
        // this should already have been caught in the verify_attribute() function
        throw internal_error("invalid attribute / flag in node::verify_attribute()"); // LCOV_EXCL_LINE

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing above
    // note: verify_attribute(() is called before this function
    //       and it catches completely invalid attribute numbers
    //       (i.e. negative, larger than NODE_ATTR_max)
    }

    if(conflict)
    {
        // this can be a user error so we emit an error instead of throwing
        //
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_position);
        msg << "Attributes " << names << " are mutually exclusive. Only one of them can be used.";
        return false;
    }

    return true;
}


/** \brief Compare a set of attributes with the current attributes of this node.
 *
 * This function compares the specified set of attributes with the node's
 * attributes. If the sets are equal, then the function returns true.
 * Otherwise the function returns false.
 *
 * This function compares all the attributes, whether or not they are
 * valid for the current node type.
 *
 * \param[in] s  The set of attributes to compare with.
 *
 * \return true if \p s is equal to the node attributes.
 */
bool node::compare_all_attributes(attribute_set_t const& s) const
{
    return f_attributes == s;
}


/** \brief Convert an attribute to a string.
 *
 * This function converts an attribute to a string. This is most often used
 * to print out an error about an attribute.
 *
 * \param[in] attr  The attribute to convert to a string.
 *
 * \return A static string pointer representing the attribute.
 */
char const *node::attribute_to_string(attribute_t const attr)
{
    if(static_cast<int>(attr) < 0
    || attr >= attribute_t::NODE_ATTR_max) [[unlikely]]
    {
        throw internal_error("unknown attribute number (out of range) in node::attribute_to_string().");
    }
#ifdef _DEBUG
    if(g_attribute_names[static_cast<std::size_t>(attr)] == nullptr) [[unlikely]]
    {
        throw internal_error(
              "attribute number "
            + std::to_string(static_cast<int>(attr))
            + " not defined in our array of strings.");
    }
#endif

    return g_attribute_names[static_cast<int>(attr)];
}



} // namespace as2js
// vim: ts=4 sw=4 et
