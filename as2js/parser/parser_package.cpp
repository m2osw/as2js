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


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER PACKAGE  *************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::package(node::pointer_t & n)
{
    std::string name;

    n = f_lexer->get_new_node(node_t::NODE_PACKAGE);

    if(f_node->get_type() == node_t::NODE_IDENTIFIER)
    {
        name = f_node->get_string();
        get_token();
        while(f_node->get_type() == node_t::NODE_MEMBER)
        {
            get_token();
            if(f_node->get_type() != node_t::NODE_IDENTIFIER)
            {
                // unexpected token/missing name
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                msg << "invalid package name (expected an identifier after the last '.').";
                if(f_node->get_type() == node_t::NODE_OPEN_CURVLY_BRACKET
                || f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET
                || f_node->get_type() == node_t::NODE_SEMICOLON)
                {
                    break;
                }
                // try some more...
            }
            else
            {
                name += ".";
                name += f_node->get_string();
            }
            get_token();
        }
    }
    else if(f_node->get_type() == node_t::NODE_STRING)
    {
        name = f_node->get_string();
        // TODO: Validate Package Name (in case of a STRING)
        //       I think we need to check out the name here to make sure
        //       it is a valid package name (not too sure though whether
        //       we cannot just have any name?)
        //
        get_token();
    }

    // set the name and flags of this package
    //
    n->set_string(name);

    if(f_node->get_type() == node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'{' expected after the package name.";
        // TODO: should we return and not try to read the package?
    }

    node::pointer_t directives;
    directive_list(directives);
    n->append_child(directives);

    // when we return we should have a '}'
    if(f_node->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
        msg << "'}' expected after the package declaration.";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IMPORT  **************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::import(node::pointer_t & n)
{
    n = f_lexer->get_new_node(node_t::NODE_IMPORT);

    if(f_node->get_type() == node_t::NODE_IMPLEMENTS)
    {
        n->set_flag(flag_t::NODE_IMPORT_FLAG_IMPLEMENTS, true);
        get_token();
    }

    if(f_node->get_type() == node_t::NODE_IDENTIFIER)
    {
        std::string name;
        node::pointer_t first(f_node);
        get_token();
        bool const is_renaming = f_node->get_type() == node_t::NODE_ASSIGNMENT;
        if(is_renaming)
        {
            // add first as the package alias
            //
            n->append_child(first);

            get_token();
            if(f_node->get_type() == node_t::NODE_STRING)
            {
                name = f_node->get_string();
                get_token();
                if(f_node->get_type() == node_t::NODE_MEMBER
                || f_node->get_type() == node_t::NODE_RANGE
                || f_node->get_type() == node_t::NODE_REST)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                    msg << "a package name is either a string or a list of identifiers separated by periods (.); you cannot mixed both.";
                }
            }
            else if(f_node->get_type() == node_t::NODE_IDENTIFIER)
            {
                name = f_node->get_string();
                get_token();
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                msg << "the name of a package was expected.";
            }
        }
        else
        {
            name = first->get_string();
        }

        int everything(0);
        while(f_node->get_type() == node_t::NODE_MEMBER
           || f_node->get_type() == node_t::NODE_RANGE
           || f_node->get_type() == node_t::NODE_REST)
        {
            if(f_node->get_type() == node_t::NODE_RANGE
            || f_node->get_type() == node_t::NODE_REST)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                msg << "the name of a package is expected to be separated by single periods (.).";
            }
            if(everything == 1)
            {
                everything = 2;
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                msg << "the * notation can only be used once at the end of a name.";
            }
            name += ".";
            get_token();
            if(f_node->get_type() == node_t::NODE_MULTIPLY)
            {
                if(is_renaming && everything == 0)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                    msg << "the * notation cannot be used when renaming an import.";
                    everything = 2;
                }
                // everything in that directory
                name += "*";
                if(everything == 0)
                {
                    everything = 1;
                }
            }
            else if(f_node->get_type() != node_t::NODE_IDENTIFIER)
            {
                if(f_node->get_type() == node_t::NODE_STRING)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                    msg << "a package name is either a string or a list of identifiers separated by periods (.); you cannot mixed both.";
                    // skip the string, just in case
                    get_token();
                }
                else
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
                    msg << "the name of a package was expected.";
                }
                if(f_node->get_type() == node_t::NODE_MEMBER
                || f_node->get_type() == node_t::NODE_RANGE
                || f_node->get_type() == node_t::NODE_REST)
                {
                    // in case of another '.' (or a few other '.')
                    continue;
                }
                break;
            }
            else
            {
                name += f_node->get_string();
            }
            get_token();
        }

        n->set_string(name);
    }
    else if(f_node->get_type() == node_t::NODE_STRING)
    {
        // TODO: Validate Package Name (in case of a STRING)
        n->set_string(f_node->get_string());
        get_token();
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_position());
        msg << "a composed name or a string was expected after 'import'.";
        if(f_node->get_type() != node_t::NODE_SEMICOLON && f_node->get_type() != node_t::NODE_COMMA)
        {
            get_token();
        }
    }

    // Any namespace and/or include/exclude info?
    // NOTE: We accept multiple namespace and multiple include
    //     or exclude.
    //     However, include and exclude are mutually exclusive.
    long include_exclude = 0;
    while(f_node->get_type() == node_t::NODE_COMMA)
    {
        get_token();
        if(f_node->get_type() == node_t::NODE_NAMESPACE)
        {
            get_token();
            // read the namespace (an expression)
            node::pointer_t expr;
            conditional_expression(expr, false);
            node::pointer_t use(f_lexer->get_new_node(node_t::NODE_USE /*namespace*/));
            use->append_child(expr);
            n->append_child(use);
        }
        else if(f_node->get_type() == node_t::NODE_IDENTIFIER)
        {
            if(f_node->get_string() == "include")
            {
                if(include_exclude == 2)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_IMPORT, f_lexer->get_position());
                    msg << "include and exclude are mutually exclusive.";
                    include_exclude = 3;
                }
                else if(include_exclude == 0)
                {
                    include_exclude = 1;
                }
                get_token();
                // read the list of inclusion (an expression)
                node::pointer_t expr;
                conditional_expression(expr, false);
                node::pointer_t include(f_lexer->get_new_node(node_t::NODE_INCLUDE));
                include->append_child(expr);
                n->append_child(include);
            }
            else if(f_node->get_string() == "exclude")
            {
                if(include_exclude == 1)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_IMPORT, f_lexer->get_position());
                    msg << "include and exclude are mutually exclusive.";
                    include_exclude = 3;
                }
                else if(include_exclude == 0)
                {
                    include_exclude = 2;
                }
                get_token();
                // read the list of exclusion (an expression)
                node::pointer_t expr;
                conditional_expression(expr, false);
                node::pointer_t exclude(f_lexer->get_new_node(node_t::NODE_EXCLUDE));
                exclude->append_child(expr);
                n->append_child(exclude);
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_IMPORT, f_lexer->get_position());
                msg << "namespace, include or exclude was expected after the comma.";
            }
        }
        else if(f_node->get_type() == node_t::NODE_COMMA)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_IMPORT, f_lexer->get_position());
            msg << "two commas in a row is not allowed while describing an import.";
        }
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER NAMESPACE  ***********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::use_namespace(node::pointer_t & n)
{
    node::pointer_t expr;
    expression(expr);
    n = f_lexer->get_new_node(node_t::NODE_USE /*namespace*/);
    n->append_child(expr);
}



void parser::namespace_block(node::pointer_t & n, node::pointer_t& attr_list)
{
    n = f_lexer->get_new_node(node_t::NODE_NAMESPACE);

    if(f_node->get_type() == node_t::NODE_IDENTIFIER)
    {
        // save the name of the namespace
        n->set_string(f_node->get_string());
        get_token();
    }
    else
    {
        bool has_private(false);
        if(!attr_list)
        {
            attr_list = f_lexer->get_new_node(node_t::NODE_ATTRIBUTES);
        }
        else
        {
            size_t const max_attrs(attr_list->get_children_size());
            for(size_t idx(0); idx < max_attrs; ++idx)
            {
                if(attr_list->get_child(idx)->get_type() == node_t::NODE_PRIVATE)
                {
                    // already set, so we do not need to add another private attribute
                    has_private = true;
                    break;
                }
            }
        }
        if(!has_private)
        {
            node::pointer_t private_node(f_lexer->get_new_node(node_t::NODE_PRIVATE));
            attr_list->append_child(private_node);
        }
    }

    if(f_node->get_type() != node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NAMESPACE, f_lexer->get_position());
        msg << "'{' missing after the name of this namespace.";
        // TODO: write code to search for the next ';'?
    }
    else
    {
        node::pointer_t directives;
        directive_list(directives);
        n->append_child(directives);
    }
}



} // namespace as2js
// vim: ts=4 sw=4 et
