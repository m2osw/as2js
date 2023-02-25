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
#include    "as2js/node.h"


// snaodev
//
#include    <snapdev/enum_class_math.h>


// C++
//
#include    <list>



namespace as2js
{



// some operations specific to the operation class
//
constexpr node_t const      NODE_LOAD       = node_t::NODE_max + 1;
constexpr node_t const      NODE_NEGATE     = node_t::NODE_max + 2;
constexpr node_t const      NODE_POSITIVE   = node_t::NODE_max + 3;
constexpr node_t const      NODE_SAVE       = node_t::NODE_max + 4;


class data
{
public:
    typedef std::shared_ptr<data>   pointer_t;
    typedef std::list<pointer_t>    list_t;

                            data(node::pointer_t n);

    node_t                  get_data_type() const;
    std::string const &     get_string() const;
    bool                    get_boolean() const;
    integer                 get_integer() const;
    floating_point          get_floating_point() const;

private:
    node::pointer_t         f_node = node::pointer_t();
    //std::string             f_data = std::string();
};


class variable
{
public:
    typedef std::shared_ptr<variable>           pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                            variable(node::pointer_t n);

    node_t                  get_type() const;
    std::string const &     get_name() const;

private:
    node::pointer_t         f_node = node::pointer_t();
};


class operation
{
public:
    typedef std::shared_ptr<operation>  pointer_t;
    typedef std::list<pointer_t>        list_t;

                            operation(node_t op, node::pointer_t n);

    node_t                  get_operation() const;
    node::pointer_t         get_node() const;

    void                    set_string(std::string const & s);
    std::string const &     get_string() const;

    void                    set_boolean(bool b);
    bool                    get_boolean() const;

    void                    set_integer(integer const & n);
    integer const &         get_integer() const;

    void                    set_floating_point(floating_point const & n);
    floating_point const &  get_floating_point() const;

private:
    node_t                  f_operation = node_t::NODE_UNKNOWN;
    node::pointer_t         f_node = node::pointer_t();
    std::string             f_string = std::string();
    bool                    f_boolean = false;
    integer                 f_integer = integer();
    floating_point          f_floating_point = floating_point();
};


operation::list_t           flatten(node::pointer_t root);



} // namespace as2js
// vim: ts=4 sw=4 et
