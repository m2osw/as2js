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
#pragma once

// C++
//
#include    <list>
#include    <string>



namespace as2js
{



// "Resources" support to load .rc files
class resources
{
public:
    typedef std::list<std::string>  script_paths_t;

                                resources();

    void                        reset();
    void                        init(bool const accept_if_missing);

    script_paths_t const &      get_scripts() const;
    void                        set_scripts(std::string const & scripts, bool warning_about_invalid = false);
    std::string const &         get_db() const;
    void                        set_db(std::string const & db);
    std::string const &         get_temporary_variable_name() const;
    void                        set_temporary_variable_name(std::string const & name);

    static std::string const &  get_home();

private:
    script_paths_t              f_scripts = script_paths_t();
    std::string                 f_db = std::string();
    std::string                 f_temporary_variable_name = std::string();
};



} // namespace as2js
// vim: ts=4 sw=4 et
