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
#include    "as2js/binary.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"
#include    "as2js/output.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


binary::binary(
          base_stream::pointer_t output
        , options::pointer_t options)
    : f_output(output)
    , f_options(options)
{
}


base_stream::pointer_t binary::get_output()
{
    return f_output;
}


options::pointer_t binary::get_options()
{
    return f_options;
}


int binary::output(node::pointer_t root)
{
    int const save_errcnt(error_count());

std::cerr << "----- start flattening...\n";
    operation::list_t operations(flatten(root));
std::cerr << "----- end flattening... (" << operations.size() << ")\n";

for(auto const & it : operations)
{
std::cerr << "  ++  " << it->to_string() << "\n";
}

    if(operations.empty())
    {
        // generate binary output
        //
    }

    return error_count() - save_errcnt;
}



} // namespace as2js
// vim: ts=4 sw=4 et
