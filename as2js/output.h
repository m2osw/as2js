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
#include    <as2js/compiler.h>


// C++
//
#include    <list>



namespace as2js
{





class data
{
public:
    typedef std::shared_ptr<data>               pointer_t;
    typedef std::list<pointer_t>                list_t;
    typedef std::map<std::string, pointer_t>    map_t;
    typedef std::vector<pointer_t>              vector_t;

                            data(node::pointer_t n);

    node_t                  get_data_type() const;
    bool                    is_temporary() const;
    bool                    no_init() const;
    bool                    is_extern() const;
    integer_size_t          get_integer_size() const;
    node::pointer_t         get_node() const;
    std::string const &     get_string() const;
    bool                    get_boolean() const;
    integer                 get_integer() const;
    floating_point          get_floating_point() const;

    void                    set_data_name(std::string const & name);
    std::string const &     get_data_name() const;

private:
    node::pointer_t         f_node = node::pointer_t();
    std::string             f_data_name = std::string();
};




class operation
{
public:
    typedef std::shared_ptr<operation>  pointer_t;
    typedef std::list<pointer_t>        list_t;

                            operation(node_t op, node::pointer_t n);

    node_t                  get_operation() const;
    node::pointer_t         get_node() const;

    void                    set_left_handside(data::pointer_t d);
    data::pointer_t         get_left_handside() const;
    void                    set_right_handside(data::pointer_t d);
    data::pointer_t         get_right_handside() const;
    void                    add_additional_parameter(data::pointer_t d);    // i.e. for a CALL, an additional variable for the list of parameters
    std::size_t             get_parameter_size() const;
    data::pointer_t         get_parameter(int idx) const;
    void                    set_result(data::pointer_t d);
    data::pointer_t         get_result() const;
    void                    set_label(std::string const & l);
    std::string const &     get_label() const;

    std::string             to_string() const; // for display

private:
    node_t                  f_operation = node_t::NODE_UNKNOWN;
    node::pointer_t         f_node = node::pointer_t();
    data::pointer_t         f_left_handside = data::pointer_t();
    data::pointer_t         f_right_handside = data::pointer_t();
    data::vector_t          f_additional_parameters = data::vector_t();
    data::pointer_t         f_result = data::pointer_t();
    std::string             f_label = std::string();
};


class flatten_nodes
{
public:
    typedef std::shared_ptr<flatten_nodes>  pointer_t;

                            flatten_nodes(
                                  node::pointer_t root
                                , compiler::pointer_t c);

    void                    run();

    node::pointer_t         get_root() const;
    operation::list_t const &
                            get_operations() const;
    data::list_t const &    get_data() const;             // floating points, strings, etc.
    void                    add_variable(data::pointer_t var);
    data::map_t const &     get_variables() const;        // user defined variables

private:
    void                    directive_list(node::pointer_t n);
    data::pointer_t         node_to_operation(node::pointer_t n, bool force_full_variable = false);

    node::pointer_t         f_root = node::pointer_t();
    operation::list_t       f_operations = operation::list_t();
    compiler::pointer_t     f_compiler = compiler::pointer_t();
    data::list_t            f_data = data::list_t();
    data::map_t             f_variables = data::map_t();
    std::size_t             f_next_temp_var = 0;
    std::size_t             f_next_label = 0;
};




flatten_nodes::pointer_t    flatten(node::pointer_t root, compiler::pointer_t c);



} // namespace as2js
// vim: ts=4 sw=4 et
