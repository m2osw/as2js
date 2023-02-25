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
#include    "as2js/compiler.h"

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  COMPILE  ********************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief "Compile" the code, which means optimize and make compatible.
 *
 * The following functions "compile" the code:
 *
 * \li It will optimize everything it can by reducing expressions that
 *     can be computed at "compile" time;
 * \li It transforms advanced features of as2js such as classes into
 *     JavaScript compatible code such as prototypes.
 *
 * On other words, it means that the compiler (1) tries to resolve all
 * the references that are found in the current tree; (2) loads the
 * libraries referenced by the different import instructions which
 * are necessary (or at least seem to be); (3) and run the optimizer
 * against the code at various times.
 *
 * The compiler calls the optimizer for you because it is important in
 * various places and the optimizations applied will vary depending on
 * the compiler changes and further changes may be applied after the
 * optimizations. So on return the tree is expected to be 100% compatible
 * with a JavaScript all browser interpreters and optimized as much as
 * possible to be output as minimized as can be.
 *
 * \param[in,out] root  The root node or program to compile.
 *
 * \return The number of errors generated while compiling. If zero, no errors
 *         so you can proceed with the tree.
 */
int compiler::compile(node::pointer_t & root)
{
    int const save_errcnt(error_count());

    if(root != nullptr)
    {
        // all the "use namespace ... / with ..." currently in effect
        //
        f_scope = root->create_replacement(node_t::NODE_SCOPE);

        if(root->get_type() == node_t::NODE_PROGRAM)
        {
            program(root);
        }
        else if(root->get_type() == node_t::NODE_ROOT)
        {
            node_lock ln(root);
            size_t const max_children(root->get_children_size());
            for(size_t idx(0); idx < max_children; ++idx)
            {
                node::pointer_t child(root->get_child(idx));
                if(child->get_type() == node_t::NODE_PROGRAM)
                {
                    program(child);
                }
            }
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, root->get_position());
            msg << "the compiler::compile() function expected a root or a program node to start with.";
        }

        if(f_options->get_option(options::option_t::OPTION_USER_SCRIPT) != 0
        && !f_result_found)
        {
            message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_NOT_FOUND);
            msg << "standalone expressions missing in user script; return value will be 0.";
        }
    }

    return error_count() - save_errcnt;
}











// note that we search for labels in functions, programs, packages
// [and maybe someday classes, but for now classes can't have
// code and thus no labels]
void compiler::find_labels(node::pointer_t function_node, node::pointer_t n)
{
    // NOTE: function may also be a program or a package.
    switch(n->get_type())
    {
    case node_t::NODE_LABEL:
    {
        node::pointer_t label(function_node->find_label(n->get_string()));
        if(label)
        {
            // TODO: test function type
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, function_node->get_position());
            msg << "label \"" << n->get_string() << "\" defined twice in the same program, package or function.";
        }
        else
        {
            function_node->add_label(n);
        }
    }
        return;

    // sub-declarations and expressions are just skipped
    // decls:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_VAR:
    case node_t::NODE_PACKAGE:    // ?!
    case node_t::NODE_PROGRAM:    // ?!
    // expr:
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
    case node_t::NODE_MEMBER:
    case node_t::NODE_NEW:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
        return;

    default:
        // other nodes may have children we want to check out
        break;

    }

    node_lock ln(n);
    size_t max_children(n->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(n->get_child(idx));
        find_labels(function_node, child);
    }
}


































void compiler::print_search_errors(node::pointer_t name)
{
    // all failed, check whether we have errors...
    if(f_err_flags == SEARCH_ERROR_NONE)
    {
        return;
    }

    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_MATCH, name->get_position());
    msg << "the name \"" << name->get_string() << "\" could not be resolved because:\n";

    if((f_err_flags & SEARCH_ERROR_PRIVATE) != 0)
    {
        msg << "   You cannot access a private class member from outside that very class.\n";
    }
    if((f_err_flags & SEARCH_ERROR_PROTECTED) != 0)
    {
        msg << "   You cannot access a protected class member from outside a class or its derived classes.\n";
    }
    if((f_err_flags & SEARCH_ERROR_PROTOTYPE) != 0)
    {
        msg << "   One or more functions were found, but none matched the input parameters.\n";
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PRIVATE) != 0)
    {
        msg << "   You cannot use the private attribute outside of a package or a class.\n";
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PROTECTED) != 0)
    {
        msg << "   You cannot use the protected attribute outside of a class.\n";
    }
    if((f_err_flags & SEARCH_ERROR_PRIVATE_PACKAGE) != 0)
    {
        msg << "   You cannot access a package private declaration from outside of that package.\n";
    }
}


} // namespace as2js
// vim: ts=4 sw=4 et
