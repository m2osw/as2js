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

/** \brief Manage archives of run-time functions.
 *
 * The binary code comes with a set of run-time functions defined along the
 * compiler. These functions are saved in one archive which is used to
 * compile code.
 *
 * For example, the `**` operator requires us to have an integer-based
 * function to computer a power. This is found in the rt_power.s file.
 * The system compiles that file in a .bin file which is just the binary.
 * Then we use our as2js command line tool to generate an archive with
 * all of these .bin files. String handling will also be defined in the
 * runtime library.
 */

// self
//
//#include    <as2js/node.h>
//#include    <as2js/options.h>
//#include    <as2js/output.h>
#include    <as2js/stream.h>



// versiontheca
//
//#include    <versiontheca/versiontheca.h>


// C++
//
#include    <map>



namespace as2js
{



typedef std::vector<std::uint8_t>   rt_text_t;


class rt_function
{
public:
    typedef std::shared_ptr<rt_function>        pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

    void                    set_name(std::string const & name);
    std::string const &     get_name() const;

    void                    set_code(rt_text_t const & code);
    rt_text_t const &       get_code() const;

private:
    std::string             f_name = std::string();
    rt_text_t               f_code = rt_text_t();
};


rt_function::pointer_t      load_function(base_stream::pointer_t in);
rt_function::pointer_t      load_function(std::string const & filename);


class archive
{
public:
    bool                    create(std::vector<std::string> const & patterns);
    bool                    add_from_pattern(std::string const & pattern);

    bool                    load(base_stream::pointer_t in);
    bool                    save(base_stream::pointer_t out);

    void                    add_function(rt_function::pointer_t const & func);
    rt_function::pointer_t  find_function(std::string const & name) const;
    rt_function::map_t      get_functions() const;

private:
    rt_function::map_t      f_functions = rt_function::map_t();
};



} // namespace as2js
// vim: ts=4 sw=4 et
