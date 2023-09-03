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


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{






flatten_nodes::flatten_nodes(node::pointer_t root, compiler::pointer_t c)
    : f_root(root)
    , f_compiler(c)
{
}


void flatten_nodes::run()
{
    node_to_operation(f_root);

    // convert the very last %temp variable in an external variable
    // named %result so that way we do not have special code to handle
    // that case -- since we are in control of handling the result
    // in the run() function, from the outside, this is transparent
    //
    if(!f_operations.empty())
    {
        operation::pointer_t last_operation(f_operations.back());
        data::pointer_t result(last_operation->get_result());
        node::pointer_t var(result->get_node());
        var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, false);
        var->set_attribute(attribute_t::NODE_ATTR_EXTERN, true);
        auto it(f_variables.find(var->get_string()));
        if(it == f_variables.end())
        {
            throw internal_error(
                      std::string("could not find last result variable \"")
                    + var->get_string()
                    + "\".");
        }
        f_variables.erase(it);
        var->set_string("%result");
        f_variables["%result"] = result;
    }
}


void flatten_nodes::directive_list(node::pointer_t n)
{
    std::size_t const max(n->get_children_size());
    for(std::size_t idx(0); idx < max; ++idx)
    {
        node_to_operation(n->get_child(idx));
    }
}


data::pointer_t flatten_nodes::node_to_operation(node::pointer_t n, bool force_full_variable)
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
            if(force_full_variable)
            {
                var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
            }
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
                op = std::make_shared<operation>(node_t::NODE_IDENTITY, n);
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

    case node_t::NODE_ALMOST_EQUAL:
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
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_COMPARE:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_EQUAL:
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
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SMART_MATCH:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            if(force_full_variable)
            {
                var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
            }
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
std::cerr << "--------------------------------------------- this print starts\n";
std::cerr << "--- this very ASSIGNMENT?\n" << *n << "\n";
if(n->get_type_node() != nullptr) std::cerr << " -> type: " << n->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(0)->get_type_node() != nullptr) std::cerr << " -> -- LHS type: " << n->get_child(0)->get_type_node()->get_string() << "\n";
if(n->get_child(1) != nullptr && n->get_child(1)->get_type_node() != nullptr) std::cerr << " -> -- RHS type: " << n->get_child(1)->get_type_node()->get_string() << "\n";
std::cerr << "\n -> variable:\n" << *var;
std::cerr << "\n";
std::cerr << "--------------------------------------------- this print ends\n";
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

    case node_t::NODE_ARRAY:
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
            if(n->get_children_size() >= 3) // TODO: this is wrong, we instead want to support a Range object
            {
                // the array supports a range which means a third parameter
                //
                op->add_additional_parameter(node_to_operation(n->get_child(2)));
            }
            op->set_result(result);
            f_operations.push_back(op);
            return result;
        }
        break;

    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
        {
            operation::pointer_t op;
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            if(force_full_variable)
            {
                var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
            }
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
            if(force_full_variable)
            {
                var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
            }
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            if(n->get_children_size() == 1)
            {
                op = std::make_shared<operation>(node_t::NODE_NEGATE, n);
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

    case node_t::NODE_CONDITIONAL:
        // the conditional is a bit more involved
        // we generate the following:
        //
        //         cmp $0, var
        //         je false_case
        //         <true expr>
        //         mov %rax, mem  // store result to temp
        //         jmp after
        //     false_case:
        //         <false expr>
        //         mov %rax, mem  // store result to temp
        //     after:
        //
        // Note: the 'mov %rax, mem' at the end is generated by the
        //       <true expr> and <false expr> respectively so we
        //       cannot have it just once after the 'after:' label
        //       (the expr could return something else than rax too)
        {
            std::string after(".L");
            ++f_next_label;
            after += std::to_string(f_next_label);

            std::string false_case(".L");
            ++f_next_label;
            false_case += std::to_string(f_next_label);

            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            if(force_full_variable)
            {
                var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
            }

            // it is assumed that the compiler did its job properly and
            // that child 1 and 2 have the same type at this point
            //
            var->set_type_node(n->get_child(1)->get_type_node());

            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
            node::pointer_t assignment(n->create_replacement(node_t::NODE_ASSIGNMENT));
            assignment->set_type_node(n->get_child(1)->get_type_node());

            operation::pointer_t store1(std::make_shared<operation>(assignment->get_type(), assignment));
            store1->set_result(result);

            operation::pointer_t store2(std::make_shared<operation>(assignment->get_type(), assignment));
            store2->set_result(result);

            node::pointer_t if_false(n->create_replacement(node_t::NODE_IF_FALSE));
            operation::pointer_t op(std::make_shared<operation>(node_t::NODE_IF_FALSE, if_false));
            op->set_label(false_case);
            op->set_left_handside(node_to_operation(n->get_child(0)));  // compute condition
            f_operations.push_back(op);

            // insert true case instructions
            //
            store1->set_left_handside(node_to_operation(n->get_child(1)));
            f_operations.push_back(store1);

            // jump after
            //
            node::pointer_t to_after(n->create_replacement(node_t::NODE_GOTO));
            op = std::make_shared<operation>(node_t::NODE_GOTO, to_after);
            op->set_label(after);
            f_operations.push_back(op);

            // where we jump on FALSE
            //
            node::pointer_t false_label(n->create_replacement(node_t::NODE_LABEL));
            op = std::make_shared<operation>(node_t::NODE_LABEL, false_label);
            op->set_label(false_case);
            f_operations.push_back(op);

            // insert false case instructions
            //
            store2->set_left_handside(node_to_operation(n->get_child(2)));
            f_operations.push_back(store2);

            // the AFTER label
            //
            node::pointer_t after_label(n->create_replacement(node_t::NODE_LABEL));
            op = std::make_shared<operation>(node_t::NODE_LABEL, after_label);
            op->set_label(after);
            f_operations.push_back(op);

            return result;
        }
        break;

    case node_t::NODE_MEMBER:
        {
            // "member" is one to one the same as ARRAY ("[]") except that
            // the name has to be an identifier (and even that... you can
            // use the toString() of any object for the purpose)
            //
            // however, we have a special case for functions because those
            // get called and we don't really need to save a "pointer" to
            // call the function, we can instead directly call the function
            //
            node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
            var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(var));
            f_variables[temp] = result;
std::cerr << "--------------------------------------------- this print starts\n";
std::cerr << "--- MEMBER:\n" << *n << "\n";
if(n->get_type_node() != nullptr) std::cerr << " -> type: " << n->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(0)->get_type_node() != nullptr) std::cerr << " -> -- LHS type: " << n->get_child(0)->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(1)->get_type_node() != nullptr) std::cerr << " -> -- RHS type: " << n->get_child(1)->get_type_node()->get_string() << "\n";
std::cerr << "\n -> variable:\n" << *var;
std::cerr << "\n";
std::cerr << "--------------------------------------------- this print ends\n";

            operation::pointer_t op(std::make_shared<operation>(node_t::NODE_ARRAY, n));

            node::pointer_t instance(n->get_child(0)->get_instance());
            if(instance != nullptr
            && instance->get_type() == node_t::NODE_CLASS)
            {
                data::pointer_t class_static(std::make_shared<data>(n->get_child(0)));
                op->set_left_handside(class_static);
            }
            else
            {
                op->set_left_handside(node_to_operation(n->get_child(0)));
            }

            // the right handside is an IDENTIFIER, but it is not a global
            // variable so we handle it specially here
            //
            data::pointer_t member(std::make_shared<data>(n->get_child(1)));
            op->set_right_handside(member);

            op->set_result(result);
            f_operations.push_back(op);

            return result;
        }
        break;

    case node_t::NODE_CALL:
        {
            node::pointer_t lhs(n->get_child(0));
            node::pointer_t rhs(n->get_child(1));

            node::pointer_t object;
            node::pointer_t object_instance;
            node::pointer_t field;
            node::pointer_t field_instance;

            if(lhs->get_type() == node_t::NODE_MEMBER
            && lhs->get_children_size() >= 2)
            {
                object = lhs->get_child(0);
                object_instance = object->get_instance();
                field = lhs->get_child(1);
                field_instance = field->get_instance();
            }

            if(object != nullptr
            && object_instance != nullptr
            && field != nullptr
            && field_instance != nullptr
            && object->get_type() == node_t::NODE_IDENTIFIER
            && object_instance->get_type() == node_t::NODE_CLASS
            && field->get_type() == node_t::NODE_IDENTIFIER
            && field_instance->get_type() == node_t::NODE_FUNCTION)
            {
                std::string const class_name(object->get_string());
                std::string const name(field->get_string());

                if(class_name == "Math"
                && name == "abs"
                && rhs->get_type() == node_t::NODE_LIST
                && rhs->get_children_size() == 1)
                {
                    operation::pointer_t op;
                    node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
                    var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
                    var->set_type_node(n->get_type_node());
                    std::string temp("%temp");
                    ++f_next_temp_var;
                    temp += std::to_string(f_next_temp_var);
                    var->set_string(temp);
std::cerr << "--------------------------------------------- this print starts\n";
std::cerr << "--- the ABSOLUTE_VALUE?\n" << *n << "\n";
if(n->get_type_node() != nullptr) std::cerr << " -> type: " << n->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(0)->get_type_node() != nullptr) std::cerr << " -> -- LHS type: " << n->get_child(0)->get_type_node()->get_string() << "\n";
if(n->get_child(1) != nullptr && n->get_child(1)->get_type_node() != nullptr) std::cerr << " -> -- RHS type: " << n->get_child(1)->get_type_node()->get_string() << "\n";
std::cerr << "\n -> variable:\n" << *var;
std::cerr << "\n";
std::cerr << "--------------------------------------------- this print ends\n";
                    data::pointer_t result(std::make_shared<data>(var));
                    f_variables[temp] = result;
                    node::pointer_t abs(n->create_replacement(node_t::NODE_ABSOLUTE_VALUE));
                    abs->set_type_node(n->get_type_node());
                    op = std::make_shared<operation>(node_t::NODE_ABSOLUTE_VALUE, abs);
                    op->set_left_handside(node_to_operation(rhs->get_child(0)));
                    op->set_result(result);
                    f_operations.push_back(op);
                    return result;
                }

                if(class_name == "Math"
                && (name == "min" || name == "max")
                && rhs->get_type() == node_t::NODE_LIST)
                {
                    operation::pointer_t op;
                    node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
                    var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
                    var->set_type_node(n->get_type_node());
                    std::string temp("%temp");
                    ++f_next_temp_var;
                    temp += std::to_string(f_next_temp_var);
                    var->set_string(temp);
std::cerr << "--------------------------------------------- this print starts\n";
std::cerr << "--- the ABSOLUTE_VALUE?\n" << *n << "\n";
if(n->get_type_node() != nullptr) std::cerr << " -> type: " << n->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(0)->get_type_node() != nullptr) std::cerr << " -> -- LHS type: " << n->get_child(0)->get_type_node()->get_string() << "\n";
if(n->get_child(1) != nullptr && n->get_child(1)->get_type_node() != nullptr) std::cerr << " -> -- RHS type: " << n->get_child(1)->get_type_node()->get_string() << "\n";
std::cerr << "\n -> variable:\n" << *var;
std::cerr << "\n";
std::cerr << "--------------------------------------------- this print ends\n";
                    data::pointer_t result(std::make_shared<data>(var));
                    f_variables[temp] = result;
                    node_t type(name == "min" ? node_t::NODE_MINIMUM : node_t::NODE_MAXIMUM);
                    node::pointer_t minmax(n->create_replacement(type));
                    minmax->set_type_node(n->get_type_node());
                    op = std::make_shared<operation>(type, minmax);
                    std::size_t const max(rhs->get_children_size());
                    for(std::size_t idx(0); idx < max; ++idx)
                    {
                        op->add_additional_parameter(node_to_operation(rhs->get_child(idx)));
                    }
                    op->set_result(result);
                    f_operations.push_back(op);
                    return result;
                }
            }

            // create the result variable
            //
            node::pointer_t result_var(n->create_replacement(node_t::NODE_VARIABLE));
            result_var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            result_var->set_type_node(n->get_type_node());
            std::string temp("%temp");
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            result_var->set_string(temp);
            data::pointer_t result(std::make_shared<data>(result_var));
            f_variables[temp] = result;
std::cerr << "--------------------------------------------- this print starts\n";
std::cerr << "--- CALL RESULT VAR:\n" << *n << "\n";
if(n->get_type_node() != nullptr) std::cerr << " -> type: " << n->get_type_node()->get_string() << "\n";
if(n->get_child(0) != nullptr && n->get_child(0)->get_type_node() != nullptr) std::cerr << " -> -- LHS type: " << n->get_child(0)->get_type_node()->get_string() << "\n";
if(n->get_child(1) != nullptr && n->get_child(1)->get_type_node() != nullptr) std::cerr << " -> -- RHS type: " << n->get_child(1)->get_type_node()->get_string() << "\n";
std::cerr << "\n -> variable:\n" << *result_var;
std::cerr << "\n";
std::cerr << "--------------------------------------------- this print ends\n";

            // create the parameters variable
            //
            node::pointer_t param_var(n->create_replacement(node_t::NODE_VARIABLE));
            param_var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
            param_var->set_flag(flag_t::NODE_VARIABLE_FLAG_NOINIT, true);
            node::pointer_t rtype;
            f_compiler->resolve_internal_type(n, "Array", rtype);
            param_var->set_type_node(rtype);
            temp = "%params";
            ++f_next_temp_var;
            temp += std::to_string(f_next_temp_var);
            param_var->set_string(temp);
            data::pointer_t params(std::make_shared<data>(param_var));
            f_variables[temp] = params;

            operation::pointer_t op(std::make_shared<operation>(node_t::NODE_CALL, n));
            op->add_additional_parameter(params);

            if(field != nullptr
            && field_instance != nullptr
            && field->get_type() == node_t::NODE_IDENTIFIER
            && field_instance->get_type() == node_t::NODE_FUNCTION)
            {
                // here we need the variable part (MEMBER) however the
                // FIELD part is not going to be flatten, it _just_
                // participate in the CALL generation
                //
                op->set_left_handside(node_to_operation(lhs->get_child(0)));

                if(field_instance->get_attribute(attribute_t::NODE_ATTR_UNIMPLEMENTED))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNIMPLEMENTED, n->get_position());
                    msg << "can't call function \""
                        << field_instance->get_string()
                        << "\"; it is not yet implemented.";
                }
            }
            else
            {
                // TBD: at the moment I don't think this will work
                //      since that will return something on the stack
                //      which we need to use to call the function
                //
                op->set_left_handside(node_to_operation(lhs));
            }

            // compute each parameter
            //
            node::pointer_t param_list(n->get_child(1));
            std::size_t const max(param_list->get_children_size());
            for(std::size_t idx(0); idx < max; ++idx)
            {
                node::pointer_t param(param_list->get_child(idx));
                data::pointer_t d(node_to_operation(param, true));
                node::pointer_t param_type;
                switch(d->get_data_type())
                {
                case node_t::NODE_BOOLEAN:
                    f_compiler->resolve_internal_type(n, "Boolean", param_type);
                    break;

                case node_t::NODE_INTEGER:
                    f_compiler->resolve_internal_type(n, "Integer", param_type);
                    break;

                case node_t::NODE_FLOATING_POINT:
                    f_compiler->resolve_internal_type(n, "Double", param_type);
                    break;

                default:
                    // anything else is already inside a VARIABLE so no
                    // special handling required
                    //
                    break;

                }
                if(param_type != nullptr)
                {
                    // in thi case we have a straight value but our list of
                    // parameters requires us to use a VARIABLE
                    //
                    node::pointer_t native_var(n->create_replacement(node_t::NODE_VARIABLE));
                    native_var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
                    native_var->set_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE, true);
                    native_var->set_flag(flag_t::NODE_VARIABLE_FLAG_NOINIT, true);
                    native_var->set_type_node(n->get_type_node());
                    temp = "%temp";
                    ++f_next_temp_var;
                    temp += std::to_string(f_next_temp_var);
                    native_var->set_string(temp);
                    data::pointer_t p(std::make_shared<data>(native_var));
                    f_variables[temp] = p;

                    operation::pointer_t param_op(std::make_shared<operation>(node_t::NODE_PARAM, param));
                    param_op->set_left_handside(d);
                    param_op->set_result(p);
                    f_operations.push_back(param_op);

                    op->add_additional_parameter(p);
                }
                else
                {
                    op->add_additional_parameter(d);
                }
            }

            op->set_result(result);
            f_operations.push_back(op);

            return result;
        }
        break;

    case node_t::NODE_LIST:
        {
            std::size_t const max(n->get_children_size());
            if(max > 0)
            {
                operation::pointer_t op(std::make_shared<operation>(node_t::NODE_LIST, n));
                data::pointer_t result;
                for(std::size_t idx(0); idx < max; ++idx)
                {
                    result = node_to_operation(n->get_child(idx));
                    op->add_additional_parameter(result);
                }

                if(result->get_data_type() != node_t::NODE_VARIABLE)
                {
                    node::pointer_t var(n->create_replacement(node_t::NODE_VARIABLE));
                    var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
                    var->set_type_node(result->get_node()->get_type_node());
                    std::string temp("%temp");
                    ++f_next_temp_var;
                    temp += std::to_string(f_next_temp_var);
                    var->set_string(temp);
                    result = std::make_shared<data>(var);
                    f_variables[temp] = result;
                    op->set_result(result);
                }
                else
                {
                    // avoid a copy whenever possible
                    //
                    op->set_result(result);
                }

                f_operations.push_back(op);

                return result;
            }
        }
        break;

    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_ASYNC:
    case node_t::NODE_AWAIT:
    case node_t::NODE_BREAK:
    case node_t::NODE_BYTE:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CHAR:
    case node_t::NODE_CLASS:
    case node_t::NODE_COALESCE:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DO:
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
    case node_t::NODE_LONG:
    case node_t::NODE_MATCH:
    case node_t::NODE_NAME:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_NATIVE:
    case node_t::NODE_NEW:
    case node_t::NODE_NOT_MATCH:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_OPTIONAL_MEMBER:
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

    case node_t::NODE_ABSOLUTE_VALUE:   // we generate one of those from here, but not the compiler so we should never see it here
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
    case node_t::NODE_IDENTITY:
    case node_t::NODE_IF_FALSE:
    case node_t::NODE_IF_TRUE:
    case node_t::NODE_NEGATE:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REQUIRE:
    case node_t::NODE_REST:
    case node_t::NODE_RETURN:
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


void flatten_nodes::add_variable(data::pointer_t var)
{
    f_variables[var->get_string()] = var;
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


bool data::no_init() const
{
    return f_node->get_flag(flag_t::NODE_VARIABLE_FLAG_NOINIT);
}


bool data::is_extern() const
{
    return f_node->get_attribute(attribute_t::NODE_ATTR_EXTERN);
}


integer_size_t data::get_integer_size() const
{
    switch(f_node->get_type())
    {
    case node_t::NODE_INTEGER:
        return f_node->get_integer().get_smallest_size();

    case node_t::NODE_FLOATING_POINT:
        return integer_size_t::INTEGER_SIZE_FLOATING_POINT;

    default:
        return integer_size_t::INTEGER_SIZE_UNKNOWN;
    }
    snapdev::NOT_REACHED();
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


void data::set_data_name(std::string const & name)
{
    f_data_name = name;
}


std::string const & data::get_data_name() const
{
    return f_data_name;
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


void operation::add_additional_parameter(data::pointer_t d)
{
    f_additional_parameters.push_back(d);
}


std::size_t operation::get_parameter_size() const
{
    return f_additional_parameters.size();
}


data::pointer_t operation::get_parameter(int idx) const
{
    return f_additional_parameters.at(idx);
}


void operation::set_result(data::pointer_t d)
{
    f_result = d;
}


data::pointer_t operation::get_result() const
{
    return f_result;
}


void operation::set_label(std::string const & l)
{
    f_label = l;
}


std::string const & operation::get_label() const
{
    return f_label;
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
    switch(f_node->get_type())
    {
    //case node_t::NODE_CALL: -- it would be neat to get the function name, but that's in a sub-node somewhere...
    case node_t::NODE_STRING:
        ss << " string:\""
           << f_node->get_string()
           << "\"";
        break;

    default:
        break;

    }
    node::pointer_t type(f_node->get_type_node());
    if(type != nullptr)
    {
        if(type->get_type() == node_t::NODE_CLASS)
        {
            ss << " type:"
               << type->get_string();
        }
    }
    if(!f_label.empty())
    {
        ss << " label:"
           << f_label;
    }

    if(f_left_handside != nullptr)
    {
        ss << " lhs: "
           << node::type_to_string(f_left_handside->get_data_type());
        switch(f_left_handside->get_data_type())
        {
        case node_t::NODE_FLOATING_POINT:
            ss << " flt:"
               << f_left_handside->get_floating_point().get();
            break;

        case node_t::NODE_IDENTIFIER:
            ss << " id:"
               << f_left_handside->get_string();
            break;

        case node_t::NODE_INTEGER:
            ss << " int:"
               << f_left_handside->get_integer().get();
            break;

        case node_t::NODE_STRING:
            ss << " str:"
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
flatten_nodes::pointer_t flatten(node::pointer_t root, compiler::pointer_t c)
{
    int const save_errcnt(error_count());

    flatten_nodes::pointer_t fn(std::make_shared<flatten_nodes>(root, c));
    fn->run();

    if(error_count() == save_errcnt)
    {
        return fn;
    }

    return flatten_nodes::pointer_t();
}



} // namespace as2js
// vim: ts=4 sw=4 et
