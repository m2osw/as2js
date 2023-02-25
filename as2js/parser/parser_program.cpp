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
#include    "as2js/parser.h"

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER PROGRAM  *************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::program(node::pointer_t & program)
{
    program = f_lexer->get_new_node(node_t::NODE_PROGRAM);
    while(f_node->get_type() != node_t::NODE_EOF)
    {
        node::pointer_t directives;
        directive_list(directives);
        program->append_child(directives);

        if(f_node->get_type() == node_t::NODE_ELSE)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_KEYWORD, f_lexer->get_position());
            msg << "\"else\" not expected without an \"if\" keyword.";
            get_token();
        }
        else if(f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "\"}\" not expected without a \"{\".";
            get_token();
        }
    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
