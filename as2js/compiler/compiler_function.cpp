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


// snapdev
//
#include    <snapdev/join_strings.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  FUNCTION  *******************************************************/
/**********************************************************************/
/**********************************************************************/


namespace
{


attribute_t g_member_function_attributes[] =
{
    attribute_t::NODE_ATTR_ABSTRACT,
    attribute_t::NODE_ATTR_STATIC,
    attribute_t::NODE_ATTR_PROTECTED,
    attribute_t::NODE_ATTR_VIRTUAL,
    attribute_t::NODE_ATTR_CONSTRUCTOR,
    attribute_t::NODE_ATTR_FINAL,
};


}
// no name namespace


void compiler::parameters(node::pointer_t parameters_node)
{
    node_lock ln(parameters_node);
    size_t max_children(parameters_node->get_children_size());

    // clear the reference flags
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t param(parameters_node->get_child(idx));
        param->set_flag(flag_t::NODE_PARAM_FLAG_REFERENCED, false);
        param->set_flag(flag_t::NODE_PARAM_FLAG_PARAMREF, false);
    }

    // verify unicity and compute the NODE_SET and parameter type
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t param(parameters_node->get_child(idx));

        // verify whether it is defined twice or more
        for(size_t k(0); k < idx; ++k)
        {
            node::pointer_t prev(parameters_node->get_child(k));
            if(prev->get_string() == param->get_string())
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, parameters_node->get_position());
                msg << "parameter \"" << param->get_string() << "\" is defined two or more times in the same list of parameters.";
                break;
            }
        }

//param.Display(stderr);

        node_lock ln_param(param);
        size_t const jmax(param->get_children_size());
        for(size_t j(0); j < jmax; ++j)
        {
            node::pointer_t child(param->get_child(j));
            switch(child->get_type())
            {
            case node_t::NODE_SET:
//std::cerr << "Oh... parameter SET!!!\n" << *child->get_child(0) << "--- parse expression now: ";
                expression(child->get_child(0));
//std::cerr << "Hmmm... what is NODE_SET opposed to NODE_ASSIGNMENT?\n";
                break;

            case node_t::NODE_TYPE:
                {
//std::cerr << "Parameter type = ...\n";
                    node::pointer_t expr(child->get_child(0));
                    expression(expr);
                    node::pointer_t type(child->get_instance());
                    if(type != nullptr)
                    {
                        node::pointer_t existing_type(param->get_type_node());
                        if(existing_type == nullptr)
                        {
                            param->set_type_node(type);
                        }
                        else if(existing_type != type)
                        {
                            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, param->get_position());
                            msg << "Existing type is:\n" << existing_type << "\nNew type would be:\n" << type << "\n";
                        }
                    }
//std::cerr << "Done with parameter type!\n";
                }
                break;

            case node_t::NODE_ASSIGNMENT:
                {
//std::cerr << "Oh... assignment of parameter!!!\n";
                    node::pointer_t expr(child->get_child(0));
                    expression(expr);
//std::cerr << "Done with parameter assignment...\n";
                }
                break;

            default:
                throw internal_error("found incompatible node in the list of parameters.");

            }
        }
    }

    // if some parameter was referenced by another, mark it as such
//std::cerr << "Check children...\n";
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t param(parameters_node->get_child(idx));
        if(param->get_flag(flag_t::NODE_PARAM_FLAG_REFERENCED))
        {
            // if referenced, we want to keep it so mark it as necessary
            param->set_flag(flag_t::NODE_PARAM_FLAG_PARAMREF, true);
        }
    }
}


void compiler::function(node::pointer_t function_node)
{
    // skip "deleted" functions
    //
    if(get_attribute(function_node, attribute_t::NODE_ATTR_UNUSED)
    || get_attribute(function_node, attribute_t::NODE_ATTR_FALSE))
    {
        return;
    }

    // Here we search for a parent for this function.
    // The parent can be a class, an interface or a package in which
    // case the function is viewed as a member. Otherwise it is
    // just a local (parent is a function) or global definition (no parents
    // of interest...) Different attributes are only valid on members
    // and some attributes have specific effects which need to be tested
    // here (i.e. a function marked final in a class cannot be overwritten.)

    node::pointer_t parent(function_node);
    node::pointer_t list;
    bool more(true);
    bool member(false);
    bool package(false);
    do
    {
        parent = parent->get_parent();
        if(parent == nullptr)
        {
            // more = false; -- not required here
            break;
        }
        switch(parent->get_type())
        {
        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
            more = false;
            member = true;
            break;

        case node_t::NODE_PACKAGE:
            more = false;
            package = true;
            break;

        case node_t::NODE_CATCH:
        case node_t::NODE_DO:
        case node_t::NODE_ELSE:
        case node_t::NODE_FINALLY:
        case node_t::NODE_FOR:
        case node_t::NODE_FUNCTION:
        case node_t::NODE_IF:
        case node_t::NODE_PROGRAM:
        case node_t::NODE_ROOT:
        case node_t::NODE_SWITCH:
        case node_t::NODE_TRY:
        case node_t::NODE_WHILE:
        case node_t::NODE_WITH:
            more = false;
            break;

        case node_t::NODE_DIRECTIVE_LIST:
            if(list != nullptr)
            {
                list = parent;
            }
            break;

        default:
            break;

        }
    }
    while(more);

    if(member && parent == nullptr)
    {
        throw internal_error("compiler_function.cpp: compiler::function(): member cannot be true if parent is null.");
    }

    // some attributes imply that the function is defined in a class
    // as a function member
    //
    if(!member)
    {
        std::vector<std::string> member_attributes;
        for(auto const & a : g_member_function_attributes)
        {
            if(get_attribute(function_node, a))
            {
                member_attributes.push_back(node::attribute_to_string(a));
            }
        }
        if(!member_attributes.empty())
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, function_node->get_position());
            msg << "function \""
                << function_node->get_string()
                << "\" was defined with attribute"
                << (member_attributes.size() == 1 ? "" : "s")
                << " \""
                << snapdev::join_strings(member_attributes, "\", \"")
                << "\" which can only be used with a function member inside a class definition.";
        }
    }
    // the operator flag also implies that the operator was defined in a class
    if(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR))
    {
        if(!member)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, function_node->get_position());
            msg << "operator \"" << function_node->get_string() << "\" can only be defined inside a class definition.";
        }
    }

    // any one of the following flags implies that the function is
    // defined in a class or a package; check to make sure!
    if(get_attribute(function_node, attribute_t::NODE_ATTR_PRIVATE))
    {
        if(!package && !member)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, function_node->get_position());
            msg << "function \""
                << function_node->get_string()
                << "\" was defined with the \"PRIVATE\" attribute which can"
                   " only be used inside a class or package definition.";
        }
    }

    // define_function_type() may be recursive so we make sure that it
    // is called before we lock function_node
//std::cerr << "Ready to check the function type!\n";
    if(!define_function_type(function_node))
    {
        return;
    }

    node::pointer_t end_list, directive_list_node, the_class;
    node_lock ln(function_node);
    size_t const max_children(function_node->get_children_size());
//std::cerr << "Function being checked has " << max_children << " children.\n";
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(function_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_PARAMETERS:
            // parse the parameters which have a default value
//std::cerr << "Checking parameters of function...\n";
            parameters(child);
            break;

        case node_t::NODE_DIRECTIVE_LIST:
//std::cerr << "Checking directive list of function...\n";
            if(get_attribute(function_node, attribute_t::NODE_ATTR_ABSTRACT))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, function_node->get_position());
                msg << "the function \"" << function_node->get_string() << "\" is marked \"ABSTRACT\" and cannot have a body.";
            }
            // find all the labels of this function
            //
            find_labels(function_node, child);

            // parse the function body
            //
            end_list = directive_list(child);
            directive_list_node = child;
            break;

        case node_t::NODE_TYPE:
            // the expression represents the function return type
//std::cerr << "Checking type of function...\n";
            if(child->get_children_size() == 1)
            {
                node::pointer_t expr(child->get_child(0));
//std::cerr << "  +--> Calculating type expression...\n";
                expression(expr);
                // constructors only support Void (or should
                // it be the same name as the class?)
//std::cerr << "  +--> Is constructor?...\n";
                if(is_constructor(function_node, the_class))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_RETURN_TYPE, function_node->get_position());
                    msg << "a constructor must return \"Void\" and nothing else, \"" << function_node->get_string() << "\" is invalid.";
                }
            }
            break;

        default:
            break;

        }
    }

    // now that the types and flags we can verify the following:
    //
    // member functions need to not be defined as final in a super class
    // since that means it cannot be overwritten
    //
    // you can overload a function, but the list of the parameter types
    // must be distinct (now that the types were computed we can go ahead
    // and verify that)
    //
    if(member)
    {
        if(check_final_functions(function_node, parent))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERLOAD, function_node->get_position());
            msg << "function \"" << function_node->get_string() << "\" was marked as final in a super class and thus it cannot be defined in class \"" << parent->get_string() << "\".";
        }
        check_unique_functions(function_node, parent, true);
    }
    else if(list != nullptr)
    {
        check_unique_functions(function_node, list, false);
    }

//std::cerr << "Okay... check the NEVER flag now\n";
    if(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_NEVER) && is_constructor(function_node, the_class))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_RETURN_TYPE, function_node->get_position());
        msg << "a constructor must return (it cannot be marked Never).";
    }

    // test for a return whenever necessary
    if(!end_list
    && directive_list_node
    && (get_attribute(function_node, attribute_t::NODE_ATTR_ABSTRACT)
            || get_attribute(function_node, attribute_t::NODE_ATTR_NATIVE))
    && (function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_VOID)
            || function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_NEVER)))
    {
        optimizer::optimize(directive_list_node);
        find_labels(function_node, directive_list_node);
        end_list = directive_list(directive_list_node);
        if(!end_list)
        {
            // TODO: we need a much better control flow to make
            // sure that this isn't a spurious error (i.e. you
            // don't need to have a return after a loop which
            // never exits)
            // It should be an error
            //f_error_stream->ErrMsg(, , );
        }
    }
}


bool compiler::define_function_type(node::pointer_t function_node)
{
    // define the type of the function when not available yet
    if(function_node->get_type_node() != nullptr)
    {
        return true;
    }

    size_t const max_children(function_node->get_children_size());
    if(max_children < 1)
    {
        // Should we put the default of Object if not VOID?
        // (see at the bottom of the function)
        //
        return function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_VOID);
    }

//std::cerr << "define_function_type() -- ???\n";
    size_t idx(0);
    {
        node_lock ln(function_node);

        for(; idx < max_children; ++idx)
        {
            node::pointer_t type(function_node->get_child(idx));
            if(type->get_type() == node_t::NODE_TYPE
            && type->get_children_size() == 1)
            {
                // then this is the type definition
                //
                node::pointer_t expr(type->get_child(0));
                expr->set_attribute_tree(attribute_t::NODE_ATTR_TYPE, true);
                expression(expr);
                node::pointer_t resolution;
                if(resolve_name(expr, expr, resolution, node::pointer_t(), 0))
                {
#if 0
                    // we may want to have that in
                    // different places for when we
                    // specifically look for a type
                    if(resolution->get_type() == node_t::NODE_FUNCTION)
                    {
                        node::pointer_t parent(resolution);
                        for(;;)
                        {
                            parent = parent->get_parent();
                            if(!parent)
                            {
                                break;
                            }
                            if(parent->get_type() == node_t::NODE_CLASS)
                            {
                                if(parent->get_string() == resolution->get_string())
                                {
                                    // ha! we hit the constructor of a class, use the class instead!
                                    resolution = parent;
                                }
                                break;
                            }
                            if(parent->get_type() == node_t::NODE_INTERFACE
                            || parent->get_type() == node_t::NODE_PACKAGE
                            || parent->get_type() == node_t::NODE_ROOT)
                            {
                                break;
                            }
                        }
                    }
#endif

//std::cerr << "  for function:\n" << *function_node << "\n";
//std::cerr << "  final function type is:\n" << *resolution << "\n";

                    ln.unlock();
                    function_node->set_type_node(resolution);
//std::cerr << "  -- type saved!!!\n";
                }
                break;
            }
        }
    }

    if(idx == max_children)
    {
        node::pointer_t the_class;
        if(is_constructor(function_node, the_class))
        {
            // With constructors we want a Void type
            node::pointer_t void_type(new node(node_t::NODE_VOID));
            function_node->set_type_node(void_type);
        }
        else
        {
            // if no type defined, put a default of Object
            node::pointer_t object;
            resolve_internal_type(function_node, "Object", object);
            function_node->set_type_node(object);
        }
    }

    return true;
}


/** \brief Check wether type t1 matches type t2.
 *
 * This function checks whether the type defined as t1 matches the
 * type defined as t2.
 *
 * t1 or t2 may be a null pointer, in which case it never matches.
 *
 * If t1 is not directly equal to t2, then all t1's ancestors are
 * checked too. The ancestors are found as extends or implements
 * of the t1 class.
 *
 * It is expected that t2 will be a NODE_PARAM in which case
 * we accept an empty node or a node without a type definition
 * as a 'match any' special type.
 *
 * Otherwise we make sure we transform the type expression in
 * a usable type and compare it with t1 and its ancestors.
 *
 * The function returns the depth at which the match occurs.
 * If a match occurs because t2 is some form of 'match any'
 * then LOWEST_DEPTH (INT_MAX / 2) is return as the depth.
 * This means it has the lowest possible priority.
 *
 * The function returns MATCH_HIGHEST_DEPTH (1) if t1 matches t2
 * directly. This is the highest possible priority so if no other
 * function matches with that depth, this is the one which is
 * going to be used.
 *
 * The function returns MATCH_NOT_FOUND (0) if it cannot find a
 * match between t1 and t2. That means no function was found here.
 *
 * \param[in] t1  A type to match against.
 * \param[in] t2  A type that has to match t1.
 *
 * \return The depth at which the type matched.
 */
depth_t compiler::match_type(node::pointer_t t1, node::pointer_t t2)
{
    // Some invalid input?
    if(!t1 || !t2)
    {
        return MATCH_NOT_FOUND;
    }

    // special case for function parameters
    if(t2->get_type() == node_t::NODE_PARAM)
    {
        if(t2->get_flag(flag_t::NODE_PARAM_FLAG_OUT))
        {
            // t1 MUST be an identifier which references
            // a variable which we can set on exit
            if(t1->get_type() != node_t::NODE_IDENTIFIER)
            {
                // NOTE: we can't generate an error here
                //     because there could be another
                //     valid function somewhere else...
                message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_MISSSING_VARIABLE_NAME, t1->get_position());
                msg << "a variable name is expected for a function parameter flagged as an OUT parameter.";
                return MATCH_NOT_FOUND;
            }
        }
        if(t2->get_children_size() == 0)
        {
            return MATCH_LOWEST_DEPTH;
        }
        node::pointer_t id(t2->get_child(0));
        // make sure we have a type definition, if it is
        // only a default set, then it is equal anyway
        if(id->get_type() == node_t::NODE_SET)
        {
            return MATCH_LOWEST_DEPTH;
        }
        node::pointer_t resolution(id->get_type_node());
        if(!resolution)
        {
            if(!resolve_name(t2, id, resolution, node::pointer_t(), 0))
            {
                return MATCH_NOT_FOUND;
            }
            id->set_type_node(resolution);
        }
        t2 = id;
    }

    node::pointer_t tp1(t1->get_type_node());
    node::pointer_t tp2(t2->get_type_node());

    if(!tp1)
    {
        type_expr(t1);
        tp1 = t1->get_type_node();
        if(!tp1)
        {
            return MATCH_HIGHEST_DEPTH;
        }
    }

// The exact same type?
    if(tp1 == tp2)
    {
        return MATCH_HIGHEST_DEPTH;
    }
    // TODO: if we keep the class <id>; definition, then we need
    //       to also check for a full definition

// if one of the types is Object, then that's a match
    node::pointer_t object;
    resolve_internal_type(t1, "Object", object);
    if(tp1 == object)
    {
        // whatever tp2, we match (bad user practice of
        // untyped variables...)
        return MATCH_HIGHEST_DEPTH;
    }
    if(tp2 == object)
    {
        // this is a "bad" match -- anything else will be better
        return MATCH_LOWEST_DEPTH;
    }
    // TODO: if we find a [class Object;] definition
    //       instead of a complete definition

    // Okay, still not equal, check ancestors of tp1 if
    // permitted (and if tp1 is a class).
    if(tp1->get_type() != node_t::NODE_CLASS)
    {
        return MATCH_NOT_FOUND;
    }

    return find_class(tp1, tp2, 2);
}


bool compiler::check_function(
          node::pointer_t function_node
        , node::pointer_t & resolution
        , std::string const & name
        , node::pointer_t params
        , int const search_flags)
{
    // The fact that a function is marked UNUSED should
    // be an error, but overloading prevents us from
    // generating an error here...
//std::cerr << "check_function(): " << function_node->get_string() << " (" << function_node->get_type_name() << ") / " << name << "\n";
//std::cerr << "check_function(): attributes\n";
    if(get_attribute(function_node, attribute_t::NODE_ATTR_UNUSED))
    {
        return false;
    }

//std::cerr << "check_function(): getter/setter or " << name << "\n";
    if(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER)
    && (search_flags & SEARCH_FLAG_GETTER) != 0)
    {
        std::string getter("->");
        getter += name;
        if(function_node->get_string() != getter)
        {
            return false;
        }
    }
    else if(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER)
         && (search_flags & SEARCH_FLAG_SETTER) != 0)
    {
        std::string setter("<-");
        setter += name;
        if(function_node->get_string() != setter)
        {
            return false;
        }
    }
    else if(function_node->get_string() != name)
    {
//std::cerr << "check_function(): false then?\n";
        return false;
    }

    // That is a function!
    // Find the perfect match (testing prototypes)
    //
    if(params == nullptr)
    {
        // getters and setters do not have parameters
        //
        if(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER)
        || function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER))
        {
            // warning: we have to check whether we hit a constructor
            //          before generating an error
            //
            node::pointer_t the_class;
            if(!is_constructor(function_node, the_class))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_MISMATCH_FUNC_VAR, function_node->get_position());
                msg << "a variable name was expected, we found the function \"" << function_node->get_string() << "\" instead.";
            }
            return false;
        }
        define_function_type(function_node);
    }

    resolution = function_node;

    return true;
}


// check whether the list of input parameters matches the function
// prototype; note that if the function is marked as "no prototype"
// then it matches automatically, but it gets a really low score.
int compiler::check_function_with_params(node::pointer_t function_node, node::pointer_t params)
{
    // At this time, I am not too sure what I can do if params is
    // null. Maybe that is when you try to do var a = <funcname>;?
    if(!params)
    {
        return 0;
    }

    node::pointer_t match(function_node->create_replacement(node_t::NODE_PARAM_MATCH));
    match->set_instance(function_node);

    // define the type of the function when not available yet
    if(!define_function_type(function_node))
    {
        // error: this function definition is no good
        //        (don't report that, we should have had an error in
        //        the parser already)
        return -1;
    }

    size_t const count(params->get_children_size());
    size_t const max_children(function_node->get_children_size());
    if(max_children == 0)
    {
        // no parameters; check whether the user specifically
        // used void or Void as the list of parameters
        if(!function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_NOPARAMS))
        {
            // TODO:
            // this function accepts whatever
            // however, the function wasn't marked as such and
            // therefore we could warn about this...
            match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
            params->append_child(match);
            return 0;
        }
        if(count == 0)
        {
            params->append_child(match);
            return 0;
        }
        // caller has one or more parameters, but function
        // only accepts 0 (i.e. Void)
        return 0;
    }

    node_lock ln_function(function_node);
    node::pointer_t parameters_node(function_node->get_child(0));
    if(parameters_node->get_type() != node_t::NODE_PARAMETERS)
    {
        match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
        params->append_child(match);
        return 0;
    }

    // params doesn't get locked, we expect to add to that list
    node_lock ln_parameters(parameters_node);
    size_t const max_parameters(parameters_node->get_children_size());
    if(max_parameters == 0)
    {
        // this function accepts 0 parameters
        if(count > 0)
        {
            // error: cannot accept any parameter
            return -1;
        }
        params->append_child(match);
        return 0;
    }

    // check whether the user marked the function as unprototyped;
    // if so, then we are done
    node::pointer_t unproto(parameters_node->get_child(0));
    if(unproto->get_flag(flag_t::NODE_PARAM_FLAG_UNPROTOTYPED))
    {
        // this function is marked to accept whatever
        match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
        params->append_child(match);
        return 0;
    }

    // we cannot choose which list to use because the user
    // parameters can be named and thus we want to search
    // the caller parameters in the function parameter list
    // and not the opposite
    size_t size(max_parameters > count ? max_parameters : count);

    match->set_param_size(size);

    size_t min(0);
    size_t rest(max_parameters);
    size_t idx;
    size_t j;
    for(idx = 0; idx < count; ++idx)
    {
        node::pointer_t p(params->get_child(idx));
        if(p->get_type() == node_t::NODE_PARAM_MATCH)
        {
            // skip NODE_PARAM_MATCH etnries
            continue;
        }

        size_t cm(p->get_children_size());
        std::string name;
        for(size_t c(0); c < cm; ++c)
        {
            node::pointer_t child(p->get_child(c));
            if(child->get_type() == node_t::NODE_NAME)
            {
                // the parameter name is specified
                if(child->get_children_size() != 1)
                {
                    // an error in the parser?
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, function_node->get_position());
                    msg << "found a NODE_NAME without children.";
                    return -1;
                }
                node::pointer_t name_node(child->get_child(0));
                if(name_node->get_type() != node_t::NODE_IDENTIFIER)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, function_node->get_position());
                    msg << "the name of a parameter needs to be an identifier.";
                    return -1;
                }
                name = name_node->get_string();
                break;
            }
        }
        // search for the parameter (fp == found parameter)
        // NOTE: because the children aren't deleted, keep a
        //     bare pointer is fine here.
        node::pointer_t fp;
        if(!name.empty())
        {
            // search for a parameter with that name
            for(j = 0; j < max_parameters; ++j)
            {
                node::pointer_t pm(parameters_node->get_child(j));
                if(pm->get_string() == name)
                {
                    fp = pm;
                    break;
                }
            }
            if(!fp)
            {
                // cannot find a parameter with that name...
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, function_node->get_position());
                msg << "no parameter named \"" << name << "\" was found in this function declaration.";
                return -1;
            }
            // if already used, make sure it is a REST node
            if(match->get_param_depth(j) != MATCH_NOT_FOUND)
            {
                if(fp->get_flag(flag_t::NODE_PARAM_FLAG_REST))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, function_node->get_position());
                    msg << "function parameter name \"" << name << "\" already used & not a \"rest\" (...).";
                    return -1;
                }
            }
        }
        else
        {
            // search for the first parameter
            // which wasn't used yet
            for(j = min; j < max_parameters; ++j)
            {
                if(match->get_param_depth(j) == MATCH_NOT_FOUND)
                {
                    fp = parameters_node->get_child(j);
                    break;
                }
            }
            min = j;
            if(j == max_parameters)
            {
                // all parameters are already taken
                // check whether the last parameter
                // is of type REST
                fp = parameters_node->get_child(max_parameters - 1);
                if(fp->get_flag(flag_t::NODE_PARAM_FLAG_REST))
                {
                    // parameters in the function list
                    // of params are all used up!
                    //
                    // TODO: we cannot err here yet; we need to do it only if none of the
                    //       entries are valid!
                    return -1;
                }
                // ha! we accept this one!
                j = rest;
                ++rest;
            }
        }
        // We reach here only if we find a parameter
        // now we need to check the type to make sure
        // it really is valid
        depth_t const depth(match_type(p, fp));
        if(depth == MATCH_NOT_FOUND)
        {
            // type does not match
            return -1;
        }
        match->set_param_depth(j, depth);
        match->set_param_index(idx, j);
    }

    // if some parameters are not defined, then we need to
    // either have a default value (initializer) or they
    // need to be marked as optional (unchecked)
    // a rest is viewed as an optional parameter
    for(j = min; j < max_parameters; ++j)
    {
        if(match->get_param_depth(j) == MATCH_NOT_FOUND)
        {
            match->set_param_index(idx, j);
            idx++;
            node::pointer_t param(parameters_node->get_child(j));
            if(param->get_flag(flag_t::NODE_PARAM_FLAG_UNCHECKED)
            || param->get_flag(flag_t::NODE_PARAM_FLAG_REST))
            {
                node::pointer_t set;
                size_t cnt(param->get_children_size());
                for(size_t k(0); k < cnt; ++k)
                {
                    node::pointer_t child(param->get_child(k));
                    if(child->get_type() == node_t::NODE_SET)
                    {
                        set = child;
                        break;
                    }
                }
                if(!set)
                {
                    // TODO: we cannot warn here, instead we need to register this function
                    //     as a possible candidate for that call in case no function does
                    //     match (and even so, in ECMAScript, we cannot really know until
                    //     run time...)
                    //std::cerr << "WARNING: missing parameters to call function.\n";
                    return -1;
                }
            }
        }
    }

    params->append_child(match);

    return 0;
}


bool compiler::best_param_match_derived_from(node::pointer_t& best, node::pointer_t match)
{
    node::pointer_t the_super_class;

    if(are_objects_derived_from_one_another(best, match, the_super_class))
    {
        // if best is in a class derived from
        // the class where we found match, then
        // this is not an error, we just keep best
        return true;
    }

    if(are_objects_derived_from_one_another(match, best, the_super_class))
    {
        // if match is in a class derived from
        // the class where we found best, then
        // this isn't an error, we just keep match
        best = match;
        return true;
    }

    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, best->get_position());
    msg << "found two functions named \"" << best->get_string() << "\" and both have the same prototype. Cannot determine which one to use.";

    return false;
}


bool compiler::best_param_match(node::pointer_t& best, node::pointer_t match)
{
    // unprototyped?
    size_t const b_sz(best->get_param_size());
    size_t const m_sz(match->get_param_size());
    if(b_sz == 0)
    {
        if(m_sz == 0)
        {
            return best_param_match_derived_from(best, match);
        }
        // best had no prototype, but match has one, so we keep match
        best = match;
        return true;
    }

    if(m_sz == 0)
    {
        // we keep best in this case since it has a prototype
        // and not match
        return true;
    }

    size_t b_more(0);
    size_t m_more(0);
    for(size_t idx(0); idx < b_sz && idx < m_sz; ++idx)
    {
        // TODO: We must verify that "idx" is correct for those calls.
        //       At this point, it seems to me that it needs to be
        //       changed to 'j' as in:
        //
        //           j = best.get_param_index(idx);
        //           depth = best.get_param_depth(j);
        //
        //       But it was like this (wrong?) in the original.
        //
        int const r(best->get_param_depth(idx) - match->get_param_depth(idx));
        if(r < 0)
        {
            b_more++;
        }
        else if(r > 0)
        {
            m_more++;
        }
    }

    // if both are 0 or both not 0 then we cannot decide
    if((b_more != 0) ^ (m_more == 0))
    {
        return best_param_match_derived_from(best, match);
    }

    // "match" is better!
    if(m_more != 0)
    {
        best = match;
    }

    return true;
}


/** \brief One or more function was found, select the best one.
 *
 * This function checks all the functions we found and selects the best
 * match according to the parameter types and count.
 *
 * \param[in] params  The parameters of the function.
 * \param[out] resolution  The function we consider to be the best.
 *
 * \return true if a best function was found and resolution set to that node.
 */
bool compiler::select_best_func(
      node::pointer_t params
    , node::pointer_t & resolution)
{
    if(params == nullptr)
    {
        throw internal_error("params cannot be a null pointer in select_best_func().");
    }

    bool found(true);

    // search for the best match
    //node_lock ln(directive_list); -- we're managing this list here
    size_t max_children(params->get_children_size());
    node::pointer_t best;
    size_t idx = 0, prev = -1;
    while(idx < max_children)
    {
        node::pointer_t match(params->get_child(idx));
        if(match->get_type() == node_t::NODE_PARAM_MATCH)
        {
            if(best)
            {
                // compare best & match
                if(!best_param_match(best, match))
                {
                    found = false;
                }
                if(best == match)
                {
                    params->delete_child(prev);
                    prev = idx;
                }
                else
                {
                    params->delete_child(idx);
                }
                // TODO: see whether we should set to unknown instead of deleting
                --max_children;
            }
            else
            {
                prev = idx;
                best = match;
                ++idx;
            }
        }
        else
        {
            ++idx;
        }
    }

    // we should always have a best node
    //
    if(best == nullptr)
    {
        throw internal_error("did not find at least one best function, even though we cannot have an empty list of choices when called.");
    }

    if(found)
    {
        // we found a better one! and no error occured
        resolution = best->get_instance();
    }

    return found;
}


bool compiler::funcs_name(int & funcs, node::pointer_t resolution, bool const increment)
{
    if(resolution != nullptr)
    {
        return true;
    }

    if(resolution->get_type() != node_t::NODE_FUNCTION)
    {
        // TODO: do we really ignore those?!
        return funcs == 0;
    }
    if(resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER)
    || resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER))
    {
        // this is viewed as a variable; also, there is no
        // parameters to a getter and thus no way to overload
        // those; the setter has a parameter though but you
        // cannot decide what it is going to be
        //
        return funcs == 0;
    }

    if(increment)
    {
        ++funcs;
    }

    return false;
}


void compiler::call_add_missing_params(node::pointer_t call, node::pointer_t params)
{
    // any children?
    std::size_t idx(params->get_children_size());
    if(idx == 0)
    {
        return;
    }

    // if we have a parameter match, it has to be at the end
    //
    --idx;
    node::pointer_t match(params->get_child(idx));
    if(match->get_type() != node_t::NODE_PARAM_MATCH)
    {
        // not a param match with a valid best match?!
        //
        throw internal_error("call_add_missing_params() called when the list of parameters do not include a NODE_PARAM_MATCH.");
    }

    // found it
    //
    // TODO: "now we want to copy the array of indices to the
    //       call instruction" -- old comment; we were copying
    //       the array pointer to the call, but I think that
    //       was only so we could delete the match node right
    //       away... maybe I am wrong now and it would be
    //       necessary to have that array in the call?
    //
    params->delete_child(idx);

    std::size_t size(match->get_param_size());
    if(idx < size)
    {
        // get the list of parameters of the function
        //
        node::pointer_t function_node(call->get_instance());
        if(function_node == nullptr)
        {
            // should never happen
            //
            return;
        }
        node::pointer_t parameters_node(function_node->find_first_child(node_t::NODE_PARAMETERS));
        if(parameters_node == nullptr)
        {
            // should never happen
            //
            return;
        }

        // functions with no parameters just have no parameters node
        //
        std::size_t max_children(parameters_node->get_children_size());
        while(idx < size)
        {
            std::size_t j(match->get_param_index(idx));
            if(j >= max_children)
            {
                throw internal_error("somehow a parameter index is larger than the maximum number of children available.");
            }
            node::pointer_t param(parameters_node->get_child(j));
            bool has_set = false;
            std::size_t const cnt(param->get_children_size());
            for(std::size_t k(0); k < cnt; ++k)
            {
                node::pointer_t set(param->get_child(k));
                if(set->get_type() == node_t::NODE_SET
                && set->get_children_size() > 0)
                {
                    has_set = true;
                    node::pointer_t auto_param(call->create_replacement(node_t::NODE_AUTO));
                    auto_param->set_instance(set->get_child(0));
                    params->append_child(auto_param);
                    break;
                }
            }
            if(!has_set)
            {
                // although it should be automatic we actually force
                // the undefined value here (we can optimize it out on
                // output later)
                node::pointer_t undefined(call->create_replacement(node_t::NODE_UNDEFINED));
                params->append_child(undefined);
            }
            idx++;
        }
    }
}


bool compiler::resolve_call(node::pointer_t call)
{
    std::size_t max_children(call->get_children_size());
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
    node::pointer_t type_of_lhs;
    node::pointer_t params(call->get_child(1));
    std::size_t const count(params->get_children_size());
    for(std::size_t idx(0); idx < count; ++idx)
    {
        node::pointer_t child(params->get_child(idx));
        expression(child);

        if(idx == 0
        && count == 2
        && call->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR))
        {
            // in this case we want to search for an operator so the
            // parameters are really 'this' (left handside) and 'rhs'
            // in that case the type of the `lhs` is used to find a
            // class and search the operator in that class
            //
            type_of_lhs = child->get_type_node();
        }
    }

    // check the name expression
    //
    node::pointer_t id(call->get_child(0));

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
        std::size_t const params_count(expr_params->get_children_size());
        if(params_count > 0)
        {
            node::pointer_t last(expr_params->get_child(params_count - 1));
            if(last->get_type() == node_t::NODE_PARAM_MATCH)
            {
                expr_params->delete_child(params_count - 1);
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
std::cerr << "  -- call was resolved via resolve_operator? resolution =\n"
<< *resolution
<< "\n";
            node::pointer_t operator_class(class_of_member(resolution));
std::cerr << "  -- call was resolved, got class? " << reinterpret_cast<void*>(operator_class.get()) << "\n";
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

    if(resolve_name(id, id, resolution, params, SEARCH_FLAG_GETTER))
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
std::cerr << "  -- oh, the resolution is a class or interface?\n";
            return true;
        }
        else if(resolution->get_type() == node_t::NODE_VARIABLE)
        {
            // if it is a variable, we need to test
            // the type for a "()" operator
            node::pointer_t var_class(resolution->get_type_node());
            if(var_class)
            {
                id->set_instance(var_class);
                // search for a function named "()"
                //NodePtr l;
                //l.CreateNode(NODE_IDENTIFIER);
                //Data& lname = l.GetData();
                //lname.f_str = "left";
                ln.unlock();
                node::pointer_t all_params(call->get_child(1));
                call->delete_child(1);
                //NodePtr op_params;
                //op_params.CreateNode(NODE_LIST);
                //op_params.AddChild(l);
                node::pointer_t op(call->create_replacement(node_t::NODE_IDENTIFIER));
                op->set_string("()");
                op->append_child(all_params);
                node::pointer_t func;
                size_t del(call->get_children_size());
                call->append_child(op);
                int funcs(0);
                bool const result(find_field(var_class, op, funcs, func, params, 0));
                call->delete_child(del);
                if(result)
                {
                    resolution = func;
                    node::pointer_t identifier(id);
                    node::pointer_t member(call->create_replacement(node_t::NODE_MEMBER));
                    call->set_child(0, member);
                    op->delete_child(0);
                    if(call->get_children_size() > 1)
                    {
                        call->set_child(1, all_params);
                    }
                    else
                    {
                        call->append_child(all_params);
                    }
                    member->append_child(identifier);
                    member->append_child(op);
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_OPERATOR, call->get_position());
                    msg << "no \"()\" operators found in \"" << var_class->get_string() << "\".";
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
std::cerr << "  -- call was resolved, got class? " << reinterpret_cast<void*>(res_class.get()) << "\n";
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
        msg << "function named \"" << id->get_string() << "\" not found.";
    }

    return false;
}


bool compiler::resolve_operator(
      node::pointer_t type
    , node::pointer_t id
    , node::pointer_t & resolution
    , node::pointer_t params)
{
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
    std::size_t const max_items(list->get_children_size());
    for(std::size_t idx(0); idx < max_items; ++idx)
    {
        node::pointer_t function(list->get_child(idx));
        if(function->get_type() != node_t::NODE_FUNCTION)
        {
            continue;
        }
        if(function->get_string() != id->get_string())
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


/** \brief Check whether that function was not marked as final before.
 *
 * \return true if the function is marked as final in a super definition.
 */
bool compiler::find_final_functions(
      node::pointer_t & function_node
    , node::pointer_t & super)
{
    std::size_t const max_children(super->get_children_size());
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(super->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_EXTENDS:
        {
            node::pointer_t next_super(child->get_instance());
            if(next_super)
            {
                if(find_final_functions(function_node, next_super)) // recursive
                {
                    return true;
                }
            }
        }
            break;

        case node_t::NODE_DIRECTIVE_LIST:
            if(find_final_functions(function_node, child)) // recursive
            {
                return true;
            }
            break;

        case node_t::NODE_FUNCTION:
            // TBD: are we not also expected to check the number of
            //      parameters to know that it is the same function?
            //      (see compare_parameters() below)
            //
            if(function_node->get_string() == child->get_string())
            {
                // we found a function of the same name
                if(get_attribute(child, attribute_t::NODE_ATTR_FINAL))
                {
                    // Ooops! it was final...
                    //
                    return true;
                }
            }
            break;

        default:
            break;

        }
    }

    return false;
}


/** \brief Check whether that function was not marked as final before.
 *
 * This function searches the children of the class node for extends. If
 * it has one or more extends, then it verifies that the super definition
 * does not mark the function as final, if such is found, we may have an
 * error.
 *
 * \note
 * Since we do not limit the number of 'extends' used with a class, we
 * check all the children to make sure we check all the possible
 * extensions.
 *
 * \param[in] function_node  The function to check.
 * \param[in] class_node  The class the function is defined in.
 *
 * \return true if the function is marked as final in a super definition.
 */
bool compiler::check_final_functions(node::pointer_t& function_node, node::pointer_t& class_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(class_node->get_child(idx));
        if(child->get_type() == node_t::NODE_EXTENDS
        && child->get_children_size() > 0)
        {
            // this points to another class which may define
            // the same function as final
            node::pointer_t name(child->get_child(0));
            node::pointer_t super(name->get_instance());
            if(super
            && find_final_functions(function_node, super))
            {
                return true;
            }
        }
    }

    return false;
}


/** \brief Compare prototypes of two functions.
 *
 * This function goes through the list of prototypes of the left
 * handed function and the right handed function and determines
 * whether the prototypes do match.
 *
 * Prototypes never match if the count is different (one function
 * has no parameters and the other has three, for example.)
 *
 * \param[in,out] lfunction  Left handed function to compare.
 * \param[in,out] rfunction  Right handed function to compare.
 *
 * \return true if the two functions prototypes are one to one equivalent.
 */
bool compiler::compare_parameters(
      node::pointer_t & lfunction
    , node::pointer_t & rfunction)
{
    // search for the list of parameters in each function
    //
    node::pointer_t lparams(lfunction->find_first_child(node_t::NODE_PARAMETERS));
    node::pointer_t rparams(rfunction->find_first_child(node_t::NODE_PARAMETERS));

    // get the number of parameters in each list
    //
    size_t const lmax(lparams ? lparams->get_children_size() : 0);
    size_t const rmax(rparams ? rparams->get_children_size() : 0);

    // if we do not have the same number of parameters, already, we know it
    // is not the same, even if one has just a rest in addition
    //
    if(lmax != rmax)
    {
        return false;
    }

    // same number of parameters, compare the types
    //
    for(size_t idx(0); idx < lmax; ++idx)
    {
        // Get the PARAM
        //
        node::pointer_t lp(lparams->get_child(idx));
        node::pointer_t rp(rparams->get_child(idx));

        // Get the type of each PARAM
        // TODO: test that lp and rp have at least one child?
        //
        node::pointer_t lt(lp->find_first_child(node_t::NODE_TYPE));
        node::pointer_t rt(rp->find_first_child(node_t::NODE_TYPE));

        if(lt->get_children_size() != 1
        || rt->get_children_size() != 1)
        {
            throw internal_error("compiler_function.cpp: compiler::compare_parameters(): unexpected number of children in NODE_TYPE.");
        }

        node::pointer_t ltype(lt->get_child(0));
        node::pointer_t rtype(rt->get_child(0));

        if(ltype->get_type() != rtype->get_type())
        {
            // they need to be the exact same type (i.e. IDENTIFIER)
            //
            return false;
        }

        node::pointer_t link_ltype(ltype->get_type_node());
        node::pointer_t link_rtype(rtype->get_type_node());

        if(link_ltype != link_rtype)
        {
            // the types are not equal
            return false;
        }
    }

    return true;
}



bool compiler::check_unique_functions(node::pointer_t function_node, node::pointer_t class_node, bool const all_levels)
{
    if(!class_node)
    {
        throw internal_error("compiler_function.cpp: compiler::check_unique_functions(): class_node cannot be a null pointer.");
    }

    std::size_t const max(class_node->get_children_size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            if(all_levels)
            {
                if(check_unique_functions(function_node, child, true)) // recursive
                {
                    return true;
                }
            }
            break;

        case node_t::NODE_FUNCTION:
            // TODO: stop recursion properly
            //
            // this condition is not enough to stop this
            // recursive process; but I think it is good
            // enough for most cases; the only problem is
            // anyway that we will eventually get the same
            // error multiple times...
            if(child == function_node)
            {
                return false;
            }

            if(function_node->get_string() == child->get_string())
            {
                if(compare_parameters(function_node, child))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, function_node->get_position());
                    msg << "you cannot define two functions with the same name (" << function_node->get_string()
                                << ") and prototype (list of parameters and their type) in the same scope, class or interface.";
                    return true;
                }
            }
            break;

        case node_t::NODE_VAR:
        {
            size_t const cnt(child->get_children_size());
            for(size_t j(0); j < cnt; ++j)
            {
                node::pointer_t variable_node(child->get_child(j));
                if(function_node->get_string() == variable_node->get_string())
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, function_node->get_position());
                    msg << "you cannot define a function and a variable (found at line #"
                            << variable_node->get_position().get_line()
                            << ") with the same name ("
                            << function_node->get_string()
                            << ") in the same scope, class or interface.";
                    return true;
                }
            }
        }
            break;

        default:
            break;

        }
    }

    return false;
}



}
// namespace as2js

// vim: ts=4 sw=4 et
