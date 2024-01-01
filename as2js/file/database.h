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
#include    <as2js/json.h>


namespace as2js
{

// The database uses a json object defined as:
//
// {
//   "<package_name>": {
//     "<element name>": {
//       "type": <type>,
//       "filename": <filename>,
//       "line": <line>
//     },
//     // ... repeat for each element ...
//   },
//   // ... repeat for each package ...
// };
//
// The old database was one line per element as in:
// <package name> <element name> <type> <filename> <line>


class database
{
public:
    typedef std::shared_ptr<database>   pointer_t;

    class element
    {
    public:
        typedef std::shared_ptr<element>            pointer_t;
        typedef std::map<std::string, pointer_t>    map_t;
        typedef std::vector<pointer_t>              vector_t;

                                    element(std::string const & element_name, json::json_value::pointer_t element);

        void                        set_type(std::string const & type);
        void                        set_filename(std::string const & filename);
        void                        set_line(position::counter_t line);

        std::string                 get_element_name() const;
        std::string                 get_type() const;
        std::string                 get_filename() const;
        position::counter_t         get_line() const;

    private:
        std::string const           f_element_name;
        std::string                 f_type = std::string();
        std::string                 f_filename = std::string();
        position::counter_t         f_line = position::DEFAULT_COUNTER;

        json::json_value::pointer_t  f_element = json::json_value::pointer_t();
    };

    class package
    {
    public:
        typedef std::shared_ptr<package>            pointer_t;
        typedef std::map<std::string, pointer_t>    map_t;
        typedef std::vector<pointer_t>              vector_t;

                                    package(std::string const& package_name, json::json_value::pointer_t package);

        std::string                 get_package_name() const;

        element::vector_t           find_elements(std::string const & pattern) const;
        element::pointer_t          get_element(std::string const & element_name) const;
        element::pointer_t          add_element(std::string const & element_name);

    private:
        std::string const           f_package_name;

        json::json_value::pointer_t f_package = json::json_value::pointer_t();
        element::map_t              f_elements = element::map_t();
    };

    bool                        load(std::string const & filename);
    void                        save() const;

    package::vector_t           find_packages(std::string const & pattern) const;
    package::pointer_t          get_package(std::string const & package_name) const;
    package::pointer_t          add_package(std::string const & package_name);

    static bool                 match_pattern(std::string const & name, std::string const & pattern);

private:
    std::string                 f_filename = std::string();
    json::pointer_t             f_json = json::pointer_t();
    json::json_value::pointer_t f_value = json::json_value::pointer_t(); // json

    package::map_t              f_packages = package::map_t();
};



} // namespace as2js
// vim: ts=4 sw=4 et
