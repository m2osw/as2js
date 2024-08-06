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
#pragma once

// self
//
#include    <as2js/lexer.h>


namespace as2js
{


// OLD DOCUMENTATION
// We do not have a separate interface for now...
//
// The parser class is mostly hidden to you.
// You can't derive from it. You call the CreateParser() to use it.
// Once you are finished with the parser, delete it.
// Note that deleting the parser doesn't delete the nodes and thus
// you can work with the tree even after you deleted the parser.
//
// You use like this:
//
//    using namespace as2js; // a using namespace is not recommended, though
//    MyInput input;
//    Parser *parser = Parser::CreateParser();
//    parser->SetInput(input);
//    // it is optional to set the options
//    parser->SetOptions(options);
//    NodePtr root = parser->Parse();
//
// NOTE: the input and options are NOT copied, a pointer to these
// object is saved in the parser. Delete the Parser() before you
// delete them. Also, this means you can change the options as the
// parsing goes on (i.e. usually this happens in Input::Error().).
class parser
{
public:
    typedef std::shared_ptr<parser>     pointer_t;

                        parser(base_stream::pointer_t input, options::pointer_t options);

    node::pointer_t     parse();

private:
    void                get_token(bool regex_allowed = true);
    void                unget_token(node::pointer_t & data);
    bool                has_option_set(option_t option) const;

    void                additive_expression(node::pointer_t & n);
    void                assignment_expression(node::pointer_t & n);
    void                attributes(node::pointer_t & attr_list);
    void                bitwise_and_expression(node::pointer_t & n);
    void                bitwise_or_expression(node::pointer_t & n);
    void                bitwise_xor_expression(node::pointer_t & n);
    void                block(node::pointer_t & n);
    void                break_continue(node::pointer_t & n, node_t type);
    void                case_directive(node::pointer_t & n);
    void                catch_directive(node::pointer_t& n);
    void                class_declaration(node::pointer_t & n, node_t type);
    void                conditional_expression(node::pointer_t & n, bool assignment);
    void                contract_declaration(node::pointer_t & n, node_t type);
    void                debugger(node::pointer_t & n);
    void                default_directive(node::pointer_t & n);
    void                directive(node::pointer_t & n);
    void                directive_list(node::pointer_t & n);
    void                do_directive(node::pointer_t & n);
    void                enum_declaration(node::pointer_t & n);
    void                equality_expression(node::pointer_t & n);
    void                expression(node::pointer_t & n);
    void                function(node::pointer_t & n, bool const expression);
    void                for_directive(node::pointer_t & n);
    void                forced_block(node::pointer_t & n, node::pointer_t statement);
    void                goto_directive(node::pointer_t & n);
    void                if_directive(node::pointer_t & n);
    void                import(node::pointer_t & n);
    void                list_expression(node::pointer_t & n, bool rest, bool empty);
    void                logical_and_expression(node::pointer_t & n);
    void                logical_or_expression(node::pointer_t & n);
    void                logical_xor_expression(node::pointer_t & n);
    void                match_expression(node::pointer_t & n);
    void                member_expression(node::pointer_t & n);
    void                min_max_expression(node::pointer_t & n);
    void                multiplicative_expression(node::pointer_t & n);
    void                namespace_block(node::pointer_t & n, node::pointer_t & attr_list);
    void                numeric_type(node::pointer_t & numeric_type_node, node::pointer_t & name);
    void                object_literal_expression(node::pointer_t & n);
    void                parameter_list(node::pointer_t & n, bool & has_out);
    void                pragma();
    void                pragma_option(option_t option, bool prima, node::pointer_t & argument, option_value_t value);
    void                program(node::pointer_t & n);
    void                package(node::pointer_t & n);
    void                postfix_expression(node::pointer_t & n);
    void                power_expression(node::pointer_t & n);
    void                primary_expression(node::pointer_t & n);
    void                relational_expression(node::pointer_t & n);
    void                return_directive(node::pointer_t & n);
    void                shift_expression(node::pointer_t & n);
    void                switch_directive(node::pointer_t & n);
    void                synchronized(node::pointer_t & n);
    void                throw_directive(node::pointer_t & n);
    void                try_finally(node::pointer_t & n, node_t const type);
    void                unary_expression(node::pointer_t & n);
    void                use_namespace(node::pointer_t & n);
    void                variable(node::pointer_t & n, node_t const type);
    void                with_while(node::pointer_t & n, node_t const type);
    void                yield(node::pointer_t & n);

    lexer::pointer_t            f_lexer   = lexer::pointer_t();
    options::pointer_t          f_options = options::pointer_t();
    node::pointer_t             f_root    = node::pointer_t();
    node::pointer_t             f_node    = node::pointer_t();    // last data read by get_token()
    node::vector_of_pointers_t  f_unget   = node::vector_of_pointers_t();
};



} // namespace as2js
// vim: ts=4 sw=4 et
