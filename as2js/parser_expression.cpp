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
#include    "as2js/parser.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER EXPRESSION  **********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::expression(node::pointer_t & n)
{
    list_expression(n, false, false);

    if(!n)
    {
        // should not happen, if it does, we have got a really bad internal error
        throw internal_error("expression() cannot return a null node pointer"); // LCOV_EXCL_LINE
    }
}


void parser::list_expression(node::pointer_t & n, bool rest, bool empty)
{
    if(n)
    {
        // should not happen, if it does, we have got a really bad internal error
        throw internal_error("list_expression() called with a non-null node pointer"); // LCOV_EXCL_LINE
    }

    int has_rest(0);
    if(empty && f_node->get_type() == node::node_t::NODE_COMMA)
    {
        // empty at the start of the array
        n = f_lexer->get_new_node(node::node_t::NODE_EMPTY);
    }
    else if(rest && f_node->get_type() == node::node_t::NODE_REST)
    {
        // the '...' in a function call is used to mean pass
        // my own rest down to the callee
        n = f_lexer->get_new_node(node::node_t::NODE_REST);
        get_token();
        has_rest = 1;
        // note: we expect ')' here but we
        // let the user put ',' <expr> still
        // and err in case it happens
    }
    else if(rest && f_node->get_type() == node::node_t::NODE_IDENTIFIER)
    {
        // identifiers ':' -> named parameter
        node::pointer_t save(f_node);
        // skip the identifier
        get_token();
        if(f_node->get_type() == node::node_t::NODE_COLON)
        {
            // skip the ':'
            get_token();
            n = f_lexer->get_new_node(node::node_t::NODE_NAME);
            n->set_string(save->get_string());
            if(f_node->get_type() == node::node_t::NODE_REST)
            {
                // the '...' in a function call is used to mean pass
                // my own rest down to the callee
                node::pointer_t rest_of_args(f_lexer->get_new_node(node::node_t::NODE_REST));
                n->append_child(rest_of_args);
                get_token();
                has_rest = 1;
                // note: we expect ')' here but we
                // let the user put ',' <expr> still
                // and err in case it happens
            }
            else
            {
                node::pointer_t value;
                assignment_expression(value);
                n->append_child(value);
            }
        }
        else
        {
            unget_token(f_node);
            f_node = save;
            assignment_expression(n);
        }
    }
    else
    {
        assignment_expression(n);
    }

    if(f_node->get_type() == node::node_t::NODE_COMMA)
    {
        node::pointer_t first_item(n);

        n = f_lexer->get_new_node(node::node_t::NODE_LIST);
        n->append_child(first_item);

        while(f_node->get_type() == node::node_t::NODE_COMMA)
        {
            get_token();
            if(has_rest == 1)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_REST, f_lexer->get_position());
                msg << "'...' was expected to be the last expression in this function call.";
                has_rest = 2;
            }
            if(empty && f_node->get_type() == node::node_t::NODE_COMMA)
            {
                // empty inside the array
                node::pointer_t empty_node(f_lexer->get_new_node(node::node_t::NODE_EMPTY));
                n->append_child(empty_node);
            }
            else if(empty && f_node->get_type() == node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                // empty at the end of the array
                node::pointer_t empty_node(f_lexer->get_new_node(node::node_t::NODE_EMPTY));
                n->append_child(empty_node);
            }
            else if(rest && f_node->get_type() == node::node_t::NODE_REST)
            {
                // the '...' in a function call is used to mean pass
                // my own rest down to the callee
                node::pointer_t rest_node(f_lexer->get_new_node(node::node_t::NODE_REST));
                n->append_child(rest_node);
                get_token();
                if(has_rest == 0)
                {
                    has_rest = 1;
                }
                // note: we expect ')' here but we
                // let the user put ',' <expr> still
                // and err in case it happens
            }
            else if(rest && f_node->get_type() == node::node_t::NODE_IDENTIFIER)
            {
                node::pointer_t item;

                // identifiers ':' -> named parameter
                node::pointer_t save(f_node);
                get_token();
                if(f_node->get_type() == node::node_t::NODE_COLON)
                {
                    get_token();
                    item = f_lexer->get_new_node(node::node_t::NODE_NAME);
                    item->set_string(save->get_string());

                    if(f_node->get_type() == node::node_t::NODE_REST)
                    {
                        // the '...' in a function call is used to mean pass
                        // my own rest down to the callee
                        node::pointer_t rest_of_args(f_lexer->get_new_node(node::node_t::NODE_REST));
                        item->append_child(rest_of_args);
                        get_token();
                        if(has_rest == 0)
                        {
                            has_rest = 1;
                        }
                        // note: we expect ')' here but we
                        // let the user put ',' <expr> still
                        // and err in case it happens
                    }
                    else
                    {
                        node::pointer_t value;
                        assignment_expression(value);
                        item->append_child(value);
                    }
                    n->append_child(item);
                }
                else
                {
                    unget_token(f_node);
                    f_node = save;
                    assignment_expression(item);
                    n->append_child(item);
                }
            }
            else
            {
                node::pointer_t item;
                assignment_expression(item);
                n->append_child(item);
            }
        }

        // TODO: check that the list ends with a NODE_REST
    }
}


void parser::assignment_expression(node::pointer_t & n)
{
    conditional_expression(n, true);

    // TODO: check that the result is a postfix expression
    switch(f_node->get_type())
    {
    case node::node_t::NODE_ASSIGNMENT:
    case node::node_t::NODE_ASSIGNMENT_ADD:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node::node_t::NODE_ASSIGNMENT_MODULO:
    case node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node::node_t::NODE_ASSIGNMENT_SUBTRACT:
        break;

    case node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case node::node_t::NODE_ASSIGNMENT_POWER:
    case node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        break;

    default:
        return;

    }

    node::pointer_t left(n);

    n = f_node;

    get_token();
    node::pointer_t right;
    assignment_expression(right);

    n->append_child(left);
    n->append_child(right);
}


void parser::conditional_expression(node::pointer_t & n, bool const assignment)
{
    min_max_expression(n);

    if(f_node->get_type() == node::node_t::NODE_CONDITIONAL)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t left;
        // not like C/C++, not a list expression here
        if(assignment)
        {
            assignment_expression(left);
        }
        else
        {
            conditional_expression(left, false);
        }
        n->append_child(left);

        if(f_node->get_type() == node::node_t::NODE_COLON)
        {
            get_token();
            node::pointer_t right;
            if(assignment)
            {
                assignment_expression(right);
            }
            else
            {
                conditional_expression(right, false);
            }
            n->append_child(right);
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CONDITIONAL, f_lexer->get_position());
            msg << "invalid use of the conditional operator, ':' was expected.";
        }
    }
}



void parser::min_max_expression(node::pointer_t & n)
{
    logical_or_expression(n);

    if(f_node->get_type() == node::node_t::NODE_MINIMUM
    || f_node->get_type() == node::node_t::NODE_MAXIMUM)
    {
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        logical_or_expression(right);
        n->append_child(right);
    }
}


void parser::logical_or_expression(node::pointer_t & n)
{
    logical_xor_expression(n);

    if(f_node->get_type() == node::node_t::NODE_LOGICAL_OR)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        logical_xor_expression(right);
        n->append_child(right);
    }
}


void parser::logical_xor_expression(node::pointer_t & n)
{
    logical_and_expression(n);

    if(f_node->get_type() == node::node_t::NODE_LOGICAL_XOR)
    {
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '^^' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        logical_and_expression(right);
        n->append_child(right);
    }
}


void parser::logical_and_expression(node::pointer_t & n)
{
    bitwise_or_expression(n);

    if(f_node->get_type() == node::node_t::NODE_LOGICAL_AND)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        bitwise_or_expression(right);
        n->append_child(right);
    }
}



void parser::bitwise_or_expression(node::pointer_t & n)
{
    bitwise_xor_expression(n);

    if(f_node->get_type() == node::node_t::NODE_BITWISE_OR)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        bitwise_xor_expression(right);
        n->append_child(right);
    }
}


void parser::bitwise_xor_expression(node::pointer_t & n)
{
    bitwise_and_expression(n);

    if(f_node->get_type() == node::node_t::NODE_BITWISE_XOR)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        bitwise_and_expression(right);
        n->append_child(right);
    }
}


void parser::bitwise_and_expression(node::pointer_t & n)
{
    equality_expression(n);

    if(f_node->get_type() == node::node_t::NODE_BITWISE_AND)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        equality_expression(right);
        n->append_child(right);
    }
}


void parser::equality_expression(node::pointer_t & n)
{
    relational_expression(n);

    node::node_t type(f_node->get_type());
    while(type == node::node_t::NODE_EQUAL
       || type == node::node_t::NODE_NOT_EQUAL
       || type == node::node_t::NODE_STRICTLY_EQUAL
       || type == node::node_t::NODE_STRICTLY_NOT_EQUAL
       || type == node::node_t::NODE_COMPARE
       || type == node::node_t::NODE_SMART_MATCH)
    {
        if((type == node::node_t::NODE_COMPARE || type == node::node_t::NODE_SMART_MATCH)
        && !has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        relational_expression(right);
        n->append_child(right);

        type = f_node->get_type();
    }
}


void parser::relational_expression(node::pointer_t & n)
{
    shift_expression(n);

    while(f_node->get_type() == node::node_t::NODE_LESS
       || f_node->get_type() == node::node_t::NODE_GREATER
       || f_node->get_type() == node::node_t::NODE_LESS_EQUAL
       || f_node->get_type() == node::node_t::NODE_GREATER_EQUAL
       || f_node->get_type() == node::node_t::NODE_IS
       || f_node->get_type() == node::node_t::NODE_AS
       || f_node->get_type() == node::node_t::NODE_IN
       || f_node->get_type() == node::node_t::NODE_INSTANCEOF)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        shift_expression(right);
        n->append_child(right);

        // with IN we accept a range (optional)
        if(n->get_type() == node::node_t::NODE_IN
        && (f_node->get_type() == node::node_t::NODE_RANGE || f_node->get_type() == node::node_t::NODE_REST))
        {
            if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
                msg << "the 'x in min .. max' operator is only available when extended operators are authorized (use extended_operators;).";
            }

            get_token();
            node::pointer_t end;
            shift_expression(end);
            n->append_child(end);
        }
    }
}


void parser::shift_expression(node::pointer_t & n)
{
    additive_expression(n);

    node::node_t type(f_node->get_type());
    while(type == node::node_t::NODE_SHIFT_LEFT
       || type == node::node_t::NODE_SHIFT_RIGHT
       || type == node::node_t::NODE_SHIFT_RIGHT_UNSIGNED
       || type == node::node_t::NODE_ROTATE_LEFT
       || type == node::node_t::NODE_ROTATE_RIGHT)
    {
        if((type == node::node_t::NODE_ROTATE_LEFT || type == node::node_t::NODE_ROTATE_RIGHT)
        && !has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        additive_expression(right);
        n->append_child(right);

        type = f_node->get_type();
    }
}


void parser::additive_expression(node::pointer_t & n)
{
    multiplicative_expression(n);

    while(f_node->get_type() == node::node_t::NODE_ADD
       || f_node->get_type() == node::node_t::NODE_SUBTRACT)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        multiplicative_expression(right);
        n->append_child(right);
    }
}


void parser::multiplicative_expression(node::pointer_t & n)
{
    match_expression(n);

    while(f_node->get_type() == node::node_t::NODE_MULTIPLY
       || f_node->get_type() == node::node_t::NODE_DIVIDE
       || f_node->get_type() == node::node_t::NODE_MODULO)
    {
        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        match_expression(right);
        n->append_child(right);
    }
}


void parser::match_expression(node::pointer_t & n)
{
    power_expression(n);

    while(f_node->get_type() == node::node_t::NODE_MATCH
       || f_node->get_type() == node::node_t::NODE_NOT_MATCH)
    {
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        power_expression(right);
        n->append_child(right);
    }
}


void parser::power_expression(node::pointer_t & n)
{
    unary_expression(n);

    if(f_node->get_type() == node::node_t::NODE_POWER)
    {
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '**' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(n);
        n = f_node;

        get_token();
        node::pointer_t right;
        power_expression(right); // right to left
        n->append_child(right);
    }
}



void parser::unary_expression(node::pointer_t & n)
{
    if(n)
    {
        throw internal_error("unary_expression() called with a non-null node pointer");  // LCOV_EXCL_LINE
    }

    switch(f_node->get_type())
    {
    case node::node_t::NODE_DELETE:
    case node::node_t::NODE_INCREMENT:
    case node::node_t::NODE_DECREMENT:
    {
        n = f_node;
        get_token();
        node::pointer_t postfix;
        postfix_expression(postfix);
        n->append_child(postfix);
    }
        break;

    case node::node_t::NODE_VOID:
    case node::node_t::NODE_TYPEOF:
    case node::node_t::NODE_ADD: // +<value>
    case node::node_t::NODE_SUBTRACT: // -<value>
    case node::node_t::NODE_BITWISE_NOT:
    case node::node_t::NODE_LOGICAL_NOT:
    {
        n = f_node;
        get_token();
        node::pointer_t unary;
        unary_expression(unary);
        n->append_child(unary);
    }
        break;

    case node::node_t::NODE_SMART_MATCH:
    {
        // we support the ~~ for Smart Match, but if found as a unary
        // operator the user had to mean '~' and '~' separated as in:
        //     a = ~ ~ b
        // so here we generate two bitwise not (DO NOT OPTIMIZE, if one
        // writes a = ~~b it is NOT the same as a = b because JavaScript
        // forces a conversion of b to a 32 bit integer when applying the
        // bitwise not operator.)
        //
        n = f_lexer->get_new_node(node::node_t::NODE_BITWISE_NOT);
        node::pointer_t child(f_lexer->get_new_node(node::node_t::NODE_BITWISE_NOT));
        n->append_child(child);
        get_token();
        node::pointer_t unary;
        unary_expression(unary);
        child->append_child(unary);
    }
        break;

    case node::node_t::NODE_NOT_MATCH:
    {
        // we support the !~ for Not Match, but if found as a unary
        // operator the user had to mean '!' and '~' separated as in:
        //     a = ! ~ b
        // so here we generate two not (DO NOT OPTIMIZE, if one
        // writes a = !~b it is NOT the same as a = b because JavaScript
        // forces a conversion of b to a 32 bit integer when applying the
        // bitwise not operator.)
        //
        n = f_lexer->get_new_node(node::node_t::NODE_LOGICAL_NOT);
        node::pointer_t child(f_lexer->get_new_node(node::node_t::NODE_BITWISE_NOT));
        n->append_child(child);
        get_token();
        node::pointer_t unary;
        unary_expression(unary);
        child->append_child(unary);
    }
        break;

    default:
        postfix_expression(n);
        break;

    }
}


void parser::postfix_expression(node::pointer_t & n)
{
    primary_expression(n);

    for(;;)
    {
        switch(f_node->get_type())
        {
        case node::node_t::NODE_MEMBER:
        {
            f_node->append_child(n);
            n = f_node;

            get_token();
            node::pointer_t right;
            primary_expression(right);
            n->append_child(right);
        }
            break;

        case node::node_t::NODE_SCOPE:
        {
            // TBD: I do not think that we need a scope operator at all
            //      since we can use the '.' (MEMBER) operator in all cases
            //      I can currently think of (and in JavaScript you are
            //      expected to do so anyway!) therefore I only authorize
            //      it as an extension at the moment
            if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
                msg << "the '::' operator is only available when extended operators are authorized (use extended_operators;).";
            }

            f_node->append_child(n);
            n = f_node;

            get_token();
            if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
            {
                n->append_child(f_node);
                get_token();
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_SCOPE, f_lexer->get_position());
                msg << "scope operator '::' is expected to be followed by an identifier.";
            }
            // don't repeat scope (it seems)
            return;
        }
            break;

        case node::node_t::NODE_INCREMENT:
        {
            node::pointer_t decrement(f_lexer->get_new_node(node::node_t::NODE_POST_INCREMENT));
            decrement->append_child(n);
            n = decrement;
            get_token();
        }
            break;

        case node::node_t::NODE_DECREMENT:
        {
            node::pointer_t decrement(f_lexer->get_new_node(node::node_t::NODE_POST_DECREMENT));
            decrement->append_child(n);
            n = decrement;
            get_token();
        }
            break;

        case node::node_t::NODE_OPEN_PARENTHESIS:        // function call arguments
        {
            node::pointer_t left(n);
            n = f_lexer->get_new_node(node::node_t::NODE_CALL);
            n->append_child(left);

            get_token();

            // any arguments?
            node::pointer_t right;
            if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                node::pointer_t list;
                list_expression(list, true, false);
                if(list->get_type() == node::node_t::NODE_LIST)
                {
                    // already a list, use it as is
                    right = list;
                }
                else
                {
                    // not a list, so put it in a one
                    right = f_lexer->get_new_node(node::node_t::NODE_LIST);
                    right->append_child(list);
                }
            }
            else
            {
                // an empty list!
                right = f_lexer->get_new_node(node::node_t::NODE_LIST);
            }
            n->append_child(right);

            if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                get_token();
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "')' expected to end the list of arguments.";
            }
        }
            break;

        case node::node_t::NODE_OPEN_SQUARE_BRACKET:        // array/property access
        {
            node::pointer_t array(f_lexer->get_new_node(node::node_t::NODE_ARRAY));
            array->append_child(n);
            n = array;

            get_token();

            // any arguments?
            if(f_node->get_type() != node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                node::pointer_t right;
                list_expression(right, false, false);
                n->append_child(right);
            }

            if(f_node->get_type() == node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                get_token();
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SQUARE_BRACKETS_EXPECTED, f_lexer->get_position());
                msg << "']' expected to end the list of element references or declarations.";
            }
        }
            break;

        default:
            return;

        }
    }
}


void parser::primary_expression(node::pointer_t & n)
{
    switch(f_node->get_type())
    {
    case node::node_t::NODE_FALSE:
    case node::node_t::NODE_FLOATING_POINT:
    case node::node_t::NODE_IDENTIFIER:
    case node::node_t::NODE_INTEGER:
    case node::node_t::NODE_NULL:
    case node::node_t::NODE_REGULAR_EXPRESSION:
    case node::node_t::NODE_STRING:
    case node::node_t::NODE_THIS:
    case node::node_t::NODE_TRUE:
    case node::node_t::NODE_UNDEFINED:
    case node::node_t::NODE_SUPER:
        n = f_node;
        get_token();
        break;

    case node::node_t::NODE_PRIVATE:
    case node::node_t::NODE_PROTECTED:
    case node::node_t::NODE_PUBLIC:
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        n = f_node;
        get_token();
        break;

    case node::node_t::NODE_NEW:
    {
        n = f_node;
        get_token();
        node::pointer_t object_name;
        postfix_expression(object_name);
        n->append_child(object_name);
    }
        break;

    case node::node_t::NODE_OPEN_PARENTHESIS:        // grouped expressions
    {
        get_token();
        list_expression(n, false, false);

        // NOTE: the following is important in different cases
        //       such as (a).field which is dynamic (i.e. we get the
        //       content of variable a as the name of the object to
        //       access and thus it is not equivalent to a.field)
        if(n->get_type() == node::node_t::NODE_IDENTIFIER)
        {
            n->to_videntifier();
        }
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to match the '('.";
        }
    }
        break;

    case node::node_t::NODE_OPEN_SQUARE_BRACKET: // array declaration
    {
        n = f_lexer->get_new_node(node::node_t::NODE_ARRAY_LITERAL);
        get_token();

        node::pointer_t elements;
        list_expression(elements, false, true);
        n->append_child(elements);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_SQUARE_BRACKET)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SQUARE_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "']' expected to match the '[' of this array.";
        }
    }
        break;

    case node::node_t::NODE_OPEN_CURVLY_BRACKET: // object declaration
    {
        get_token();
        object_literal_expression(n);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "'}' expected to match the '{' of this object literal.";
        }
    }
        break;

    case node::node_t::NODE_FUNCTION:
    {
        get_token();
        function(n, true);
    }
        break;

    default:
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, f_lexer->get_position());
        msg << "unexpected token '" << f_node->get_type_name() << "' found in an expression.";

        // callers expect to receive a node... give them something
        n = f_lexer->get_new_node(node::node_t::NODE_FALSE);
        break;

    }
}



void parser::object_literal_expression(node::pointer_t & n)
{
    node::pointer_t name;
    node::node_t    type;

    n = f_lexer->get_new_node(node::node_t::NODE_OBJECT_LITERAL);
    for(;;)
    {
        name = f_lexer->get_new_node(node::node_t::NODE_NAME);
        type = f_node->get_type();
        switch(type)
        {
        case node::node_t::NODE_OPEN_PARENTHESIS: // (<expr>)::<name> only
        {
            get_token();  // we MUST skip the '(' otherwise the '::' is eaten from within
            node::pointer_t expr;
            expression(expr);
            if(expr->get_type() == node::node_t::NODE_IDENTIFIER)
            {
                // an identifier becomes a VIDENTIFIER to remain dynamic.
                expr->to_videntifier();
            }
            name->append_child(expr);
            if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, f_lexer->get_position());
                msg << "')' is expected to close a dynamically named object field.";
            }
            else
            {
                get_token();
            }
        }
            goto and_scope;

        case node::node_t::NODE_IDENTIFIER:     // <name> or <namespace>::<name>
            // NOTE: an IDENTIFIER here remains NODE_IDENTIFIER
            //       so it does not look like the previous expression
            //       (i.e. an expression literal can be just an
            //       identifier but it will be marked as
            //       NODE_VIDENTIFIER instead)
            name->set_string(f_node->get_string());
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case node::node_t::NODE_PRIVATE:        // private::<name> only
        case node::node_t::NODE_PROTECTED:      // protected::<name> only
        case node::node_t::NODE_PUBLIC:         // public::<name> only
            get_token();
and_scope:
            if(f_node->get_type() == node::node_t::NODE_SCOPE)
            {
                // TBD: I do not think that we need a scope operator at all
                //      since we can use the '.' (MEMBER) operator in all cases
                //      I can currently think of (and in JavaScript you are
                //      expected to do so anyway!) therefore I only authorize
                //      it as an extension at the moment
                if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
                    msg << "the '::' operator is only available when extended operators are authorized (use extended_operators;).";
                }

                get_token();
                if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
                {
                    name->append_child(f_node);
                    get_token();
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_SCOPE, f_lexer->get_position());
                    msg << "'::' is expected to always be followed by an identifier.";
                }
            }
            else if(type != node::node_t::NODE_IDENTIFIER)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, f_lexer->get_position());
                msg << "'public', 'protected', or 'private' or a dynamic scope cannot be used as a field name, '::' was expected.";
            }
            break;

        case node::node_t::NODE_FLOATING_POINT:
        case node::node_t::NODE_INTEGER:
        case node::node_t::NODE_STRING:
            name = f_node;
            get_token();
            break;

        default:
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD, f_lexer->get_position());
            msg << "the name of a field was expected.";
            break;

        }

        if(f_node->get_type() == node::node_t::NODE_COLON)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COLON_EXPECTED, f_lexer->get_position());
            msg << "':' expected after the name of a field.";

            // if we have a closing brace here, the programmer
            // tried to end his list improperly; we just
            // accept that one silently! (like in C/C++)
            if(f_node->get_type() == node::node_t::NODE_CLOSE_CURVLY_BRACKET
            || f_node->get_type() == node::node_t::NODE_SEMICOLON)
            {
                // this is probably the end...
                return;
            }

            // if we have a comma here, the programmer
            // just forgot a few things...
            if(f_node->get_type() == node::node_t::NODE_COMMA)
            {
                get_token();
                // we accept a comma at the end here too!
                if(f_node->get_type() == node::node_t::NODE_CLOSE_CURVLY_BRACKET
                || f_node->get_type() == node::node_t::NODE_SEMICOLON)
                {
                    return;
                }
                continue;
            }
        }

        // add the name only now so we have a mostly
        // valid tree from here on
        n->append_child(name);

        node::pointer_t set(f_lexer->get_new_node(node::node_t::NODE_SET));
        node::pointer_t value;
        assignment_expression(value);
        set->append_child(value);
        n->append_child(set);

        // got to the end?
        if(f_node->get_type() == node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            return;
        }

        if(f_node->get_type() != node::node_t::NODE_COMMA)
        {
            if(f_node->get_type() == node::node_t::NODE_SEMICOLON)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, f_lexer->get_position());
                msg << "'}' expected before the ';' to end an object literal.";
                return;
            }
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, f_lexer->get_position());
            msg << "',' or '}' expected after the value of a field.";
        }
        else
        {
            get_token();
        }
    }
    snapdev::NOT_REACHED();
}



} // namespace as2js
// vim: ts=4 sw=4 et
