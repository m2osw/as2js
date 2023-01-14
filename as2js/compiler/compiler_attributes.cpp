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

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{



void compiler::variable_to_attrs(node::pointer_t node, node::pointer_t var_node)
{
    if(var_node->get_type() != node_t::NODE_SET)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_VARIABLE, var_node->get_position());
        msg << "an attribute variable has to be given a value.";
        return;
    }

    node::pointer_t a(var_node->get_child(0));
    switch(a->get_type())
    {
    case node_t::NODE_FALSE:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_INLINE:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_TRUE:
        node_to_attrs(node, a);
        return;

    default:
        // expect a full boolean expression in this case
        break;

    }

    // compute the expression
    expression(a);
    optimizer::optimize(a);

    switch(a->get_type())
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        node_to_attrs(node, a);
        return;

    default:
        break;

    }

    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, var_node->get_position());
    msg << "an attribute which is an expression needs to result in a boolean value (true or false).";
}


void compiler::identifier_to_attrs(node::pointer_t n, node::pointer_t a)
{
    // note: an identifier cannot be an empty string
    //
    std::string const identifier(a->get_string());
    switch(identifier[0])
    {
    case 'a':
        if(identifier == "array")
        {
            n->set_attribute(attribute_t::NODE_ATTR_ARRAY, true);
            return;
        }
        if(identifier == "autobreak")
        {
            n->set_attribute(attribute_t::NODE_ATTR_AUTOBREAK, true);
            return;
        }
        break;

    case 'c':
        if(identifier == "constructor")
        {
            n->set_attribute(attribute_t::NODE_ATTR_CONSTRUCTOR, true);
            return;
        }
        break;

    case 'd':
        if(identifier == "deprecated")
        {
            n->set_attribute(attribute_t::NODE_ATTR_DEPRECATED, true);
            return;
        }
        if(identifier == "dynamic")
        {
            n->set_attribute(attribute_t::NODE_ATTR_DYNAMIC, true);
            return;
        }
        break;

    case 'e':
        if(identifier == "enumerable")
        {
            n->set_attribute(attribute_t::NODE_ATTR_ENUMERABLE, true);
            return;
        }
        break;

    case 'f':
        if(identifier == "foreach")
        {
            n->set_attribute(attribute_t::NODE_ATTR_FOREACH, true);
            return;
        }
        break;

    case 'i':
        if(identifier == "internal")
        {
            n->set_attribute(attribute_t::NODE_ATTR_INTERNAL, true);
            return;
        }
        break;

    case 'n':
        if(identifier == "nobreak")
        {
            n->set_attribute(attribute_t::NODE_ATTR_NOBREAK, true);
            return;
        }
        break;

    case 'u':
        if(identifier == "unsafe")
        {
            n->set_attribute(attribute_t::NODE_ATTR_UNSAFE, true);
            return;
        }
        if(identifier == "unused")
        {
            n->set_attribute(attribute_t::NODE_ATTR_UNUSED, true);
            return;
        }
        break;

    case 'v':
        if(identifier == "virtual")
        {
            n->set_attribute(attribute_t::NODE_ATTR_VIRTUAL, true);
            return;
        }
        break;

    }

    // it could be a user defined variable list of attributes
    //
    node::pointer_t resolution;
    if(!resolve_name(n, a, resolution, node::pointer_t(), SEARCH_FLAG_NO_PARSING))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, a->get_position());
        msg << "cannot find a variable named '" << a->get_string() << "'.";
        return;
    }
    if(!resolution)
    {
        // TODO: do we expect an error here?
        //
        return;
    }
    if(resolution->get_type() != node_t::NODE_VARIABLE
    && resolution->get_type() != node_t::NODE_VAR_ATTRIBUTES)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DYNAMIC, a->get_position());
        msg << "a dynamic attribute name can only reference a variable and '" << a->get_string() << "' is not one.";
        return;
    }

    // it is a variable, go through the list and call ourselves recursively
    // with each identifiers; but make sure we do not loop forever
    if(resolution->get_flag(flag_t::NODE_VARIABLE_FLAG_ATTRS))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_LOOPING_REFERENCE, a->get_position());
        msg << "the dynamic attribute variable '" << a->get_string() << "' is used circularly (it loops).";
        return;
    }

    resolution->set_flag(flag_t::NODE_VARIABLE_FLAG_ATTRS, true); // to avoid infinite loop
    resolution->set_flag(flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES, true);
    node_lock ln(resolution);
    size_t const max_children(resolution->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(resolution->get_child(idx));
        variable_to_attrs(n, child);
    }
    resolution->set_flag(flag_t::NODE_VARIABLE_FLAG_ATTRS, false);
}


void compiler::node_to_attrs(node::pointer_t n, node::pointer_t a)
{
    switch(a->get_type())
    {
    case node_t::NODE_ABSTRACT:
        n->set_attribute(attribute_t::NODE_ATTR_ABSTRACT, true);
        break;

    case node_t::NODE_FALSE:
        n->set_attribute(attribute_t::NODE_ATTR_FALSE, true);
        break;

    case node_t::NODE_FINAL:
        n->set_attribute(attribute_t::NODE_ATTR_FINAL, true);
        break;

    case node_t::NODE_IDENTIFIER:
        identifier_to_attrs(n, a);
        break;

    case node_t::NODE_INLINE:
        n->set_attribute(attribute_t::NODE_ATTR_INLINE, true);
        break;

    case node_t::NODE_NATIVE: // Note: I called this one INTRINSIC before
        n->set_attribute(attribute_t::NODE_ATTR_NATIVE, true);
        break;

    case node_t::NODE_PRIVATE:
        n->set_attribute(attribute_t::NODE_ATTR_PRIVATE, true);
        break;

    case node_t::NODE_PROTECTED:
        n->set_attribute(attribute_t::NODE_ATTR_PROTECTED, true);
        break;

    case node_t::NODE_PUBLIC:
        n->set_attribute(attribute_t::NODE_ATTR_PUBLIC, true);
        break;

    case node_t::NODE_STATIC:
        n->set_attribute(attribute_t::NODE_ATTR_STATIC, true);
        break;

    case node_t::NODE_TRANSIENT:
        n->set_attribute(attribute_t::NODE_ATTR_TRANSIENT, true);
        break;

    case node_t::NODE_TRUE:
        n->set_attribute(attribute_t::NODE_ATTR_TRUE, true);
        break;

    case node_t::NODE_VOLATILE:
        n->set_attribute(attribute_t::NODE_ATTR_VOLATILE, true);
        break;

    default:
        // TODO: this is a scope (user defined name)
        // ERROR: unknown attribute type
        // Note that will happen whenever someone references a
        // variable which is an expression which does not resolve
        // to a valid attribute and thus we need a user error here
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, a->get_position());
        msg << "unsupported attribute data type, dynamic expressions for attributes need to be resolved as constants.";
        break;

    }
}


void compiler::prepare_attributes(node::pointer_t n)
{
    // done here?
    //
    if(n->get_attribute(attribute_t::NODE_ATTR_DEFINED))
    {
        return;
    }

    // mark ourselves as done even if errors occur
    //
    n->set_attribute(attribute_t::NODE_ATTR_DEFINED, true);

    if(n->get_type() == node_t::NODE_PROGRAM)
    {
        // programs do not get any specific attributes
        // (early optimization)
        //
        return;
    }

    node::pointer_t attr(n->get_attribute_node());
    if(attr != nullptr)
    {
        node_lock ln(attr);
        std::size_t const max_attr(attr->get_children_size());
        for(std::size_t idx(0); idx < max_attr; ++idx)
        {
            node_to_attrs(n, attr->get_child(idx));
        }
    }

    // check whether native is already set
    // (in which case it is probably an error)
    //
    bool const has_direct_native(n->get_attribute(attribute_t::NODE_ATTR_NATIVE));

    // note: we already returned if it is equal
    //       to program; here it is just documentation
    //
    if(n->get_type() != node_t::NODE_PACKAGE
    && n->get_type() != node_t::NODE_PROGRAM)
    {
        node::pointer_t parent(n->get_parent());
        if(parent != nullptr
        && parent->get_type() != node_t::NODE_PACKAGE
        && parent->get_type() != node_t::NODE_PROGRAM
        && parent->get_type() != node_t::NODE_CLASS         // except that NATIVE, DYNAMIC, FINAL and a few others probably needs to flow through classes and interfaces?
        && parent->get_type() != node_t::NODE_INTERFACE
        && parent->get_type() != node_t::NODE_FUNCTION)
        {
            // recurse against all parents as required
            //
            prepare_attributes(parent);

            // child can redefine (ignore parent if any defined)
            // [TODO: should this be an error if conflicting?]
            //
            if(!n->get_attribute(attribute_t::NODE_ATTR_PUBLIC)
            && !n->get_attribute(attribute_t::NODE_ATTR_PRIVATE)
            && !n->get_attribute(attribute_t::NODE_ATTR_PROTECTED))
            {
                n->set_attribute(attribute_t::NODE_ATTR_PUBLIC,    parent->get_attribute(attribute_t::NODE_ATTR_PUBLIC));
                n->set_attribute(attribute_t::NODE_ATTR_PRIVATE,   parent->get_attribute(attribute_t::NODE_ATTR_PRIVATE));
                n->set_attribute(attribute_t::NODE_ATTR_PROTECTED, parent->get_attribute(attribute_t::NODE_ATTR_PROTECTED));
            }

            // child can redefine (ignore parent if defined)
            //
            if(!n->get_attribute(attribute_t::NODE_ATTR_STATIC)
            && !n->get_attribute(attribute_t::NODE_ATTR_ABSTRACT)
            && !n->get_attribute(attribute_t::NODE_ATTR_VIRTUAL))
            {
                n->set_attribute(attribute_t::NODE_ATTR_STATIC,   parent->get_attribute(attribute_t::NODE_ATTR_STATIC));
                n->set_attribute(attribute_t::NODE_ATTR_ABSTRACT, parent->get_attribute(attribute_t::NODE_ATTR_ABSTRACT));
                n->set_attribute(attribute_t::NODE_ATTR_VIRTUAL,  parent->get_attribute(attribute_t::NODE_ATTR_VIRTUAL));
            }

            if(!n->get_attribute(attribute_t::NODE_ATTR_FINAL))
            {
                n->set_attribute(attribute_t::NODE_ATTR_FINAL, parent->get_attribute(attribute_t::NODE_ATTR_FINAL));
            }

            // inherit
            //
            n->set_attribute(attribute_t::NODE_ATTR_NATIVE,     parent->get_attribute(attribute_t::NODE_ATTR_NATIVE));
            n->set_attribute(attribute_t::NODE_ATTR_ENUMERABLE, parent->get_attribute(attribute_t::NODE_ATTR_ENUMERABLE));

            // false has priority
            //
            if(parent->get_attribute(attribute_t::NODE_ATTR_FALSE))
            {
                n->set_attribute(attribute_t::NODE_ATTR_TRUE, false);
                n->set_attribute(attribute_t::NODE_ATTR_FALSE, true);
            }

            if(!n->get_attribute(attribute_t::NODE_ATTR_DYNAMIC))
            {
                n->set_attribute(attribute_t::NODE_ATTR_DYNAMIC, parent->get_attribute(attribute_t::NODE_ATTR_DYNAMIC));
            }
        }
    }

    // a function which has a body cannot be intrinsic
    if(n->get_attribute(attribute_t::NODE_ATTR_NATIVE)
    && n->get_type() == node_t::NODE_FUNCTION)
    {
        node_lock ln(n);
        size_t const max(n->get_children_size());
        for(size_t idx(0); idx < max; ++idx)
        {
            node::pointer_t list(n->get_child(idx));
            if(list->get_type() == node_t::NODE_DIRECTIVE_LIST)
            {
                // it is an error if the user defined
                // it directly on the function; it is
                // fine if it comes from the parent
                if(has_direct_native)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NATIVE, n->get_position());
                    msg << "'native' is not permitted on a function with a body.";
                }
                n->set_attribute(attribute_t::NODE_ATTR_NATIVE, false);
                break;
            }
        }
    }
}


bool compiler::get_attribute(node::pointer_t n, attribute_t const a)
{
    prepare_attributes(n);
    return n->get_attribute(a);
}



} // namespace as2js
// vim: ts=4 sw=4 et
