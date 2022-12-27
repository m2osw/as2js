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
#include    "as2js/message.h"


// C++
//
#include    <algorithm>
#include    <sstream>
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implement the basic node functions.
 *
 * This file includes the node allocation, switch operator, position,
 * links, variables, and label.
 *
 * Other parts are in other files. It was broken up as the node object
 * implementation is quite large.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE  ***********************************************************/
/**********************************************************************/
/**********************************************************************/



/** \brief Initialize a node.
 *
 * This function initializes a new node. The specified type is assigned to
 * the new node as expected.
 *
 * If the \p type parameter does not represent a valid type of node, then
 * the function throws. This means only valid type of nodes can be created.
 *
 * Once created, a node representing a literal can have its value defined
 * using one of the set_...() functions. Note that the set_boolean()
 * function is a special case which converts the node to either NODE_TRUE
 * or NODE_FALSE.
 *
 * It is also expected that you will set the position of the token using
 * the set_position() function.
 *
 * \note
 * At this time we accept all the different types at creation time. We
 * may restrict this later to only nodes that are expected to be created
 * in this way. For example, a NODE_VIDENTIFIER cannot be created directly,
 * instead it is expected that you would create a NODE_IDENTIFIER and then
 * call the to_videntifier() function to conver the node.
 *
 * \exception incompatible_node_type
 * This exception is raised of the specified type does not correspond to
 * one of the allowed node_t::NODE_... definitions.
 *
 * \param[in] type  The type of node to create.
 *
 * \sa to_videntifier()
 * \sa set_boolean()
 * \sa set_integer()
 * \sa set_floating_point()
 * \sa set_string()
 * \sa set_position()
 */
node::node(node_t type)
    : f_type(type)
{
    switch(type)
    {
    case node_t::NODE_EOF:
    case node_t::NODE_UNKNOWN:
    case node_t::NODE_ADD:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_GREATER:
    case node_t::NODE_LESS:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_MEMBER:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_ABSTRACT:
    case node_t::NODE_ARRAY:
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_AS:
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
    case node_t::NODE_ATTRIBUTES:
    case node_t::NODE_AUTO:
    case node_t::NODE_BOOLEAN:
    case node_t::NODE_BREAK:
    case node_t::NODE_BYTE:
    case node_t::NODE_CALL:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CHAR:
    case node_t::NODE_CLASS:
    case node_t::NODE_COMPARE:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_DO:
    case node_t::NODE_DOUBLE:
    case node_t::NODE_ELSE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_ENUM:
    case node_t::NODE_ENSURE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_EXPORT:
    case node_t::NODE_FALSE:
    case node_t::NODE_FINAL:
    case node_t::NODE_FINALLY:
    case node_t::NODE_FLOAT:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_FOR:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_GOTO:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_IF:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_IMPORT:
    case node_t::NODE_IN:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INLINE:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_INTEGER:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_INVARIANT:
    case node_t::NODE_IS:
    case node_t::NODE_LABEL:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LIST:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_LONG:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_NAME:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_NATIVE:
    case node_t::NODE_NEW:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_NOT_MATCH:
    case node_t::NODE_NULL:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_PACKAGE:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REQUIRE:
    case node_t::NODE_REST:
    case node_t::NODE_RETURN:
    case node_t::NODE_ROOT:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SET:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SMART_MATCH:
    case node_t::NODE_SHORT:
    case node_t::NODE_STATIC:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_STRING:
    case node_t::NODE_SUPER:
    case node_t::NODE_SWITCH:
    case node_t::NODE_SYNCHRONIZED:
    case node_t::NODE_THEN:
    case node_t::NODE_THIS:
    case node_t::NODE_THROW:
    case node_t::NODE_THROWS:
    case node_t::NODE_TRANSIENT:
    case node_t::NODE_TRUE:
    case node_t::NODE_TRY:
    case node_t::NODE_TYPE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_USE:
    case node_t::NODE_VAR:
    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_VOLATILE:
    case node_t::NODE_WHILE:
    case node_t::NODE_WITH:
    case node_t::NODE_YIELD:
        break;

    // WARNING: we use default here because some people may call the
    //          function with something other than a properly defined
    //          node_t type
    default:
        // ERROR: some values are not valid as a type
        throw incompatible_node_type("invalid type used to create a node.");

    }
}


/** \brief Verify that a node is clean when deleting it.
 *
 * This function ensures that a node is clean, as in, not locked,
 * when it gets deleted.
 *
 * If we properly make use of the node_lock, then a node cannot get
 * deleted until all the locks get canceled with an unlock() call.
 */
node::~node()
{
    if(f_lock > 0)
    {
        // LCOV_EXCL_START
        // This should never happen.
        //
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_ALLOWED);
        msg << "a node got deleted while still locked.";
        std::terminate();
        // LCOV_EXCL_STOP
    }
}



/**********************************************************************/
/**********************************************************************/
/***  NODE SWITCH  ****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Retrieve the switch operator.
 *
 * A switch statement can be constrained to use a specific operator
 * using the with() syntax as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * This operator is saved in the switch node and can later be retrieved
 * with this function.
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised.
 *
 * \return The operator of the switch statement, or NODE_UNKNOWN if undefined.
 *
 * \sa set_switch_operator()
 */
node_t node::get_switch_operator() const
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw internal_error("get_switch_operator() called on a node which is not a switch node.");
    }

    return f_switch_operator;
}


/** \brief Set the switch statement operator.
 *
 * This function saves the operator defined following the switch statement
 * using the with() instruction as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * The currently supported operators are:
 *
 * \li NODE_UNKNOWN -- remove the operator
 * \li NODE_STRICTLY_EQUAL -- "===", this is considered the default
 *                                   behavior for a JavaScript switch()
 * \li NODE_EQUAL -- "=="
 * \li NODE_NOT_EQUAL -- "!="
 * \li NODE_STRICTLY_NOT_EQUAL -- "!=="
 * \li NODE_MATCH -- "~="
 * \li NODE_IN -- "in", this makes use of ranges
 * \li NODE_IS -- "is"
 * \li NODE_AS -- "as"
 * \li NODE_INSTANCEOF -- "instanceof"
 * \li NODE_LESS -- "<"
 * \li NODE_LESS_EQUAL -- "<="
 * \li NODE_GREATER -- ">"
 * \li NODE_GREATER_EQUAL -- ">="
 * \li NODE_DEFAULT -- this is the default label case
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised. It will also raise this exception
 * if the specified operator is not an operator supported by the
 * switch statement.
 *
 * \param[in] op  The new operator to save in this switch statement.
 *
 * \sa get_switch_operator()
 */
void node::set_switch_operator(node_t op)
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw internal_error("set_switch_operator() called on a node which is not a switch node.");
    }

    switch(op)
    {
    case node_t::NODE_UNKNOWN:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_EQUAL:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_MATCH:
    case node_t::NODE_IN:
    case node_t::NODE_IS:
    case node_t::NODE_AS:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_DEFAULT:
        break;

    default:
        throw internal_error("set_switch_operator() called with an operator which is not valid for switch.");

    }

    f_switch_operator = op;
}


/**********************************************************************/
/**********************************************************************/
/***  NODE POSITION  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Create a clone of a basic node.
 *
 * This function creates a new node which is a copy of this node.
 * The function really only works with basic nodes, namely,
 * literals.
 *
 * This function cannot be used to create a copy of a node that
 * has children or other pointers.
 *
 * \return A new node pointer.
 *
 * \sa create_replacement()
 */
node::pointer_t node::clone_basic_node() const
{
    node::pointer_t n(new node(f_type));

    // this is why we want to have a function instead of doing new node().
    n->f_type_node = f_type_node;
    n->f_flags = f_flags;
    n->f_attribute_node = f_attribute_node;
    n->f_attributes = f_attributes;
    n->f_switch_operator = f_switch_operator;
    //n->f_lock = f_lock; -- that would not make any sense here
    n->f_position = f_position;
    //n->f_param_depth = f_param_depth; -- specific to functions
    //n->f_param_index = f_param_index;
    //n->f_parent = f_parent; -- tree parameters cannot be changed here
    //n->f_offset = f_offset;
    //n->f_children = f_children;
    n->f_instance = f_instance;
    n->f_goto_enter = f_goto_enter;
    n->f_goto_exit = f_goto_exit;
    n->f_variables = f_variables;
    n->f_labels = f_labels;

    switch(f_type)
    {
    case node_t::NODE_FALSE:
    case node_t::NODE_TRUE:
    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
        break;

    case node_t::NODE_FLOATING_POINT:
        n->f_float = f_float;
        break;

    case node_t::NODE_INTEGER:
        n->f_int = f_int;
        break;

    case node_t::NODE_STRING:
    case node_t::NODE_REGULAR_EXPRESSION:
        n->f_str = f_str;
        break;

    //case node_t::NODE_OBJECT_LITERAL: -- this one has children... TBD

    default:
        throw internal_error("node.cpp: clone_basic_node(): called with a node which is not considered to be a basic node.");

    }

    return n;
}


/** \brief Create a new node with the given type.
 *
 * This function creates a new node that is expected to be used as a
 * replacement of this node.
 *
 * Note that the input node does not get modified by this call.
 *
 * This is similar to creating a node directly and then setting up the
 * position of the new node to the position information of 'this' node.
 * In other words, a short hand for this:
 *
 * \code
 *      node::pointer_t n(new node(type));
 *      n->set_position(node->get_position());
 * \endcode
 *
 * \param[in] type  The type of the new node.
 *
 * \return A new node pointer.
 *
 * \sa set_position()
 * \sa clone_basic_node()
 */
node::pointer_t node::create_replacement(node_t type) const
{
    // TBD: should we limit the type of replacement nodes?
    node::pointer_t n(new node(type));

    // this is why we want to have a function instead of doing new node().
    n->f_position = f_position;

    return n;
}


/** \brief Change the position of the node.
 *
 * As you are reading a file, a position object gets updated. That position
 * object represents the location where different token are found in the
 * source files. It is saved in a node as it is created to represent the
 * position where the data was found. This helps in indicating to the user
 * where an error occurred.
 *
 * The position used as input can later change as the node keeps a copy of
 * the parameter passed to it.
 *
 * The position can later be retrieved with the get_position() function.
 *
 * When the compiler creates new nodes as required, it generally will make
 * use of the create_replacement() function which creates a new node with
 * a new type, but keeps the position information of the old node.
 *
 * \param[in] position  The new position to copy in this node.
 *
 * \sa get_position()
 */
void node::set_position(position const & position)
{
    f_position = position;
}


/** \brief The position of the node.
 *
 * This function returns a reference to the position of the node.
 * The position represents the filename, line number, character position,
 * function name, etc. where this specific node was read. It can be used
 * to print out the line to the user and to show him exactly where the
 * error occurred.
 *
 * This position can be changed with the set_position() function. By
 * default a node has a default position: no file name, no function name,
 * and positions are all set to 1.
 *
 * \return The position of this node.
 *
 * \sa set_position()
 */
position const & node::get_position() const
{
    return f_position;
}


// /**********************************************************************/
// /**********************************************************************/
// /***  NODE LINK  ******************************************************/
// /**********************************************************************/
// /**********************************************************************/
// 
// 
// /** \brief Save a link in this node.
//  *
//  * This function saves a link pointer in this node. It can later be
//  * retrieved using the get_link() function.
//  *
//  * If a link was already defined at that offset, the function raises
//  * an exception and the existing offset is not modified.
//  *
//  * It is possible to clear a link by passing an empty smart pointer
//  * down (i.e. pass nullptr). If you first clear a link in this way,
//  * you can then replace it with another pointer.
//  *
//  * \code
//  *     // do not throw because we reset the link first:
//  *     node->set_link(node::link_t::LINK_TYPE, nullptr);
//  *     node->set_link(node::link_t::LINK_TYPE, link);
//  * \endcode
//  *
//  * Links are used to save information about a node such as its
//  * type and attributes.
//  *
//  * \note
//  * Links are saved as full smart pointers, not weak pointers. This means
//  * a node that references another in this way may generate loops that
//  * will not easily break when trying to release the whole tree.
//  *
//  * \note
//  * The node must not be locked.
//  *
//  * \exception out_of_range
//  * The index is out of range. Links make use of a very few predefined
//  * indexes such as node::link_t::LINK_ATTRIBUTES. However,
//  * node::link_t::LINK_max cannot be used as an index.
//  *
//  * \exception already_defined
//  * The link at that index is already defined and the function was called
//  * anyway. This is an internal error because you should check whether the
//  * value was already defined and if so use that value.
//  *
//  * \param[in] index  The index of the link to save.
//  * \param[in] link  A smart pointer to the link.
//  *
//  * \sa get_link()
//  */
// void node::set_link(link_t index, pointer_t link)
// {
//     modifying();
// 
//     if(index >= link_t::LINK_max)
//     {
//         throw out_of_range("set_link() called with an index out of bounds.");
//     }
// 
//     // make sure the size is reserved on first set
//     if(f_link.empty())
//     {
//         f_link.resize(static_cast<vector_of_pointers_t::size_type>(link_t::LINK_max));
//     }
// 
//     if(link)
//     {
//         // link already set?
//         if(f_link[static_cast<size_t>(index)].lock())
//         {
//             throw already_defined("a link was set twice at the same offset");
//         }
// 
//         f_link[static_cast<size_t>(index)] = link;
//     }
//     else
//     {
//         f_link[static_cast<size_t>(index)].reset();
//     }
// }
// 
// 
// /** \brief Retrieve a link previously saved with set_link().
//  *
//  * This function returns a pointer to a link that was previously
//  * saved in this node using the set_link() function.
//  *
//  * Links are used to save information about a node such as its
//  * type and attributes.
//  *
//  * The function may return a null pointer. You are responsible
//  * for checking the validity of the link.
//  *
//  * \exception out_of_range
//  * The index is out of range. Links make use of a very few predefined
//  * indexes such as node::link_t::LINK_ATTRIBUTES. However,
//  * node::link_t::LINK_max cannot be used as an index.
//  *
//  * \param[in] index  The index of the link to retrieve.
//  *
//  * \return A smart pointer to this link node.
//  */
// node::pointer_t node::get_link(link_t index)
// {
//     if(index >= link_t::LINK_max)
//     {
//         throw out_of_range("get_link() called with an index out of bounds.");
//     }
// 
//     if(f_link.empty())
//     {
//         return node::pointer_t();
//     }
// 
//     return f_link[static_cast<size_t>(index)].lock();
// }
// 

/**********************************************************************/
/**********************************************************************/
/***  NODE GOTO  ******************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Retrieve the "Goto Enter" pointer.
 *
 * This function returns a pointer to the "Goto Enter" node. The
 * pointer may be null.
 *
 * \return The "Goto Enter" pointer or nullptr.
 */
node::pointer_t node::get_goto_enter() const
{
    return f_goto_enter.lock();
}


/** \brief Retrieve the "Goto Exit" pointer.
 *
 * This function returns a pointer to the "Goto Exit" node. The
 * pointer may be null.
 *
 * \return The "Goto Exit" pointer or nullptr.
 */
node::pointer_t node::get_goto_exit() const
{
    return f_goto_exit.lock();
}


/** \brief Define the "Goto Enter" pointer.
 *
 * This function saves the specified \p node pointer as the
 * "Goto Enter" node. The pointer may be null.
 *
 * \param[in] node  The "Goto Enter" pointer.
 */
void node::set_goto_enter(pointer_t n)
{
    f_goto_enter = n;
}


/** \brief Define the "Goto Exit" pointer.
 *
 * This function saves the specified \p node pointer as the
 * "Goto Exit" node. The pointer may be null.
 *
 * \param[in] node  The "Goto Exit" pointer.
 */
void node::set_goto_exit(pointer_t n)
{
    f_goto_exit = n;
}




/**********************************************************************/
/**********************************************************************/
/***  NODE VARIABLE  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Add a variable to this node.
 *
 * A node can hold pointers to variable nodes. This is used to
 * handle variable scopes properly. Note that the \p variable
 * parameter must be a node of type NODE_VARIABLE.
 *
 * \note
 * This is not an execution environment and as such the variables are
 * simply added one after another (not sorted, no attempt to later
 * retrieve variables by name.) This may change in the future though.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables. For that purpose, we
 * need to know what those types are.
 *
 * \exception incompatible_node_type
 * This exception is raised if the \p variable parameter is not of type
 * NODE_VARIABLE.
 *
 * \param[in] variable  The variable to be added.
 *
 * \sa get_variable()
 * \sa get_variable_size()
 */
void node::add_variable(pointer_t variable)
{
    if(node_t::NODE_VARIABLE != variable->f_type)
    {
        throw incompatible_node_type("the variable parameter of the add_variable() function must be a \"NODE_VARIABLE\".");
    }
    // TODO: test the destination (i.e. this) to make sure only valid nodes
    //       accept variables; make it a separate function as all the
    //       variable functions should call it!

    f_variables.push_back(variable);
}


/** \brief Retrieve the number of variables defined in this node.
 *
 * A node can hold variable pointers. This is used to handle variable
 * scopes properly.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables.
 *
 * \return The number of variables currently held in this node.
 *
 * \sa add_variable()
 * \sa get_variable()
 */
size_t node::get_variable_size() const
{
    return f_variables.size();
}


/** \brief Retrieve the variable at the specified index.
 *
 * This function retrieves the variable at the specified index. If the
 * index is out of the variable array bounds, then the function raises
 * an error.
 *
 * The current boundaries are from 0 to get_variable_size() - 1. This
 * set may be empty if no variables were added to this node.
 *
 * This function will not return a null pointer. An index out of range
 * raises an exception instead.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables.
 *
 * \todo
 * The returned pointer may be a null pointer since we use a weak
 * pointer for variables.
 *
 * \param[in] index  The index of the variable to retrieve.
 *
 * \return A pointer to the specified variable or a null pointer.
 *
 * \sa add_variable()
 * \sa get_variable_size()
 */
node::pointer_t node::get_variable(int index) const
{
    if(static_cast<std::size_t>(index) >= f_variables.size())
    {
        throw out_of_range("get_variable() called with an out of range index.");
    }
    return f_variables.at(index).lock();
}


/**********************************************************************/
/**********************************************************************/
/***  NODE LABEL  *****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Add a label to a function.
 *
 * This function adds a label to this function node. Labels are saved
 * using a map so we can quickly find them.
 *
 * \note
 * After a label was added to a function, its name should never get
 * modified or it will be out of synchronization with the function.
 *
 * \exception incompatible_node_type
 * If this function is called with objects other than a NODE_LABEL
 * as the label parameter and a NODE_FUNCTION as 'this' parameter,
 * then this exception is raised.
 *
 * \exception incompatible_node_data
 * If the node representing the label does not have a valid string
 * attached to it (i.e. if it is empty) then this exception is
 * raised.
 *
 * \exception already_defined
 * If the label was already defined, then this exception is raised.
 * Within one function each label must be unique, however, sub-functions
 * have their own scope and thus can be a label with the same name as
 * a label in their parent function.
 *
 * \param[in] label  A smart pointer to the label node to add.
 *
 * \sa find_label()
 */
void node::add_label(pointer_t label)
{
    if(node_t::NODE_LABEL != label->f_type
    || node_t::NODE_FUNCTION != f_type)
    {
        throw incompatible_node_type("invalid type of node to call add_label() with.");
    }
    if(label->f_str.empty())
    {
        throw incompatible_node_data("a label without a valid name cannot be added to a function.");
    }
    if(f_labels.find(label->f_str) != f_labels.end())
    {
        throw already_defined("a label with the same name is already defined in this function.");
    }

    f_labels[label->f_str] = label;
}


/** \brief Find a label previously added with the add_label() function.
 *
 * This function checks whether a label was defined in this function. If
 * so, then its smart pointer gets returned.
 *
 * The \p name parameter represents the name of the label exactly. The
 * returned label will have the same name.
 *
 * \param[in] name  The name of the label to retrieve.
 *
 * \return A pointer to the label if it exists, a null pointer otherwise.
 *
 * \sa add_label()
 */
node::pointer_t node::find_label(std::string const& name) const
{
    map_of_weak_pointers_t::const_iterator it(f_labels.find(name));
    return it == f_labels.end() ? pointer_t() : it->second.lock();
}




}
// namespace as2js

// vim: ts=4 sw=4 et
