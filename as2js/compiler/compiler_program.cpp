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
#include    "as2js/compiler.h"

#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PROGRAM  ********************************************************/
/**********************************************************************/
/**********************************************************************/



void compiler::program(node::pointer_t program_node)
{
    // This is the root. Whenever you search to resolve a reference,
    // do not go past that node! What's in the parent of a program is
    // not part of that program...
    //
    f_program = program_node;

#if 0
std::cerr << "program:\n" << *program_node << "\n";
#endif
    // get rid of any declaration marked false
    //
    // TODO: this probably needs to be recursive
    //
    size_t const org_max(program_node->get_children_size());
    for(size_t idx(0); idx < org_max; ++idx)
    {
        node::pointer_t child(program_node->get_child(idx));
        if(get_attribute(child, attribute_t::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }
    program_node->clean_tree();

    node_lock ln(program_node);

    // look for all the labels in this program (for goto's)
    //
    for(size_t idx(0); idx < org_max; ++idx)
    {
        node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
        {
            find_labels(program_node, child);
        }
    }

    // a program is composed of directives (usually just one list)
    // which we want to compile
    //
    for(size_t idx(0); idx < org_max; ++idx)
    {
        node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
        {
            directive_list(child);
        }
    }

#if 0
if(Message::error_count() > 0)
std::cerr << program_node;
#endif
}



} // namespace as2js
// vim: ts=4 sw=4 et
