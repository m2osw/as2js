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

// self
//
#include    "as2js/compiler.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{



/** \brief Initialize the compiler object.
 *
 * The compiler includes many sub-classes that it initializes here.
 * Especially, it calls the internal_imports() function to load all
 * the internal modules, database, resource file.
 *
 * The options parameter represents the command line options setup
 * by a user and within the code with the 'use' keyword (i.e. pragmas).
 *
 * \param[in] o  The options object to use while compiling.
 * \param[in] retriever  The retriever to find modules. If set to nullptr
 *                       then the default search algorithm is used.
 */
compiler::compiler(options::pointer_t o, input_retriever::pointer_t retriever)
    : f_time(time(nullptr))
    , f_options(o)
{
    snapdev::NOT_USED(set_input_retriever(retriever));
    internal_imports();
}


compiler::~compiler()
{
}


/** \brief Setup another retriever.
 *
 * This function can be used to change the retriever as required.
 *
 * The retriever is a template. If implemented, it can be used to load
 * a file in the lexer/parser. The idea is that in some cases you want
 * to have a special way to find module files.
 *
 * \param[in] retriever  The retriever to assign to the compiler.
 *
 * \return The previous input retriever is returned. You can thus restore
 * it later.
 */
input_retriever::pointer_t compiler::set_input_retriever(input_retriever::pointer_t retriever)
{
    f_input_retriever.swap(retriever);
    return retriever;
}



} // namespace as2js
// vim: ts=4 sw=4 et
