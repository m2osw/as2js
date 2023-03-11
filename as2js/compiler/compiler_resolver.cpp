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

#include    "as2js/exception.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{



void compiler::check_member(node::pointer_t ref, node::pointer_t field, node::pointer_t field_name)
{
    if(field == nullptr)
    {
        // search for the class this field (ref) is defined in since we are
        // interested in knowing whether that class is dynamic or not
        //
        node::pointer_t type(class_of_member(ref));
        if(!is_dynamic_class(type))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STATIC, ref->get_position());
            msg << '"'
                << type->get_string()
                << '.'
                << ref->get_string()
                << "\" is not dynamic and thus it cannot be used with unknown member \""
                << field_name->get_string()
                << "\".";
        }
        return;
    }

    node::pointer_t obj(ref->get_instance());
    if(!obj)
    {
        return;
    }

    // If the link is directly a class or an interface
    // then the field needs to be a sub-class, sub-interface,
    // static function, static variable or constant variable.
    if(obj->get_type() != node_t::NODE_CLASS
    && obj->get_type() != node_t::NODE_INTERFACE)
    {
        return;
    }

    bool err(false);
    switch(field->get_type())
    {
    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
        break;

    case node_t::NODE_FUNCTION:
        //
        // note that constructors are considered static, but
        // you cannot just call a constructor...
        //
        // operators are static and thus we will be fine with
        // operators (since you need to call operators with
        // all the required inputs)
        //
        err = !get_attribute(field, attribute_t::NODE_ATTR_STATIC)
           && !field->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR);
        break;

    case node_t::NODE_VARIABLE:
        // static const foo = 123; is fine
        err = !get_attribute(field, attribute_t::NODE_ATTR_STATIC)
           && !field->get_flag(flag_t::NODE_VARIABLE_FLAG_CONST);
        break;

    default:
        err = true;
        break;

    }

    if(err)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INSTANCE_EXPECTED, ref->get_position());
        msg << "you cannot directly access non-static functions and non-static/constant variables in a class (\""
            << field->get_string()
            << "\" here); you need to use an instance instead.";
    }
}


bool compiler::find_in_extends(
      node::pointer_t link
    , node::pointer_t field
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
    // try to see if we are inheriting that field...
    //
    node_lock ln(link);
    std::size_t const max_children(link->get_children_size());
    std::size_t count(0);
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t extends(link->get_child(idx));
        if(extends->get_type() == node_t::NODE_EXTENDS)
        {
            // TODO: support list of extends (see IMPLEMENTS below!)
            if(extends->get_children_size() == 1)
            {
                node::pointer_t type(extends->get_child(0));
                link_type(type);
                node::pointer_t sub_link(type->get_instance());
                if(sub_link == nullptr)
                {
                    // we cannot search a field in nothing...
                    //
                    message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                    msg << "type not linked, cannot lookup member.";
                }
                else if(find_any_field(sub_link, field, resolution, params, all_matches, search_flags))
                {
                    ++count;
                }
            }
//std::cerr << "Extends existing! (" << extends.GetChildCount() << ")\n";
        }
        else if(extends->get_type() == node_t::NODE_IMPLEMENTS)
        {
            if(extends->get_children_size() == 1)
            {
                node::pointer_t type(extends->get_child(0));
                if(type->get_type() == node_t::NODE_LIST)
                {
                    size_t cnt(type->get_children_size());
                    for(size_t j(0); j < cnt; ++j)
                    {
                        node::pointer_t child(type->get_child(j));
                        link_type(child);
                        node::pointer_t sub_link(child->get_instance());
                        if(!sub_link)
                        {
                            // we cannot search a field in nothing...
                            message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                            msg << "type not linked, cannot lookup member.";
                        }
                        else if(find_any_field(sub_link, field, resolution, params, all_matches, search_flags)) // recursive
                        {
                            ++count;
                        }
                    }
                }
                else
                {
                    link_type(type);
                    node::pointer_t sub_link(type->get_instance());
                    if(!sub_link)
                    {
                        // we can't search a field in nothing...
                        message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                        msg << "type not linked, cannot lookup member.";
                    }
                    else if(find_any_field(sub_link, field, resolution, params, all_matches, search_flags)) // recursive
                    {
                        ++count;
                    }
                }
            }
        }
    }

    if(count == 1
    || (all_matches != nullptr && all_matches->get_children_size() != 0))
    {
        return true;
    }

    if(count == 0)
    {
        // NOTE: warning? error? This actually would just turn
        //     on a flag.
        //     As far as I know I now have an error in case
        //     the left hand side expression is a static
        //     class (opposed to a dynamic class which can
        //     have members added at runtime)
//std::cerr << "     field not found...\n";
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, field->get_position());
        msg << "found more than one match for \"" << field->get_string() << "\".";
    }

    return false;
}


bool compiler::check_field(
      node::pointer_t link
    , node::pointer_t field
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
    node_lock link_ln(link);
    std::size_t const max_children(link->get_children_size());
//std::cerr << "  +++ compiler_resolver.cpp: check_field() " << max_children << " +++\n";
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t list(link->get_child(idx));
        if(list->get_type() != node_t::NODE_DIRECTIVE_LIST)
        {
            // extends, implements, empty...
            //
            continue;
        }

        // search in this list!
        //
        node_lock list_ln(list);
        std::size_t const max_list_children(list->get_children_size());
        for(std::size_t j(0); j < max_list_children; ++j)
        {
            // if we have a sub-list, do a recursive call
            //
            node::pointer_t child(list->get_child(j));
            if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
            {
                if(check_field(list, field, resolution, params, all_matches, search_flags)) // recursive
                {
                    if(funcs_name(resolution, all_matches))
                    {
                        return true;
                    }
                }
            }
            else if(child->get_type() != node_t::NODE_EMPTY)
            {
//std::cerr << "  +--> compiler_resolver.cpp: check_field(): call check_name()"
//    " as we are searching for a \"class\" field named \""
//    << field->get_string()
//    << "\" (actually this may be any object that can be given a name, we may be in a package too)\n";
                if(check_name(list, j, resolution, field, params, all_matches, search_flags))
                {
//std::cerr << "  +--> compiler_resolver.cpp: check_field(): check_name()"
//    " found a \"class\" field or \"package\" definition named \""
//    << field->get_string()
//    << "\"! Matches:\n";
//if(all_matches == nullptr)
//{
//std::cerr << "<all_matches is (null)>\n";
//}
//else
//{
//std::cerr << *all_matches << "\n";
//}
                    if(funcs_name(resolution, all_matches))
                    {
                        node::pointer_t inst(field->get_instance());
                        if(inst == nullptr)
                        {
                            field->set_instance(resolution);
                        }
                        else if(inst != resolution)
                        {
                            // if already defined, it should be the same or
                            // we have a real problem
                            //
                            throw internal_error("found an instance twice, but it was different each time");
                        }
//std::cerr << "  +++ compiler_resolver.cpp: check_field(): accept this resolution as the answer! +++\n";
                        return true;
                    }
                }
            }
        }
    }

//std::cerr << "  +++ compiler_resolver.cpp: check_field(): failed -- no resolution yet +++\n";
    return false;
}


bool compiler::check_name(
      node::pointer_t list
    , int idx
    , node::pointer_t & resolution
    , node::pointer_t id
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
    if(static_cast<std::size_t>(idx) >= list->get_children_size())
    {
        throw internal_error(std::string("compiler::check_name() index too large for this list."));
    }

    node::pointer_t child(list->get_child(idx));

    // turned off?
    //if(get_attribute(child, attribute_t::NODE_ATTR_FALSE))
    //{
    //    return false;
    //}

    bool result(false);
//std::cerr << "  +--> compiler_resolver.cpp: check_name() processing a child node type: \"" << child->get_type_name() << "\" ";
//if(child->get_type() == node_t::NODE_CLASS
//|| child->get_type() == node_t::NODE_PACKAGE
//|| child->get_type() == node_t::NODE_IMPORT
//|| child->get_type() == node_t::NODE_ENUM
//|| child->get_type() == node_t::NODE_FUNCTION)
//{
//    std::cerr << " \"" << child->get_string() << "\"";
//}
//std::cerr << "\n";
    std::string const & name(id->get_string());
    switch(child->get_type())
    {
    case node_t::NODE_VAR:    // a VAR is composed of VARIABLEs
        {
            node_lock ln(child);
            std::size_t const max_children(child->get_children_size());
            for(std::size_t j(0); j < max_children; ++j)
            {
                node::pointer_t variable_node(child->get_child(j));
                if(variable_node->get_string() == name)
                {
                    // this is a variable!
                    // make sure it was parsed
                    //
                    if((search_flags & SEARCH_FLAG_NO_PARSING) == 0)
                    {
                        variable(variable_node, false);
                    }

                    // check whether we are in a call, because if we are,
                    // the resolution is the "()" operator of that class
                    //
                    if(params != nullptr
                    && (search_flags & SEARCH_FLAG_RESOLVING_CALL) == 0)
                    {
                        message msg(
                                  message_level_t::MESSAGE_LEVEL_FATAL
                                , err_code_t::AS_ERR_INTERNAL_ERROR
                                , id->get_position());
                        msg << "handling of () operator within a call is not yet properly handled.";
                        throw as2js_exit(msg.str(), 1);
                    }
                    resolution = variable_node;
                    result = true;
                    break;
                }
            }
        }
        break;

    case node_t::NODE_PARAM:
//std::cerr << "  +--> param = " << child->get_string() << " against " << id->get_string() << "\n";
        if(child->get_string() == name)
        {
            resolution = child;
            resolution->set_flag(flag_t::NODE_PARAM_FLAG_REFERENCED, true);
            return true;
        }
        break;

    case node_t::NODE_FUNCTION:
//std::cerr << "  +--> name = " << child->get_string() << "\n";
        {
            node::pointer_t the_class;
            if(is_constructor(child, the_class))
            {
                // this is a special case as the function name is the same
                // as the class name and the type resolution is thus the
                // class and not the function and we have to catch this
                // special case otherwise we get a never ending loop
                //
                if(the_class->get_string() == name)
                {
                    // just in case we replace the child pointer so we
                    // avoid potential side effects of having a function
                    // declaration in the child pointer
                    //
                    child = the_class;
                    resolution = the_class;
                    result = true;
//std::cerr << "  +--> this was a class! => " << child->get_string() << "\n";
                }
            }
            else
            {
                result = check_function(child, resolution, id->get_string(), params, search_flags);
            }
        }
        break;

    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
        if(child->get_string() == name)
        {
            // That is a class name! (good for a typedef, etc.)
            resolution = child;
            node::pointer_t type(resolution->get_type_node());
//std::cerr << "  +--> so we got a type of CLASS or INTERFACE for " << id->get_string()
//          << " ... [" << (type ? "has a current type ptr" : "no current type ptr") << "]\n";
            if(type == nullptr)
            {
                // A class (interface) represents itself as far as type goes (TBD)
                resolution->set_type_node(child);
            }
            resolution->set_flag(flag_t::NODE_IDENTIFIER_FLAG_TYPED, true);
            result = true;
        }
        break;

    case node_t::NODE_ENUM:
        {
            // first we check whether the name of the enum is what
            // is being referenced (i.e. the type)
            //
            if(child->get_string() == name)
            {
                resolution = child;
                resolution->set_flag(flag_t::NODE_ENUM_FLAG_INUSE, true);
                return true;
            }

            // inside an enum we have references to other
            // identifiers of that enum and these need to be
            // checked here
            std::size_t const max(child->get_children_size());
            for(std::size_t j(0); j < max; ++j)
            {
                node::pointer_t entry(child->get_child(j));
                if(entry->get_type() == node_t::NODE_VARIABLE)
                {
                    if(entry->get_string() == name)
                    {
                        // this cannot be a function, right? so the following
                        // call is probably not really useful
                        resolution = entry;
                        resolution->set_flag(flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                        return true;
                    }
                }
            }
        }
        break;

    case node_t::NODE_PACKAGE:
        if(child->get_string() == name)
        {
            // That is a package... we have to see packages
            // like classes, to search for more, you need
            // to search inside this package and none other.
            resolution = child;
            return true;
        }
#if 0
        // TODO: auto-import? this works, but I do not think we
        //       want an automatic import of even internal packages?
        //       do we?
        //
        //       At this point I would say that we do for the
        //       internal packages only. That being said, the
        //       Google closure compiler does that for all
        //       browser related declarations.
        //
        // if it is not the package itself, it could be an
        // element inside the package
        {
            int funcs(0);
            if(!find_field(child, id, funcs, resolution, params, search_flags))
            {
                break;
            }
        }
        result = true;
//std::cerr << "Found inside package! [" << id->get_string() << "]\n";
        if(!child->get_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED))
        {
//std::cerr << "Compile package now!\n";
            directive_list(child);
            child->set_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED);
        }
#endif
        break;

    case node_t::NODE_IMPORT:
        return check_import(child, resolution, name, params, search_flags);

    default:
        // ignore anything else for now
        break;

    }

    if(!result)
    {
        return false;
    }

    if(resolution == nullptr)
    {
        // this is kind of bad since we cannot test for the scope...
        //
        return true;
    }

//std::cerr << "  +--> yeah! resolved ID " << reinterpret_cast<long *>(resolution.get()) << "\n";
//std::cerr << "  +--> check_name(): private?\n";
    if(get_attribute(resolution, attribute_t::NODE_ATTR_PRIVATE))
    {
//std::cerr << "  +--> check_name(): resolved private...\n";
        // Note that an interface and a package
        // can also have private members
        node::pointer_t the_resolution_class(class_of_member(resolution));
        if(the_resolution_class == nullptr)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
        if(the_resolution_class->get_type() == node_t::NODE_PACKAGE)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE_PACKAGE;
            resolution.reset();
            return false;
        }
        if(the_resolution_class->get_type() != node_t::NODE_CLASS
        && the_resolution_class->get_type() != node_t::NODE_INTERFACE)
        {
            f_err_flags |= SEARCH_ERROR_WRONG_PRIVATE;
            resolution.reset();
            return false;
        }
        node::pointer_t the_id_class(class_of_member(id));
        if(the_id_class == nullptr)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
        if(the_id_class != the_resolution_class)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
    }

    if(get_attribute(resolution, attribute_t::NODE_ATTR_PROTECTED))
    {
//std::cerr << "  +--> check_name(): resolved protected...\n";
        // Note that an interface can also have protected members
        node::pointer_t the_super_class;
        if(!are_objects_derived_from_one_another(id, resolution, the_super_class))
        {
            if(the_super_class != nullptr
            && the_super_class->get_type() != node_t::NODE_CLASS
            && the_super_class->get_type() != node_t::NODE_INTERFACE)
            {
                f_err_flags |= SEARCH_ERROR_WRONG_PROTECTED;
            }
            else
            {
                f_err_flags |= SEARCH_ERROR_PROTECTED;
            }
            resolution.reset();
            return false;
        }
    }

    if(child->get_type() == node_t::NODE_FUNCTION
    && params != nullptr)
    {
//std::cerr << "  +--> compiler_resolver.cpp: check_name() verify function with parameters... params:\n"
//<< *params << "\nAnd Matches:\n" << *all_matches << "\n";
        if(check_function_with_params(child, params, all_matches) < 0)
        {
//std::cerr << "  +--> compiler_resolver.cpp: check_name() parameters do not match.... (see an error?)\n";
            resolution.reset();
            return false;
        }
//std::cerr << "  +--> compiler_resolver.cpp: check_name() parameters match!!! -> " << resolution.get() << "\n";
    }

    return true;
}


bool compiler::find_any_field(
      node::pointer_t link
    , node::pointer_t field
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
//std::cerr << "  *** find_any_field()\n";
    if(check_field(link, field, resolution, params, all_matches, search_flags))
    {
//std::cerr << "Check Field true...\n";
        return true;
    }
    if(all_matches != nullptr
    && all_matches->get_children_size() != 0)
    {
        // TODO: stronger validation of functions
        //
        //       this is wrong, we need a depth test on the best
        //       functions but we need to test all the functions
        //       of inherited fields too
        //
//std::cerr << "funcs != 0 true...\n";
        return true;
    }

//std::cerr << "FindInExtends?!...\n";
    return find_in_extends(link, field, resolution, params, all_matches, search_flags); // recursive
}


bool compiler::find_field(
      node::pointer_t link
    , node::pointer_t field
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
    // protect current compiler error flags while searching
    //
    restore_flags save_flags(this);

    bool const r(find_any_field(link, field, resolution, params, all_matches, search_flags));
    if(!r)
    {
        print_search_errors(field);
    }

    return r;
}


bool compiler::resolve_field(
      node::pointer_t object
    , node::pointer_t field
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
    // this is to make sure it is optimized, etc.
    //expression(field); -- we cannot have this here or it generates loops

    // just in case the caller is re-using the same node
    //
    resolution.reset();

    node::pointer_t link;
    node::pointer_t type;

    // check that the object is indeed an object (i.e. a variable
    // which references a class)
    //
    switch(object->get_type())
    {
    case node_t::NODE_VARIABLE:
    case node_t::NODE_PARAM:
        // it is a variable or a parameter, check for the type
        //node_lock ln(object);
        {
            size_t const max(object->get_children_size());
            size_t idx;
            for(idx = 0; idx < max; ++idx)
            {
                type = object->get_child(idx);
                if(type->get_type() != node_t::NODE_SET
                && type->get_type() != node_t::NODE_VAR_ATTRIBUTES)
                {
                    // we found the type
                    break;
                }
            }
            if(idx >= max || !type)
            {
                // TODO: should this be an error instead?
                //
                message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INCOMPATIBLE, object->get_position());
                msg << "variables and parameters without a type should not be used with members.";
                return false;
            }
        }

        // we need to have a link to the class
        //
        link_type(type);
        link = type->get_instance();
        if(link == nullptr)
        {
            // NOTE: we can't search a field in nothing...
            //       if I'm correct, it will later bite the
            //       user if the class isn't dynamic
            //
            return false;
        }
        break;

    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
        link = object;
        break;

    case node_t::NODE_PACKAGE:
        // a package may include "globals" (things not defined inside a class)
        //
        link = object;
        break;

    default:
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_TYPE, object->get_position());
            msg << "object of type \""
                << object->get_type_name()
                << "\" is not known to have members.";
        }
        return false;

    }

    if(field->get_type() != node_t::NODE_IDENTIFIER
    && field->get_type() != node_t::NODE_VIDENTIFIER
    && field->get_type() != node_t::NODE_STRING)
    {
        // we cannot determine at compile time whether a
        // dynamic field is valid...
        //std:cerr << "WARNING: cannot check a dynamic field.\n";

        // TODO: maybe look into using a counter to warn the user
        //       of the number of unresolved dynamic cases
        return false;
    }
//std::cerr << "  +--> compiler_resolver.cpp: link is: " << link.get() << "\n";

    bool const r(find_field(link, field, resolution, params, all_matches, search_flags));
    if(!r)
    {
//std::cerr << "  +--> compiler_resolver.cpp: resolve_field(): find_field() somehow failed!?.\n"
//<< *field
//<< "\n";
        return false;
    }

//std::cerr << "  +--> compiler_resolver.cpp: resolve_field(): find_field() found " << all_matches->get_children_size() << " functions!?.\n";
    if(all_matches->get_children_size() != 0)
    {
//std::cerr << "  +--> compiler_resolver.cpp: resolve_field(): field \"" << field.get() << "\" is a function, check for the best resolution.\n";
        resolution.reset();
        return select_best_func(all_matches, resolution);
    }

    return true;
}


bool compiler::find_member(
      node::pointer_t member
    , node::pointer_t & resolution
    , node::pointer_t params
    , int search_flags)
{
//std::cerr << "find_member()\n";
    // Just in case the caller is re-using the same node
    resolution.reset();

    // Invalid member node? If so don't generate an error because
    // we most certainly already mentioned that to the user
    // (and if not that's a bug earlier than here).
    if(member->get_children_size() != 2)
    {
        return false;
    }
    node_lock ln(member);

//std::cerr << "Searching for Member...\n";

    bool must_find = false;
    node::pointer_t object; // our sub-resolution

    node::pointer_t name(member->get_child(0));
    switch(name->get_type())
    {
    case node_t::NODE_MEMBER:
        // This happens when you have an expression such as:
        //        a.b.c
        // Then the child most MEMBER will be the identifier 'a'
        if(!find_member(name, object, params, search_flags))  // recursive
        {
            return false;
        }
        // If we reach here, the resolution (object variable here)
        // is the node we want to use next to resolve the field(s)
        break;

    case node_t::NODE_SUPER:
    {
        // SUPER cannot be used on the right side of a NODE_MEMBER
        // -- this is not correct, we could access the super of a
        //    child member (a.super.blah represents field blah in
        //    the class a is derived from)
        //node::pointer_t parent(name->get_parent());
        //if(parent->get_type() == node_t::NODE_MEMBER
        //&& name->get_offset() != 0)
        //{
        //    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
        //    msg << "you cannot use \"super\" after a period (.), it has to be first.";
        //}

        // super should only be used in classes, but we can
        // find standalone functions using this keyword too...
        // here we search for the class and if we find it then
        // we try to get access to the extends. If the object
        // is Object, then we generate an error (i.e. there is
        // no super to Object).
        check_super_validity(name);
        node::pointer_t class_node(class_of_member(member));
        // NOTE: Interfaces can use super but we cannot
        //       know what it is at compile time.
        if(class_node != nullptr
        && class_node->get_type() == node_t::NODE_CLASS)
        {
            if(class_node->get_string() == "Object")
            {
                // this should never happen!
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
                msg << "you cannot use \"super\" within the \"Object\" class.";
            }
            else
            {
                std::size_t const max_children(class_node->get_children_size());
                for(std::size_t idx(0); idx < max_children; ++idx)
                {
                    node::pointer_t child(class_node->get_child(idx));
                    if(child->get_type() == node_t::NODE_EXTENDS)
                    {
                        if(child->get_children_size() == 1)
                        {
                            node::pointer_t child_name(child->get_child(0));
                            object = child_name->get_instance();
                        }
                        if(object == nullptr)
                        {
                            // there is another error...
                            //
                            return false;
                        }
                        break;
                    }
                }
                if(object == nullptr)
                {
                    // default to Object if no extends
                    //
                    resolve_internal_type(class_node, "Object", object);
                }
                must_find = true;
            }
        }
    }
        break;

    default:
        expression(name);
        break;

    }

    // do the field expression so we possibly detect more errors
    // in the field now instead of the next compile
    //
    node::pointer_t field(member->get_child(1));
    if(field->get_type() != node_t::NODE_IDENTIFIER)
    {
        expression(field);
    }

    if(object == nullptr)
    {
        // TODO: this is totally wrong, what we need is the type, not
        //     just the name; thus if we have a string, the type is
        //     the String class.
        //
        if(name->get_type() != node_t::NODE_IDENTIFIER
        && name->get_type() != node_t::NODE_STRING)
        {
            // A dynamic name can't be resolved now; we can only
            // hope that it will be a valid name at run time.
            // However, we still want to resolve everything we
            // can in the list of field names.
            // FYI, this happens in this case:
            //    ("test_" + var).hello
            //
            return true;
        }

        node::pointer_t all_matches(member->create_replacement(node_t::NODE_LIST));
        if(!resolve_name(name, name, object, params, all_matches, search_flags))
        {
            // we cannot even find the first name!
            // we will not search for fields since we need to have
            // an object for that purpose!
            //
            return false;
        }
    }

    // we avoid errors by returning no resolution but 'success'
    //
    if(object != nullptr)
    {
        node::pointer_t all_matches(member->create_replacement(node_t::NODE_LIST));
        bool const result(resolve_field(object, field, resolution, params, all_matches, search_flags));
        if(!result && must_find)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
            msg << "\"super\" must name a valid field of the super class.";
        }
        else
        {
            check_member(name, resolution, field);
        }
        return result;
    }

    return true;
}


void compiler::resolve_member(node::pointer_t expr, node::pointer_t params, int const search_flags)
{
//std::cerr << "compiler::resolve_member()\n";
    node::pointer_t resolution;
    if(!find_member(expr, resolution, params, search_flags))
    {
        return;
    }

    // we got a resolution; but dynamic names
    // cannot be fully resolved at compile time
    //
    if(resolution == nullptr)
    {
        return;
    }

    // the name was fully resolved, check it out
    //
    if(replace_constant_variable(expr, resolution))
    {
        // just a constant, we're done
        //
        return;
    }

    // copy the type whenever available
    //
    expr->set_instance(resolution);
    node::pointer_t type(resolution->get_type_node());
    if(type)
    {
        expr->set_type_node(type);
    }

    // if we have a Getter, transform the MEMBER into a CALL
    // to a MEMBER
    //
    if(resolution->get_type() == node_t::NODE_FUNCTION
    && resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER))
    {
//std::cerr << "CAUGHT! getter...\n";
        // so expr is a MEMBER at this time
        // it has two children
        //
        node::pointer_t left(expr->get_child(0));
        node::pointer_t right(expr->get_child(1));
        expr->delete_child(0);
        expr->delete_child(0);    // 1 is now 0

        // create a new node since we do not want to move the
        // call (expr) node from its parent.
        //
        node::pointer_t member(expr->create_replacement(node_t::NODE_MEMBER));
        member->set_instance(resolution);
        member->set_type_node(type);
        member->append_child(left);
        member->append_child(right);

        expr->append_child(member);

        // we need to change the name to match the getter
        //
        // NOTE: we know that the right data is an identifier,
        //       a v-identifier, or a string so the following
        //       will always work
        //
        std::string getter_name("->");
        getter_name += right->get_string();
        right->set_string(getter_name);

        // the call needs a list of parameters (empty)
        //
        node::pointer_t empty_params(expr->create_replacement(node_t::NODE_LIST));
        expr->append_child(empty_params);

        // and finally, we transform the member in a call!
        //
        expr->to_call();
    }
}


bool compiler::resolve_call(node::pointer_t call)
{
    std::size_t const max_children(call->get_children_size());
    if(max_children != 2)
    {
        return false;
    }

    node_lock ln(call);

    // resolve all the parameters' expressions first
    // the parameters are always in a NODE_LIST
    // and no parameters (Void) is equivalent to an empty NODE_LIST
    // and that is an expression, but we do not want to type
    // that expression since it is not necessary so we go
    // through the list here instead
    //
    node::pointer_t params(call->get_child(1));
    std::size_t const count(params->get_children_size());
    for(std::size_t idx(0); idx < count; ++idx)
    {
        node::pointer_t child(params->get_child(idx));
        expression(child);
    }

    // by default we expected an identifier (CALL to a named function)
    //
    node::pointer_t id(call->get_child(0));

    // if the CALL is to a MEMBER, then the OPERATOR flag may not yet have
    // leaked to the CALL itself, check that now
    //
    node::pointer_t sub_id;
    if(id->get_type() == node_t::NODE_MEMBER)
    {
        sub_id = id->get_child(1);
        if(sub_id->get_type() == node_t::NODE_IDENTIFIER
        && sub_id->get_flag(flag_t::NODE_IDENTIFIER_FLAG_OPERATOR))
        {
            call->set_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        }
    }

    node::pointer_t type_of_lhs;
    if(count > 0
    && count <= 2
    && call->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR))
    {
        // in this case we want to search for an operator so the
        // parameters are really 'this' (left handside) and 'rhs'
        // in that case the type of the `lhs` is used to find a
        // class and search the operator in that class
        //
        // note that for most unary operators, there is no 'rhs'
        //
        type_of_lhs = params->get_child(0)->get_type_node();
    }

    // if possible, resolve the function name
    //
    if(id->get_type() != node_t::NODE_IDENTIFIER)
    {
        // a dynamic expression cannot always be
        // resolved at compile time
        //
        node::pointer_t expr_params;
        expression(id, expr_params);

        // remove the NODE_PARAM_MATCH if there is one
        //
        if(expr_params != nullptr)
        {
            std::size_t const params_count(expr_params->get_children_size());
            if(params_count > 0)
            {
                node::pointer_t last(expr_params->get_child(params_count - 1));
                if(last->get_type() == node_t::NODE_PARAM_MATCH)
                {
                    expr_params->delete_child(params_count - 1);
                }
            }
        }

        call->set_type_node(id->get_type_node());

        return false;
    }

    int const save_errcnt(error_count());

    // straight identifiers can be resolved at compile time;
    // these need to be function names
    //
    node::pointer_t resolution;

    // if we have an lhs type, then we search that specific class and
    // that's it, this is a special case
    //
    if(type_of_lhs != nullptr
    && type_of_lhs->get_type() == node_t::NODE_CLASS)   // TBD: interface too? I think it has to be re-defined in the class and that's enough?
    {
        if(resolve_operator(type_of_lhs, id, resolution, params))
        {
            node::pointer_t operator_class(class_of_member(resolution));
            if(operator_class == nullptr)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_OPERATOR, call->get_position());
                msg << "could not determine class of \""
                    << id->get_string()
                    << "\" operator:"
                    << *resolution;
                return false;
            }

            ln.unlock();
            node::pointer_t member(call->create_replacement(node_t::NODE_MEMBER));
            call->set_child(0, member);
            // TBD: the parameters here are "left" & "right" instead of the
            //      actual parameters
            //
            node::pointer_t this_arg(params->get_child(0));
            //this_arg->set_<what?>(operator_class); // TBD: do we need this specific operator class (it may be one in the extends list of this parameter type)
            member->append_child(this_arg);
            member->append_child(id);

            call->set_instance(resolution);
            node::pointer_t type(resolution->get_type_node());
            if(type != nullptr)
            {
                call->set_type_node(type);
            }
            return true;
        }
    }

    node::pointer_t all_matches(call->create_replacement(node_t::NODE_LIST));
    if(resolve_name(id, id, resolution, params, all_matches, SEARCH_FLAG_GETTER | SEARCH_FLAG_RESOLVING_CALL))
    {
        if(resolution->get_type() == node_t::NODE_CLASS
        || resolution->get_type() == node_t::NODE_INTERFACE)
        {
            // this looks like a cast, but if the parent is
            // the NEW operator, then it is really a call!
            // yet that is caught in expression_new()
            //
            ln.unlock();
            node::pointer_t type(call->get_child(0));
            node::pointer_t expr(call->get_child(1));
            call->delete_child(0);
            call->delete_child(0);    // 1 is now 0
            call->append_child(expr);
            call->append_child(type);
            type->set_instance(resolution);
            call->to_as();
            return true;
        }

        if(resolution->get_type() == node_t::NODE_VARIABLE)
        {
            // if it is a variable, we need to check the type for
            // a "()" operator; remember that in a variable, the
            // type is defined in a sub-sub-node like so:
            //
            //     VARIABLE: '<varname>'
            //       TYPE
            //         IDENTIFIER: '<typename>'
            //
            node::pointer_t var_class(resolution->get_type_node());
            if(var_class != nullptr)
            {
                id->set_instance(var_class);
                ln.unlock();
                node::pointer_t op(call->create_replacement(node_t::NODE_IDENTIFIER));
                op->set_string("()");
                node::pointer_t func;
                if(find_field(var_class, op, func, params, all_matches, 0))
                {
                    // TODO: I think this should not be done this way...
                    //       but to resolove the issue, I have to review
                    //       the whole stack at some point.
                    //
                    if(all_matches->get_children_size() != 0)
                    {
                        if(!select_best_func(all_matches, func))
                        {
                            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_OPERATOR, call->get_position());
                            msg << "two or more functions have a similar signature.";
                            return false;
                        }
                    }

                    call->set_instance(func);
                    node::pointer_t type(func->get_type_node());
                    if(type != nullptr)
                    {
                        call->set_type_node(type);
                    }

                    if(get_attribute(call, attribute_t::NODE_ATTR_NATIVE))
                    {
                        // native means that the `f()` syntax will work as
                        // is in JavaScript (i.e. see class Function)
                        //
                        resolution = func;
                    }
                    else
                    {
                        resolution = func;

                        node::pointer_t member(call->create_replacement(node_t::NODE_MEMBER));
                        call->insert_child(0, member);
                        node::pointer_t lhs(call->get_child(1));
                        lhs->set_type_node(var_class);
                        member->append_child(lhs);
                        op->set_instance(resolution);
                        member->append_child(op);
                    }

                    return true;
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_OPERATOR, call->get_position());
                    msg << "no \"()\" operators found in \""
                        << var_class->get_string()
                        << "\".";
                    return false;
                }
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, resolution->get_position());
                msg << "getters and setters not supported yet (what is that error message saying?!).";
            }
        }
        else if(resolution->get_type() != node_t::NODE_FUNCTION)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_TYPE, id->get_position());
            msg << "\"" << id->get_string() << "\" was expected to be a type, a variable or a function.";
            return false;
        }
        //
        // If the resolution is in a class that means it is in 'this'
        // class and thus we want to change the call to a member call:
        //
        //    this.<name>(params);
        //
        // This is important for at least Flash 7 which doesn't get it
        // otherwise, I don't think it would be required otherwise (i.e Flash
        // 7.x searches for a global function on that name!)
        //
        node::pointer_t res_class(class_of_member(resolution));
        if(res_class != nullptr)
        {
            ln.unlock();
            node::pointer_t identifier(id);
            node::pointer_t member(call->create_replacement(node_t::NODE_MEMBER));
            call->set_child(0, member);
            node::pointer_t this_expr(call->create_replacement(node_t::NODE_THIS));
            member->append_child(this_expr);
            member->append_child(identifier);
        }
        call->set_instance(resolution);
        node::pointer_t type(resolution->get_type_node());
        if(type != nullptr)
        {
            call->set_type_node(type);
        }
        call_add_missing_params(call, params);
        return true;
    }

    if(save_errcnt == error_count())
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, id->get_position());
        msg << "function named \""
            << id->get_string()
            << "\" not found.";
    }

    return false;
}


bool compiler::resolve_operator(
      node::pointer_t type
    , node::pointer_t id
    , node::pointer_t & resolution
    , node::pointer_t params)
{
    // first search for the list of directives
    //
    node::pointer_t extends;
    node::pointer_t list;
    std::size_t const max_children(type->get_children_size());
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(type->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_EXTENDS:
            // recursive search in case we do not find it in this class
            //
            extends = child;
            break;

        case node_t::NODE_DIRECTIVE_LIST:
            // this is the list of declarations inside the class
            // (i.e. functions & variables)
            //
            list = child;
            break;

        default:
            // ignore anything else
            break;

        }
    }
    if(list == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, type->get_position());
        msg << "resolved operator called with the wrong node (i.e. could not find a NODE_DIRECTIVE_LIST).";
        throw as2js_exit(msg.str(), 1);
    }

    node::pointer_t expected_type;
    std::size_t const expected_parameters(params->get_children_size() - 1);
    if(expected_parameters == 1)
    {
        expected_type = params->get_child(1)->get_type_node();
    }
    std::string const & function_name(id->get_string());
    std::size_t const max_items(list->get_children_size());
    for(std::size_t idx(0); idx < max_items; ++idx)
    {
        node::pointer_t function(list->get_child(idx));
        if(function->get_type() != node_t::NODE_FUNCTION)
        {
            continue;
        }
        if(function->get_string() != function_name)
        {
            continue;
        }
        node::pointer_t function_params(function->find_first_child(node_t::NODE_PARAMETERS));
        if(function_params == nullptr
        || function_params->get_children_size() == 0)
        {
            if(expected_parameters == 0)
            {
                resolution = function;
                return true;
            }
            continue;
        }
        if(function_params->get_children_size() != 1
        || expected_parameters != 1)
        {
            // operators are already checking validity in the number
            // of parameters so if not 1 here it would already have
            // been reported
            //
            continue;
        }

        node::pointer_t rhs_param(function_params->get_child(0));
        if(rhs_param == nullptr)
        {
            continue;
        }
        node::pointer_t rhs_type(rhs_param->find_first_child(node_t::NODE_TYPE));
        if(rhs_type == nullptr)
        {
            continue;
        }
        node::pointer_t param_type(rhs_type->find_first_child(node_t::NODE_IDENTIFIER));
        if(param_type == nullptr)
        {
            continue;
        }
        if(param_type->get_type_node() == nullptr)
        {
            // TODO: determine why at times this is necessary (it should not be
            //       at this location)
            //
            node::pointer_t instance(param_type->get_instance());
            if(instance == nullptr)
            {
                throw internal_error("unknown type of identifier " + param_type->get_string());
            }
            param_type->set_type_node(instance);
        }
        if(is_derived_from(expected_type, param_type->get_type_node()))
        {
            resolution = function;
            return true;
        }
    }

    if(extends != nullptr
    && extends->get_children_size() == 1)
    {
        node::pointer_t extends_name(extends->get_child(0));
        return resolve_operator(extends_name->get_type_node(), id, resolution, params);
    }

    return false;
}


void compiler::resolve_internal_type(
      node::pointer_t parent
    , char const * type
    , node::pointer_t & resolution)
{
    // create a temporary identifier
    node::pointer_t id(parent->create_replacement(node_t::NODE_IDENTIFIER));
    id->set_string(type);

    // TBD: identifier ever needs a parent?!
    //int const idx(parent->get_children_size());
//std::cerr << "Do some invalid append now?\n";
    //parent->append_child(id);
//std::cerr << "Done the invalid append?!\n";

    // search for the identifier which is an internal type name
    bool r;
    {
        // TODO: we should be able to start the search from the native
        //       definitions since this is only used for native types
        //       (i.e. Object, Boolean, etc.)
        node_lock ln(parent);
        r = resolve_name(parent, id, resolution, node::pointer_t(), node::pointer_t(), 0);
    }

    // get rid of the temporary identifier
    //parent->delete_child(idx);

    if(!r)
    {
        // if the compiler cannot find an internal type, that is really bad!
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, parent->get_position());
        msg << "cannot find internal type \"" << type << "\".";
        throw as2js_exit(msg.str(), 1);
    }

    return;
}



}
// namespace as2js
// vim: ts=4 sw=4 et
