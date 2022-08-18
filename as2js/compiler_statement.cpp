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

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{



void compiler::with(node::pointer_t& with_node)
{
    size_t const max_children(with_node->get_children_size());
    if(max_children != 2)
    {
        // invalid, ignore
        return;
    }
    node_lock ln(with_node);

    // object name defined in an expression
    // (used to resolve identifiers as members in the following
    // expressions until it gets popped)
    node::pointer_t object(with_node->get_child(0));

    if(object->get_type() == node::node_t::NODE_THIS)
    {
        // TODO: could we avoid erring here?!
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, object->get_position());
        msg << "'with' cannot use 'this' as an object.";
    }

    expression(object);

    // we create two nodes; one so we know we have a WITH instruction
    // and a child of that node which is the object itself; these are
    // deleted once we return from this function
    //NodePtr t;
    //t.CreateNode();
    //t.SetData(object.GetData());
    //NodePtr w;
    //w.CreateNode(NODE_WITH);
    //w.AddChild(t);
    //int p = f_scope.GetChildCount();
    //f_scope.AddChild(w);

    node::pointer_t sub_directives(with_node->get_child(1));
    directive_list(sub_directives);

    // the effect of this with() ends with the end of its
    // list of directives
    //f_scope.DeleteChild(p);
}



/** \brief Compile the goto directive.
 *
 * Note that JavaScript in browsers do not support the goto instruction.
 * They have a similar behavior when using while() loop and either a
 * continue (goto at the start) or the break (goto after the while()
 * loop.).
 *
 * This function is kept here, although we are very unlikely to implement
 * the instruction in your browser, it may end up being useful in case
 * we again work on ActionScript.
 *
 * \param[in] goto_node  The node representing the goto statement.
 */
void compiler::goto_directive(node::pointer_t& goto_node)
{
    node::vector_of_pointers_t parents;
    node::pointer_t label;
    node::pointer_t parent(goto_node);
    do
    {
        parent = parent->get_parent();
        if(!parent)
        {
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, goto_node->get_position());
            msg << "compiler::goto(): out of parents before we find function, program or package parent?!";
            throw as2js_exit("compiler::goto(): out of parents before we find function, program or package parent?!", 1);
        }

        switch(parent->get_type())
        {
        case node::node_t::NODE_CLASS:
        case node::node_t::NODE_INTERFACE:
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, goto_node->get_position());
                msg << "cannot have a GOTO instruction in a 'class' or 'interface'.";
            }
            return;

        case node::node_t::NODE_FUNCTION:
        case node::node_t::NODE_PACKAGE:
        case node::node_t::NODE_PROGRAM:
            label = parent->find_label(goto_node->get_string());
            if(!label)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_LABEL_NOT_FOUND, goto_node->get_position());
                msg << "label '" << goto_node->get_string() << "' for goto instruction not found.";
                return;
            }
            break;

        // We most certainly want to test those with some user
        // options to know whether we should accept or refuse
        // inter-frame gotos
        //case node::node_t::NODE_WITH:
        //case node::node_t::NODE_TRY:
        //case node::node_t::NODE_CATCH:
        //case node::node_t::NODE_FINALLY:

        default:
            break;

        }
        parents.push_back(parent);
    }
    while(!label);
    goto_node->set_goto_enter(label);

    // Now we have to do the hardest part:
    //    find the common parent frame where both, the goto
    //    and the label can be found
    //    for this purpose we created an array with all the
    //    frames (parents) and then we search that array with each
    //    parent of the label

    parent = label;
    for(;;)
    {
        parent = parent->get_parent();
        if(!parent)
        {
            // never found a common parent?!
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, goto_node->get_position());
            msg << "compiler::goto(): out of parent before we find the common node?!";
            throw as2js_exit("compiler::goto(): out of parent before we find the common node?!", 1);
        }
        for(size_t idx(0); idx < parents.size(); ++idx)
        {
            if(parents[idx] == parent)
            {
                // found the first common parent
                goto_node->set_goto_exit(parent);
                return;
            }
        }
    }
}



void compiler::for_directive(node::pointer_t& for_node)
{
    // support for the two forms: for(foo in blah) ... and for(a;b;c) ...
    // (Note: first case we have 3 children: foo, blah, directives
    //        second case we have 4 children: a, b, c, directives
    size_t max(for_node->get_children_size());
    if(max < 3)
    {
        return;
    }
    node_lock ln(for_node);

    for(size_t idx(0); idx < max; ++idx)
    {
        node::pointer_t child(for_node->get_child(idx));
        switch(child->get_type())
        {
        case node::node_t::NODE_EMPTY:
            // do nothing
            break;

        case node::node_t::NODE_DIRECTIVE_LIST:
            directive_list(child);
            break;

        case node::node_t::NODE_VAR:
            var(child);
            break;

        default:    // expression
            expression(child);
            break;

        }
    }
}


void compiler::switch_directive(node::pointer_t& switch_node)
{
    size_t const max_children(switch_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }

    node_lock ln_sn(switch_node);
    expression(switch_node->get_child(0));

    // make sure that the list of directive starts
    // with a label [this is a requirement which
    // really makes sense but the parser does not
    // enforce it]
    node::pointer_t directive_list_node(switch_node->get_child(1));
    size_t const max_directives(directive_list_node->get_children_size());
    if(max_directives > 0)
    {
        node::pointer_t child(directive_list_node->get_child(0));
        if(child->get_type() != node::node_t::NODE_CASE
        && child->get_type() != node::node_t::NODE_DEFAULT)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INACCESSIBLE_STATEMENT, switch_node->get_position());
            msg << "the list of instructions of a 'switch()' statement must start with a 'case' or 'default' label.";
        }
    }
    // else -- should we warn when empty?

    directive_list(directive_list_node);

    // reset the DEFAULT flag just in case we get compiled a second
    // time (which happens when testing for missing return statements)
    switch_node->set_flag(node::flag_t::NODE_SWITCH_FLAG_DEFAULT, false);

    // TODO: If EQUAL or STRICTLY EQUAL we may
    //       want to check for duplicates.
    //       (But cases can be dynamic so it
    //       does not really make sense, does it?!)
}


void compiler::case_directive(node::pointer_t& case_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    node::pointer_t parent(case_node->get_parent());
    if(!parent)
    {
        // ?!?
        return;
    }
    parent = parent->get_parent();
    if(!parent)
    {
        // ?!?
        return;
    }
    if(parent->get_type() != node::node_t::NODE_SWITCH)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, case_node->get_position());
        msg << "a 'case' statement can only be used within a 'switch()' block.";
        return;
    }

    size_t const max_children(case_node->get_children_size());
    if(max_children > 0)
    {
        expression(case_node->get_child(0));
        if(max_children > 1)
        {
            switch(parent->get_switch_operator())
            {
            case node::node_t::NODE_UNKNOWN:
            case node::node_t::NODE_IN:
                break;

            default:
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, case_node->get_position());
                    msg << "a range on a 'case' statement can only be used with the 'in' and 'default' switch() operators.";
                }
                break;

            }
            expression(case_node->get_child(1));
        }
    }
}


void compiler::default_directive(node::pointer_t& default_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    node::pointer_t parent(default_node->get_parent());
    if(!parent) {
        // ?!?
        return;
    }
    parent = parent->get_parent();
    if(!parent) {
        // ?!?
        return;
    }
    if(parent->get_type() != node::node_t::NODE_SWITCH)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INACCESSIBLE_STATEMENT, default_node->get_position());
        msg << "a 'default' statement can only be used within a 'switch()' block.";
        return;
    }

    if(parent->get_flag(node::flag_t::NODE_SWITCH_FLAG_DEFAULT))
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, default_node->get_position());
        msg << "only one 'default' statement can be used within one 'switch()'.";
    }
    else
    {
        parent->set_flag(node::flag_t::NODE_SWITCH_FLAG_DEFAULT, true);
    }
}


void compiler::if_directive(node::pointer_t& if_node)
{
    size_t const max_children(if_node->get_children_size());
    if(max_children < 2)
    {
        return;
    }
    node_lock ln(if_node);

    // TBD: check whether the first expression
    //      is a valid boolean? (for strict mode
    //      maybe, but JavaScript is very lax on
    //      that just like C/C++)
    expression(if_node->get_child(0));
    directive_list(if_node->get_child(1));
    if(max_children == 3)
    {
        // else part
        directive_list(if_node->get_child(2));
    }
}


void compiler::while_directive(node::pointer_t& while_node)
{
    size_t const max_children(while_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }
    node_lock ln(while_node);

    // If the first expression is a constant boolean,
    // the optimizer will replace the while()
    // loop in a loop forever; or remove it entirely.
    expression(while_node->get_child(0));
    directive_list(while_node->get_child(1));
}


void compiler::do_directive(node::pointer_t& do_node)
{
    size_t const max_children(do_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }
    node_lock ln(do_node);

    // If the second expression is a constant boolean,
    // the optimizer will replace the do/while()
    // loop in a loop forever; or execute the first
    // list of directives once.
    directive_list(do_node->get_child(0));
    expression(do_node->get_child(1));
}


void compiler::break_continue(node::pointer_t& break_node)
{
    bool const no_label(break_node->get_string().empty());
    bool const accept_switch(!no_label || break_node->get_type() == node::node_t::NODE_BREAK);
    bool found_switch(false);
    node::pointer_t parent(break_node);
    for(;;)
    {
        parent = parent->get_parent();
        if(parent->get_type() == node::node_t::NODE_SWITCH)
        {
            found_switch = true;
        }
        if((parent->get_type() == node::node_t::NODE_SWITCH && accept_switch)
        || parent->get_type() == node::node_t::NODE_FOR
        || parent->get_type() == node::node_t::NODE_DO
        || parent->get_type() == node::node_t::NODE_WHILE)
        {
            if(no_label)
            {
                // just break the current 'switch', 'for',
                // 'while', 'do' when there is no name.
                break;
            }
            // check whether this statement has a label
            // and whether it matches the requested name
            int32_t const offset(parent->get_offset());
            if(offset > 0)
            {
                node::pointer_t p(parent->get_parent());
                node::pointer_t previous(p->get_child(offset - 1));
                if(previous->get_type() == node::node_t::NODE_LABEL
                && previous->get_string() == break_node->get_string())
                {
                    // found a match
                    break;
                }
            }
        }
        if(parent->get_type() == node::node_t::NODE_FUNCTION
        || parent->get_type() == node::node_t::NODE_PROGRAM
        || parent->get_type() == node::node_t::NODE_CLASS       // ?!
        || parent->get_type() == node::node_t::NODE_INTERFACE   // ?!
        || parent->get_type() == node::node_t::NODE_PACKAGE)
        {
            // not found?! a break/continue outside a loop or
            // switch?! or the label was not found
            if(no_label)
            {
                if(found_switch)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, break_node->get_position());
                    msg << "you cannot use a 'continue' statement outside a loop (and you need a label to make it work with a 'switch' statement).";
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, break_node->get_position());
                    msg << "you cannot use a 'break' or 'continue' instruction outside a loop or 'switch' statement.";
                }
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_LABEL_NOT_FOUND, break_node->get_position());
                msg << "could not find a loop or 'switch' statement labelled '" << break_node->get_string() << "' for this 'break' or 'continue'.";
            }
            return;
        }
    }

    // We just specify which node needs to be reached
    // on this break/continue.
    //
    // We do not replace these with a simple goto instruction
    // because that way the person using the tree later can
    // program the break and/or continue the way they feel
    // (using a variable, a special set of instructions,
    // etc. so as to be able to unwind all the necessary
    // data in a way specific to the break/continue).
    //
    // Also in browsers, JavaScript does not offer a goto.
    break_node->set_goto_exit(parent);
}



void compiler::throw_directive(node::pointer_t& throw_node)
{
    if(throw_node->get_children_size() != 1)
    {
        return;
    }

    expression(throw_node->get_child(0));
}


void compiler::try_directive(node::pointer_t& try_node)
{
    if(try_node->get_children_size() != 1)
    {
        return;
    }

    // we want to make sure that we are followed
    // by a catch or a finally
    bool correct(false);
    node::pointer_t parent(try_node->get_parent());
    size_t const max_parent_children(parent->get_children_size());
    size_t const offset(static_cast<size_t>(try_node->get_offset()) + 1);
    if(offset < max_parent_children)
    {
        node::pointer_t next(parent->get_child(offset));
        correct = next->get_type() == node::node_t::NODE_CATCH
               || next->get_type() == node::node_t::NODE_FINALLY;
    }
    if(!correct)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_TRY, try_node->get_position());
        msg << "a 'try' statement needs to be followed by at least one of 'catch' or 'finally'.";
    }

    directive_list(try_node->get_child(0));
}


void compiler::catch_directive(node::pointer_t& catch_node)
{
    if(catch_node->get_children_size() != 2)
    {
        return;
    }

    // we want to make sure that we are preceded by a try
    bool correct(false);
    node::pointer_t parent(catch_node->get_parent());
    int32_t const offset(catch_node->get_offset());
    if(offset > 0)
    {
        node::pointer_t prev(parent->get_child(offset - 1));
        if(prev->get_type() == node::node_t::NODE_TRY)
        {
            correct = true;
        }
        else if(prev->get_type() == node::node_t::NODE_CATCH)
        {
            correct = true;

            // correct syntactically, however, the previous catch
            // must clearly be typed
            if(!prev->get_flag(node::flag_t::NODE_CATCH_FLAG_TYPED))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_TYPE, catch_node->get_position());
                msg << "only the last 'catch' statement can have a parameter without a valid type.";
            }
        }
    }
    if(!correct)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, catch_node->get_position());
        msg << "a 'catch' statement needs to be preceded by a 'try' or another typed 'catch' statement.";
    }

    node::pointer_t parameters_node(catch_node->get_child(0));
    parameters(parameters_node);
    if(parameters_node->get_children_size() > 0)
    {
        node::pointer_t param(parameters_node->get_child(0));
        param->set_flag(node::flag_t::NODE_PARAM_FLAG_CATCH, true);
    }

    directive_list(catch_node->get_child(1));
}


void compiler::finally(node::pointer_t& finally_node)
{
    if(finally_node->get_children_size() != 1)
    {
        return;
    }

    // we want to make sure that we are preceeded by a try or a catch
    bool correct(false);
    node::pointer_t parent(finally_node->get_parent());
    int32_t const offset(finally_node->get_offset());
    if(offset > 0)
    {
        node::pointer_t prev(parent->get_child(offset - 1));
        correct = prev->get_type() == node::node_t::NODE_TRY
               || prev->get_type() == node::node_t::NODE_CATCH;
    }
    if(!correct)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, finally_node->get_position());
        msg << "a 'finally' statement needs to be preceded by a 'try' or 'catch' statement.";
    }

    directive_list(finally_node->get_child(0));
}



node::pointer_t compiler::return_directive(node::pointer_t return_node)
{
    // 1. a return is only valid in a function (procedure)
    // 2. a return must return a value in a function
    // 3. a return can't return anything in a procedure
    // 4. you must assume that the function is returning
    //    Void when the function is a constructor and
    //    thus return can't have an expression in this case

    bool more;
    bool bad(false);
    node::pointer_t function_node;
    node::pointer_t parent(return_node);
    do
    {
        more = false;
        parent = parent->get_parent();
        if(!parent)
        {
            bad = true;
            break;
        }
        switch(parent->get_type())
        {
        case node::node_t::NODE_FUNCTION:
            function_node = parent;
            break;

        case node::node_t::NODE_CLASS:
        case node::node_t::NODE_INTERFACE:
        case node::node_t::NODE_PACKAGE:
        case node::node_t::NODE_PROGRAM:
        case node::node_t::NODE_ROOT:
            bad = true;
            break;

        default:
            more = true;
            break;

        }
    }
    while(more);
    if(bad)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, return_node->get_position());
        msg << "'return' can only be used inside a function.";
    }
    else
    {
        if(function_node->get_flag(node::flag_t::NODE_FUNCTION_FLAG_NEVER))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, return_node->get_position());
            msg << "'return' was used inside '" << function_node->get_string() << "', a function Never returning.";
        }

        node::pointer_t the_class;
        size_t const max_children(return_node->get_children_size());
        if(max_children == 1)
        {
            if(function_node->get_flag(node::flag_t::NODE_FUNCTION_FLAG_VOID)
            || is_constructor(function_node, the_class))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, return_node->get_position());
                msg << "'return' was used with an expression inside '" << function_node->get_string() << "', a function returning Void or a constructor.";
            }
            expression(return_node->get_child(0));
        }
        else
        {
            // NOTE:
            // This actually needs to be transformed to
            // returning 'undefined' in the execution
            // environment... maybe we will add this
            // here at some point.
            if(!function_node->get_flag(node::flag_t::NODE_FUNCTION_FLAG_VOID)
            && !is_constructor(function_node, the_class))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, return_node->get_position());
                msg << "'return' was used without an expression inside '" << function_node->get_string() << "', a function which expected a value to be returned.";
            }
        }
    }

    return parent;
}


void compiler::use_namespace(node::pointer_t & use_namespace_node)
{
    if(use_namespace_node->get_children_size() != 1)
    {
        return;
    }
    node_lock ln(use_namespace_node);

    // type/scope name defined in an expression
    // (needs to be resolved in an identifiers, members composed of
    // identifiers or a string representing a valid type name)
    node::pointer_t qualifier(use_namespace_node->get_child(0));
    expression(qualifier);

    // TODO: I'm not too sure what the qualifier can end up being at this
    //       point, but if it is a whole tree of node, we do not know
    //       how to copy it... (because using qualifier directly instead
    //       of using q as defined below would completely break the
    //       existing namespace...)
    if(qualifier->get_type() != node::node_t::NODE_STRING)
    {
        throw internal_error("type qualifier is not just a string, we cannot duplicate it at this point");
    }

    // we create two nodes; one so we know we have a NAMESPACE instruction
    // and a child of that node which is the type itself; these are
    // deleted once we return from the directive_list() function and not
    // this function
    node::pointer_t q(qualifier->create_replacement(qualifier->get_type()));
    q->set_string(qualifier->get_string());
    node::pointer_t n(qualifier->create_replacement(node::node_t::NODE_NAMESPACE));
    n->append_child(q);
    f_scope->append_child(n);
}
























}
// namespace as2js

// vim: ts=4 sw=4 et
