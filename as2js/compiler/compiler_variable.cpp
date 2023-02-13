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
#include    "as2js/compiler.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{





// we can simplify constant variables with their content whenever it is
// a string, number or other non-dynamic constant
bool compiler::replace_constant_variable(node::pointer_t & replace, node::pointer_t resolution)
{
    if(resolution->get_type() != node_t::NODE_VARIABLE)
    {
        return false;
    }

    if(!resolution->get_flag(flag_t::NODE_VARIABLE_FLAG_CONST))
    {
        return false;
    }

    node_lock resolution_ln(resolution);
    size_t const max_children(resolution->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t set(resolution->get_child(idx));
        if(set->get_type() != node_t::NODE_SET)
        {
            continue;
        }

        optimizer::optimize(set);

        if(set->get_children_size() != 1)
        {
            return false;
        }
        node_lock set_ln(set);

        node::pointer_t value(set->get_child(0));
        type_expr(value);

        switch(value->get_type())
        {
        case node_t::NODE_STRING:
        case node_t::NODE_INTEGER:
        case node_t::NODE_FLOATING_POINT:
        case node_t::NODE_TRUE:
        case node_t::NODE_FALSE:
        case node_t::NODE_NULL:
        case node_t::NODE_UNDEFINED:
        case node_t::NODE_REGULAR_EXPRESSION:
            {
                node::pointer_t clone(value->clone_basic_node());
                replace->replace_with(clone);
                replace = clone;
            }
            return true;

        default:
            // dynamic expression, can't
            // be resolved at compile time...
            return false;

        }
        /*NOTREACHED*/
    }

    return false;
}


void compiler::var(node::pointer_t var_node)
{
    // when variables are used, they are initialized
    // here, we initialize them only if they have
    // side effects; this is because a variable can
    // be used as an attribute and it would often
    // end up as an error (i.e. attributes not
    // found as identifier(s) defining another
    // object)
    node_lock ln(var_node);
    size_t const vcnt(var_node->get_children_size());
    for(size_t v(0); v < vcnt; ++v)
    {
        node::pointer_t variable_node(var_node->get_child(v));
        variable(variable_node, true);
    }
}


void compiler::variable(node::pointer_t variable_node, bool const side_effects_only)
{
    std::size_t const max_children(variable_node->get_children_size());

    // if we already have a type, we have been parsed
    if(variable_node->get_flag(flag_t::NODE_VARIABLE_FLAG_DEFINED)
    || variable_node->get_flag(flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES))
    {
        if(!side_effects_only)
        {
            if(!variable_node->get_flag(flag_t::NODE_VARIABLE_FLAG_COMPILED))
            {
                for(std::size_t idx(0); idx < max_children; ++idx)
                {
                    node::pointer_t child(variable_node->get_child(idx));
                    if(child->get_type() == node_t::NODE_SET)
                    {
                        node::pointer_t expr(child->get_child(0));
                        expression(expr);
                        variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_COMPILED, true);
                        break;
                    }
                }
            }
            variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_INUSE, true);
        }
        return;
    }

    variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_DEFINED, true);
    variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_INUSE, !side_effects_only);

    bool const constant(variable_node->get_flag(flag_t::NODE_VARIABLE_FLAG_CONST));

    // make sure to get the attributes before the node gets locked
    // (we know that the result is true in this case)
    if(!get_attribute(variable_node, attribute_t::NODE_ATTR_DEFINED))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, variable_node->get_position());
        msg << "get_attribute() did not return true as expected for NODE_ATTR_DEFINED.";
        throw as2js_exit(msg.str(), 1);
    }

    node_lock ln(variable_node);
    int set(0);

    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(variable_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_UNKNOWN:
            break;

        case node_t::NODE_SET:
            {
                node::pointer_t expr(child->get_child(0));
                if(expr->get_type() == node_t::NODE_PRIVATE
                || expr->get_type() == node_t::NODE_PUBLIC)
                {
                    // this is a list of attributes
                    ++set;
                }
                else if(set == 0
                     && (!side_effects_only || expr->has_side_effects()))
                {
                    variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_COMPILED, true);
                    variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                    expression(expr);
                }
                ++set;
            }
            break;

        case node_t::NODE_TYPE:
            // define the variable type in this case
            {
                variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_COMPILED, true);

                node::pointer_t expr(child->get_child(0));
                expression(expr);
                if(variable_node->get_type_node() == nullptr)
                {
                    ln.unlock();
                    variable_node->set_instance(expr->get_instance());
                    variable_node->set_type_node(expr->get_type_node());
                }
            }
            break;

        default:
            {
                message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, variable_node->get_position());
                msg << "variable has a child node of an unknown type.";
                throw as2js_exit(msg.str(), 1);
            }

        }
    }

    if(set > 1)
    {
        variable_node->to_var_attributes();
        if(!constant)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NEED_CONST, variable_node->get_position());
            msg << "a variable cannot be a list of attributes unless it is made constant and \""
                << variable_node->get_string()
                << "\" is not constant.";
        }
    }
    else
    {
        // read the initializer (we're expecting an expression, but
        // if this is only one identifier or PUBLIC or PRIVATE then
        // we're in a special case...)
        //
        add_variable(variable_node);
    }
}


void compiler::add_variable(node::pointer_t variable_node)
{
    // For variables, we want to save a link in the
    // first directive list; this is used to clear
    // all the variables whenever a frame is left
    // and enables us to declare local variables as
    // such in functions
    //
    // [i.e. local variables defined in a frame are
    // undefined once you quit that frame; we do that
    // because the Flash instructions don't give us
    // correct frame management and a goto inside a
    // frame would otherwise possibly use the wrong
    // variable value!]
    node::pointer_t parent(variable_node);
    bool first(true);
    for(;;)
    {
        parent = parent->get_parent();
        switch(parent->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            if(first)
            {
                first = false;
                parent->add_variable(variable_node);
            }
            break;

        case node_t::NODE_FUNCTION:
            // mark the variable as local
            variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_LOCAL, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
            // mark the variable as a member of this class or interface
            variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_MEMBER, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case node_t::NODE_PROGRAM:
        case node_t::NODE_PACKAGE:
            // variable is global
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        default:
            break;

        }
    }
}



} // namespace as2js
// vim: ts=4 sw=4 et
