// Copyright (c) 2005-2024  Made to Order Software Corp.  All Rights Reserved
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
/***  PARSER VARIABLE  ************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Parse a variable definition.
 *
 * Variables can be introduce with the VAR keyword:
 *
 * \code
 *      VAR name;
 *      VAR name = expression;
 * \endcode
 *
 * Variables can also be marked constant with the CONST keyword, in that
 * case the VAR keyword is optional. In this case, the value of the
 * variable must be defined:
 *
 * \code
 *      CONST VAR name = expression;
 *      CONST name = expression;
 * \endcode
 *
 * Variables can also be marked final with the FINAL keyword, in that case
 * the VAR keyword is optional. A final variable can be initialized once
 * only, but it does not need to happen at the time the variable is declared:
 *
 * \code
 *      FINAL VAR name;
 *      FINAL VAR name = expresion;
 *      FINAL name;
 *      FINAL name = expression;
 * \endcode
 *
 * \param[out] n  The node where the variable (NODE_VAR) is saved.
 * \param[in] variable_type  The type of variable (NODE_VAR, NODE_CONST, or
 *                           NODE_FINAL).
 */
void parser::variable(node::pointer_t & n, node_t const variable_type)
{
    n = f_lexer->get_new_node(node_t::NODE_VAR);
    for(;;)
    {
        node::pointer_t variable_node(f_lexer->get_new_node(node_t::NODE_VARIABLE));
        if(variable_type == node_t::NODE_CONST)
        {
            variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_CONST, true);
        }
        else if(variable_type == node_t::NODE_FINAL)
        {
            variable_node->set_flag(flag_t::NODE_VARIABLE_FLAG_FINAL, true);
        }
        n->append_child(variable_node);

        if(f_node->get_type() == node_t::NODE_IDENTIFIER)
        {
            variable_node->set_string(f_node->get_string());
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_VARIABLE, f_lexer->get_position());
            std::string type_name(
                    variable_type == node_t::NODE_CONST
                        ? "const"
                        : variable_type == node_t::NODE_FINAL
                            ? "final"
                            : "var"
                );
            msg << "expected an identifier after the \"" << type_name << "\" keyword.";
        }

        if(f_node->get_type() == node_t::NODE_COLON)
        {
            get_token();
            node::pointer_t type(f_lexer->get_new_node(node_t::NODE_TYPE));
            node::pointer_t expr;
            conditional_expression(expr, false);
            type->append_child(expr);
            variable_node->append_child(type);
        }

        if(f_node->get_type() == node_t::NODE_ASSIGNMENT)
        {
            // TBD: should we avoid the NODE_SET on each attribute?
            //      at this time we get one expression per attribute...
            get_token();
            do
            {
                // TODO: to really support all attributes we need to have
                //       a switch here to include all the keyword based
                //       attributes (i.e. private, abstract, etc.)
                //
                //       [however, we must make sure we do not interfere with
                //       other uses of those keywords in expressions, private
                //       and public are understood as scoping keywords!]
                //
                node::pointer_t initializer(f_lexer->get_new_node(node_t::NODE_SET));
                node::pointer_t expr;
                conditional_expression(expr, false);
                initializer->append_child(expr);
                variable_node->append_child(initializer);
                // We loop in case we have a list of attributes!
                // This could also be a big syntax error (a missing
                // operator in most cases.) We will report the error
                // later once we know where the variable is being
                // used.
            }
            while(variable_type != node_t::NODE_VAR
                && f_node->get_type() != node_t::NODE_COMMA
                && f_node->get_type() != node_t::NODE_SEMICOLON
                && f_node->get_type() != node_t::NODE_OPEN_CURVLY_BRACKET
                && f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET
                && f_node->get_type() != node_t::NODE_CLOSE_PARENTHESIS);
        }

        if(f_node->get_type() != node_t::NODE_COMMA)
        {
            return;
        }
        get_token();
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
