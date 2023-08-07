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
    //
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
    //
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
    //
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
            //
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
                //
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
    //
    if(end_list == nullptr
    && directive_list_node != nullptr
    && !(get_attribute(function_node, attribute_t::NODE_ATTR_ABSTRACT)
            || get_attribute(function_node, attribute_t::NODE_ATTR_NATIVE))
    && !(function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_VOID)
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

    std::size_t const max_children(function_node->get_children_size());
    if(max_children < 1)
    {
        // Should we put the default of Object if not VOID?
        // (see at the bottom of the function)
        //
        return function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_VOID);
    }

//std::cerr << "define_function_type() -- ???\n";
    std::size_t idx(0);
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
                if(resolve_name(expr, expr, resolution, node::pointer_t(), node::pointer_t(), 0))
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
            //
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
    //
    if(t1 == nullptr || t2 == nullptr)
    {
        return MATCH_NOT_FOUND;
    }

    // special case for function parameters
    //
    if(t2->get_type() == node_t::NODE_PARAM)
    {
        if(t2->get_flag(flag_t::NODE_PARAM_FLAG_OUT))
        {
            // t1 MUST be an identifier which references
            // a variable we can set on exit
            //
            if(t1->get_type() != node_t::NODE_IDENTIFIER)
            {
                // NOTE: we can't generate an error here
                //       because there could be another
                //       valid function somewhere else...
                //
                message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_MISSSING_VARIABLE_NAME, t1->get_position());
                msg << "a variable name is expected for a function parameter flagged as an OUT parameter.";
                return MATCH_NOT_FOUND;
            }
        }
        if(t2->get_children_size() == 0)
        {
            return MATCH_LOWEST_DEPTH;
        }
        node::pointer_t type_node(t2->get_child(0));
        if(type_node->get_type() != node_t::NODE_TYPE)
        {
            return MATCH_LOWEST_DEPTH;
        }

        // make sure we have a type definition, if it is
        // only a default set, then it is equal anyway
        //
        node::pointer_t id(type_node->get_child(0));
        if(id->get_type() == node_t::NODE_SET)
        {
            return MATCH_LOWEST_DEPTH;
        }
        node::pointer_t resolution(id->get_type_node());
        if(resolution == nullptr)
        {
            if(!resolve_name(type_node, id, resolution, node::pointer_t(), node::pointer_t(), 0))
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
    // The fact that a function is marked UNUSED should be an error,
    // but overriding prevents us from generating an error here...
    //
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
//std::cerr << "check_function(): false then? (name !=)\n";
        return false;
    }

    // That is a function!
    // We can collect it and later find the perfect match (testing prototypes)
    //
    if(params == nullptr)
    {
//std::cerr << "check_function(): params == nullptr\n";
        // getters and setters do not have parameters
        //
        // TBD: I think that the following test does not make sense;
        //      it's either inverted (!a && !b) or params should never
        //      be a nullptr if we find a function?
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
//
int compiler::check_function_with_params(
      node::pointer_t function_node
    , node::pointer_t params
    , node::pointer_t all_matches)
{
    if(all_matches == nullptr)
    {
        throw internal_error("all_matches not defined while calling check_function_with_params()");
    }

    // At this time, I am not too sure what I can do if params is
    // nullptr. Maybe that is when you try to do var a = <funcname>;?
    //
    if(params == nullptr)
    {
        return 0;
    }

    // define the type of the function when not available yet
    //
    if(!define_function_type(function_node))
    {
        // error: this function definition is no good
        //        (don't report that, we should have had an error in
        //        the parser already)
        //
        return -1;
    }

    // create the match node now but add it only if the function is
    // a match (i.e. unprototyped, same list of parameters, function
    // has a REST or defaults, etc.)
    //
    node::pointer_t match(function_node->create_replacement(node_t::NODE_PARAM_MATCH));
    match->set_instance(function_node);

    std::size_t const count(params->get_children_size());
    std::size_t const max_children(function_node->get_children_size());
//std::cerr << "  +--> compiler_function.cpp: params: " << count << " vs max.children " << max_children << "\n";
    if(max_children == 0)
    {
//std::cerr << "  +--> compiler_function.cpp: check_function_with_params() found no children in function_node...\n";
        // flag_t::NODE_FUNCTION_FLAG_NOPARAMS is set when the function
        // definition uses (Void) or (void) as a list of parameters
        // which means that function does not accept any parameters
        //
        // if that flag is not set, then the function accepts "whatever,"
        // which is called UNPROTOTYPED
        //
        if(!function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_NOPARAMS))
        {
            // TODO: this function accepts "whatever" but if it appears
            //       before a function that has a specific set of parameters
            //       that we can match, we have to skip it; in the end, if
            //       selected, we probably want to emit a warning
            //
            //       (i.e. we used the fallback here)
            //
            match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
            all_matches->append_child(match);
            return 0;
        }
        if(count == 0)
        {
            all_matches->append_child(match);
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
        if(!function_node->get_flag(flag_t::NODE_FUNCTION_FLAG_NOPARAMS))
        {
            match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
            all_matches->append_child(match);
            return 0;
        }
        if(count == 0)
        {
            all_matches->append_child(match);
            return 0;
        }
        return 0;
    }

    // params doesn't get locked, we expect to add to that list
    //
    node_lock ln_parameters(parameters_node);
    std::size_t const max_parameters(parameters_node->get_children_size());
    if(max_parameters == 0)
    {
        // this function accepts 0 parameters
        //
        if(count > 0)
        {
            // error: cannot accept any parameter
            //
            return -1;
        }
        all_matches->append_child(match);
        return 0;
    }

    // check whether the user marked the function as unprototyped;
    // if so, then we are done
    //
    node::pointer_t unproto(parameters_node->get_child(0));
    if(unproto->get_flag(flag_t::NODE_PARAM_FLAG_UNPROTOTYPED))
    {
        // this function is marked to accept whatever
        //
        match->set_flag(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED, true);
        all_matches->append_child(match);
        return 0;
    }

    // we cannot choose which list to use because the user
    // parameters can be named and thus we want to search
    // the caller parameters in the function parameter list
    // and not the opposite
    //
    std::size_t size(max_parameters > count ? max_parameters : count);

    match->set_param_size(size);

    std::size_t min(0);
    std::size_t rest(max_parameters);
    std::size_t idx;
    std::size_t j;
    for(idx = 0; idx < count; ++idx)
    {
        node::pointer_t p(params->get_child(idx));
        if(p->get_type() == node_t::NODE_PARAM_MATCH)
        {
            // skip NODE_PARAM_MATCH etnries
            continue;
        }

        std::size_t const cm(p->get_children_size());
        std::string name;
        for(std::size_t c(0); c < cm; ++c)
        {
            node::pointer_t child(p->get_child(c));
            if(child->get_type() == node_t::NODE_NAME)
            {
                // the parameter name is specified
                //
                if(child->get_children_size() != 1)
                {
                    // an error in the parser?
                    //
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
        //
        node::pointer_t fp;
        if(!name.empty())
        {
            // search for a parameter with that name
            //
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
                //
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, function_node->get_position());
                msg << "no parameter named \""
                    << name
                    << "\" was found in this function declaration.";
                return -1;
            }
            // if already used, make sure it is a REST node
            //
            if(match->get_param_depth(j) != MATCH_NOT_FOUND)
            {
                if(fp->get_flag(flag_t::NODE_PARAM_FLAG_REST))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, function_node->get_position());
                    msg << "function parameter name \""
                        << name
                        << "\" already used & not a \"rest\" (...) parameter.";
                    return -1;
                }
            }
        }
        else
        {
            // search for the first parameter which wasn't used yet
            //
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
                // all parameters are already taken check whether the
                // last parameter is of type REST
                //
                fp = parameters_node->get_child(max_parameters - 1);
                if(!fp->get_flag(flag_t::NODE_PARAM_FLAG_REST))
                {
                    // parameters are all used up!
                    //
                    // TODO: we cannot err here yet; we need to do it only
                    //       if none of the entries are valid!
                    return -1;
                }

                // ha! we accept this one!
                //
                j = rest;
                ++rest;
            }
        }

        // We reach here only if we find a parameter
        // now we need to check the type to make sure
        // it really is valid
        //
        depth_t const depth(match_type(p, fp));
        if(depth == MATCH_NOT_FOUND)
        {
            // type does not match
            //
            return -1;
        }
        match->set_param_depth(j, depth);
//std::cerr << "--- set param index(" << idx << ", " << j << ") in main loop...\n";
        match->set_param_index(idx, j);
    }

    // if some parameters are not defined, then we need to
    // either have a default value (initializer) or they
    // need to be marked as UNCHECKED (a.k.a. optional)
    // a REST is always viewed as an optional parameter
    //
    for(j = min; j < max_parameters; ++j)
    {
        if(match->get_param_depth(j) == MATCH_NOT_FOUND)
        {
//std::cerr << "--- set param index(" << idx << ", " << j << ") in last chance loop...\n";
            match->set_param_index(idx, j);
            idx++;
            node::pointer_t param(parameters_node->get_child(j));
            if(!param->get_flag(flag_t::NODE_PARAM_FLAG_UNCHECKED)
            && !param->get_flag(flag_t::NODE_PARAM_FLAG_REST))
            {
                node::pointer_t set(param->find_first_child(node_t::NODE_SET));
                //size_t cnt(param->get_children_size());
                //for(size_t k(0); k < cnt; ++k)
                //{
                //    node::pointer_t child(param->get_child(k));
                //    if(child->get_type() == node_t::NODE_SET)
                //    {
                //        set = child;
                //        break;
                //    }
                //}
                if(set == nullptr)
                {
                    // TODO: we cannot warn here, instead we need to register this function
                    //       as a possible candidate for that call in case no function does
                    //       match (and even so, in ECMAScript, we cannot really know until
                    //       run time...)
                    //
                    //std::cerr << "WARNING: missing parameters to call function.\n";
                    return -1;
                }
            }
        }
    }

    all_matches->append_child(match);

    return 0;
}


bool compiler::best_param_match_derived_from(node::pointer_t & best, node::pointer_t match)
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


bool compiler::best_param_match(node::pointer_t & best, node::pointer_t match)
{
    // unprototyped?
    //
    std::size_t const b_sz(best->get_param_size());
    std::size_t const m_sz(match->get_param_size());
    if(b_sz == 0)
    {
        if(m_sz == 0)
        {
            return best_param_match_derived_from(best, match);
        }
        // best had no prototype, but match has one, so we keep match
        //
        best = match;
        return true;
    }

    if(m_sz == 0)
    {
        // we keep best in this case since it has a prototype
        // and not match
        //
        return true;
    }

    std::size_t b_more(0);
    std::size_t m_more(0);
    for(std::size_t idx(0); idx < b_sz && idx < m_sz; ++idx)
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
    //
    if((b_more != 0) ^ (m_more == 0))
    {
        return best_param_match_derived_from(best, match);
    }

    // "match" is better!
    //
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
 * \param[in] all_matches  Complete list of matches while looking for a function.
 * \param[out] resolution  The function we consider to be the best.
 *
 * \return true if a best function was found and resolution set to that node.
 */
bool compiler::select_best_func(
      node::pointer_t all_matches
    , node::pointer_t & resolution)
{
    if(all_matches == nullptr)
    {
        throw internal_error("\"matches\" cannot be a null pointer in select_best_func().");
    }

    bool found(true);

    // search for the best match
    //
    std::size_t const max_children(all_matches->get_children_size());
//std::cerr << " +--> compiler_function.cpp: select_best_func() ... " << max_children << "\n";
    node::pointer_t best;
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t match(all_matches->get_child(idx));
        if(match->get_type() == node_t::NODE_PARAM_MATCH)
        {
            if(best == nullptr)
            {
                // first time, just use that match
                //
                best = match;
            }
            else
            {
                // compare best & match
                //
                node::pointer_t previous_best(best);
                if(!best_param_match(best, match))
                {
                    // this happens if two functions are considered equivalent
                    //
                    found = false;
                }
                else if(!found && previous_best != best)
                {
                    // TBD: I think that if something is better than a tie,
                    //      then it should be used hence back to true here
                    //      but this is true only if `best` changed in our
                    //      last call
                    //
                    found = true;
                }
            }
        }
    }

    // we should always have a best node
    //
    if(best == nullptr)
    {
        throw internal_error("did not find at least one best function, even though we cannot have an empty list of choices when called."); // LCOV_EXCL_LINE
    }

    if(found)
    {
        // we found a better one! and no errors occurred
        //
        resolution = best->get_instance();
    }

    return found;
}


bool compiler::funcs_name(node::pointer_t resolution, node::pointer_t all_matches)
{
    std::size_t const count(all_matches == nullptr ? 0ULL : all_matches->get_children_size());

//std::cerr << " --- +++ funcs_name() called with " << resolution.get()
//<< " all_matches? " << all_matches.get() << "\n";
    if(resolution == nullptr)
    {
        // TBD: should this one always be false?
        //
        return count == 0;
    }

    if(resolution->get_type() != node_t::NODE_FUNCTION)
    {
        // TODO: do we really ignore those?!
        //
        return count == 0;
    }
    if(resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER)
    || resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER))
    {
        // this is viewed as a variable; also, there is no
        // parameters to a getter and thus no way to overload
        // those; the setter has a parameter though but you
        // cannot decide what it is going to be
        //
        return count == 0;
    }

    return false;
}


void compiler::call_add_missing_params(node::pointer_t call, node::pointer_t params)
{
    // any children?
    //
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



} // namespace as2js
// vim: ts=4 sw=4 et
