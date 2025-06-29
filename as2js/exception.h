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

// libexcept
//
#include    <libexcept/exception.h>



namespace as2js
{

DECLARE_LOGIC_ERROR(internal_error);
DECLARE_LOGIC_ERROR(not_implemented);
DECLARE_LOGIC_ERROR(out_of_range);

DECLARE_MAIN_EXCEPTION(as2js_exception);

DECLARE_EXCEPTION(as2js_exception, already_defined);
DECLARE_EXCEPTION(as2js_exception, cannot_open_file);
DECLARE_EXCEPTION(as2js_exception, cyclical_structure);
DECLARE_EXCEPTION(as2js_exception, execution_error);
DECLARE_EXCEPTION(as2js_exception, file_already_open);
DECLARE_EXCEPTION(as2js_exception, incompatible_data);
DECLARE_EXCEPTION(as2js_exception, incompatible_type);
DECLARE_EXCEPTION(as2js_exception, invalid_data);
DECLARE_EXCEPTION(as2js_exception, invalid_float);
DECLARE_EXCEPTION(as2js_exception, invalid_index); // in this case the index is a string (map)
DECLARE_EXCEPTION(as2js_exception, locked_node);
DECLARE_EXCEPTION(as2js_exception, no_parent);
DECLARE_EXCEPTION(as2js_exception, parent_child);


// the process is viewed as done, exit now
class as2js_exit
    : public as2js_exception
{
public:
    as2js_exit(std::string const & msg, int code)
        : as2js_exception(msg)
        , f_code(code)
    {
        set_parameter("exit_code", std::to_string(code));
    }

    int code() const
    {
        return f_code;
    }

private:
    int             f_code;
};



} // namespace as2js
// vim: ts=4 sw=4 et
