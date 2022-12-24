// Copyright (c) 2014-2022  Made to Order Software Corp.  All Rights Reserved
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

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER BLOCK  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::block(node::pointer_t & n)
{
    // handle the emptiness right here
    if(f_node->get_type() != node::node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        directive_list(n);
    }

    if(f_node->get_type() != node::node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'}' expected to close a block.";
    }
    else
    {
        // skip the '}'
        get_token();
    }
}


void parser::forced_block(node::pointer_t & n, node::pointer_t statement)
{
    // if user turned on the forced block flag (bit 1 in extended statements)
    // then we much have the '{' and '}' for all sorts of blocks
    // (while, for, do, with, if, else)
    // in a way this is very similar to the try/catch/finally which
    // intrinsicly require the curvly brackets
    if(f_options
    && (f_options->get_option(options::option_t::OPTION_EXTENDED_STATEMENTS) & 2) != 0)
    {
        // in this case we force users to use '{' and '}' for all blocks
        if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
        {
            get_token();

            // although the extra directive list may look useless, it may
            // be very important if the user declared variables (because
            // we support proper variable definition on a per block basis)
            n = f_lexer->get_new_node(node::node_t::NODE_DIRECTIVE_LIST);
            node::pointer_t block_node;
            block(block_node);
            n->append_child(block_node);
        }
        else
        {
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
                msg << "'{' expected to open the '" << statement->get_type_name() << "' block.";
            }

            // still read one directive
            directive(n);
        }
    }
    else
    {
        directive(n);
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER BREAK & CONTINUE  ****************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Read a break or continue statement.
 *
 * The statement is a break or continue optionally followed by a label
 * (an identifier) or the default keyword (a special label meaning
 * use the default behavior.)
 *
 * Then we expect a semi-colon.
 *
 * The label is saved in the break or continue statement as the string
 * of the break or continue node.
 *
 * \code
 *     // A break by itself or the default break
 *     break;
 *     break default;
 *    
 *     // A break with a label
 *     break label;
 * \endcode
 *
 * \param[out] n  The node to be created.
 * \param[in] type  The type of node (break or continue).
 */
void parser::break_continue(node::pointer_t & n, node::node_t type)
{
    n = f_lexer->get_new_node(type);

    if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
    {
        n->set_string(f_node->get_string());
        get_token();
    }
    else if(f_node->get_type() == node::node_t::NODE_DEFAULT)
    {
        // default is equivalent to no label
        get_token();
    }

    if(f_node->get_type() != node::node_t::NODE_SEMICOLON)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_LABEL, f_lexer->get_position());
        msg << "'break' and 'continue' can be followed by one label only.";
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER CASE  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::case_directive(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_CASE);
    node::pointer_t expr;
    expression(expr);
    n->append_child(expr);

    // check for 'case <expr> ... <expr>:'
    if(f_node->get_type() == node::node_t::NODE_REST
    || f_node->get_type() == node::node_t::NODE_RANGE)
    {
        if(!has_option_set(options::option_t::OPTION_EXTENDED_STATEMENTS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "ranges in a 'case' statement are only accepted when extended statements are allowed (use extended_statements;).";
        }
        get_token();
        node::pointer_t expr_to;
        expression(expr_to);
        n->append_child(expr_to);
    }

    if(f_node->get_type() == node::node_t::NODE_COLON)
    {
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CASE_LABEL, f_lexer->get_position());
        msg << "case expression expected to be followed by ':'.";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER CATCH  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::catch_directive(node::pointer_t & n)
{
    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_CATCH);
        get_token();
        node::pointer_t parameters;
        bool unused;
        parameter_list(parameters, unused);
        if(!parameters)
        {
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CATCH, f_lexer->get_position());
                msg << "the 'catch' statement cannot be used with void as its list of parameters.";
            }

            // silently close the parenthesis if possible
            if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                get_token();
            }
            return;
        }
        n->append_child(parameters);
        // we want exactly ONE parameter
        size_t const count(parameters->get_children_size());
        if(count != 1)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CATCH, f_lexer->get_position());
            msg << "the 'catch' keyword expects exactly one parameter.";
        }
        else
        {
            // There is just one parameter, make sure there
            // is no initializer
            bool has_type(false);
            node::pointer_t param(parameters->get_child(0));
            size_t idx(param->get_children_size());
            while(idx > 0)
            {
                --idx;
                if(param->get_child(idx)->get_type() == node::node_t::NODE_SET)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CATCH, f_lexer->get_position());
                    msg << "'catch' parameters do not support initializers.";
                    break;
                }
                has_type = true;
            }
            if(has_type)
            {
                n->set_flag(node::flag_t::NODE_CATCH_FLAG_TYPED, true);
            }
        }
        if(f_node->get_type() == node::node_t::NODE_IF)
        {
            // to support the Netscape extension of conditional catch()'s
            node::pointer_t if_node(f_node);
            get_token();
            node::pointer_t expr;
            expression(expr);
            if_node->append_child(expr);
            n->append_child(if_node);
        }
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
            if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
            {
                get_token();
                node::pointer_t one_block;
                block(one_block);
                n->append_child(one_block);
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
                msg << "'{' expected after the 'catch' parameter list.";
            }
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to end the 'catch' parameter list.";
        }
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected after the 'catch' keyword.";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER DEBBUGER  ************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::debugger(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_DEBUGGER);
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER DEFAULT  *************************************************/
/**********************************************************************/
/**********************************************************************/

// NOTE: if default wasn't a keyword, then it could be used as a
//       label like any user label!
//
//       The fact that it is a keyword allows us to forbid default with
//       the goto instruction without having to do any extra work.
//
void parser::default_directive(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_DEFAULT);

    // default is just itself!
    if(f_node->get_type() == node::node_t::NODE_COLON)
    {
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DEFAULT_LABEL, f_lexer->get_position());
        msg << "default label expected to be followed by ':'.";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER DO  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::do_directive(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_DO);

    node::pointer_t one_directive;
    forced_block(one_directive, n);
    n->append_child(one_directive);

    if(f_node->get_type() == node::node_t::NODE_WHILE)
    {
        get_token();
        if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
        {
            get_token();
            node::pointer_t expr;
            expression(expr);
            n->append_child(expr);
            if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "')' expected to end the 'while' expression.";
            }
            else
            {
                get_token();
            }
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "'(' expected after the 'while' keyword.";
        }
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_DO, f_lexer->get_position());
        msg << "'while' expected after the block of a 'do' keyword.";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER FOR  *****************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::for_directive(node::pointer_t & n)
{
    // for each(...)
    bool const for_each(f_node->get_type() == node::node_t::NODE_IDENTIFIER
                     && f_node->get_string() == "each");
    if(for_each)
    {
        get_token(); // skip the 'each' "keyword"
    }
    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_FOR);

        get_token(); // skip the '('
        if(f_node->get_type() == node::node_t::NODE_SEMICOLON)
        {
            // *** EMPTY ***
            // When we have ';' directly we have got an empty initializer!
            node::pointer_t empty(f_lexer->get_new_node(node::node_t::NODE_EMPTY));
            n->append_child(empty);
        }
        else if(f_node->get_type() == node::node_t::NODE_CONST
             || f_node->get_type() == node::node_t::NODE_VAR)
        {
            // *** VARIABLE ***
            bool const constant(f_node->get_type() == node::node_t::NODE_CONST);
            if(constant)
            {
                n->set_flag(node::flag_t::NODE_FOR_FLAG_CONST, true);
                get_token(); // skip the 'const'
                if(f_node->get_type() == node::node_t::NODE_VAR)
                {
                    // allow just 'const' or 'const var'
                    get_token(); // skip the 'var'
                }
            }
            else
            {
                get_token(); // skip the 'var'
            }
            node::pointer_t variables;
            // TODO: add support for NODE_FINAL if possible here?
            variable(variables, constant ? node::node_t::NODE_CONST : node::node_t::NODE_VAR);
            n->append_child(variables);

            // This can happen when we return from the
            // variable() function
            if(f_node->get_type() == node::node_t::NODE_IN)
            {
                // *** IN ***
                get_token();
                node::pointer_t expr;
                expression(expr);
                // TODO: we probably want to test whether the expression we
                //       just got includes a comma (NODE_LIST) and/or
                //       another 'in' and generate a WARNING in that case
                //       (although the compiler should err here if necessary)
                n->append_child(expr);
                n->set_flag(node::flag_t::NODE_FOR_FLAG_IN, true);
            }
        }
        else
        {
            node::pointer_t expr;
            expression(expr);

            // Note: if there is more than one expression (Variable
            //       definition) then the expression() function returns
            //       a NODE_LIST, not a NODE_IN

            if(expr->get_type() == node::node_t::NODE_IN)
            {
                // *** IN ***
                // if the last expression uses 'in' then break it up in two
                // (the compiler will check that the left hand side is valid
                // for the 'in' keyword here)
                node::pointer_t left(expr->get_child(0));
                node::pointer_t right(expr->get_child(1));
                expr->delete_child(0);
                expr->delete_child(0);
                n->append_child(left);
                n->append_child(right);
                n->set_flag(node::flag_t::NODE_FOR_FLAG_IN, true);
            }
            else
            {
                n->append_child(expr);
            }
        }

        // if not marked as an IN for loop,
        // then get the 2nd and 3rd expressions
        if(!n->get_flag(node::flag_t::NODE_FOR_FLAG_IN))
        {
            if(f_node->get_type() == node::node_t::NODE_SEMICOLON)
            {
                // *** SECOND EXPRESSION ***
                get_token();
                node::pointer_t expr;
                if(f_node->get_type() == node::node_t::NODE_SEMICOLON)
                {
                    // empty expression
                    expr = f_lexer->get_new_node(node::node_t::NODE_EMPTY);
                }
                else
                {
                    expression(expr);
                }
                n->append_child(expr);
                if(f_node->get_type() == node::node_t::NODE_SEMICOLON)
                {
                    // *** THIRD EXPRESSION ***
                    get_token();
                    node::pointer_t thrid_expr;
                    if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
                    {
                        thrid_expr = f_lexer->get_new_node(node::node_t::NODE_EMPTY);
                    }
                    else
                    {
                        expression(thrid_expr);
                    }
                    n->append_child(thrid_expr);
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_position());
                    msg << "';' expected between the last two 'for' expressions.";
                }
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_position());
                msg << "';' or 'in' expected between the 'for' expressions.";
            }
        }

        if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to close the 'for' expressions.";
        }
        else
        {
            get_token();
        }

        if(for_each)
        {
            if(n->get_children_size() == 2)
            {
                n->set_flag(node::flag_t::NODE_FOR_FLAG_FOREACH, true);
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "'for each()' only available with an enumeration for.";
            }
        }

        // *** DIRECTIVES ***
        node::pointer_t one_directive;
        forced_block(one_directive, n);
        n->append_child(one_directive);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected following the 'for' keyword.";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER GOTO  ****************************************************/
/**********************************************************************/
/**********************************************************************/

// although JavaScript does not support a goto directive, we support it
// in the parser; however, the compiler is likely to reject it
void parser::goto_directive(node::pointer_t & n)
{
    if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_GOTO);

        // save the label
        n->set_string(f_node->get_string());

        // skip the label
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_GOTO, f_lexer->get_position());
        msg << "'goto' expects a label as parameter.";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IF  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::if_directive(node::pointer_t & n)
{
    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_IF);
        get_token();
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to end the 'if' expression.";
        }

        if(f_node->get_type() == node::node_t::NODE_ELSE)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_IMPROPER_STATEMENT, f_lexer->get_position());
            msg << "statements expected following the 'if' expression, 'else' found instead.";
        }
        else
        {
            // IF part
            node::pointer_t one_directive;
            forced_block(one_directive, n);
            n->append_child(one_directive);
        }

        // Note that this is the only place where ELSE is permitted!
        if(f_node->get_type() == node::node_t::NODE_ELSE)
        {
            get_token();

            // ELSE part
            //
            // TODO: when calling the forced_block() we call with the 'if'
            //       node which means errors are presented as if the 'if'
            //       block was wrong and not the 'else'
            node::pointer_t else_directive;
            forced_block(else_directive, n);
            n->append_child(else_directive);
        }
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected after the 'if' keyword.";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER RETURN  **************************************************/
/**********************************************************************/
/**********************************************************************/



void parser::return_directive(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_RETURN);
    if(f_node->get_type() != node::node_t::NODE_SEMICOLON)
    {
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER TRY & FINALLY  *******************************************/
/**********************************************************************/
/**********************************************************************/

void parser::try_finally(node::pointer_t & n, node::node_t const type)
{
    if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
        n = f_lexer->get_new_node(type);
        node::pointer_t one_block;
        block(one_block);
        n->append_child(one_block);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'{' expected after the '" << (type == node::node_t::NODE_TRY ? "try" : "finally") << "' keyword.";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER SWITCH  **************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::switch_directive(node::pointer_t & n)
{
    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_SWITCH);

        // a default comparison is important to support ranges properly
        //n->set_switch_operator(node::node_t::NODE_UNKNOWN); -- this is the default

        get_token();
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to end the 'switch' expression.";
        }
        if(f_node->get_type() == node::node_t::NODE_WITH)
        {
            if(!has_option_set(options::option_t::OPTION_EXTENDED_STATEMENTS))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
                msg << "a switch() statement can be followed by a 'with' only if extended statements were turned on (use extended_statements;).";
            }
            get_token();
            bool const has_open(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS);
            if(has_open)
            {
                get_token();
            }
            switch(f_node->get_type())
            {
            // equality
            case node::node_t::NODE_STRICTLY_EQUAL:
            case node::node_t::NODE_EQUAL:
            case node::node_t::NODE_NOT_EQUAL:
            case node::node_t::NODE_STRICTLY_NOT_EQUAL:
            // relational
            case node::node_t::NODE_MATCH:
            case node::node_t::NODE_IN:
            case node::node_t::NODE_IS:
            case node::node_t::NODE_AS:
            case node::node_t::NODE_INSTANCEOF:
            case node::node_t::NODE_LESS:
            case node::node_t::NODE_LESS_EQUAL:
            case node::node_t::NODE_GREATER:
            case node::node_t::NODE_GREATER_EQUAL:
            // so the user can specify the default too
            case node::node_t::NODE_DEFAULT:
                n->set_switch_operator(f_node->get_type());
                get_token();
                break;

            default:
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "'" << f_node->get_type_name() << "' is not a supported operator for a 'switch() with()' expression.";

                if(f_node->get_type() != node::node_t::NODE_OPEN_CURVLY_BRACKET)
                {
                    // the user probably used an invalid operator, skip it
                    get_token();
                }
            }
                break;

            }
            if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                get_token();
                if(!has_open)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                    msg << "'(' was expected to start the 'switch() with()' expression.";
                }
            }
            else if(has_open)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "')' expected to end the 'switch() with()' expression.";
            }
        }
        node::pointer_t attr_list;
        attributes(attr_list);
        if(attr_list && attr_list->get_children_size() > 0)
        {
            n->set_attribute_node(attr_list);
        }
        if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
        {
            get_token();
            node::pointer_t one_block;
            block(one_block);
            n->append_child(one_block);
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "'{' expected after the 'switch' expression.";
        }
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected after the 'switch' keyword.";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER SYNCHRONIZED  ********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::synchronized(node::pointer_t & n)
{
    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_SYNCHRONIZED);
        get_token();

        // retrieve the object being synchronized
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to end the 'synchronized' expression.";
        }
        if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
        {
            get_token();
            node::pointer_t one_block;
            block(one_block);
            n->append_child(one_block);
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "'{' expected after the 'synchronized' expression.";
        }
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected after the 'synchronized' keyword.";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER THROW  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::throw_directive(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node::node_t::NODE_THROW);

    // if we already have a semi-colon, the user is rethrowing
    if(f_node->get_type() != node::node_t::NODE_SEMICOLON)
    {
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER WITH & WHILE  ********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::with_while(node::pointer_t & n, node::node_t const type)
{
    char const *inst = type == node::node_t::NODE_WITH ? "with" : "while";

    if(type == node::node_t::NODE_WITH)
    {
        if(!has_option_set(options::option_t::OPTION_ALLOW_WITH))
        {
            // WITH is just not allowed at all by default
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "'WITH' is not allowed; you may authorize it with a pragam (use allow_with;) but it is not recommended.";
        }
        else if(has_option_set(options::option_t::OPTION_STRICT))
        {
            // WITH cannot be used in strict mode (see ECMAScript)
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED_IN_STRICT_MODE, f_lexer->get_position());
            msg << "'WITH' is not allowed in strict mode.";
        }
    }

    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        n = f_lexer->get_new_node(type);
        get_token();
        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
            msg << "')' expected to end the '" << inst << "' expression.";
        }
        node::pointer_t one_directive;
        forced_block(one_directive, n);
        n->append_child(one_directive);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
        msg << "'(' expected after the '" << inst << "' keyword.";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER YIELD  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::yield(node::pointer_t & n)
{
    if(f_node->get_type() != node::node_t::NODE_SEMICOLON)
    {
        n = f_lexer->get_new_node(node::node_t::NODE_YIELD);

        node::pointer_t expr;
        expression(expr);
        n->append_child(expr);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_EXPRESSION_EXPECTED, f_lexer->get_position());
        msg << "yield is expected to be followed by an expression.";
    }
}






}
// namespace as2js

// vim: ts=4 sw=4 et
