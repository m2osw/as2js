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
#include    "as2js/compiler.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{



bool compiler::is_dynamic_class(node::pointer_t class_node)
{
    // can we know?
    //
    if(class_node == nullptr)
    {
        return true;
    }

    // already determined?
    //
    if(get_attribute(class_node, attribute_t::NODE_ATTR_DYNAMIC))
    {
        return true;
    }

    size_t const max_children(class_node->get_children_size());
    for(size_t idx = 0; idx < max_children; ++idx)
    {
        node::pointer_t child(class_node->get_child(idx));
        if(child->get_type() == node_t::NODE_EXTENDS)
        {
            // TODO: once we support multiple extends, work on
            //       the list of them, in which case one instance
            //       is not going to be too good
            //
            node::pointer_t name(child->get_child(0));
            node::pointer_t extends(name ? name->get_instance() : name);
            if(extends)
            {
                if(extends->get_string() == "Object")
                {
                    // we ignore the dynamic flag of Object (that is a
                    // hack in the language reference!)
                    return false;
                }
                // continue at the next level (depth increasing)
                return is_dynamic_class(extends); // recursive
            }
            break;
        }
    }

    return false;
}


/** \brief Check whether a function is a constructor.
 *
 * This function checks a node representing a function to determine whether
 * it represents a constructor or not.
 *
 * By default, if a function is marked as a constructor by the programmer,
 * then this function considers the function as a constructor no matter
 * what (outside of the fact that it has to be a function defined in a
 * class, obviously.)
 *
 * \param[in] function_node  A node representing a function definition.
 * \param[out] the_class  If the function is a constructor, this is the
 *                        class it was defined in.
 *
 * \return true if the function is a constructor and in that case the_class
 *         is set to the class node pointer; otherwise the_class is set to
 *         nullptr.
 */
bool compiler::is_constructor(node::pointer_t function_node, node::pointer_t & the_class)
{
    the_class.reset();

    if(function_node->get_type() != node_t::NODE_FUNCTION)
    {
        throw internal_error(std::string("compiler::is_constructor() was called with a node which is not a NODE_FUNCTION, it is ") + function_node->get_type_name());
    }

    // search the first NODE_CLASS with the same name
    for(node::pointer_t parent(function_node->get_parent()); parent; parent = parent->get_parent())
    {
        // Note: here I made a note that sub-functions cannot be
        //       constructors which is true in ActionScript, but
        //       not in JavaScript. We actually make use of a
        //       sub-function to create inheritance that works
        //       in JavaScript (older browsers required a "new Object"
        //       to generate inheritance which was a big problem.)
        //       However, in our language, we probably want people
        //       to make use of the class keyword anyway, so they
        //       could create a sub-class inside a function and
        //       we are back in business!
        //
        switch(parent->get_type())
        {
        case node_t::NODE_PACKAGE:
        case node_t::NODE_PROGRAM:
        case node_t::NODE_FUNCTION:    // sub-functions cannot be constructors
        case node_t::NODE_INTERFACE:
            return false;

        case node_t::NODE_CLASS:
            // we found the class in question

            // user defined constructor?
            if(get_attribute(function_node, attribute_t::NODE_ATTR_CONSTRUCTOR)
            || parent->get_string() == function_node->get_string())
            {
                the_class = parent;
                return true;
            }
            return false;

        default:
            // ignore all the other nodes
            break;

        }
    }

    if(get_attribute(function_node, attribute_t::NODE_ATTR_CONSTRUCTOR))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, function_node->get_position());
        msg << "\"constructor " << function_node->get_string() << "()\" cannot be used outside of a class declaration.";
    }

    return false;
}


void compiler::check_super_validity(node::pointer_t expr)
{
    if(expr == nullptr)
    {
        return;
    }
    node::pointer_t parent(expr->get_parent());
    if(!parent)
    {
        return;
    }

    bool const needs_constructor(parent->get_type() == node_t::NODE_CALL);
    bool first_function(true);
    bool continue_testing(true);
    for(; parent && continue_testing; parent = parent->get_parent())
    {
        switch(parent->get_type())
        {
        case node_t::NODE_FUNCTION:
            if(first_function)
            {
                // We have two super's
                // 1) super(params) in constructors
                // 2) super.field(params) in non-static functions
                // case 1 is recognized as having a direct parent
                // of type call (see at start of function!)
                // case 2 is all other cases
                // in both cases we need to be defined in a class
                node::pointer_t the_class;
                if(needs_constructor)
                {
                    if(!is_constructor(parent, the_class))
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
                        msg << "\"super()\" cannot be used outside of a constructor function.";
                        return;
                    }
                }
                else
                {
                    if(parent->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR)
                    || get_attribute(parent, attribute_t::NODE_ATTR_STATIC)
                    || get_attribute(parent, attribute_t::NODE_ATTR_CONSTRUCTOR)
                    || is_constructor(parent, the_class))
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
                        msg << "\"super.member()\" cannot be used in a static function nor a constructor.";
                        return;
                    }
                }
                // TODO: Test that this is correct, it was missing...
                //       Once false, we skip all the tests from this
                //       block.
                first_function = false;
            }
            else
            {
                // Can it be used in sub-functions?
                // If we arrive here then we can err if
                // super and/or this are not available
                // in sub-functions... TBD
            }
            break;

        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
            return;

        case node_t::NODE_PROGRAM:
        case node_t::NODE_ROOT:
            continue_testing = false;
            break;

        default:
            break;

        }
    }

    if(needs_constructor)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "\"super()\" cannot be used outside a class definition.";
    }
}


void compiler::link_type(node::pointer_t type)
{
//std::cerr << "find_type()\n";
    // already linked?
    //
    node::pointer_t link(type->get_instance());
    if(link != nullptr)
    {
        return;
    }

    if(type->get_type() != node_t::NODE_IDENTIFIER
    && type->get_type() != node_t::NODE_STRING)
    {
        // we cannot link (determine) the type at compile time
        // if we have a type expression
        //
//std::cerr << "WARNING: dynamic type?!\n";
        return;
    }

    if(type->get_flag(flag_t::NODE_IDENTIFIER_FLAG_TYPED))
    {
        // if it failed already, fail only once...
        //
        return;
    }
    type->set_flag(flag_t::NODE_IDENTIFIER_FLAG_TYPED, true);

    node::pointer_t object;
    if(!resolve_name(type, type, object, node::pointer_t(), node::pointer_t(), 0))
    {
        // unknown type?! -- should we return a link to Object?
        //
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, type->get_position());
        msg << "cannot find a class definition for type \"" << type->get_string() << "\".";
        return;
    }

    if(object->get_type() != node_t::NODE_CLASS
    && object->get_type() != node_t::NODE_INTERFACE)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, type->get_position());
        msg << "the name \"" << type->get_string() << "\" is not referencing a class nor an interface.";
        return;
    }

    // it worked.
    //
    type->set_instance(object);
}


depth_t compiler::find_class(node::pointer_t class_type, node::pointer_t type, depth_t depth)
{
    node_lock ln(class_type);
    std::size_t const max_children(class_type->get_children_size());

    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(class_type->get_child(idx));
        if(child->get_type() == node_t::NODE_IMPLEMENTS
        || child->get_type() == node_t::NODE_EXTENDS)
        {
            if(child->get_children_size() == 0)
            {
                // should never happen
                continue;
            }
            node_lock child_ln(child);
            node::pointer_t super_name(child->get_child(0));
            node::pointer_t super(super_name->get_instance());
            if(super == nullptr)
            {
                expression(super_name);
                super = super_name->get_instance();
            }
            if(super == nullptr)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, class_type->get_position());
                msg << "cannot find the type named in an \"extends\" or \"implements\" list.";
                continue;
            }
            if(super == type)
            {
                return depth;
            }
        }
    }

    depth += 1;

    depth_t result(MATCH_NOT_FOUND);
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(class_type->get_child(idx));
        if(child->get_type() == node_t::NODE_IMPLEMENTS
        || child->get_type() == node_t::NODE_EXTENDS)
        {
            if(child->get_children_size() == 0)
            {
                // should never happen
                continue;
            }
            node_lock child_ln(child);
            node::pointer_t super_name(child->get_child(0));
            node::pointer_t super(super_name->get_instance());
            if(super == nullptr)
            {
                continue;
            }
            depth_t const r(find_class(super, type, depth));  // recursive
            if(r > result)
            {
                result = r;
            }
        }
    }

    return result;
}


bool compiler::is_derived_from(
      node::pointer_t derived_class
    , node::pointer_t super_class)
{
    if(super_class == nullptr)
    {
        throw internal_error("is_derived_from() called with unset super_class (nullptr).");
    }

    if(derived_class == super_class)
    {
        // exact same object, it is "derived from"
        //
        return true;
    }

    std::size_t const max(derived_class->get_children_size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        node::pointer_t extends(derived_class->get_child(idx));
        if(extends->get_type() != node_t::NODE_EXTENDS
        && extends->get_type() != node_t::NODE_IMPLEMENTS)
        {
            continue;
        }
        node::pointer_t type(extends->get_child(0));
        // TODO: we probably want to accept lists of extends too
        //       because JavaScript gives us the ability to create
        //       objects with multiple derivations (not exactly
        //       100% true, but close enough and it makes a lot
        //       of things MUCH easier).
        //
        if(type->get_type() == node_t::NODE_LIST
        && extends->get_type() == node_t::NODE_IMPLEMENTS)
        {
            // IMPLEMENTS accepts lists
            //
            std::size_t const cnt(type->get_children_size());
            for(std::size_t j(0); j < cnt; ++j)
            {
                node::pointer_t sub_type(type->get_child(j));
                link_type(sub_type);
                node::pointer_t instance(sub_type->get_instance());
                if(instance == nullptr)
                {
                    continue;
                }
                if(is_derived_from(instance, super_class))
                {
                    return true;
                }
            }
        }
        else
        {
            // TODO: review the "extends ..." implementation so it supports
            //       lists in the parser and then here
            //
            link_type(type);
            node::pointer_t instance(type->get_instance());
            if(instance == nullptr)
            {
                continue;
            }
            if(is_derived_from(instance, super_class))
            {
                return true;
            }
        }
    }

    return false;
}


/** \brief Search for a class or interface node.
 *
 * This function searches for a node of type NODE_CLASS or NODE_INTERFACE
 * starting with \p class_node. The search checks \p class_node and all
 * of its parents.
 *
 * The search stops prematuraly if a NODE_PACKAGE, NODE_PROGRAM, or
 * NODE_ROOT is found first.
 *
 * \param[in] class_node  The object from which a class is to be searched.
 *
 * \return The class or interface, or a null pointer if not found.
 */
node::pointer_t compiler::class_of_member(node::pointer_t class_node)
{
    while(class_node != nullptr)
    {
        if(class_node->get_type() == node_t::NODE_CLASS
        || class_node->get_type() == node_t::NODE_INTERFACE)
        {
            // got the class/interface definition
            //
            return class_node;
        }
        if(class_node->get_type() == node_t::NODE_PACKAGE
        || class_node->get_type() == node_t::NODE_PROGRAM
        || class_node->get_type() == node_t::NODE_ROOT)
        {
            // not found, we reached one of package/program/root instead
            //
            break;
        }
        class_node = class_node->get_parent();
    }

    return node::pointer_t();
}


/** \brief Check whether derived_class is extending super_class.
 *
 * This function checks whether the object defined as derived_class
 * has an extends or implements one that includes super_class.
 *
 * The \p the_super_class parameter is set to the class of the
 * super_class object. This can be used to determine different
 * types of errors.
 *
 * Note that if derived_class or super_class are not objects defined
 * in a class, then the function always returns false.
 *
 * \param[in] derived_class  The class which is checked to know whether it
 *                           derives from super_class.
 * \param[in] super_class  The class that is expected to be in the extends
 *                         or implements lists.
 * \param[out] the_super_class  The actual class object in which super_class
 *                              is defined.
 *
 * \return true if derived_class is derived from super_class.
 */
bool compiler::are_objects_derived_from_one_another(
      node::pointer_t derived_class
    , node::pointer_t super_class
    , node::pointer_t & the_super_class)
{
    the_super_class = class_of_member(super_class);
    if(the_super_class == nullptr)
    {
std::cerr << "--- the super class (param 2) has no CLASS definition?!\n";
        return false;
    }
    node::pointer_t the_derived_class(class_of_member(derived_class));
    if(the_derived_class == nullptr)
    {
std::cerr << "--- the derived class (param 1) has no CLASS definition?!\n";
        return false;
    }
std::cerr << "--- class or super_class (param 2) is:\n" << *the_super_class << "\n";
std::cerr << "--- class or derived_class (param 1) is:\n" << *the_derived_class << "\n";

    return is_derived_from(the_derived_class, the_super_class);
}


void compiler::declare_class(node::pointer_t class_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        //node_lock ln(class_node);
        node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            declare_class(child); // recursive!
            break;

        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
            class_directive(child);
            break;

        case node_t::NODE_ENUM:
            enum_directive(child);
            break;

        case node_t::NODE_FUNCTION:
//std::cerr << "Got a function member in that class...\n";
            function(child);
            break;

        case node_t::NODE_VAR:
            var(child);
            break;

        default:
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NODE, child->get_position());
                msg << "the \"" << child->get_type_name() << "\" token cannot be a class member.";
            }
            break;

        }
    }
}


void compiler::extend_class(node::pointer_t class_node, bool const extend, node::pointer_t extend_name)
{
    expression(extend_name);

    node::pointer_t super(extend_name->get_instance());
    if(super)
    {
        switch(super->get_type())
        {
        case node_t::NODE_CLASS:
            if(class_node->get_type() == node_t::NODE_INTERFACE)
            {
                // (super) 'class A', 'interface B extends A'
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "class \"" << super->get_string() << "\" cannot extend interface \"" << class_node->get_string() << "\".";
            }
            else if(!extend)
            {
                // (super) 'class A', '... implements A'
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "class \"" << super->get_string() << "\" cannot implement class \"" << class_node->get_string() << "\". Use \"extends\" instead.";
            }
            else if(get_attribute(super, attribute_t::NODE_ATTR_FINAL))
            {
                // (super) 'final class A', 'class B extends A'
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_FINAL, class_node->get_position());
                msg << "class \"" << super->get_string() << "\" is marked final and it cannot be extended by \"" << class_node->get_string() << "\".";
            }
            break;

        case node_t::NODE_INTERFACE:
            if(class_node->get_type() == node_t::NODE_INTERFACE && !extend)
            {
                // (super) 'interface A', 'interface B implements A'
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "interface \"" << super->get_string() << "\" cannot implement interface \"" << class_node->get_string() << "\". Use \"extends\" instead.";
            }
            else if(get_attribute(super, attribute_t::NODE_ATTR_FINAL))
            {
                // TODO: prove that this error happens earlier and thus that
                //       we do not need to generate it here
                //
                // (super) 'final interface A'
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_FINAL, class_node->get_position());
                msg << "interface \"" << super->get_string() << "\" is marked final, which is not legal.";
            }
            break;

        default:
            // this should never happen
            throw internal_error("found a LINK_INSTANCE which is neither a class nor an interface.");

        }
    }
    //else -- TBD: should already have gotten an error by now?
}


void compiler::class_directive(node::pointer_t & class_node)
{
    // TBD: Should we instead of looping check nodes in order to
    //      enforce order? Or do we trust that the parser already
    //      did that properly?
    std::size_t const max(class_node->get_children_size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        //node_lock ln(class_node);
        node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            declare_class(child);
            break;

        case node_t::NODE_EXTENDS:
            extend_class(class_node, true, child->get_child(0));
            break;

        case node_t::NODE_IMPLEMENTS:
            extend_class(class_node, false, child->get_child(0));
            break;

        case node_t::NODE_EMPTY:
            break;

        default:
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, class_node->get_position());
                msg << "invalid token \"" << child->get_type_name() << "\" in a class definition.";
            }
            break;

        }
    }
}


/** \brief Enum directive.
 *
 * Enumerations are like classes defining a list of constant values.
 *
 * \param[in] enum_node  The enumeration node to work on.
 */
void compiler::enum_directive(node::pointer_t& enum_node)
{
    node_lock ln(enum_node);
    size_t const max_children(enum_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t entry(enum_node->get_child(idx));
        if(entry->get_children_size() != 1)
        {
            // this happens in case of an empty enumeration
            // entry type should be NODE_EMPTY
            continue;
        }
        node::pointer_t set(entry->get_child(0));
        if(set->get_type() != node_t::NODE_SET
        || set->get_children_size() != 1)
        {
            // not valid, skip
            //
            // TODO: for test purposes we could create an invalid tree to hit
            //       this line and have coverage
            //
            continue; // LCOV_EXCL_LINE
        }
        // compile the expression
        expression(set->get_child(0));
    }
}



} // namespace as2js
// vim: ts=4 sw=4 et
