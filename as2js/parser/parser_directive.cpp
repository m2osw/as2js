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
#include    "as2js/exception.h"


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
/***  PARSER DIRECTIVES  **********************************************/
/**********************************************************************/
/**********************************************************************/

void parser::attributes(node::pointer_t & attr)
{
    // Attributes are read first.
    //
    // Depending on what follows the first set of attributes
    // we can determine what we've got (expression, statement,
    // etc.)
    //
    // There may not be any attribute and the last IDENTIFIER may
    // not be an attribute but a function name or such...
    //
    for(;;)
    {
        switch(f_node->get_type())
        {
        case node_t::NODE_ABSTRACT:
        case node_t::NODE_EXTERN:
        case node_t::NODE_FALSE:
        case node_t::NODE_FINAL:
        case node_t::NODE_IDENTIFIER:
        case node_t::NODE_NATIVE:
        case node_t::NODE_PRIVATE:
        case node_t::NODE_PROTECTED:
        case node_t::NODE_PUBLIC:
        case node_t::NODE_STATIC:
        case node_t::NODE_TRANSIENT:
        case node_t::NODE_TRUE:
        case node_t::NODE_VOLATILE:
            // TODO: Check that we don't find the same type twice...
            //       We may also want to enforce an order in some cases?
            break;

        default:
            return;

        }

        if(attr == nullptr)
        {
            attr = f_lexer->get_new_node(node_t::NODE_ATTRIBUTES);
        }

        // at this point attributes are kept as nodes, the directive()
        // function saves them as a link in the ATTRIBUTES node, later
        // the compiler transforms them in actual NODE_ATTR_... flags
        //
        attr->append_child(f_node);
        get_token();
    }
}




void parser::directive_list(node::pointer_t & list)
{
    if(list != nullptr)
    {
        // should not happen, if it does, we have got a really bad internal error
        //
        throw internal_error("directive_list() called with a non-null node pointer."); // LCOV_EXCL_LINE
    }

    list = f_lexer->get_new_node(node_t::NODE_DIRECTIVE_LIST);
    for(;;)
    {
        // skip empty statements immediately
        //
        while(f_node->get_type() == node_t::NODE_SEMICOLON)
        {
            get_token();
        }

        switch(f_node->get_type())
        {
        case node_t::NODE_EOF:
        case node_t::NODE_ELSE:
        case node_t::NODE_CLOSE_CURVLY_BRACKET:
            // these end the list of directives
            //
            return;

        default:
            directive(list);
            break;

        }
    }
    snapdev::NOT_REACHED();
}


void parser::directive(node::pointer_t & d)
{
    // we expect node to be a list of directives already
    // when defined (see directive_list())
    //
    if(d == nullptr)
    {
        d = f_lexer->get_new_node(node_t::NODE_DIRECTIVE_LIST);
    }

    // read attributes (identifiers, public/private, true/false)
    // if we find attributes and the directive accepts them,
    // then they are added to the directive as the last entry
    //
    node::pointer_t attr_list;
    attributes(attr_list);
    std::size_t attr_count(attr_list != nullptr ? attr_list->get_children_size() : 0);
    node::pointer_t instruction_node(f_node);
    node_t type(f_node->get_type());
    node::pointer_t last_attr;

    // depending on the following token, we may want to restore
    // the last "attribute" (if it is an identifier)
    //
    switch(type)
    {
    case node_t::NODE_COLON:
        if(attr_count == 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_position());
            msg << "unexpected \":\" without an identifier.";
            // skip the spurious colon and return
            get_token();
            return;
        }
        last_attr = attr_list->get_child(attr_count - 1);
        if(last_attr->get_type() != node_t::NODE_IDENTIFIER)
        {
            // special cases of labels in classes
            if(last_attr->get_type() != node_t::NODE_PRIVATE
            && last_attr->get_type() != node_t::NODE_PROTECTED
            && last_attr->get_type() != node_t::NODE_PUBLIC)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_position());
                msg << "unexpected \":\" without a valid label.";
                // skip the spurious colon and return
                get_token();
                return;
            }
            last_attr->to_identifier();
        }
#if __cplusplus >= 201700
        [[fallthrough]];
#endif
    case node_t::NODE_ADD:
    case node_t::NODE_AS:
    case node_t::NODE_ASSIGNMENT:
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
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_COMMA:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_IN:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_IS:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MEMBER:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_POWER:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REST:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_SUBTRACT:
        if(attr_count > 0)
        {
            --attr_count;
            last_attr = attr_list->get_child(attr_count);
            unget_token(f_node);
            f_node = last_attr;
            attr_list->delete_child(attr_count);
            if(type != node_t::NODE_COLON)
            {
                type = last_attr->get_type();
            }
        }
        break;

    default:
        // do nothing here
        break;

    }

    // we have a special case where a USE can be
    // followed by NAMESPACE vs. an identifier.
    // (i.e. use a namespace or define a pragma)
    //
    if(type == node_t::NODE_USE)
    {
        get_token();
        // Note that we do not change the variable 'type' here!
    }

    // check for directives which cannot have attributes
    //
    if(attr_count > 0)
    {
        switch(type)
        {
        case node_t::NODE_IDENTIFIER:
            {
                // "final identifier [= expression]" is legal but needs
                // to be transformed here to work properly
                //
                node::pointer_t child(attr_list->get_child(0));
                if(attr_count == 1 && child->get_type() == node_t::NODE_FINAL)
                {
                    attr_list.reset();
                    type = node_t::NODE_FINAL;
                }
                else
                {
                    attr_count = 0;
                }
            }
            break;

        case node_t::NODE_USE:
            // pragma cannot be annotated
            //
            if(f_node->get_type() != node_t::NODE_NAMESPACE)
            {
                attr_count = 0;
            }
            break;

        case node_t::NODE_ADD:
        case node_t::NODE_ARRAY_LITERAL:
        case node_t::NODE_BITWISE_NOT:
        case node_t::NODE_BREAK:
        case node_t::NODE_CONTINUE:
        case node_t::NODE_CASE:
        case node_t::NODE_CATCH:
        case node_t::NODE_COLON:
        case node_t::NODE_DECREMENT:
        case node_t::NODE_DEFAULT:
        case node_t::NODE_DELETE:
        case node_t::NODE_DO:
        case node_t::NODE_FALSE:
        case node_t::NODE_FLOATING_POINT:
        case node_t::NODE_FOR:
        case node_t::NODE_FINALLY:
        case node_t::NODE_GOTO:
        case node_t::NODE_IF:
        case node_t::NODE_INCREMENT:
        case node_t::NODE_INTEGER:
        case node_t::NODE_LOGICAL_NOT:
        case node_t::NODE_NEW:
        case node_t::NODE_NULL:
        case node_t::NODE_OBJECT_LITERAL:
        case node_t::NODE_OPEN_PARENTHESIS:
        case node_t::NODE_OPEN_SQUARE_BRACKET:
        case node_t::NODE_REGULAR_EXPRESSION:
        case node_t::NODE_RETURN:
        case node_t::NODE_SEMICOLON: // annotated empty statements are not allowed
        case node_t::NODE_SMART_MATCH: // TBD?
        case node_t::NODE_STRING:
        case node_t::NODE_SUBTRACT:
        case node_t::NODE_SUPER:    // will accept commas too even in expressions
        case node_t::NODE_SWITCH:
        case node_t::NODE_THIS:
        case node_t::NODE_THROW:
        case node_t::NODE_TRUE:
        case node_t::NODE_TRY:
        case node_t::NODE_TYPEOF:
        case node_t::NODE_UNDEFINED:
        case node_t::NODE_VIDENTIFIER:
        case node_t::NODE_VOID:
        case node_t::NODE_WITH:
        case node_t::NODE_WHILE:
            attr_count = 0;
            break;

        // everything else can be annotated
        default:
            break;

        }
        if(attr_count == 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_position());
            msg << "no attributes were expected here (statements, expressions and pragmas cannot be annotated).";
            attr_list.reset();
        }
        if(attr_list == nullptr)
        {
            attr_count = 0;
        }

        // make sure each attribute is unique (i.e. final final final is not acceptable)
        //
        std::size_t count(attr_list == nullptr ? 0 : attr_list->get_children_size());
        if(count > 1)
        {
            for(std::size_t i(0); i < count; ++i)
            {
                for(std::size_t j(i + 1); j < count; ++j)
                {
                    if(attr_list->get_child(i)->get_type() == attr_list->get_child(j)->get_type()
                    && (attr_list->get_child(i)->get_type() != node_t::NODE_IDENTIFIER
                        || attr_list->get_child(i)->get_string() == attr_list->get_child(j)->get_string()))
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, f_lexer->get_position());
                        if(attr_list->get_child(i)->get_type() == node_t::NODE_IDENTIFIER)
                        {
                            msg << "attribute \""
                                << attr_list->get_child(i)->get_string()
                                << "\" found twice.";
                        }
                        else
                        {
                            // TODO: make type name lowercase
                            msg << "attribute \""
                                << attr_list->get_child(i)->get_type_name()
                                << "\" found twice.";
                        }
                    }
                }
            }
        }
    }

    // The directive node, if created by a sub-function, will
    // be added to the list of directives.
    node::pointer_t directive_node;
    switch(type)
    {
    // *** PRAGMA ***
    case node_t::NODE_USE:
        // we already did a get_token() to skip the NODE_USE
        //
        if(f_node->get_type() == node_t::NODE_NAMESPACE)
        {
            // use namespace ... ';'
            //
            get_token();
            use_namespace(directive_node);
            break;
        }
        if(f_node->get_type() == node_t::NODE_IDENTIFIER)
        {
            node::pointer_t name(f_node);
            get_token();
            if(f_node->get_type() == node_t::NODE_AS)
            {
                // creating a numeric type
                //
                numeric_type(directive_node, name);
                break;
            }

            // not a numeric type, must be a pragma
            //
            unget_token(f_node);
            f_node = name;
        }
        // TODO? Pragmas are not part of the tree
        //
        // Note: pragmas affect the Options and are
        //       not currently added to the final
        //       tree of nodes. [is that correct?! it
        //       should be fine as long as we do not
        //       have run-time pragmas]
        //
        pragma();
        break;

    // *** PACKAGE ***
    case node_t::NODE_PACKAGE:      // TODO: I think this can appear anywhere which is wrong, it can only be at the top (a package within a package is not supported)
        get_token();
        package(directive_node);
        break;

    case node_t::NODE_IMPORT:
        get_token();
        import(directive_node);
        break;

    // *** CLASS DEFINITION ***
    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
        get_token();
        class_declaration(directive_node, type);
        break;

    case node_t::NODE_ENUM:
        get_token();
        enum_declaration(directive_node);
        break;

    case node_t::NODE_INVARIANT:
        get_token();
        contract_declaration(directive_node, type);
        break;

    // *** FUNCTION DEFINITION ***
    case node_t::NODE_FUNCTION:
        get_token();
        function(directive_node, false);
        break;

    // *** VARIABLE DEFINITION ***
    case node_t::NODE_CONST:
        get_token();
        if(f_node->get_type() == node_t::NODE_VAR)
        {
            get_token();
        }
        variable(directive_node, node_t::NODE_CONST);
        break;

    case node_t::NODE_FINAL:
        // this special case happens when the user forgets to put
        // a variable name (final = 5) or the var keyword is not
        // used; the variable() function generates the correct
        // error and skips the entry as required if necessary
        if(f_node->get_type() == node_t::NODE_FINAL)
        {
            // skip the FINAL keyword
            // otherwise we are already on the IDENTIFIER keyword
            get_token();
        }
        variable(directive_node, node_t::NODE_FINAL);
        break;

    case node_t::NODE_VAR:
        {
            get_token();

            // in this case the VAR keyword may be preceeded by
            // the FINAL keyword which this far is viewed as an
            // attribute; so make it a keyword again
            //
            bool found(false);
            for(size_t idx(0); idx < attr_count; ++idx)
            {
                node::pointer_t child(attr_list->get_child(idx));
                if(child->get_type() == node_t::NODE_FINAL)
                {
                    // got it, remove it from the list
                    //
                    found = true;
                    attr_list->delete_child(idx);
                    --attr_count;
                    break;
                }
            }
            if(found)
            {
                variable(directive_node, node_t::NODE_FINAL);
            }
            else
            {
                variable(directive_node, node_t::NODE_VAR);
            }
        }
        break;

    // *** STATEMENT ***
    case node_t::NODE_OPEN_CURVLY_BRACKET:
        get_token();
        block(directive_node);
        break;

    case node_t::NODE_SEMICOLON:
        // empty statements are just skipped
        //
        // NOTE: we reach here only when we find attributes
        //       which are not identifiers and this means
        //       we will have gotten an error.
        get_token();
        break;

    case node_t::NODE_BREAK:
    case node_t::NODE_CONTINUE:
        get_token();
        break_continue(directive_node, type);
        break;

    case node_t::NODE_CASE:
        get_token();
        case_directive(directive_node);
        break;

    case node_t::NODE_CATCH:
        get_token();
        catch_directive(directive_node);
        break;

    case node_t::NODE_DEBUGGER:    // just not handled yet...
        get_token();
        debugger(directive_node);
        break;

    case node_t::NODE_DEFAULT:
        get_token();
        default_directive(directive_node);
        break;

    case node_t::NODE_DO:
        get_token();
        do_directive(directive_node);
        break;

    case node_t::NODE_FOR:
        get_token();
        for_directive(directive_node);
        break;

    case node_t::NODE_FINALLY:
    case node_t::NODE_TRY:
        get_token();
        try_finally(directive_node, type);
        break;

    case node_t::NODE_GOTO:
        get_token();
        goto_directive(directive_node);
        break;

    case node_t::NODE_IF:
        get_token();
        if_directive(directive_node);
        break;

    case node_t::NODE_NAMESPACE:
        get_token();
        namespace_block(directive_node, attr_list);
        break;

    case node_t::NODE_RETURN:
        get_token();
        return_directive(directive_node);
        break;

    case node_t::NODE_SWITCH:
        get_token();
        switch_directive(directive_node);
        break;

    case node_t::NODE_SYNCHRONIZED:
        get_token();
        synchronized(directive_node);
        break;

    case node_t::NODE_THROW:
        get_token();
        throw_directive(directive_node);
        break;

    case node_t::NODE_WITH:
    case node_t::NODE_WHILE:
        get_token();
        with_while(directive_node, type);
        break;

    case node_t::NODE_YIELD:
        get_token();
        yield(directive_node);
        break;

    case node_t::NODE_COLON:
        // the label was the last identifier in the
        // attributes which is now in f_node
        f_node->to_label();
        directive_node = f_node;
        // we skip the identifier here
        get_token();
        // and then the ':'
        get_token();
        break;

    // *** EXPRESSION ***
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_FALSE:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INTEGER:
    case node_t::NODE_NEW:
    case node_t::NODE_NULL:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_STRING:
    case node_t::NODE_SUPER:    // will accept commas too even in expressions
    case node_t::NODE_TEMPLATE:
    case node_t::NODE_TEMPLATE_HEAD:
    case node_t::NODE_THIS:
    case node_t::NODE_TRUE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_ADD:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_SMART_MATCH: // if here, need to be broken up to ~ and ~
    case node_t::NODE_NOT_MATCH: // if here, need to be broken up to ! and ~
        expression(directive_node);
        break;

    // *** TERMINATOR ***
    case node_t::NODE_EOF:
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_EOF, f_lexer->get_position());
        msg << "unexpected end of file reached.";
    }
        return;

    case node_t::NODE_CLOSE_CURVLY_BRACKET:
        // this error does not seem required at this point
        // we get the error from the program already
    //{
    //    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
    //    msg << "unexpected \"}\".";
    //}
        return;

    // *** INVALID ***
    // The following are for sure invalid tokens in this
    // context. If it looks like some of these could be
    // valid when this function returns, just comment
    // out the corresponding case.
    case node_t::NODE_ALMOST_EQUAL:
    case node_t::NODE_ARROW:
    case node_t::NODE_AS:
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_ASSIGNMENT_ADD:
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node_t::NODE_ASSIGNMENT_COALESCE:
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
    case node_t::NODE_ASYNC:
    case node_t::NODE_AWAIT:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COALESCE:
    case node_t::NODE_COMMA:
    case node_t::NODE_COMPARE:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_IN:
    case node_t::NODE_IS:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MEMBER:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_OPTIONAL_MEMBER:
    case node_t::NODE_POWER:
    case node_t::NODE_RANDOM:
    case node_t::NODE_RANGE:
    case node_t::NODE_REST:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_VARIABLE:
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_position());
        msg << "unexpected operator \"" << instruction_node->get_type_name() << "\".";
        get_token();
    }
        break;

    case node_t::NODE_ELSE:
    case node_t::NODE_ENSURE:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_REQUIRE:
    case node_t::NODE_THEN:
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_KEYWORD, f_lexer->get_position());
        msg << "unexpected keyword \"" << instruction_node->get_type_name() << "\".";
        get_token();
    }
        break;

    case node_t::NODE_ABSTRACT:
    case node_t::NODE_EXTERN:
    //case node_t::NODE_FALSE:
    case node_t::NODE_INLINE:
    case node_t::NODE_NATIVE:
    //case node_t::NODE_PRIVATE:
    //case node_t::NODE_PROTECTED:
    //case node_t::NODE_PUBLIC:
    case node_t::NODE_STATIC:
    case node_t::NODE_TRANSIENT:
    //case node_t::NODE_TRUE:
    case node_t::NODE_VOLATILE:
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_position());
        msg << "a statement with only attributes ("
            << node::type_to_string(type)
            << ") is not allowed.";
        attr_list.reset();
        attr_count = 0;

        // skip that attribute which we cannot do anything with
        get_token();
    }
        break;

    // *** NOT POSSIBLE ***
    // These should never happen since they should be caught
    // before this switch is reached or it can't be a node
    // read by the lexer.
    case node_t::NODE_ABSOLUTE_VALUE:
    case node_t::NODE_ACOS:
    case node_t::NODE_ACOSH:
    case node_t::NODE_ARRAY:
    case node_t::NODE_ASIN:
    case node_t::NODE_ASINH:
    case node_t::NODE_ATAN:
    case node_t::NODE_ATAN2:
    case node_t::NODE_ATANH:
    case node_t::NODE_ATTRIBUTES:
    case node_t::NODE_AUTO:
    case node_t::NODE_BOOLEAN:
    case node_t::NODE_BYTE:
    case node_t::NODE_CALL:
    case node_t::NODE_CBRT:
    case node_t::NODE_CEIL:
    case node_t::NODE_CHAR:
    case node_t::NODE_COS:
    case node_t::NODE_COSH:
    case node_t::NODE_CLZ32:
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_DOUBLE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXP:
    case node_t::NODE_EXPM1:
    case node_t::NODE_EXPORT:
    case node_t::NODE_FLOAT:
    case node_t::NODE_FLOOR:
    case node_t::NODE_FROUND:
    case node_t::NODE_HYPOT:
    case node_t::NODE_IDENTITY:
    case node_t::NODE_IF_FALSE:
    case node_t::NODE_IF_TRUE:
    case node_t::NODE_IMUL:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_LABEL:
    case node_t::NODE_LIST:
    case node_t::NODE_LOG:
    case node_t::NODE_LOG1P:
    case node_t::NODE_LOG10:
    case node_t::NODE_LOG2:
    case node_t::NODE_LONG:
    case node_t::NODE_NAME:
    case node_t::NODE_NEGATE:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_ROOT:
    case node_t::NODE_ROUND:
    case node_t::NODE_SET:
    case node_t::NODE_SHORT:
    case node_t::NODE_SIGN:
    case node_t::NODE_SIN:
    case node_t::NODE_SINH:
    case node_t::NODE_SQRT:
    case node_t::NODE_TAN:
    case node_t::NODE_TANH:
    case node_t::NODE_TEMPLATE_MIDDLE:
    case node_t::NODE_TEMPLATE_TAIL:
    case node_t::NODE_THROWS:
    case node_t::NODE_TRUNC:
    case node_t::NODE_TYPE:
    case node_t::NODE_UNKNOWN:    // ?!
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_other:      // no node should be of this type
    case node_t::NODE_max:        // no node should be of this type
        // LCOV_EXCL_START
        {
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, f_lexer->get_position());
            msg << "INTERNAL ERROR: invalid node (" << node::type_to_string(type) << ") in directive_list.";
            throw internal_error(msg.str());
        }
        // LCOV_EXCL_STOP

    }
    if(directive_node != nullptr)
    {
        // if there are attributes link them to the directive
        //
        if(attr_list != nullptr
        && attr_list->get_children_size() > 0)
        {
            directive_node->set_attribute_node(attr_list);
        }
        d->append_child(directive_node);
    }

    // now make sure we have a semicolon for
    // those statements which have to have it.
    //
    switch(type)
    {
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_BREAK:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DO:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_GOTO:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_IMPORT:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INTEGER:
    case node_t::NODE_NEW:
    case node_t::NODE_NULL:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_RETURN:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_STRING:
    case node_t::NODE_SUPER:
    case node_t::NODE_THIS:
    case node_t::NODE_THROW:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_USE:
    case node_t::NODE_VAR:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_ADD:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_BITWISE_NOT:
        // accept missing ';' when we find a '}' next
        //
        if(f_node->get_type() != node_t::NODE_SEMICOLON
        && f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_position());
            msg << "\";\" was expected after \""
                << instruction_node->get_type_name()
                << "\" (current token: \""
                << f_node->get_type_name()
                << "\").";
        }

        // skip all that whatever up to the next end of this
        //
        while(f_node->get_type() != node_t::NODE_SEMICOLON
           && f_node->get_type() != node_t::NODE_OPEN_CURVLY_BRACKET
           && f_node->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET
           && f_node->get_type() != node_t::NODE_ELSE
           && f_node->get_type() != node_t::NODE_EOF)
        {
            get_token();
        }
        // we need to skip one semi-colon here
        // in case we are not in a directive_list()
        //
        if(f_node->get_type() == node_t::NODE_SEMICOLON)
        {
            get_token();
        }
        break;

    default:
        break;

    }
}





}
// namespace as2js
// vim: ts=4 sw=4 et
