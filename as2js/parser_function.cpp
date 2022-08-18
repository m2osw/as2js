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
/***  PARSER FUNCTION  ************************************************/
/**********************************************************************/
/**********************************************************************/

void parser::parameter_list(node::pointer_t& node, bool& has_out)
{
    // accept function stuff(void) { ... } as in C/C++
    // Note that we also accept Void (void is a keyword, Void is a type)
    if(f_node->get_type() == node::node_t::NODE_VOID
    || (f_node->get_type() == node::node_t::NODE_IDENTIFIER && f_node->get_string() == "Void"))
    {
        get_token();
        return;
    }

    node = f_lexer->get_new_node(node::node_t::NODE_PARAMETERS);

    // special case which explicitly says that a function definition
    // is not prototyped (vs. an empty list of parameters which is
    // equivalent to a (void)); this means the function accepts
    // parameters, their type & number are just not defined
    if(f_node->get_type() == node::node_t::NODE_IDENTIFIER
    && f_node->get_string() == "unprototyped")
    {
        node::pointer_t param(f_lexer->get_new_node(node::node_t::NODE_PARAM));
        param->set_flag(node::flag_t::NODE_PARAM_FLAG_UNPROTOTYPED, true);
        node->append_child(param);
        get_token();
        return;
    }

    bool invalid(false);
    for(;;)
    {
        node::pointer_t param(f_lexer->get_new_node(node::node_t::NODE_PARAM));

        // get all the attributes for the parameters
        // (var, const, in, out, named, unchecked, ...)
        bool more(true);
        bool param_has_out(false);
        do
        {
            // TODO: it seems that any one flag should only be accepted
            //       once, 'var' first, and '...' last.
            switch(f_node->get_type())
            {
            case node::node_t::NODE_REST:
                param->set_flag(node::flag_t::NODE_PARAM_FLAG_REST, true);
                invalid = false;
                get_token();
                break;

            case node::node_t::NODE_CONST:
                param->set_flag(node::flag_t::NODE_PARAM_FLAG_CONST, true);
                invalid = false;
                get_token();
                break;

            case node::node_t::NODE_IN:
                param->set_flag(node::flag_t::NODE_PARAM_FLAG_IN, true);
                invalid = false;
                get_token();
                break;

            case node::node_t::NODE_VAR:
                // TBD: should this be forced first?
                invalid = false;
                get_token();
                break;

            case node::node_t::NODE_IDENTIFIER:
                if(f_node->get_string() == "out")
                {
                    param->set_flag(node::flag_t::NODE_PARAM_FLAG_OUT, true);
                    invalid = false;
                    get_token();
                    has_out = true; // for caller to know
                    param_has_out = true;
                    break;
                }
                if(f_node->get_string() == "named")
                {
                    param->set_flag(node::flag_t::NODE_PARAM_FLAG_NAMED, true);
                    invalid = false;
                    get_token();
                    break;
                }
                if(f_node->get_string() == "unchecked")
                {
                    param->set_flag(node::flag_t::NODE_PARAM_FLAG_UNCHECKED, true);
                    invalid = false;
                    get_token();
                    break;
                }
#if __cplusplus >= 201700
                [[fallthrough]];
#endif
            default:
                more = false;
                break;

            }
        }
        while(more); 

        if(param_has_out)
        {
            if(param->get_flag(node::flag_t::NODE_PARAM_FLAG_REST))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_position());
                msg << "you cannot use the function parameter attribute 'out' with '...'.";
            }
            if(param->get_flag(node::flag_t::NODE_PARAM_FLAG_CONST))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_position());
                msg << "you cannot use the function attributes 'out' and 'const' together.";
            }
        }

        if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
        {
            param->set_string(f_node->get_string());
            node->append_child(param);
            invalid = false;
            get_token();
            if(f_node->get_type() == node::node_t::NODE_COLON)
            {
                // TBD: what about REST? does this mean all
                //      the following parameters need to be
                //      of that type?
                get_token();
                node::pointer_t expr;
                conditional_expression(expr, false);
                node::pointer_t type(f_lexer->get_new_node(node::node_t::NODE_TYPE));
                type->append_child(expr);
                param->append_child(type);
            }
            if(f_node->get_type() == node::node_t::NODE_ASSIGNMENT)
            {
                // cannot accept when REST is set
                if(param->get_flag(node::flag_t::NODE_PARAM_FLAG_REST))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_position());
                    msg << "you cannot assign a default value to '...'.";
                    // we still parse the initializer so we get to the right
                    // place; but since we had an error anyway, the compiler
                    // won't kick in so we are fine
                }

                // initializer
                get_token();
                node::pointer_t initializer(f_lexer->get_new_node(node::node_t::NODE_SET));
                node::pointer_t expr;
                conditional_expression(expr, false);
                initializer->append_child(expr);
                param->append_child(initializer);
            }
        }
        else if(param->get_flag(node::flag_t::NODE_PARAM_FLAG_REST))
        {
            node->append_child(param);
        }

        // reached the end of the list?
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS
        || f_node->get_type() == node::node_t::NODE_IF) // special case for catch(e if e instanceof RangeError) ...
        {
            return;
        }

        if(f_node->get_type() != node::node_t::NODE_COMMA)
        {
            if(!invalid)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_position());
                msg << "expected ')' or ',' after a parameter declaration (not token " << f_node->get_type_name() << ").";
            }
            switch(f_node->get_type())
            {
            case node::node_t::NODE_EOF:
            case node::node_t::NODE_SEMICOLON:
            case node::node_t::NODE_OPEN_CURVLY_BRACKET:
            case node::node_t::NODE_CLOSE_CURVLY_BRACKET:
                // we are probably past the end of the list
                return;

            default:
                // continue, just ignore that token
                break;

            }
            if(invalid)
            {
                get_token();
            }
            invalid = true;
        }
        else
        {
            if(param->get_flag(node::flag_t::NODE_PARAM_FLAG_REST))
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_position());
                msg << "no other parameters expected after '...'.";
            }
            get_token();
        }
    }
}



void parser::function(node::pointer_t & n, bool const expression_function)
{
    n = f_lexer->get_new_node(node::node_t::NODE_FUNCTION);

    switch(f_node->get_type())
    {
    case node::node_t::NODE_IDENTIFIER:
    {
        std::string etter;
        if(f_node->get_string() == "get")
        {
            // *** GETTER ***
            n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_GETTER, true);
            etter = "->";
        }
        else if(f_node->get_string() == "set")
        {
            // *** SETTER ***
            n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_SETTER, true);
            etter = "<-";
        }
        if(!etter.empty())
        {
            // *** one of GETTER/SETTER ***
            get_token();
            if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
            {
                n->set_string(std::string(etter + f_node->get_string()));
                get_token();
            }
            else if(f_node->get_type() == node::node_t::NODE_STRING)
            {
                // this is an extension, you can't have
                // a getter or setter which is also an
                // operator overload though...
                n->set_string(etter + f_node->get_string());
                if(node::string_to_operator(f_node->get_string()) != node::node_t::NODE_UNKNOWN)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_position());
                    msg << "operator override cannot be marked as a getter nor a setter function.";
                }
                get_token();
            }
            else if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
            {
                // not a getter or setter when only get() or set()
                if(n->get_flag(node::flag_t::NODE_FUNCTION_FLAG_GETTER))
                {
                    n->set_string("get");
                }
                else
                {
                    n->set_string("set");
                }
                n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_GETTER, false);
                n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_SETTER, false);
                etter = "";
            }
            else if(!expression_function)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_position());
                msg << "getter and setter functions require a name.";
            }
            if(expression_function && !etter.empty())
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_position());
                msg << "expression functions cannot be getter nor setter functions.";
            }
        }
        else
        {
            // *** STANDARD ***
            n->set_string(f_node->get_string());
            get_token();
            if(f_node->get_type() == node::node_t::NODE_IDENTIFIER)
            {
                // Ooops? this could be that the user misspelled get or set
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_position());
                msg << "only one name is expected for a function (misspelled get or set? missing '(' before a parameter?)";
                get_token(); // <- TBD: is that really a good idea?
            }
        }
    }
        break;

    case node::node_t::NODE_STRING:
    {
        // *** OPERATOR OVERLOAD ***
        // (though we just accept any string at this time)
        n->set_string(f_node->get_string());
        if(node::string_to_operator(n->get_string()) != node::node_t::NODE_UNKNOWN)
        {
            n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        }
        get_token();
    }
        break;

    // all the operators which can be overloaded as is
    case node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case node::node_t::NODE_ASSIGNMENT_POWER:
    case node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node::node_t::NODE_COMPARE:
    case node::node_t::NODE_LOGICAL_XOR:
    case node::node_t::NODE_MATCH:
    case node::node_t::NODE_MAXIMUM:
    case node::node_t::NODE_MINIMUM:
    case node::node_t::NODE_NOT_MATCH:
    case node::node_t::NODE_POWER:
    case node::node_t::NODE_ROTATE_LEFT:
    case node::node_t::NODE_ROTATE_RIGHT:
    case node::node_t::NODE_SMART_MATCH:
        if(!has_option_set(options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
#if __cplusplus >= 201700
        [[fallthrough]];
#endif
    case node::node_t::NODE_ADD:
    case node::node_t::NODE_ASSIGNMENT:
    case node::node_t::NODE_ASSIGNMENT_ADD:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node::node_t::NODE_ASSIGNMENT_MODULO:
    case node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node::node_t::NODE_ASSIGNMENT_SUBTRACT:
    case node::node_t::NODE_BITWISE_AND:
    case node::node_t::NODE_BITWISE_XOR:
    case node::node_t::NODE_BITWISE_OR:
    case node::node_t::NODE_BITWISE_NOT:
    case node::node_t::NODE_DECREMENT:
    case node::node_t::NODE_DIVIDE:
    case node::node_t::NODE_EQUAL:
    case node::node_t::NODE_GREATER:
    case node::node_t::NODE_GREATER_EQUAL:
    case node::node_t::NODE_INCREMENT:
    case node::node_t::NODE_LESS:
    case node::node_t::NODE_LESS_EQUAL:
    case node::node_t::NODE_LOGICAL_AND:
    case node::node_t::NODE_LOGICAL_NOT:
    case node::node_t::NODE_LOGICAL_OR:
    case node::node_t::NODE_MODULO:
    case node::node_t::NODE_MULTIPLY:
    case node::node_t::NODE_NOT_EQUAL:
    case node::node_t::NODE_POST_DECREMENT:
    case node::node_t::NODE_POST_INCREMENT:
    case node::node_t::NODE_SHIFT_LEFT:
    case node::node_t::NODE_SHIFT_RIGHT:
    case node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node::node_t::NODE_STRICTLY_EQUAL:
    case node::node_t::NODE_STRICTLY_NOT_EQUAL:
    case node::node_t::NODE_SUBTRACT:
    {
        // save the operator type in the node to be able
        // to get the string
        n->set_string(node::operator_to_string(f_node->get_type()));
        n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        get_token();
    }
        break;

    // this is a complicated one because () can
    // be used as the "()" operator or for the parameters
    case node::node_t::NODE_OPEN_PARENTHESIS:
    {
        node::pointer_t restore(f_node);
        get_token();
        if(f_node->get_type() == node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            node::pointer_t save(f_node);
            get_token();
            if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
            {
                // at this point...
                // this is taken as the "()" operator!
                n->set_string("()");
                n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
                break;
            }
            else
            {
                unget_token(f_node);
                unget_token(save);
                f_node = restore;
            }
        }
        else
        {
            unget_token(f_node);
            f_node = restore;
        }
    }
#if __cplusplus >= 201700
        [[fallthrough]];
#endif
    default:
        if(!expression_function)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_position());
            msg << "function declarations are required to be named.";
        }
        break;

    }

    if(f_node->get_type() == node::node_t::NODE_OPEN_PARENTHESIS)
    {
        get_token();
        if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            // read params
            node::pointer_t params;
            bool has_out(false);
            parameter_list(params, has_out);
            if(has_out)
            {
                n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_OUT, true);
            }
            if(params)
            {
                n->append_child(params);
            }
            else
            {
                n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_NOPARAMS, true);
            }
            if(f_node->get_type() != node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_position());
                msg << "')' expected to close the list of parameters of this function.";
            }
            else
            {
                get_token();
            }
        }
        else
        {
            get_token();
        }
    }

    // return type specified?
    if(f_node->get_type() == node::node_t::NODE_COLON)
    {
        get_token();
        if(f_node->get_type() == node::node_t::NODE_VOID
        || (f_node->get_type() == node::node_t::NODE_IDENTIFIER && f_node->get_string() == "Void"))
        {
            // special case of a procedure instead of a function
            n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_VOID, true);
            get_token();
        }
        else if(f_node->get_type() == node::node_t::NODE_IDENTIFIER && f_node->get_string() == "Never")
        {
            // function is not expected to return
            n->set_flag(node::flag_t::NODE_FUNCTION_FLAG_NEVER, true);
            get_token();
        }
        else
        {
            // normal type definition
            node::pointer_t expr;
            conditional_expression(expr, false);
            node::pointer_t type(f_lexer->get_new_node(node::node_t::NODE_TYPE));
            type->append_child(expr);
            n->append_child(type);
        }
    }

    // throws exceptions?
    if(f_node->get_type() == node::node_t::NODE_THROWS)
    {
        // skip the THROWS keyword
        get_token();
        node::pointer_t throws(f_lexer->get_new_node(node::node_t::NODE_THROWS));
        n->append_child(throws);

        // exceptions are types
        for(;;)
        {
            node::pointer_t expr;
            conditional_expression(expr, false);
            throws->append_child(expr);
            if(f_node->get_type() != node::node_t::NODE_COMMA)
            {
                break;
            }
            // skip the comma
            get_token();
        }
    }

    // any requirement?
    if(f_node->get_type() == node::node_t::NODE_REQUIRE)
    {
        // skip the REQUIRE keyword
        get_token();
        bool const has_else(f_node->get_type() == node::node_t::NODE_ELSE);
        if(has_else)
        {
            // require else ... is an "or" (i.e. parent function require
            // may be negative, then this require comes to the rescue)
            // without the else, it is not valid to redeclare a require
            //
            // skip the ELSE keyword
            get_token();
        }
        node::pointer_t require;
        contract_declaration(require, node::node_t::NODE_REQUIRE);
        if(has_else)
        {
            require->set_attribute(node::attribute_t::NODE_ATTR_REQUIRE_ELSE, true);
        }
        n->append_child(require);
    }

    // any insurance?
    if(f_node->get_type() == node::node_t::NODE_ENSURE)
    {
        // skip the ENSURE keyword
        get_token();
        bool const has_then(f_node->get_type() == node::node_t::NODE_THEN);
        if(has_then)
        {
            // ensure then ... is an "and" (i.e. it is additional to
            // the parent function ensure to be valid)
            // without the then, it is not valid to redeclare an ensure
            // skip the THEN keyword
            get_token();
        }
        node::pointer_t ensure;
        contract_declaration(ensure, node::node_t::NODE_ENSURE);
        if(has_then)
        {
            ensure->set_attribute(node::attribute_t::NODE_ATTR_ENSURE_THEN, true);
        }
        n->append_child(ensure);
    }

    if(f_node->get_type() == node::node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
        if(f_node->get_type() != node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            node::pointer_t statements;
            directive_list(statements);
            n->append_child(statements);
        }
        // else ... nothing?!
        // NOTE: by not inserting anything when we have
        //       an empty definition, it looks like an abstract
        //       definition... we may want to change that at a
        //       later time.
        if(f_node->get_type() != node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_position());
            msg << "'}' expected to close the 'function' block.";
        }
        else
        {
            get_token();
        }
    }
    // empty function (a.k.a abstract or function as a type)
    // such functions are permitted in interfaces!
}



}
// namespace as2js

// vim: ts=4 sw=4 et
