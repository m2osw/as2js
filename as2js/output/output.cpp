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
#include    "as2js/output.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{






flatten_nodes::flatten_nodes(node::pointer_t root)
    : f_root(root)
{
}


void flatten_nodes::run()
{
    node_to_operation(f_root);
}


void flatten_nodes::directive_list(node::pointer_t n)
{
    std::size_t const max(n->get_children_size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        node_to_operation(n->get_child(idx));
    }
}

data::pointer_t flatten_nodes::node_to_operation(node::pointer_t n)
{
    // TODO: variables need to be scoped; program, package, class/interface,
    //       and function are 4 different scope levels and it is important
    //       to know which one we're referring (i.e. we can have many
    //       variables with the same name, just different scopes)
    //
    std::size_t const max_variables(n->get_variable_size());
    for(std::size_t idx(0); idx < max_variables; ++idx)
    {
        node::pointer_t var(n->get_variable(idx));
        std::string const & name(var->get_string());
        if(f_variables.find(name) != f_variables.end())
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, n->get_position());
            msg << "found multiple declarations of variable \""
                << name
                << "\".";
        }
        else
        {
            f_variables[name] = std::make_shared<data>(var);
        }
    }

    switch(n->get_type())
    {
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_PACKAGE:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_ROOT:
        // go through lists recursively
        //
        directive_list(n);
        break;

    case node_t::NODE_BOOLEAN:
    case node_t::NODE_FALSE:
    case node_t::NODE_INTEGER:
    case node_t::NODE_NULL:
    case node_t::NODE_TRUE:
        // direct immediate data
        //
        return std::make_shared<data>(n);

    case node_t::NODE_FLOATING_POINT:
        // immediate floating point
        {
            auto it(std::find_if(
                  f_data.begin()
                , f_data.end()
                , [n](data::pointer_t e)
                {
                    return e->get_data_type() == node_t::NODE_FLOATING_POINT
                        && n->get_floating_point().compare(e->get_floating_point()) == compare_t::COMPARE_EQUAL;
                }));
            if(it != f_data.end())
            {
                return *it;
            }
            data::pointer_t d(std::make_shared<data>(n));
            f_data.push_back(d);
            return d;
        }
        break;

    case node_t::NODE_STRING:
        // immediate string
        {
            auto it(std::find_if(
                  f_data.begin()
                , f_data.end()
                , [n](data::pointer_t e)
                {
                    return e->get_data_type() == node_t::NODE_STRING
                        && n->get_string() == e->get_string();
                }));
            if(it != f_data.end())
            {
                return *it;
            }
            data::pointer_t d(std::make_shared<data>(n));
            f_data.push_back(d);
            return d;
        }
        break;

    case node_t::NODE_IDENTIFIER:
        // variables are added at the top of a main block (package,
        // function, class) so at this point we must have the definition
        {
            std::string const name(n->get_string());
            auto it(f_variables.find(name));
            if(it == f_variables.end())
            {
                // TBD: JavaScript does not force one to create variables
                //      before using them
                //
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, n->get_position());
                msg << "variable declaration for \""
                    << name
                    << "\" not found.";

                // auto-create a variable so we can continue
                //
                data::pointer_t var(std::make_shared<data>(n));
                f_variables[name] = var;
                return var;
            }
            return it->second;
        }
        break;

    case node_t::NODE_ADD:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
//std::cerr << "--- ADD:\n" << *n
//<< " -> ADD type: " << n->get_type_node()->get_string()
//<< "\n -> ADD -- LHS type: " << n->get_child(0)->get_type_node()->get_string()
//<< "\n -> ADD -- RHS type: " << n->get_child(1)->get_type_node()->get_string()
//<< "\n -> variable:\n" << *var
//<< "\n";
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            if(n->get_children_size() == 1)
            {
                op = std::make_shared<operation>(NODE_POSITIVE, n);
                op->set_left_handside(node_to_operation(n->get_child(0)));
            }
            else
            {
                op = std::make_shared<operation>(node_t::NODE_ADD, n);
                op->set_left_handside(node_to_operation(n->get_child(0)));
                op->set_right_handside(node_to_operation(n->get_child(1)));
            }
            op->set_result(result);
            f_operations.push_back(op);
            return result;
        }
        break;

    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_GREATER:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_LESS:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_POWER:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            op = std::make_shared<operation>(n->get_type(), n);
            op->set_left_handside(node_to_operation(n->get_child(0)));
            op->set_right_handside(node_to_operation(n->get_child(1)));
            op->set_result(result);
            f_operations.push_back(op);
            return result;
        }
        break;

    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_LOGICAL_NOT:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            op = std::make_shared<operation>(n->get_type(), n);
            op->set_left_handside(node_to_operation(n->get_child(0)));
            op->set_result(result);
            f_operations.push_back(op);
            return result;
        }
        break;

    case node_t::NODE_SUBTRACT:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            if(n->get_children_size() == 1)
            {
                op = std::make_shared<operation>(NODE_NEGATE, n);
                op->set_left_handside(node_to_operation(n->get_child(0)));
            }
            else
            {
                op = std::make_shared<operation>(node_t::NODE_SUBTRACT, n);
                op->set_left_handside(node_to_operation(n->get_child(0)));
                op->set_right_handside(node_to_operation(n->get_child(1)));
            }
            op->set_result(result);
            f_operations.push_back(op);
            return result;
        }
        break;

    case node_t::NODE_ALMOST_EQUAL:
    case node_t::NODE_ARRAY:
    case node_t::NODE_ARRAY_LITERAL:
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
    case node_t::NODE_BREAK:
    case node_t::NODE_BYTE:
    case node_t::NODE_CALL:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CHAR:
    case node_t::NODE_CLASS:
    case node_t::NODE_COALESCE:
    case node_t::NODE_COMPARE:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DO:
    case node_t::NODE_EQUAL:
    case node_t::NODE_FOR:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_GOTO:
    case node_t::NODE_IF:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_IMPORT:
    case node_t::NODE_IN:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_INLINE:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_INVARIANT:
    case node_t::NODE_IS:
    case node_t::NODE_LABEL:
    case node_t::NODE_LIST: // this is used for parameters, we need to implement it if we are to call functions
    case node_t::NODE_LONG:
    case node_t::NODE_MATCH:
    case node_t::NODE_MEMBER:
    case node_t::NODE_NAME:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_NATIVE:
    case node_t::NODE_NEW:
    case node_t::NODE_NOT_MATCH:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_OPTIONAL_MEMBER:
    case node_t::NODE_SMART_MATCH:
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, n->get_position());
            msg << "binary compilation of node type \""
                << n->get_type_name()
                << "\" is not yet implemented.";
            throw not_implemented(msg.str());
        }

    case node_t::NODE_VAR:
        // just ignore these nodes
        break;

    case node_t::NODE_ABSTRACT:
    case node_t::NODE_ARROW:
    case node_t::NODE_AS:
    case node_t::NODE_ATTRIBUTES:
    case node_t::NODE_AUTO:
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_DOUBLE:
    case node_t::NODE_EOF:
    case node_t::NODE_ELSE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_ENSURE:
    case node_t::NODE_ENUM:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXPORT:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_EXTERN:
    case node_t::NODE_FINAL:
    case node_t::NODE_FINALLY:
    case node_t::NODE_FLOAT:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REQUIRE:
    case node_t::NODE_REST:
    case node_t::NODE_RETURN:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SET:
    case node_t::NODE_SHORT:
    case node_t::NODE_STATIC:
    case node_t::NODE_SUPER:
    case node_t::NODE_SWITCH:
    case node_t::NODE_SYNCHRONIZED:
    case node_t::NODE_TEMPLATE:
    case node_t::NODE_TEMPLATE_HEAD:
    case node_t::NODE_TEMPLATE_MIDDLE:
    case node_t::NODE_TEMPLATE_TAIL:
    case node_t::NODE_THEN:
    case node_t::NODE_THIS:
    case node_t::NODE_THROW:
    case node_t::NODE_THROWS:
    case node_t::NODE_TRANSIENT:
    case node_t::NODE_TRY:
    case node_t::NODE_TYPE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_UNKNOWN:
    case node_t::NODE_USE:
    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_VOLATILE:
    case node_t::NODE_WHILE:
    case node_t::NODE_WITH:
    case node_t::NODE_YIELD:
    case node_t::NODE_other:
    case node_t::NODE_max:
        throw internal_error(
                  std::string("binary compilation found an unsupported node of type \"")
                + n->get_type_name()
                + "\"");

    }

    return data::pointer_t();
}


node::pointer_t flatten_nodes::get_root() const
{
    return f_root;
}


operation::list_t const & flatten_nodes::get_operations() const
{
    return f_operations;
}


data::list_t const & flatten_nodes::get_data() const
{
    return f_data;
}


data::map_t const & flatten_nodes::get_variables() const
{
    return f_variables;
}






data::data(node::pointer_t n)
    : f_node(n)
{
}


node_t data::get_data_type() const
{
    return f_node->get_type();
}


bool data::is_temporary() const
{
    return f_node->get_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY);
}


bool data::is_extern() const
{
    return f_node->get_attribute(attribute_t::NODE_ATTR_EXTERN);
}


integer_size_t data::get_integer_size() const
{
    if(f_node->get_type() != node_t::NODE_INTEGER)
    {
        return integer_size_t::INTEGER_SIZE_UNKNOWN;
    }
    return f_node->get_integer().get_smallest_size();
}


node::pointer_t data::get_node() const
{
    return f_node;
}


std::string const & data::get_string() const
{
    return f_node->get_string();
}


bool data::get_boolean() const
{
    return f_node->get_boolean();
}


integer data::get_integer() const
{
    return f_node->get_integer();
}


floating_point data::get_floating_point() const
{
    return f_node->get_floating_point();
}






operation::operation(node_t op, node::pointer_t n)
    : f_operation(op)
    , f_node(n)
{
}


node_t operation::get_operation() const
{
    return f_operation;
}


node::pointer_t operation::get_node() const
{
    return f_node;
}


void operation::set_left_handside(data::pointer_t d)
{
    f_left_handside = d;
}


data::pointer_t operation::get_left_handside() const
{
    return f_left_handside;
}


void operation::set_right_handside(data::pointer_t d)
{
    f_right_handside = d;
}


data::pointer_t operation::get_right_handside() const
{
    return f_right_handside;
}


void operation::set_result(data::pointer_t d)
{
    f_result = d;
}


data::pointer_t operation::get_result() const
{
    return f_result;
}


std::string operation::to_string() const
{
    std::stringstream ss;
    ss << this
        << ": "
        << node::type_to_string(f_operation);
    if(f_operation != f_node->get_type())
    {
        ss << " ("
            << f_node->get_type_name()
            << ")";
    }
    if(f_left_handside != nullptr)
    {
        ss << " lhs: "
            << node::type_to_string(f_left_handside->get_data_type());
        switch(f_left_handside->get_data_type())
        {
        case node_t::NODE_INTEGER:
            ss << " int:"
                << f_left_handside->get_integer().get();
            break;

        case node_t::NODE_FLOATING_POINT:
            ss << " flt:"
                << f_left_handside->get_floating_point().get();
            break;

        case node_t::NODE_STRING:
            ss << " str:"
                << f_left_handside->get_string();
            break;

        case node_t::NODE_IDENTIFIER:
            ss << " id:"
                << f_left_handside->get_string();
            break;

        case node_t::NODE_VARIABLE:
            ss << " var:"
                << f_left_handside->get_string();
            break;

        default:
            // no data in other type of nodes
            break;

        }
    }
    if(f_right_handside != nullptr)
    {
        ss << " rhs: "
            << node::type_to_string(f_right_handside->get_data_type());
        switch(f_right_handside->get_data_type())
        {
        case node_t::NODE_INTEGER:
            ss << " int:"
                << f_right_handside->get_integer().get();
            break;

        case node_t::NODE_FLOATING_POINT:
            ss << " flt:"
                << f_right_handside->get_floating_point().get();
            break;

        case node_t::NODE_STRING:
            ss << " str:"
                << f_right_handside->get_string();
            break;

        case node_t::NODE_IDENTIFIER:
            ss << " id:"
                << f_right_handside->get_string();
            break;

        case node_t::NODE_VARIABLE:
            ss << " var:"
                << f_right_handside->get_string();
            break;

        default:
            // no data in other type of nodes
            break;

        }
    }
    if(f_result != nullptr)
    {
        ss << " result: "
            << node::type_to_string(f_result->get_data_type());
        switch(f_result->get_data_type())
        {
        case node_t::NODE_INTEGER:
            ss << " int:"
                << f_result->get_integer().get();
            break;

        case node_t::NODE_FLOATING_POINT:
            ss << " flt:"
                << f_result->get_floating_point().get();
            break;

        case node_t::NODE_STRING:
            ss << " str:"
                << f_result->get_string();
            break;

        case node_t::NODE_IDENTIFIER:
            ss << " id:"
                << f_result->get_string();
            break;

        case node_t::NODE_VARIABLE:
            ss << " var:"
                << f_result->get_string();
            break;

        default:
            // no data in other type of nodes
            break;

        }
    }
    return ss.str();
}








/** \brief Flatten the tree.
 *
 * This function flattens the tree found in \p root. That output can then
 * be used to easily generate assembly language or binary code.
 *
 * The input tree does not get modified. Instead, we create a new set
 * of objects that are pretty close to what assembly looks like. This
 * allows us to do an additional optimization step before generating
 * the final output.
 *
 * The resulting objects will look like a flat sequence of instructions.
 * For example, the output of the flatten() function for an expression
 * such as `(x + 17) * (x - 32) ** 2` is going to be:
 *
 * \code
 * ; t1 = x + 17
 * LOAD 'x'
 * LOAD 17
 * ADD
 * STORE 't1'
 *
 * ; t2 = x - 32
 * LOAD 'x'
 * LOAD 32
 * SUBTRACT
 * STORE 't2'
 *
 * ; t3 = t2 ** 2
 * LOAD 't2'
 * LOAD 2
 * POWER
 * STORE 't3'
 *
 * ; t4 = t1 * t3
 * LOAD 't1'
 * LOAD 't3'
 * MULTIPLY
 * STORE 't4'
 *
 * ; result is in t4
 * \endcode
 *
 * This is very close to the Forth language but instead of using a stack
 * we use registers so we use LOAD and STORE instead of PUSH and POP.
 * It is also very specific to our situation at hand and _limited_ to the
 * operations that we support.
 *
 * Note that from the `LOAD 17` and the `ADD` instructions can be transformed
 * to an `ADD` with an immediate value on x86 like processors:
 *
 * \code
 *     // in case 'x' is an integer
 *     mov      x_offset(%ebp), %eax
 *     add      $17, %eax
 *     mov      %eax, t1_offset(%ebp)
 *
 *     // in case 'x' is a double
 *     movsd    x_offset(%ebp), %xmm0
 *     addsd    $17, %xmm0
 *     movsd    %xmm0, t1_offset(%ebp)
 * \endcode
 *
 * \todo
 * Look at optimizing the use of temporary. Once we are done with temporary
 * `t2`, we can reuse it further down the road. This means we can allocate
 * one location for many temporary. And later we can also look at using a
 * register instead of a memory location (i.e. avoid the STORE + LOAD sequence
 * as shown above).
 *
 * \note
 * For transliteration, that step is not required because target languages
 * (JavaScript, C/C++, etc.) can handle complex expressions themselves.
 *
 * \param[in] root  The tree of nodes to flatten.
 *
 * \return A flatten_nodes pointer or nullptr.
 */
flatten_nodes::pointer_t flatten(node::pointer_t root)
{
    int const save_errcnt(error_count());

    flatten_nodes::pointer_t fn(std::make_shared<flatten_nodes>(root));
    fn->run();

    if(error_count() == save_errcnt)
    {
        return fn;
    }

    return flatten_nodes::pointer_t();
}



} // namespace as2js
// vim: ts=4 sw=4 et