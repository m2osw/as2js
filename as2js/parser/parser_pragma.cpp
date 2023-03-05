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
/***  PARSER PRAGMA  **************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::pragma()
{
    while(f_node->get_type() == node_t::NODE_IDENTIFIER)
    {
        std::string const name(f_node->get_string());
        node::pointer_t argument;
        get_token();
        if(f_node->get_type() == node_t::NODE_OPEN_PARENTHESIS)
        {
            // has zero or one argument
            //
            get_token();

            // accept an empty argument '()'
            //
            if(f_node->get_type() != node_t::NODE_CLOSE_PARENTHESIS)
            {
                bool const negative(f_node->get_type() == node_t::NODE_SUBTRACT);
                if(negative)
                {
                    // skip the '-' sign
                    get_token();
                }
                // TODO: add support for 'positive'?
                switch(f_node->get_type())
                {
                case node_t::NODE_FALSE:
                case node_t::NODE_STRING:
                case node_t::NODE_TRUE:
                    if(negative)
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
                        msg << "invalid negative argument for a pragma.";
                    }
                    argument = f_node;
                    get_token();
                    break;

                case node_t::NODE_FLOATING_POINT:
                    argument = f_node;
                    if(negative)
                    {
                        argument->set_floating_point(-argument->get_floating_point().get());
                    }
                    get_token();
                    break;

                case node_t::NODE_INTEGER:
                    argument = f_node;
                    if(negative)
                    {
                        argument->set_integer(-argument->get_integer().get());
                    }
                    get_token();
                    break;

                case node_t::NODE_CLOSE_PARENTHESIS:
                    if(negative)
                    {
                        // we cannot negate "nothingness"
                        // (i.e. use blah(-); is not valid)
                        //
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
                        msg << "a pragma argument cannot just be \"-\".";
                    }
                    break;

                default:
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
                    msg << "invalid argument type for a pragma.";
                }
                    break;

                }
            }
            if(f_node->get_type() != node_t::NODE_CLOSE_PARENTHESIS)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
                msg << "invalid argument for a pragma.";
            }
            else
            {
                get_token();
            }
        }
        bool const prima(f_node->get_type() == node_t::NODE_CONDITIONAL);
        if(prima)
        {
            // skip the '?'
            //
            get_token();
        }

        // Check out this pragma. We have the following
        // info about each pragma:
        //
        //    name        The pragma name
        //    argument    The pragma argument (unknown by default)
        //    prima       True if pragma name followed by '?'
        //
        // NOTE: pragmas that we do not recognize are simply
        //       being ignored.
        //
        option_value_t value(1);
        option_t option(option_t::OPTION_UNKNOWN);
        if(name == "allow_with")
        {
            option = option_t::OPTION_ALLOW_WITH;
        }
        else if(name == "no_allow_with")
        {
            option = option_t::OPTION_ALLOW_WITH;
            value = 0;
        }
        else if(name == "coverage")
        {
            option = option_t::OPTION_COVERAGE;
        }
        else if(name == "no_coverage")
        {
            option = option_t::OPTION_COVERAGE;
            value = 0;
        }
        else if(name == "debug")
        {
            option = option_t::OPTION_DEBUG;
        }
        else if(name == "no_debug")
        {
            option = option_t::OPTION_DEBUG;
            value = 0;
        }
        else if(name == "extended_escape_sequences")
        {
            option = option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES;
        }
        else if(name == "no_extended_escape_sequences")
        {
            option = option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES;
            value = 0;
        }
        else if(name == "extended_operators")
        {
            option = option_t::OPTION_EXTENDED_OPERATORS;
        }
        else if(name == "no_extended_operators")
        {
            option = option_t::OPTION_EXTENDED_OPERATORS;
            value = 0;
        }
        else if(name == "extended_statements")
        {
            option = option_t::OPTION_EXTENDED_STATEMENTS;
        }
        else if(name == "no_extended_statements")
        {
            option = option_t::OPTION_EXTENDED_STATEMENTS;
            value = 0;
        }
        else if(name == "octal")
        {
            option = option_t::OPTION_OCTAL;
        }
        else if(name == "no_octal")
        {
            option = option_t::OPTION_OCTAL;
            value = 0;
        }
        else if(name == "strict")
        {
            option = option_t::OPTION_STRICT;
        }
        else if(name == "no_strict")
        {
            option = option_t::OPTION_STRICT;
            value = 0;
        }
        else if(name == "trace")
        {
            option = option_t::OPTION_TRACE;
        }
        else if(name == "no_trace")
        {
            option = option_t::OPTION_TRACE;
            value = 0;
        }
        else if(name == "unsafe_math")
        {
            option = option_t::OPTION_UNSAFE_MATH;
        }
        else if(name == "no_unsafe_math")
        {
            option = option_t::OPTION_UNSAFE_MATH;
            value = 0;
        }
        if(option != option_t::OPTION_UNKNOWN)
        {
            pragma_option(option, prima, argument, value);
        }
        else
        {
std::cerr << "--- pragma not found [" << name << "]\n";
            message msg(message_level_t::MESSAGE_LEVEL_DEBUG, err_code_t::AS_ERR_UNKNOWN_PRAGMA, f_lexer->get_position());
            msg << "unknown pragma \""
                << name
                << "\" must be separated by commas.";
        }

        if(f_node->get_type() == node_t::NODE_COMMA)
        {
            get_token();
        }
        else if(f_node->get_type() == node_t::NODE_IDENTIFIER)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
            msg << "pragmas must be separated by commas.";
        }
        else if(f_node->get_type() != node_t::NODE_SEMICOLON)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_position());
            msg << "pragmas must be separated by commas and ended by a semicolon.";
            // no need for a break since the while() will exit already
        }
    }
}



void parser::pragma_option(
          option_t option
        , bool prima
        , node::pointer_t & argument
        , option_value_t value)
{
    // user overloaded the value?
    // if argument is a null pointer, then keep the input value as is
    //
    if(argument) switch(argument->get_type())
    {
    case node_t::NODE_TRUE:
        value = 1;
        break;

    case node_t::NODE_INTEGER:
        value = argument->get_integer().get();
        break;

    case node_t::NODE_FLOATING_POINT:
        // should we round up instead of using floor()?
        //
        value = static_cast<option_value_t>(argument->get_floating_point().get());
        break;

    case node_t::NODE_STRING:
    {
        // TBD: we could try to convert the string, but is that really
        //      necessary?
        //
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE_PRAGMA_ARGUMENT, f_lexer->get_position());
        msg << "incompatible pragma argument.";
    }
        break;

    default: // node_t::NODE_FALSE
        value = 0;
        break;

    }

    if(prima)
    {
        if(f_options->get_option(option) != value)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PRAGMA_FAILED, f_lexer->get_position());
            msg << "prima pragma failed.";
        }
        return;
    }

    f_options->set_option(option, value);
}





}
// namespace as2js

// vim: ts=4 sw=4 et
