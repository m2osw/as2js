// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "as2js/parser.h"

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER  *********************************************************/
/**********************************************************************/
/**********************************************************************/

parser::parser(base_stream::pointer_t input, options::pointer_t options)
    : f_lexer(std::make_shared<lexer>(input, options))
    , f_options(options)
{
}


node::pointer_t parser::parse()
{
    // This parses everything and creates ONE tree
    // with the result. The tree obviously needs to
    // fit in RAM...

    // We lose the previous tree if any and create a new
    // root node. This is our program node.
    get_token();
    program(f_root);

    return f_root;
}


void parser::get_token()
{
    bool const reget(!f_unget.empty());

    if(reget)
    {
        f_node = f_unget.back();
        f_unget.pop_back();
    }
    else
    {
        f_node = f_lexer->get_next_token();
    }
}


void parser::unget_token(node::pointer_t& node)
{
    f_unget.push_back(node);
}



/** \brief Check whether a given option is set.
 *
 * Because the parser checks options in many places, it makes use of this
 * helper function just in case we wanted to handle various special cases.
 * (I had two before, but both have been removed at this point.)
 *
 * This function checks whether the specified option is set. If so,
 * then it returns true, otherwise it returns false.
 *
 * \param[in] option  The option to check.
 *
 * \return true if the option was set, false otherwise.
 */
bool parser::has_option_set(options::option_t option) const
{
    return f_options->get_option(option) != 0;
}



}
// namespace as2js

// vim: ts=4 sw=4 et
