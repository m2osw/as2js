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



node::pointer_t compiler::directive_list(node::pointer_t directive_list_node, bool top_list)
{
    size_t const p(f_scope->get_children_size());

    // TODO: should we go through the list a first time
    //       so we get the list of namespaces for these
    //       directives at once; so in other words you
    //       could declare the namespaces in use at the
    //       start or the end of this scope and it works
    //       the same way...

    size_t const max_children(directive_list_node->get_children_size());

    // get rid of any declaration marked false
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(directive_list_node->get_child(idx));
        if(get_attribute(child, attribute_t::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }

    bool no_access(false);
    node::pointer_t end_list;

    // compile each directive one by one...
    {
        node_lock ln(directive_list_node);
        for(size_t idx(0); idx < max_children; ++idx)
        {
            node::pointer_t child(directive_list_node->get_child(idx));
            if(!no_access && end_list)
            {
                // err only once on this one
                no_access = true;
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INACCESSIBLE_STATEMENT, child->get_position());
                msg << "code is not accessible after a break, continue, goto, throw or return statement.";
            }

            if(top_list && f_result_found)
            {
                switch(child->get_type())
                {
                case node_t::NODE_FUNCTION:
                    break;

                default:
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, child->get_position());
                    msg << "a user script cannot include more than one standalone expression.";
                    break;

                }
            }
            switch(child->get_type())
            {
            case node_t::NODE_PACKAGE:
                // there is nothing to do on those
                // until users reference them...
                break;

            case node_t::NODE_DIRECTIVE_LIST:
                // Recursive!
                //
                end_list = directive_list(child);

                // TODO: we need a real control flow
                //       information to know whether this
                //       latest list had a break, continue,
                //       goto or return statement which
                //       was (really) breaking us too.
                break;

            case node_t::NODE_LABEL:
                // labels do not require any compile whatever...
                break;

            case node_t::NODE_VAR:
                var(child);
                break;

            case node_t::NODE_WITH:
                with(child);
                break;

            case node_t::NODE_USE: // TODO: should that move in a separate loop?
                use_namespace(child);
                break;

            case node_t::NODE_GOTO:
                goto_directive(child);
                end_list = child;
                break;

            case node_t::NODE_FOR:
                for_directive(child);
                break;

            case node_t::NODE_SWITCH:
                switch_directive(child);
                break;

            case node_t::NODE_CASE:
                case_directive(child);
                break;

            case node_t::NODE_DEFAULT:
                default_directive(child);
                break;

            case node_t::NODE_IF:
                if_directive(child);
                break;

            case node_t::NODE_WHILE:
                while_directive(child);
                break;

            case node_t::NODE_DO:
                do_directive(child);
                break;

            case node_t::NODE_THROW:
                throw_directive(child);
                end_list = child;
                break;

            case node_t::NODE_TRY:
                try_directive(child);
                break;

            case node_t::NODE_CATCH:
                catch_directive(child);
                break;

            case node_t::NODE_FINALLY:
                finally(child);
                break;

            case node_t::NODE_BREAK:
            case node_t::NODE_CONTINUE:
                break_continue(child);
                end_list = child;
                break;

            case node_t::NODE_ENUM:
                enum_directive(child);
                break;

            case node_t::NODE_FUNCTION:
                function(child);
                break;

            case node_t::NODE_RETURN:
                end_list = return_directive(child);
                break;

            case node_t::NODE_CLASS:
            case node_t::NODE_INTERFACE:
                // TODO: any non-intrinsic function or
                //       variable member referenced in
                //       a class requires that the
                //       whole class be assembled.
                //       (Unless we can just assemble
                //       what the user accesses.)
                //
                class_directive(child);
                break;

            case node_t::NODE_IMPORT:
                import(child);
                break;

            // standalone expressions
            //
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
            case node_t::NODE_LIST:
            case node_t::NODE_MEMBER:
            case node_t::NODE_NEW:
            case node_t::NODE_POST_DECREMENT:
            case node_t::NODE_POST_INCREMENT:
                expression(child);
                break;

            // expressions that represent a return value which are allowed
            // at the very end of a script (otherwise, the result is lost)
            // so we allow one of those and only if the .ajs is considered
            // to be a user script (i.e. not in a standard package)
            //
            case node_t::NODE_ADD:
            case node_t::NODE_BITWISE_AND:
            case node_t::NODE_BITWISE_NOT:
            case node_t::NODE_DIVIDE:
            case node_t::NODE_MODULO:
            case node_t::NODE_MULTIPLY:
            case node_t::NODE_BITWISE_OR:
            case node_t::NODE_POWER:
            case node_t::NODE_ROTATE_LEFT:
            case node_t::NODE_ROTATE_RIGHT:
            case node_t::NODE_SHIFT_LEFT:
            case node_t::NODE_SHIFT_RIGHT:
            case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
            case node_t::NODE_SUBTRACT:
            case node_t::NODE_BITWISE_XOR:
                if(!top_list)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, child->get_position());
                    msg << "standalone expressions are not allowed outside of the top declaration of a user script; directive node \""
                        << child->get_type_name()
                        << "\" is not allowed here.";
                }
                else if(f_options->get_option(option_t::OPTION_USER_SCRIPT) == 0)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, child->get_position());
                    msg << "standalone expressions are not allowed outside of a user script; directive node \""
                        << child->get_type_name()
                        << "\" is not allowed here.";
                }
                else
                {
                    f_result_found = true;
                    expression(child);
                }
                break;

            case node_t::NODE_UNKNOWN:
                // ignore nodes marked as unknown ("nearly deleted")
                break;

            default:
                // TODO: handle all directives so we can generate a cleaner
                //       error message when finding something which is not
                //       expected here
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, child->get_position());
                    msg << "directive node \""
                        << child->get_type_name()
                        << "\" not yet handled in compiler::directive_list().";
                }
                break;

            }

            if(end_list && idx + 1 < max_children)
            {
                node::pointer_t next(directive_list_node->get_child(idx + 1));
                if(next->get_type() == node_t::NODE_CASE
                || next->get_type() == node_t::NODE_DEFAULT)
                {
                    // process can continue with another case or default
                    // statement following a return, throw, etc.
                    end_list.reset();
                }
            }
        }
    }

    // TODO: this code is not going to be hit because I don't add the
    //       variables to the directive list anymore...
    //
    // The node may be a PACKAGE node in which case the "new variables"
    // does not apply (TODO: make sure of that!)
    //
    if(directive_list_node->get_type() == node_t::NODE_DIRECTIVE_LIST
    && directive_list_node->get_flag(flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES))
    {
        std::size_t const max_variables(directive_list_node->get_variable_size());
        for(std::size_t idx(0); idx < max_variables; ++idx)
        {
            node::pointer_t variable_node(directive_list_node->get_variable(idx));
            node::pointer_t var_parent(variable_node->get_parent());
            if(var_parent != nullptr
            && var_parent->get_flag(flag_t::NODE_VARIABLE_FLAG_TOADD))
            {
                // TBD: is that just the var declaration and no
                //      assignment? because the assignment needs to
                //      happen at the proper time!!!
                //
                var_parent->set_flag(flag_t::NODE_VARIABLE_FLAG_TOADD, false);
                directive_list_node->insert_child(0, var_parent); // insert at the start!
            }
        }
        directive_list_node->set_flag(flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES, false);
    }

    // go through the f_scope list and remove the "use namespace" that
    // were added while working on the items of this list
    // (why?!? because those are NOT like in C++, they are standalone
    // instructions... weird!)
    size_t max_use_namespace(f_scope->get_children_size());
    while(p < max_use_namespace)
    {
        max_use_namespace--;
        f_scope->delete_child(max_use_namespace);
    }

    return end_list;
}




} // namespace as2js
// vim: ts=4 sw=4 et
