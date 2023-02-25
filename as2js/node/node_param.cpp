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

#include    "as2js/exception.h"


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Handle nodes of type parameter.
 *
 * This file represents the implementation of the various
 * parameter functions applying to nodes.
 *
 * Parameters are used to call functions. The list of
 * parameters defined in this file represents such.
 */


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE PARAM  *****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Define the size of the parameter index and depth vectors.
 *
 * This function defines the size of the depth and index parameter
 * vectors. Until this function is called, trying to set a depth
 * or index parameter will fail.
 *
 * Also, the function cannot be called more than once and the size
 * parameter cannot be zero.
 *
 * \exception exception_internal_error
 * If this node is not of type NODE_PARAM_MATCH, if the function
 * had been called before, or if the \p size parameter is zero,
 * this exception is raised.
 *
 * \param[in] size  The number of parameters (size > 0 must be true).
 *
 * \sa get_param_size()
 * \sa get_param_depth()
 * \sa get_param_index()
 */
void node::set_param_size(size_t size)
{
    if(f_type != node_t::NODE_PARAM_MATCH)
    {
        throw internal_error("set_param_size() called with a node other than a \"NODE_PARAM_MATCH\".");
    }
    if(f_param_depth.size() != 0)
    {
        throw internal_error("set_param_size() called twice.");
    }
    if(size == 0)
    {
        throw internal_error("set_param_size() was called with a size of zero.");
    }
    f_param_depth.resize(size);
    f_param_index.resize(size);
}


/** \brief Return the size of the parameter index and depth vectors.
 *
 * This function returns zero until set_param_size() is successfully
 * called with a valid size.
 *
 * \return The current size of the parameter index and depth vectors.
 *
 * \sa set_param_size()
 */
size_t node::get_param_size() const
{
    return f_param_depth.size();
}


/** \brief Get the depth at the specified index.
 *
 * This function returns the depth parameter at the specified index.
 *
 * This function cannot be called until the set_param_size() gets
 * called with a valid size.
 *
 * \note
 * The index here is named 'j' because it represents the final
 * index in the function being called and not the index of the
 * parameter being matched. See the set_param_index() to see
 * the difference between the 'idx' and 'j' indexes.
 *
 * \exception out_of_range
 * The function throws an exception if the \p j parameter is out
 * of range. It should be defined between 0 and get_param_size() - 1.
 *
 * \param[in] j  The index of the depth to retrieve.
 *
 * \return The depth of the type of this parameter.
 *
 * \sa set_param_size()
 * \sa get_param_size()
 * \sa set_param_depth()
 */
depth_t node::get_param_depth(size_t j) const
{
    if(j >= f_param_depth.size())
    {
        throw out_of_range("get_param_depth() called with an out of range index.");
    }
    return f_param_depth.at(j);
}


/** \brief Set the depth of a parameter.
 *
 * When we search for a match of a function call, we check its parameters.
 * If a parameter has a higher class type definition, then it wins over
 * the others. This depth value represents that information.
 *
 * \note
 * The index here is named 'j' because it represents the final
 * index in the function being called and not the index of the
 * parameter being matched. See the set_param_index() to see
 * the difference between the 'idx' and 'j' indexes.
 *
 * \exception out_of_range
 * The function throws an exception if the \p j parameter is out
 * of range. It should be defined between 0 and get_param_size() - 1.
 *
 * \param[in] j  The index of the parameter for which we define the depth.
 *               (The order is the function being called order.)
 * \param[in] depth  The new depth.
 */
void node::set_param_depth(size_t j, depth_t depth)
{
    if(j >= f_param_depth.size())
    {
        throw out_of_range("set_param_depth() called with an index out of range.");
    }
    f_param_depth[j] = depth;
}


/** \brief Get the index of the parameter.
 *
 * When a user writes a function call, he can spell out the parameter
 * names as in:
 *
 * \code
 * pos = find(size => 123, characer => 'c', haystack => str);
 * \endcode
 *
 * The parameters, in the function declaration, may not be in the
 * same order:
 *
 * \code
 * function find(haystack: string, character: string, size: number = -1);
 * \endcode
 *
 * The parameter index vector holds the indices so we can reorganize the
 * call as in:
 *
 * \code
 * pos = find(str, 'c', 123);
 * \endcode
 *
 * The really cool thing is that you could call a function with
 * multiple definitions and still get the parameters in the right
 * order even though both functions define their parameters
 * in a different order.
 *
 * \exception out_of_range
 * The function throws an exception if the \p idx parameter is out
 * of range. It should be defined between 0 and get_param_size() - 1.
 *
 * \param[in] idx  The index of the parameter in the function being called.
 *
 * \return The index in the function definition.
 *
 * \sa set_param_size()
 * \sa get_param_size()
 * \sa set_param_index()
 */
std::size_t node::get_param_index(std::size_t idx) const
{
    if(idx >= f_param_index.size())
    {
        throw out_of_range("set_param_index() called with an index out of range.");
    }
    return f_param_index.at(idx);
}


/** \brief Set the parameter index.
 *
 * Save the index of the parameter in the function being called, opposed
 * to the index of the parameter in the function call.
 *
 * See function get_param_index() for more details about the indexes.
 *
 * \exception out_of_range
 * The function throws an exception if the \p idx or \p j parameters are
 * out of range. They should both be defined between 0 and
 * get_param_size() - 1.
 *
 * \param[in] idx  The index in the function call.
 * \param[in] j  The index in the function being called.
 *
 * \sa set_param_size()
 * \sa get_param_size()
 * \sa get_param_index()
 */
void node::set_param_index(std::size_t idx, std::size_t j)
{
    if(idx >= f_param_index.size()
    || j >= f_param_index.size())
    {
        throw out_of_range("set_param_index() called with one or both indexes out of range.");
    }
    f_param_index[idx] = j;
}



}
// namespace as2js

// vim: ts=4 sw=4 et
