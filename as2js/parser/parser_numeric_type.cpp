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
#include    "as2js/parser.h"

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER NUMERIC TYPE  ********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::numeric_type(node::pointer_t& numeric_type_node, node::pointer_t& name)
{
    // TBD: can we really use NODE_TYPE here?
    numeric_type_node = f_lexer->get_new_node(node_t::NODE_TYPE);

    numeric_type_node->append_child(name);

    // we are called with the current token set to NODE_AS, get
    // the following token, it has to be a literal number
    //
    // TODO: support any constant expression
    //
    get_token();
    if(f_node->get_type() == node_t::NODE_IDENTIFIER
    && f_node->get_string() == "mod")
    {
        numeric_type_node->set_flag(flag_t::NODE_TYPE_FLAG_MODULO, true);

        // skip the word 'mod'
        get_token();

        if(f_node->get_type() == node_t::NODE_SEMICOLON)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
            msg << "missing literal number for a numeric type declaration.";
            return;
        }

        if(f_node->get_type() != node_t::NODE_INTEGER
        && f_node->get_type() != node_t::NODE_FLOATING_POINT)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
            msg << "invalid numeric type declaration, the modulo must be a literal number.";

            // skip that token because it's useless, and we expect
            // a semi-colon right after that
            get_token();
            return;
        }

        // RESULT OF: use name as mod 123;
        numeric_type_node->append_child(f_node);
        get_token();
        return;
    }

    node_t left_type(f_node->get_type());
    int sign(1);
    if(left_type == node_t::NODE_ADD)
    {
        get_token();
        left_type = f_node->get_type();
    }
    else if(left_type == node_t::NODE_SUBTRACT)
    {
        sign = -1;
        get_token();
        left_type = f_node->get_type();
    }
    if(left_type == node_t::NODE_INTEGER)
    {
        integer i(f_node->get_integer());
        i.set(i.get() * sign);
        f_node->set_integer(i);
    }
    else if(left_type == node_t::NODE_FLOATING_POINT)
    {
        floating_point f(f_node->get_floating_point());
        f.set(f.get() * sign);
        f_node->set_floating_point(f);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
        msg << "invalid numeric type declaration, the range must start with a literal number.";
        // TODO: we may want to check whether the next
        //       token is '..' or ';'...
        return;
    }

    node::pointer_t left_node(f_node);
    numeric_type_node->append_child(f_node);

    // now we expect '..'
    get_token();
    if(f_node->get_type() != node_t::NODE_RANGE)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
        msg << "invalid numeric type declaration, the range must use \"..\" to separate the minimum and maximum boundaries (unexpected \"" << f_node->get_type_name() << "\").";

        // in case the user put '...' instead of '..'
        if(f_node->get_type() == node_t::NODE_REST)
        {
            get_token();
        }
    }
    else
    {
        get_token();
    }

    node_t right_type(f_node->get_type());
    sign = 1;
    if(right_type == node_t::NODE_ADD)
    {
        get_token();
        right_type = f_node->get_type();
    }
    else if(right_type == node_t::NODE_SUBTRACT)
    {
        sign = -1;
        get_token();
        right_type = f_node->get_type();
    }
    if(right_type == node_t::NODE_INTEGER)
    {
        integer i(f_node->get_integer());
        i.set(i.get() * sign);
        f_node->set_integer(i);
    }
    else if(right_type == node_t::NODE_FLOATING_POINT)
    {
        floating_point f(f_node->get_floating_point());
        f.set(f.get() * sign);
        f_node->set_floating_point(f);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
        msg << "invalid numeric type declaration, the range must end with a literal number.";
        if(f_node->get_type() != node_t::NODE_SEMICOLON)
        {
            // avoid an additional error
            get_token();
        }
        return;
    }

    // RESULT OF: use name as 0 .. 100;
    node::pointer_t right_node(f_node);
    numeric_type_node->append_child(f_node);

    get_token();

    // we verify this after the get_token() to skip the
    // second number so we do not generate yet another error
    if(right_type != left_type)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
        msg << "invalid numeric type declaration, the range must use numbers of the same type on both sides.";
    }
    else if(left_type == node_t::NODE_INTEGER)
    {
        if(left_node->get_integer().get() > right_node->get_integer().get())
        {
            message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
            msg << "numeric type declaration is empty (only accepts \"null\") because left value of range is larger than right value.";
        }
    }
    else
    {
        if(left_node->get_floating_point().get() > right_node->get_floating_point().get())
        {
            message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_position());
            msg << "numeric type declaration is empty (only accepts \"null\") because left value of range is larger than right value.";
        }
    }
}






}
// namespace as2js

// vim: ts=4 sw=4 et
