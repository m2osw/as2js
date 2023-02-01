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





bool compiler::expression_new(node::pointer_t new_node)
{
    //
    // handle the special case of:
    //    VAR name := NEW class()
    //

    if(new_node->get_children_size() == 0)
    {
        return false;
    }

//fprintf(stderr, "BEFORE:\n");
//new_node.Display(stderr);
    node::pointer_t call(new_node->get_child(0));
    if(!call)
    {
        return false;
    }

    if(call->get_type() != node_t::NODE_CALL
    || call->get_children_size() != 2)
    {
        return false;
    }

    // get the function name
    node::pointer_t id(call->get_child(0));
    if(id->get_type() != node_t::NODE_IDENTIFIER)
    {
        return false;
    }

    // determine the types of the parameters to search a corresponding object or function
    node::pointer_t params(call->get_child(1));
    size_t const count(params->get_children_size());
//fprintf(stderr, "ResolveCall() with %d expressions\n", count);
//new_node.Display(stderr);
    for(size_t idx(0); idx < count; ++idx)
    {
        expression(params->get_child(idx));
    }

    // resolve what is named
    node::pointer_t resolution;
    if(!resolve_name(id, id, resolution, params, SEARCH_FLAG_GETTER))
    {
        // an error is generated later if this is a call and no function can be found
        return false;
    }

    // is the name a class or interface?
    if(resolution->get_type() != node_t::NODE_CLASS
    && resolution->get_type() != node_t::NODE_INTERFACE)
    {
        return false;
    }

    // move the nodes under CALL up one level
    node::pointer_t type(call->get_child(0));
    node::pointer_t expr(call->get_child(1));
    new_node->delete_child(0);      // remove the CALL
    new_node->append_child(type);   // replace with TYPE + parameters (LIST)
    new_node->append_child(expr);

//fprintf(stderr, "CHANGED TO:\n");
//new_node.Display(stderr);

    return true;
}


bool compiler::is_function_abstract(node::pointer_t function_node)
{
    size_t const max_children(function_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(function_node->get_child(idx));
        if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
        {
            return false;
        }
    }

    return true;
}


bool compiler::find_overloaded_function(
      node::pointer_t class_node
    , node::pointer_t function_node)
{
    std::size_t const max_children(class_node->get_children_size());
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_EXTENDS:
        case node_t::NODE_IMPLEMENTS:
        {
            node::pointer_t names(child->get_child(0));
            if(names->get_type() != node_t::NODE_LIST)
            {
                names = child;
            }
            std::size_t const max_names(names->get_children_size());
            for(std::size_t j(0); j < max_names; ++j)
            {
                node::pointer_t super(names->get_child(j)->get_instance());
                if(super != nullptr)
                {
                    if(is_function_overloaded(super, function_node))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case node_t::NODE_DIRECTIVE_LIST:
            if(find_overloaded_function(child, function_node))
            {
                return true;
            }
            break;

        case node_t::NODE_FUNCTION:
            if(function_node->get_string() == child->get_string())
            {
                // found a function with the same name
                //
                if(compare_parameters(function_node, child))
                {
                    // yes! it is overloaded!
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


bool compiler::is_function_overloaded(node::pointer_t class_node, node::pointer_t function_node)
{
    node::pointer_t parent(class_of_member(function_node));
    if(!parent)
    {
        throw internal_error("the parent of a function being checked for overload is not defined in a class.");
    }
    if(parent->get_type() != node_t::NODE_CLASS
    && parent->get_type() != node_t::NODE_INTERFACE)
    {
        throw internal_error("somehow the class of member is not a class or interface.");
    }
    if(parent == class_node)
    {
        return false;
    }

    return find_overloaded_function(class_node, function_node);
}


bool compiler::has_abstract_functions(node::pointer_t class_node, node::pointer_t list, node::pointer_t& func)
{
    size_t max_children(list->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(list->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_EXTENDS:
        case node_t::NODE_IMPLEMENTS:
        {
            node::pointer_t names(child->get_child(0));
            if(names->get_type() != node_t::NODE_LIST)
            {
                names = child;
            }
            size_t const max_names(names->get_children_size());
            for(size_t j(0); j < max_names; ++j)
            {
                node::pointer_t super(names->get_child(j)->get_instance());
                if(super)
                {
                    if(has_abstract_functions(class_node, super, func))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case node_t::NODE_DIRECTIVE_LIST:
            if(has_abstract_functions(class_node, child, func))
            {
                return true;
            }
            break;

        case node_t::NODE_FUNCTION:
            if(is_function_abstract(child))
            {
                // see whether it was overloaded
                if(!is_function_overloaded(class_node, child))
                {
                    // not overloaded, this class cannot
                    // be instantiated!
                    func = child;
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


void compiler::can_instantiate_type(node::pointer_t expr)
{
    if(expr->get_type() != node_t::NODE_IDENTIFIER)
    {
        // dynamic, cannot test at compile time...
        return;
    }

    node::pointer_t inst(expr->get_instance());
    if(inst->get_type() == node_t::NODE_INTERFACE)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. \"" << expr->get_string() << "\" is an interface.";
        return;
    }
    if(inst->get_type() != node_t::NODE_CLASS)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. \"" << expr->get_string() << "\" does not seem to be a class.";
        return;
    }

    // check all the functions and make sure none are [still] abstract
    // in this class...
    node::pointer_t func;
    if(has_abstract_functions(inst, inst, func))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_ABSTRACT, expr->get_position());
        msg << "the class \""
            << expr->get_string()
            << "\" has an abstract function \""
            << func->get_string()
            << "\" in file \""
            << func->get_position().get_filename()
            << "\" at line #"
            << func->get_position().get_line()
            << " and cannot be instantiated. (If you have an overloaded version of that function it may have the wrong prototype.)";
        return;
    }

    // we're fine...
}


void compiler::check_this_validity(node::pointer_t expr)
{
    node::pointer_t parent(expr);
    for(;;)
    {
        parent = parent->get_parent();
        if(!parent)
        {
            return;
        }
        switch(parent->get_type())
        {
        case node_t::NODE_FUNCTION:
            // If we are in a static function, then we
            // don't have access to 'this'. Note that
            // it doesn't matter whether we're in a
            // class or not...
            {
                node::pointer_t the_class;
                //if(parent->get_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR) -- this is wrong; we do not allow operator functions outside of a class so 'this' is always available?
                if(get_attribute(parent, attribute_t::NODE_ATTR_STATIC)
                || get_attribute(parent, attribute_t::NODE_ATTR_CONSTRUCTOR)
                || is_constructor(parent, the_class))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STATIC, expr->get_position());
                    msg << "\"this\" cannot be used in a static function nor a constructor.";
                }
            }
            return;

        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
        case node_t::NODE_PROGRAM:
        case node_t::NODE_ROOT:
            return;

        default:
            break;

        }
    }
}


void compiler::unary_operator(node::pointer_t expr)
{
    if(expr->get_children_size() != 1)
    {
        return;
    }

    char const *op(expr->operator_to_string(expr->get_type()));
    if(op == nullptr)
    {
        throw internal_error("operator_to_string() returned an empty string for a unary operator.");
    }

// TODO: the binary implementation is completely different... wondering whether
//       one or the other is "incorrect" in the current implementation?!

    node::pointer_t left(expr->get_child(0));
    node::pointer_t ltype(left->get_type_node());
    if(ltype == nullptr)
    {
//std::cerr << "WARNING: operand of unary operator is not typed.\n";
        return;
    }

    node::pointer_t l(expr->create_replacement(node_t::NODE_IDENTIFIER));
    l->set_string("left");
    l->set_type_node(ltype);

    node::pointer_t params(expr->create_replacement(node_t::NODE_LIST));
//std::cerr << "Unary operator trouble?!\n";
    params->append_child(l);
//std::cerr << "Not this one...\n";

    bool const is_post = expr->get_type() == node_t::NODE_POST_DECREMENT
                      || expr->get_type() == node_t::NODE_POST_INCREMENT;
    if(is_post)
    {
        // the post increment/decrement use an argument to distinguish
        // the pre & post operators; so add that argument now
        //
        node::pointer_t r(expr->create_replacement(node_t::NODE_IDENTIFIER));
        r->set_string("right");

        node::pointer_t rtype;
        resolve_internal_type(expr, "Number", rtype);
        expr->set_type_node(rtype);

        params->append_child(r);
    }

    node::pointer_t id(expr->create_replacement(node_t::NODE_IDENTIFIER));
    id->set_string(op);
    id->append_child(params);
//std::cerr << "Not that one either...\n";

    size_t const del(expr->get_children_size());
    expr->append_child(id);
//std::cerr << "What about append of the ID?...\n";

// TODO: see binary to convert the followingcorrectly.
    node::pointer_t resolution;
    int funcs = 0;
    bool result;
    {
        node_lock ln(expr);
        result = find_field(ltype, id, funcs, resolution, params, 0);
    }

    expr->delete_child(del);
    if(!result)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, expr->get_position());
        msg << "cannot apply operator \"" << op << "\" to this object.";
        return;
    }

//std::cerr << "Found operator!!!\n";

    node::pointer_t op_type(resolution->get_type_node());

    if(get_attribute(resolution, attribute_t::NODE_ATTR_NATIVE))
    {
        switch(expr->get_type())
        {
        case node_t::NODE_INCREMENT:
        case node_t::NODE_DECREMENT:
        case node_t::NODE_POST_INCREMENT:
        case node_t::NODE_POST_DECREMENT:
            {
                node::pointer_t var_node(left->get_instance());
                if(var_node)
                {
                    if((var_node->get_type() == node_t::NODE_PARAM || var_node->get_type() == node_t::NODE_VARIABLE)
                    && var_node->get_flag(flag_t::NODE_VARIABLE_FLAG_CONST))
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, expr->get_position());
                        msg << "cannot increment or decrement a constant variable or function parameters.";
                    }
                }
            }
            break;

        default:
            break;

        }
        // we keep intrinsic operators as is
//std::cerr << "It is intrinsic...\n";
        expr->set_instance(resolution);
        expr->set_type_node(op_type);
        return;
    }
//std::cerr << "Not intrinsic...\n";

    id->set_instance(resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer won't
    // otherwise understand this operator!
    id->delete_child(0);
    id->set_type_node(op_type);

    // move operand in the new expression
    expr->delete_child(0);

    // TODO:
    // if the unary operator is post increment or decrement
    // then we need a temporary variable to save the current
    // value of the expression, compute the expression + 1
    // and restore the temporary

    node::pointer_t post_list;
    node::pointer_t assignment;
    if(is_post)
    {
        post_list = expr->create_replacement(node_t::NODE_LIST);
        // TODO: should the list get the input type instead?
        post_list->set_type_node(op_type);

        node::pointer_t temp_var(expr->create_replacement(node_t::NODE_IDENTIFIER));
        temp_var->set_string("#temp_var#");

        // Save that name for next reference!
        assignment = expr->create_replacement(node_t::NODE_ASSIGNMENT);
//std::cerr << "assignment temp_var?!\n";
        assignment->append_child(temp_var);
//std::cerr << "assignment left?!\n";
        assignment->append_child(left);

//std::cerr << "post list assignment?!\n";
        post_list->append_child(assignment);
    }

    node::pointer_t call(expr->create_replacement(node_t::NODE_CALL));
    call->set_type_node(op_type);
    node::pointer_t member(expr->create_replacement(node_t::NODE_MEMBER));
    node::pointer_t function_node;
    resolve_internal_type(expr, "Function", function_node);
    member->set_type_node(function_node);
//std::cerr << "call member?!\n";
    call->append_child(member);

    // we need a function to get the name of 'type'
    //Data& type_data = type.GetData();
    //NodePtr object;
    //object.CreateNode(NODE_IDENTIFIER);
    //Data& obj_data = object.GetData();
    //obj_data.f_str = type_data.f_str;
    if(is_post)
    {
        // TODO: we MUST call the object defined
        //       by the left expression and NOT what
        //       I'm doing here; that's all wrong!!!
        //       for that we either need a "clone"
        //       function or a dual (or more)
        //       parenting...
        node::pointer_t r(expr->create_replacement(node_t::NODE_IDENTIFIER));
        if(left->get_type() == node_t::NODE_IDENTIFIER)
        {
            r->set_string(left->get_string());
            // TODO: copy the links, flags, etc.
        }
        else
        {
            // TODO: use the same "temp var#" name
            r->set_string("#temp_var#");
        }

//std::cerr << "member r?!\n";
        member->append_child(r);
    }
    else
    {
//std::cerr << "member left?!\n";
        member->append_child(left);
    }
//std::cerr << "member id?!\n";
    member->append_child(id);

//std::cerr << "NOTE: add a list (no params)\n";
    node::pointer_t list(expr->create_replacement(node_t::NODE_LIST));
    list->set_type_node(op_type);
//std::cerr << "call and list?!\n";
    call->append_child(list);

    if(is_post)
    {
//std::cerr << "post stuff?!\n";
        post_list->append_child(call);

        node::pointer_t temp_var(expr->create_replacement(node_t::NODE_IDENTIFIER));
        // TODO: use the same name as used in the 1st temp_var#
        temp_var->set_string("#temp_var#");
//std::cerr << "temp var stuff?!\n";
        post_list->append_child(temp_var);

        expr->get_parent()->set_child(expr->get_offset(), post_list);
        //expr = post_list;
    }
    else
    {
        expr->get_parent()->set_child(expr->get_offset(), call);
        //expr = call;
    }

//std::cerr << expr << "\n";
}


void compiler::binary_operator(node::pointer_t & expr)
{
    if(expr->get_children_size() != 2)
    {
        return;
    }

    char const *op(expr->operator_to_string(expr->get_type()));
    if(op == nullptr)
    {
        throw internal_error("complier_expression.cpp: compiler::binary_operator(): operator_to_string() returned an empty string for a binary operator.");
    }

    node::pointer_t left(expr->get_child(0));
    node::pointer_t ltype(left->get_type_node());
    if(ltype == nullptr)
    {
        return;
    }

    node::pointer_t right(expr->get_child(1));
    node::pointer_t rtype(right->get_type_node());
    if(rtype == nullptr)
    {
        return;
    }

    node::pointer_t l(expr->create_replacement(node_t::NODE_IDENTIFIER));
    l->set_string("left");
    l->set_type_node(ltype);

    node::pointer_t r(expr->create_replacement(node_t::NODE_IDENTIFIER));
    r->set_string("right");
    r->set_type_node(rtype);

    node::pointer_t params(expr->create_replacement(node_t::NODE_LIST));
    params->append_child(l);
    params->append_child(r);

    node::pointer_t id(expr->create_replacement(node_t::NODE_IDENTIFIER));
    id->set_string(op);

    node::pointer_t call(expr->create_replacement(node_t::NODE_CALL));
    call->set_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
    call->append_child(id);
    call->append_child(params);

    // temporarily add the call to expr
    //
    std::size_t const del(expr->get_children_size());
    expr->append_child(call);

//std::cerr << "----------------- search for " << id->get_string()
//<< " operator using resolve_call()...\n" << *call
//<< "with left type:\n" << *ltype
//<< "with right type:\n" << *rtype
//<< "\n";

    bool const resolved(resolve_call(call));

    // get rid of the NODE_CALL we added earlier
    //
    expr->delete_child(del);

    if(resolved)
    {
std::cerr << "\n----------------- that worked!!! what do we do now?! expr children: "
<< expr->get_children_size()
<< " & call children: " << call->get_children_size()
<< " ... with old node:\n"
<< *expr
<< "\n";

        // here we have a few cases to handle:
        //
        // 1. the operation is a native one, then we do nothing
        //    (just mark the node as DEFINED, etc.)
        //
        // 2. the operation is a native one, but the function has a body
        //    (an addition by us which is not intrinsically implemented)
        //    then we add the function body inline; later we optimize
        //    those into expressions if at all possible
        //
        // 3. the operation is not native, then we change the operator
        //    to a call; if marked inline, the optimizer may inline the
        //    code later (not now)

        // replace the operator with a NODE_CALL instead
        //
        if(!expr->to_call())
        {
            // this should not happen unless we missed a binary operator
            // in the to_call() function
            //
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "could not convert binary operator \""
                << op
                << "\" to a CALL.";
            throw as2js_exit(msg.str(), 1);
        }
        //expr->set_flag(flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        // the resolve_call() function already created the necessary
        // MEMBER + this.<operator> so just copy that here
        //
        node::pointer_t function(call->get_child(0));
        function->set_child(0, left);
        //node::pointer_t parameters(call->get_child(1)); -- this are the l & r from above (not useful)

        params = expr->create_replacement(node_t::NODE_LIST);
        params->append_child(right);

        // we just moved those parameters, so we cannot use the set_child()
        //expr->set_child(0, function);
        //expr->set_child(1, params);
        expr->append_child(function);
        expr->append_child(params);

        expr->set_type_node(call->get_type_node());

std::cerr << "  -- now the node is:\n"
<< *expr
<< "\n";

        return;
    }

std::cerr << "----------------- search for " << id->get_string() << " operator...\n";
    node::pointer_t resolution;
    bool result;
    {
        node_lock ln(expr);
        result = resolve_name(id, id, resolution, params, 0);
        // TBD: not too sure what I meant to try otherwise, but that second
        //      call is exactly the same as the first so no need at this point
        //if(!result)
        //{
        //    result = resolve_name(id, id, resolution, params, 0);
        //}
    }

    if(!result)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, expr->get_position());
        msg << "cannot apply operator \"" << op << "\" to these objects.";
        return;
    }

    node::pointer_t op_type(resolution->get_type_node());

    if(get_attribute(resolution, attribute_t::NODE_ATTR_NATIVE))
    {
        // we keep intrinsic operators as is
        expr->set_instance(resolution);
        expr->set_type_node(op_type);
        return;
    }

    call->set_instance(resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer will
    // not otherwise understand this as is!
    call->delete_child(1);
    call->delete_child(0);
    call->set_type_node(op_type);

    // move left and right in the new expression
    expr->delete_child(1);
    expr->delete_child(0);

    node::pointer_t member(expr->create_replacement(node_t::NODE_MEMBER));;
    node::pointer_t function_node;
    resolve_internal_type(expr, "Function", function_node);
    member->set_type_node(function_node);
    call->append_child(member);

    member->append_child(left);
    member->append_child(id);

    node::pointer_t list(expr->create_replacement(node_t::NODE_LIST));
    list->set_type_node(op_type);
    list->append_child(right);
    call->append_child(list);

    expr->replace_with(call);
}


bool compiler::special_identifier(node::pointer_t expr)
{
    // all special identifiers are defined as "__...__"
    // that means they are at least 5 characters and they need to
    // start with '__'

    std::string const id(expr->get_string());
//std::cerr << "ID [" << id << "]\n";
    if(id.length() < 5)
    {
        return false;
    }
    if(id[0] != '_' || id[1] != '_')
    {
        return false;
    }

    // in case an error occurs
    const char *what = "?";

    node::pointer_t parent(expr);
    std::string result;
    if(id == "__FUNCTION__")
    {
        what = "a function";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == node_t::NODE_PACKAGE
            || parent->get_type() == node_t::NODE_PROGRAM
            || parent->get_type() == node_t::NODE_ROOT
            || parent->get_type() == node_t::NODE_INTERFACE
            || parent->get_type() == node_t::NODE_CLASS)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == node_t::NODE_FUNCTION)
            {
                break;
            }
        }
    }
    else if(id == "__CLASS__")
    {
        what = "a class";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == node_t::NODE_PACKAGE
            || parent->get_type() == node_t::NODE_PROGRAM
            || parent->get_type() == node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == node_t::NODE_CLASS)
            {
                break;
            }
        }
    }
    else if(id == "__INTERFACE__")
    {
        what = "an interface";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == node_t::NODE_PACKAGE
            || parent->get_type() == node_t::NODE_PROGRAM
            || parent->get_type() == node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == node_t::NODE_INTERFACE)
            {
                break;
            }
        }
    }
    else if(id == "__PACKAGE__")
    {
        what = "a package";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == node_t::NODE_PROGRAM
            || parent->get_type() == node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == node_t::NODE_PACKAGE)
            {
                break;
            }
        }
    }
    else if(id == "__NAME__")
    {
        what = "any function, class, interface or package";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == node_t::NODE_PROGRAM
            || parent->get_type() == node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == node_t::NODE_FUNCTION
            || parent->get_type() == node_t::NODE_CLASS
            || parent->get_type() == node_t::NODE_INTERFACE
            || parent->get_type() == node_t::NODE_PACKAGE)
            {
                if(result.empty())
                {
                    result = parent->get_string();
                }
                else
                {
                    // TODO: create the + operator
                    //       on string.
                    std::string p(parent->get_string());
                    p += ".";
                    p += result;
                    result = p;
                }
                if(parent->get_type() == node_t::NODE_PACKAGE)
                {
                    // we do not really care if we
                    // are yet in another package
                    // at this time...
                    break;
                }
            }
        }
    }
    else if(id == "__TIME__")
    {
        //what = "time";
        char buf[256];
        struct tm *t;
        time_t const now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__DATE__")
    {
        //what = "date";
        char buf[256];
        struct tm *t;
        time_t const now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__UNIXTIME__")
    {
        if(!expr->to_integer())
        {
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "somehow could not change expression to integer.";
            throw as2js_exit(msg.str(), 1);
        }
        integer i;
        time_t const now(f_time);       // should this be time when we start compiling instead of different each time we hit this statement?
        i.set(now);
        expr->set_integer(i);
        return true;
    }
    else if(id == "__UTCTIME__")
    {
        //what = "utctime";
        char buf[256];
        struct tm *t;
        time_t const now(f_time);
        t = gmtime(&now);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__UTCDATE__")
    {
        //what = "utcdate";
        char buf[256];
        struct tm *t;
        time_t const now(f_time);
        t = gmtime(&now);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__DATE822__")
    {
        // Sun, 06 Nov 2005 11:57:59 -0800
        //what = "utcdate";
        char buf[256];
        struct tm *t;
        time_t const now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %T %z", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else
    {
        // not a special identifier
        return false;
    }

    // even if it fails, we convert this expression into a string
    if(!expr->to_string())
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
        msg << "somehow could not change expression to a string.";
        throw as2js_exit(msg.str(), 1);
    }
    if(!result.empty())
    {
        expr->set_string(result);

//fprintf(stderr, "SpecialIdentifier Result = [%.*S]\n", result.GetLength(), result.Get());

    }
    else if(!parent)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "\"" << id << "\" was used outside " << what << ".";
        // we keep the string as is!
    }
    else
    {
        expr->set_string(parent->get_string());
    }

    return true;
}


void compiler::type_expr(node::pointer_t expr)
{
    // already typed?
    //
    if(expr->get_type_node())
    {
        return;
    }

    node::pointer_t resolution;

    switch(expr->get_type())
    {
    case node_t::NODE_STRING:
        resolve_internal_type(expr, "String", resolution);
        expr->set_type_node(resolution);
        break;

    case node_t::NODE_INTEGER:
        resolve_internal_type(expr, "Integer", resolution);
        expr->set_type_node(resolution);
        break;

    case node_t::NODE_FLOATING_POINT:
        resolve_internal_type(expr, "Double", resolution);
        expr->set_type_node(resolution);
        break;

    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        resolve_internal_type(expr, "Boolean", resolution);
        expr->set_type_node(resolution);
        break;

    case node_t::NODE_OBJECT_LITERAL:
        resolve_internal_type(expr, "Object", resolution);
        expr->set_type_node(resolution);
        break;

    case node_t::NODE_ARRAY_LITERAL:
        resolve_internal_type(expr, "Array", resolution);
        expr->set_type_node(resolution);
        break;

    default:
    {
        node::pointer_t node(expr->get_instance());
        if(node == nullptr)
        {
            break;
        }
        if(node->get_type() != node_t::NODE_VARIABLE
        || node->get_children_size() == 0)
        {
            break;
        }
        node::pointer_t type(node->get_child(0));
        if(type->get_type() == node_t::NODE_SET)
        {
            break;
        }
        // TBD: it looks like I made a "small" mistake here and needed
        //      yet another level (i.e. here type is a TYPE node and
        //      node the actual type)
        //
        if(type->get_type() == node_t::NODE_TYPE)
        {
            if(type->get_children_size() == 0)
            {
                // TODO: should we have an error here if no children defined?
                break;
            }
            type = type->get_child(0);
        }
        else
        {
std::cerr << "--- DEBUGGING: it looks like the type is not always NODE_TYPE here...\n";
        }

        node::pointer_t instance(type->get_instance());
        if(instance == nullptr)
        {
            // TODO: resolve that if not done yet (it should
            //       always already be at this time)
            //
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "type is missing when it should not.";
            throw as2js_exit(msg.str(), 1);
        }
        expr->set_type_node(instance);
    }
        break;

    }

    return;
}


void compiler::object_literal(node::pointer_t expr)
{
    // define the type of the literal (i.e. Object)
    type_expr(expr);

    // go through the list of names and
    //    1) make sure property names are unique
    //    2) make sure property names are proper
    //    3) compile expressions
    size_t const max_children(expr->get_children_size());
    if((max_children & 1) != 0)
    {
        // invalid?!
        // the number of children must be even to support pairs of
        // names and a values
        return;
    }

    for(size_t idx(0); idx < max_children; idx += 2)
    {
        node::pointer_t name(expr->get_child(idx));
        size_t const cnt(name->get_children_size());
        if(name->get_type() == node_t::NODE_TYPE)
        {
            // the first child is a dynamic name(space)
            expression(name->get_child(0));
            if(cnt == 2)
            {
                // TODO: this is a scope
                //    name.GetChild(0) :: name.GetChild(1)
                // ...
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, name->get_position());
                msg << "scopes not supported yet. (1)";
            }
        }
        else if(cnt == 1)
        {
            // TODO: this is a scope
            //    name :: name->get_child(0)
            // Here name is IDENTIFIER, PRIVATE or PUBLIC
            // ...
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, name->get_position());
            msg << "scopes not supported yet. (2)";
        }

        // compile the value
        node::pointer_t value(expr->get_child(idx + 1));
        expression(value);
    }
}


void compiler::assignment_operator(node::pointer_t expr)
{
    bool is_var = false;

    node::pointer_t var_node;    // in case this assignment is also a definition

    node::pointer_t left(expr->get_child(0));
    if(left->get_type() == node_t::NODE_IDENTIFIER)
    {
        // this may be like a VAR <name> = ...
        node::pointer_t resolution;
        if(resolve_name(left, left, resolution, node::pointer_t(), 0))
        {
            bool valid(false);
            if(resolution->get_type() == node_t::NODE_VARIABLE)
            {
                if(resolution->get_flag(flag_t::NODE_VARIABLE_FLAG_CONST))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, left->get_position());
                    msg << "you cannot assign a value to the constant variable \"" << resolution->get_string() << "\".";
                }
                else
                {
                    valid = true;
                }
            }
            else if(resolution->get_type() == node_t::NODE_PARAM)
            {
                if(resolution->get_flag(flag_t::NODE_PARAM_FLAG_CONST))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, left->get_position());
                    msg << "you cannot assign a value to the constant function parameter \"" << resolution->get_string() << "\".";
                }
                else
                {
                    valid = true;
                }
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERLOAD, left->get_position());
                msg << "you cannot assign but a variable or a function parameter.";
            }
            if(valid)
            {
                left->set_instance(resolution);
                left->set_type_node(resolution->get_type_node());
            }
        }
        else
        {
            // it is a missing VAR!
            is_var = true;

            // we need to put this variable in the function
            // in which it is encapsulated, if there is
            // such a function so it can be marked as local
            // for that we create a var ourselves
            node::pointer_t variable_node;
            node::pointer_t set;
            var_node = expr->create_replacement(node_t::NODE_VAR);
            var_node->set_flag(flag_t::NODE_VARIABLE_FLAG_TOADD, true);
            var_node->set_flag(flag_t::NODE_VARIABLE_FLAG_DEFINING, true);
            variable_node = expr->create_replacement(node_t::NODE_VARIABLE);
            var_node->append_child(variable_node);
            variable_node->set_string(left->get_string());
            node::pointer_t parent(left);
            node::pointer_t last_directive;
            for(;;)
            {
                parent = parent->get_parent();
                if(parent->get_type() == node_t::NODE_DIRECTIVE_LIST)
                {
                    last_directive = parent;
                }
                else if(parent->get_type() == node_t::NODE_FUNCTION)
                {
                    variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_LOCAL, true);
                    parent->add_variable(variable_node);
                    break;
                }
                else if(parent->get_type() == node_t::NODE_PROGRAM
                     || parent->get_type() == node_t::NODE_CLASS
                     || parent->get_type() == node_t::NODE_INTERFACE
                     || parent->get_type() == node_t::NODE_PACKAGE)
                {
                    // not found?!
                    break;
                }
            }
            left->set_instance(variable_node);

            // We cannot call InsertChild() here since it would be in our
            // locked parent. So instead we only add it to the list of
            // variables of the directive list and later we will also add it
            // at the top of the list
            //
            if(last_directive)
            {
                //parent->insert_child(0, var_node);
                last_directive->add_variable(variable_node);
                last_directive->set_flag(flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES, true);
            }
        }
    }
    else if(left->get_type() == node_t::NODE_MEMBER)
    {
        // we parsed?
        if(!left->get_type_node())
        {
            // try to optimize the expression before to compile it
            // (it can make a huge difference!)
            optimizer::optimize(left);
            //node::pointer_t right(expr->get_child(1));

            resolve_member(left, 0, SEARCH_FLAG_SETTER);

            // setters have to be treated here because within ResolveMember()
            // we do not have access to the assignment and that's what needs
            // to change to a call.
            //
            node::pointer_t resolution(left->get_instance());
            if(resolution)
            {
                if(resolution->get_type() == node_t::NODE_FUNCTION
                && resolution->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER))
                {
                    // TODO: handle setters -- this is an old comment
                    //       maybe it was not deleted? I do not think
                    //       that these work properly yet, but it looks
                    //       like I already started work on those.
//std::cerr << "CAUGHT! setter...\n";
                    // so expr is a MEMBER at this time
                    // it has two children
                    //NodePtr left = expr.GetChild(0);
                    node::pointer_t right(expr->get_child(1));
                    //expr.DeleteChild(0);
                    //expr.DeleteChild(1);    // 1 is now 0

                    // create a new node since we don't want to move the
                    // call (expr) node from its parent.
                    //NodePtr member;
                    //member.CreateNode(NODE_MEMBER);
                    //member.SetLink(NodePtr::LINK_INSTANCE, resolution);
                    //member.AddChild(left);
                    //member.AddChild(right);
                    //member.SetLink(NodePtr::LINK_TYPE, type);

                    //expr.AddChild(left);

                    // we need to change the name to match the getter
                    // NOTE: we know that the field data is an identifier
                    //     a v-identifier or a string so the following
                    //     will always work
                    node::pointer_t field(left->get_child(1));
                    std::string getter_name("<-");
                    getter_name += field->get_string();
                    field->set_string(getter_name);

                    // the call needs a list of parameters (1 parameter)
                    //
                    node::pointer_t params(expr->create_replacement(node_t::NODE_LIST));
                    /*
                    NodePtr this_expr;
                    this_expr.CreateNode(NODE_THIS);
                    params.AddChild(this_expr);
                    */
                    expr->set_child(1, params);

                    params->append_child(right);


                    // and finally, we transform the member in a call!
                    //
                    expr->to_call();
                }
            }
        }
    }
    else
    {
        // Is this really acceptable?!
        // We can certainly make it work in Macromedia Flash...
        // If the expression is resolved as a string which is
        // also a valid variable name.
        expression(left);
    }

    node::pointer_t right(expr->get_child(1));
    expression(right);

    if(var_node)
    {
        var_node->set_flag(flag_t::NODE_VARIABLE_FLAG_DEFINING, false);
    }

    node::pointer_t type(left->get_type_node());
    if(type)
    {
        expr->set_type_node(type);
        return;
    }

    if(!is_var)
    {
        // if left not typed, use right type!
        // (the assignment is this type of special case...)
        expr->set_type_node(right->get_type_node());
    }
}


void compiler::expression(node::pointer_t expr, node::pointer_t params)
{
    // we already came here on that one?
    if(expr->get_type_node())
    {
        return;
    }

    // try to optimize the expression before compiling it
    // (it can make a huge difference!)
    optimizer::optimize(expr);

    switch(expr->get_type())
    {
    case node_t::NODE_STRING:
    case node_t::NODE_INTEGER:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        type_expr(expr);
        return;

    case node_t::NODE_ARRAY_LITERAL:
        type_expr(expr);
        break;

    case node_t::NODE_OBJECT_LITERAL:
        object_literal(expr);
        optimizer::optimize(expr);
        type_expr(expr);
        return;

    case node_t::NODE_NULL:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_UNDEFINED:
        return;

    case node_t::NODE_SUPER:
        check_super_validity(expr);
        return;

    case node_t::NODE_THIS:
        check_this_validity(expr);
        return;

    case node_t::NODE_ADD:
    case node_t::NODE_ARRAY:
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
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IN:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INSTANCEOF:
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
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_RANGE:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_TYPEOF:
        break;

    case node_t::NODE_NEW:
        // TBD: we later check whether we can instantiate this 'expr'
        //      object; but if we return here, then that test will
        //      be skipped (unless the return is inapropriate or
        //      we should have if(!expression_new(expr)) ...
        if(expression_new(expr))
        {
            optimizer::optimize(expr);
            type_expr(expr);
            return;
        }
        break;

    case node_t::NODE_VOID:
        // If the expression has no side effect (i.e. doesn't
        // call a function, doesn't use ++ or --, etc.) then
        // we don't even need to keep it! Instead we replace
        // the void by undefined.
        //
        if(expr->has_side_effects())
        {
            // we need to keep some of this expression
            //
            // TODO: we need to optimize better; this
            // should only keep expressions with side
            // effects and not all expressions; for
            // instance:
            //    void (a + b(c));
            // should become:
            //    void b(c);
            // (assuming that 'a' isn't a call to a getter
            // function which could have a side effect)
            break;
        }
        // this is what void returns, assuming the expression
        // had no side effect, that's all we need here
        expr = expr->create_replacement(node_t::NODE_UNDEFINED);
        return;

    case node_t::NODE_ASSIGNMENT:
        assignment_operator(expr);
        optimizer::optimize(expr);
        type_expr(expr);
        return;

    case node_t::NODE_FUNCTION:
        function(expr);
        optimizer::optimize(expr);
        type_expr(expr);
        return;

    case node_t::NODE_MEMBER:
        resolve_member(expr, params, SEARCH_FLAG_GETTER);
        optimizer::optimize(expr);
        type_expr(expr);
        return;

    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_VIDENTIFIER:
        if(!special_identifier(expr))
        {
            node::pointer_t resolution;
//std::cerr << "Not a special identifier so resolve name... [" << *expr << "]\n";
            if(resolve_name(expr, expr, resolution, params, SEARCH_FLAG_GETTER))
            {
//std::cerr << "  +--> returned from resolve_name() with resolution\n";
                if(!replace_constant_variable(expr, resolution))
                {
                    node::pointer_t current(expr->get_instance());
//std::cerr << "  +--> not constant var... [" << (current ? "has a current ptr" : "no current ptr") << "]\n";
                    if(current)
                    {
                        if(current != resolution)
                        {
//std::cerr << "Expression already typed is (starting from parent): [" << *expr->get_parent() << "]\n";
                            // TBD: I am not exactly sure what this does right now, we
                            //      probably can ameliorate the error message, although
                            //      we should actually never get it!
                            throw internal_error("The link instance of this [V]IDENTIFIER was already defined...");
                        }
                        // should the type be checked in this case too?
                    }
                    else
                    {
                        expr->set_instance(resolution);
                        node::pointer_t type(resolution->get_type_node());
//std::cerr << "  +--> so we got an instance... [" << (type ? "has a current type ptr" : "no current type ptr") << "]\n";
                        if(type
                        && !expr->get_type_node())
                        {
                            expr->set_type_node(type);
                        }
                    }
                }
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, expr->get_position());
                msg << "cannot find any variable or class declaration for: \"" << expr->get_string() << "\".";
            }
//std::cerr << "---------- got type? ----------\n";
        }
        optimizer::optimize(expr);
        type_expr(expr);
        return;

    case node_t::NODE_CALL:
        if(resolve_call(expr))
        {
            optimizer::optimize(expr);
            type_expr(expr);
        }
        return;

    default:
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "unhandled expression data type \"" << expr->get_type_name() << "\".";
        }
        return;

    }

    // When we reach here, we want that expression to
    // compile all the children nodes as expressions.
    //
    std::size_t const max_children(expr->get_children_size());
    {
        node_lock ln(expr);
        for(std::size_t idx(0); idx < max_children; ++idx)
        {
            node::pointer_t child(expr->get_child(idx));

            // skip labels
            //
            if(child->get_type() != node_t::NODE_NAME)
            {
                expression(child); // recursive!
            }
            // TODO:
            // Do we want/have to do the following?
            //else if(child->get_children_size() > 0)
            //{
            //    node::pointer_t sub_expr(child->get_child(0));
            //    expression(sub_expr);
            //}
        }
    }

    // Now check for operators to give them a type
    //
    switch(expr->get_type())
    {
    case node_t::NODE_ADD:
    case node_t::NODE_SUBTRACT:
        if(max_children == 1)
        {
            unary_operator(expr);
        }
        else
        {
            binary_operator(expr);
        }
        break;

    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
        unary_operator(expr);
        break;

    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_POWER:
    case node_t::NODE_RANGE:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
        binary_operator(expr);
        break;

    case node_t::NODE_IN:
    case node_t::NODE_CONDITIONAL:    // cannot be overwritten!
        break;

    case node_t::NODE_ARRAY:
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_AS:
    case node_t::NODE_DELETE:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_IS:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_VOID:
        // nothing special we can do here...
        break;

    case node_t::NODE_NEW:
        can_instantiate_type(expr->get_child(0));
        break;

    case node_t::NODE_LIST:
        {
            // this is the type of the last entry
            node::pointer_t child(expr->get_child(max_children - 1));
            expr->set_type_node(child->get_type_node());
        }
        break;

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
        // TODO: we need to replace the intrinsic special
        //       assignment ops with a regular assignment
        //       (i.e. a += b becomes a = a + (b))
        binary_operator(expr);
        break;

    default:
        throw internal_error("error: there is a missing entry in the 2nd switch of compiler::expression().");

    }

    optimizer::optimize(expr);
    type_expr(expr);
}



}
// namespace as2js

// vim: ts=4 sw=4 et
