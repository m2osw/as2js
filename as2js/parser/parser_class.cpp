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
#include    "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CLASS  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::class_declaration(node::pointer_t & node, node_t type)
{
    node = f_lexer->get_new_node(type);

    // *** NAME ***
    if(f_node->get_type() != node_t::NODE_IDENTIFIER)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, f_lexer->get_position());
        msg << "the name of the class is expected after the keyword 'class'.";

        switch(f_node->get_type())
        {
        case node_t::NODE_EXTENDS:
        case node_t::NODE_IMPLEMENTS:
        case node_t::NODE_OPEN_CURVLY_BRACKET:
        //case node_t::NODE_SEMICOLON: -- not necessary here
            break;

        default:
            return;

        }
    }
    else
    {
        node->set_string(f_node->get_string());
        get_token();
    }

    // *** INHERITANCE ***
    if(f_node->get_type() == node_t::NODE_COLON)
    {
        // if we have a colon, followed by private, protected, or public
        // then it looks like a C++ declaration
        node::pointer_t save(f_node);
        get_token();
        if(f_node->get_type() == node_t::NODE_EXTENDS
        || f_node->get_type() == node_t::NODE_IMPLEMENTS)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE, f_lexer->get_position());
            msg << "the 'extends' and 'implements' instructions cannot be preceeded by a colon.";
        }
        else if(f_node->get_type() == node_t::NODE_OPEN_CURVLY_BRACKET
             || f_node->get_type() == node_t::NODE_SEMICOLON)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "the 'class' keyword cannot be followed by a colon.";
        }
    }
    enum class status_t
    {
        STATUS_EXTENDS,
        STATUS_IMPLEMENTS,
        STATUS_DONE
    };
    status_t status(status_t::STATUS_EXTENDS);
    // XXX: enforce extends, then implements? Or is that just me thinking
    //      that it should be in that order?
    while(f_node->get_type() == node_t::NODE_EXTENDS
       || f_node->get_type() == node_t::NODE_IMPLEMENTS
       || f_node->get_type() == node_t::NODE_PRIVATE
       || f_node->get_type() == node_t::NODE_PROTECTED
       || f_node->get_type() == node_t::NODE_PUBLIC)
    {
        node::pointer_t inherits(f_node);

        node_t const extend_type(f_node->get_type());

        // this is used because C++ programmers are not unlikely to use one
        // of those keywords instead of 'exends' or 'implements'
        if(f_node->get_type() == node_t::NODE_PRIVATE
        || f_node->get_type() == node_t::NODE_PROTECTED
        || f_node->get_type() == node_t::NODE_PUBLIC)
        {
            // just skip the keyword and read the expression as expected
            // the expression can be a list
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE, f_lexer->get_position());
            msg << "please use 'extends' or 'implements' to define a list of base classes. 'public', 'private', and 'protected' are used in C++ only.";

            inherits = f_node->create_replacement(node_t::NODE_EXTENDS);
        }
        else if(status != status_t::STATUS_EXTENDS
             && f_node->get_type() != node_t::NODE_IMPLEMENTS)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE, f_lexer->get_position());
            msg << "a class definition expects 'extends' first and then 'implements'.";
        }
        else if(status == status_t::STATUS_DONE)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE, f_lexer->get_position());
            msg << "a class definition expects zero or one 'extends' and then zero or one 'implements'. Use commas to separate multiple inheritance names.";
        }

        node->append_child(inherits);

        get_token();

        node::pointer_t expr;
        expression(expr);
        // TODO: EXTENDS and IMPLEMENTS do not accept assignments.
        //       verify that expr does not include any
        inherits->append_child(expr);

        if(status == status_t::STATUS_EXTENDS && extend_type == node_t::NODE_EXTENDS)
        {
            status = status_t::STATUS_IMPLEMENTS;
        }
        else
        {
            status = status_t::STATUS_DONE;
        }
    }

    if(f_node->get_type() == node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();

        // *** DECLARATION ***
        if(f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            node::pointer_t directive_list_node;
            directive_list(directive_list_node);
            node->append_child(directive_list_node);
        }
        else
        {
            // this is important to distinguish an empty node from
            // a forward declaration
            node::pointer_t empty_node(f_lexer->get_new_node(node_t::NODE_EMPTY));
            node->append_child(empty_node);
        }

        if(f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            get_token();
        }
        else
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "'}' expected to close the 'class' definition.";
        }
    }
    else if(f_node->get_type() != node_t::NODE_SEMICOLON)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'{' expected to start the 'class' definition.";
    }
    // else -- accept empty class definitions (for typedef's and forward declaration)
}


void parser::contract_declaration(node::pointer_t& node, node_t type)
{
    node = f_lexer->get_new_node(type);

    // contract are labeled expressions
    for(;;)
    {
        node::pointer_t label(f_lexer->get_new_node(node_t::NODE_LABEL));
        node->append_child(label);
        if(f_node->get_type() != node_t::NODE_IDENTIFIER)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_LABEL, f_lexer->get_position());
            msg << "'" << node->get_type_name() << "' must be followed by a list of labeled expressions.";
        }
        else
        {
            label->set_string(f_node->get_string());
            // skip the identifier
            get_token();
        }
        if(f_node->get_type() != node_t::NODE_COLON)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COLON_EXPECTED, f_lexer->get_position());
            msg << "the '" << node->get_type_name() << "' label must be followed by a colon (:).";
        }
        else
        {
            // skip the colon
            get_token();
        }
        node::pointer_t expr;
        conditional_expression(expr, false);
        label->append_child(expr);
        if(f_node->get_type() != node_t::NODE_COMMA)
        {
            break;
        }
        // skip the comma
        get_token();
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER ENUM  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::enum_declaration(node::pointer_t & node)
{
    node = f_lexer->get_new_node(node_t::NODE_ENUM);

    bool const is_class(f_node->get_type() == node_t::NODE_CLASS);
    if(is_class)
    {
        get_token();
        node->set_flag(flag_t::NODE_ENUM_FLAG_CLASS, true);
    }

    // enumerations can be unamed
    if(f_node->get_type() == node_t::NODE_IDENTIFIER)
    {
        node->set_string(f_node->get_string());
        get_token();
    }

    // in case the name was not specified, we can still have a type
    if(f_node->get_type() == node_t::NODE_COLON)
    {
        get_token();
        node::pointer_t expr;
        expression(expr);
        node::pointer_t type(f_lexer->get_new_node(node_t::NODE_TYPE));
        type->append_child(expr);
        node->append_child(type);
    }

    if(f_node->get_type() != node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        if(f_node->get_type() == node_t::NODE_SEMICOLON)
        {
            // empty enumeration (i.e. forward declaration)
            if(node->get_string().empty())
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ENUM, f_lexer->get_position());
                msg << "a forward enumeration must be named.";
            }
            return;
        }
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'{' expected to start the 'enum' definition.";
        return;
    }

    get_token();
    if(f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        // this is required to be able to distinguish between an empty
        // enumeration (how useful though?!) and a forward definition
        node::pointer_t empty_node(f_lexer->get_new_node(node_t::NODE_EMPTY));
        node->append_child(empty_node);
    }
    else
    {
        node::pointer_t previous(f_lexer->get_new_node(node_t::NODE_NULL));
        while(f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET
           && f_node->get_type() != node_t::NODE_SEMICOLON
           && f_node->get_type() != node_t::NODE_EOF)
        {
            if(f_node->get_type() == node_t::NODE_COMMA)
            {
                // skip to the next token
                get_token();

                message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_lexer->get_position());
                msg << "',' unexpected without a name.";
                continue;
            }
            std::string current_name("null");
            node::pointer_t entry(f_lexer->get_new_node(node_t::NODE_VARIABLE));
            node->append_child(entry);
            if(f_node->get_type() == node_t::NODE_IDENTIFIER)
            {
                entry->set_flag(flag_t::NODE_VARIABLE_FLAG_CONST, true);
                entry->set_flag(flag_t::NODE_VARIABLE_FLAG_ENUM, true);
                current_name = f_node->get_string();
                entry->set_string(current_name);
                get_token();
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ENUM, f_lexer->get_position());
                msg << "each 'enum' entry needs to include an identifier.";
                if(f_node->get_type() != node_t::NODE_ASSIGNMENT
                && f_node->get_type() != node_t::NODE_COMMA
                && f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET)
                {
                    // skip that token otherwise we'd loop forever doing
                    // nothing more than generate errors
                    get_token();
                }
            }
            node::pointer_t expr;
            if(f_node->get_type() == node_t::NODE_ASSIGNMENT)
            {
                get_token();
                conditional_expression(expr, false);
            }
            else if(previous->get_type() == node_t::NODE_NULL)
            {
                // very first time
                expr = f_lexer->get_new_node(node_t::NODE_INTEGER);
                //expr->set_integer(0); -- this is the default
            }
            else
            {
                expr = f_lexer->get_new_node(node_t::NODE_ADD);
                expr->append_child(previous); // left handside
                node::pointer_t one(f_lexer->get_new_node(node_t::NODE_INTEGER));
                integer int_one;
                int_one.set(1);
                one->set_integer(int_one);
                expr->append_child(one);
            }

            node::pointer_t set(f_lexer->get_new_node(node_t::NODE_SET));
            set->append_child(expr);
            entry->append_child(set);

            previous = f_lexer->get_new_node(node_t::NODE_IDENTIFIER);
            previous->set_string(current_name);

            if(f_node->get_type() == node_t::NODE_COMMA)
            {
                get_token();
            }
            else if(f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET
                 && f_node->get_type() != node_t::NODE_SEMICOLON)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, f_lexer->get_position());
                msg << "',' expected between enumeration elements.";
            }
        }
    }

    if(f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'}' expected to close the 'enum' definition.";
    }
}


} // namespace as2js
// vim: ts=4 sw=4 et
