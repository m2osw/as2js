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
#include    "as2js/node.h"

#include    "as2js/exception.h"


// C++
//
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Handle the node flags.
 *
 * Nodes accept a large set of flags (42 at time of writing).
 *
 * Flags are specific to node types. In an earlier implementation,
 * flags would overlap (i.e. the same bit would be used by different
 * flags, which flag was determine by the type of node being used.)
 * This was revamped to make use of unique flags in order to over
 * potential bugs.
 *
 * Flags being specific to a node type, the various functions below
 * make sure that the flags modified on a node are compatible with
 * that node.
 *
 * \todo
 * The conversion functions do not take flags in account. As far as
 * I know, at this point we cannot convert a node of a type that
 * accept a flag except with the to_unknown() function in which
 * case flags become irrelevant anyway. We should test that the
 * flags remain valid after a conversion.
 *
 * \todo
 * Mutually exclusive flags are not currently verified in this
 * code when it should be.
 */


namespace as2js
{


namespace
{


#define FLAG_NAME(name)        [static_cast<int>(flag_t::name)] = #name

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr char const * const g_flag_name[static_cast<std::size_t>(flag_t::NODE_FLAG_max)]
{
    FLAG_NAME(NODE_CATCH_FLAG_TYPED),
    FLAG_NAME(NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES),
    FLAG_NAME(NODE_ENUM_FLAG_CLASS),
    FLAG_NAME(NODE_ENUM_FLAG_INUSE),
    FLAG_NAME(NODE_FOR_FLAG_CONST),
    FLAG_NAME(NODE_FOR_FLAG_FOREACH),
    FLAG_NAME(NODE_FOR_FLAG_IN),
    FLAG_NAME(NODE_FUNCTION_FLAG_GETTER),
    FLAG_NAME(NODE_FUNCTION_FLAG_SETTER),
    FLAG_NAME(NODE_FUNCTION_FLAG_OUT),
    FLAG_NAME(NODE_FUNCTION_FLAG_VOID),
    FLAG_NAME(NODE_FUNCTION_FLAG_NEVER),
    FLAG_NAME(NODE_FUNCTION_FLAG_NOPARAMS),
    FLAG_NAME(NODE_FUNCTION_FLAG_OPERATOR),
    FLAG_NAME(NODE_IDENTIFIER_FLAG_WITH),
    FLAG_NAME(NODE_IDENTIFIER_FLAG_TYPED),
    FLAG_NAME(NODE_IDENTIFIER_FLAG_OPERATOR),
    FLAG_NAME(NODE_IMPORT_FLAG_IMPLEMENTS),
    FLAG_NAME(NODE_PACKAGE_FLAG_FOUND_LABELS),
    FLAG_NAME(NODE_PACKAGE_FLAG_REFERENCED),
    FLAG_NAME(NODE_PARAM_FLAG_CONST),
    FLAG_NAME(NODE_PARAM_FLAG_IN),
    FLAG_NAME(NODE_PARAM_FLAG_OUT),
    FLAG_NAME(NODE_PARAM_FLAG_NAMED),
    FLAG_NAME(NODE_PARAM_FLAG_REST),
    FLAG_NAME(NODE_PARAM_FLAG_UNCHECKED),
    FLAG_NAME(NODE_PARAM_FLAG_UNPROTOTYPED),
    FLAG_NAME(NODE_PARAM_FLAG_REFERENCED),
    FLAG_NAME(NODE_PARAM_FLAG_PARAMREF),
    FLAG_NAME(NODE_PARAM_FLAG_CATCH),
    FLAG_NAME(NODE_PARAM_MATCH_FLAG_UNPROTOTYPED),
    FLAG_NAME(NODE_PARAM_MATCH_FLAG_PROTOTYPE_UNCHECKED),
    FLAG_NAME(NODE_SWITCH_FLAG_DEFAULT),
    FLAG_NAME(NODE_TYPE_FLAG_MODULO),
    FLAG_NAME(NODE_VARIABLE_FLAG_CONST),
    FLAG_NAME(NODE_VARIABLE_FLAG_FINAL),
    FLAG_NAME(NODE_VARIABLE_FLAG_LOCAL),
    FLAG_NAME(NODE_VARIABLE_FLAG_MEMBER),
    FLAG_NAME(NODE_VARIABLE_FLAG_ATTRIBUTES),
    FLAG_NAME(NODE_VARIABLE_FLAG_ENUM),
    FLAG_NAME(NODE_VARIABLE_FLAG_COMPILED),
    FLAG_NAME(NODE_VARIABLE_FLAG_INUSE),
    FLAG_NAME(NODE_VARIABLE_FLAG_ATTRS),
    FLAG_NAME(NODE_VARIABLE_FLAG_DEFINED),
    FLAG_NAME(NODE_VARIABLE_FLAG_DEFINING),
    FLAG_NAME(NODE_VARIABLE_FLAG_TOADD),
    FLAG_NAME(NODE_VARIABLE_FLAG_TEMPORARY),
    FLAG_NAME(NODE_VARIABLE_FLAG_NOINIT),
    FLAG_NAME(NODE_VARIABLE_FLAG_VARIABLE),
};
#pragma GCC diagnostic pop


}
// no name namespace




/** \brief Get the name of the flag as a string.
 *
 * This function returns the flag name as a string.
 *
 * \param[in] f  The flag to convert to a string.
 *
 * \return A pointer to a string with the flag name.
 *
 * \sa get_flag()
 * \sa set_flag()
 * \sa verify_flag()
 * \sa compare_all_flags()
 */
char const * node::flag_to_string(flag_t f)
{
    if(f >= flag_t::NODE_FLAG_max) [[unlikely]]
    {
        throw internal_error(
              "unknown flag number "
            + std::to_string(static_cast<int>(f))
            + " (out of range).");
    }
#ifdef _DEBUG
    if(g_flag_name[static_cast<std::size_t>(f)] == nullptr) [[unlikely]]
    {
        throw internal_error(
              "flag number "
            + std::to_string(static_cast<int>(f))
            + " not defined in our array of strings.");
    }
#endif
    return g_flag_name[static_cast<std::size_t>(f)];
}


/** \brief Get the current status of a flag.
 *
 * This function returns true or false depending on the current status
 * of the specified flag.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the node type we are dealing with.
 *
 * If the flag was never set, this function returns false.
 *
 * compare_all_flags() can be used to compare all the flags at once
 * without having to load each flag one at a time. This is particularly
 * useful in our unit tests.
 *
 * \param[in] f  The flag to retrieve.
 *
 * \return true if the flag was set to true, false otherwise.
 *
 * \sa set_flag()
 * \sa verify_flag()
 * \sa compare_all_flags()
 */
bool node::get_flag(flag_t f) const
{
    verify_flag(f);
    return f_flags[static_cast<size_t>(f)];
}


/** \brief Set a flag.
 *
 * This function sets the specified flag \p f to the specified value \p v
 * in this node object.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the node type we are dealing with.
 *
 * \param[in] f  The flag to set.
 * \param[in] v  The new value for the flag.
 *
 * \sa get_flag()
 * \sa verify_flag()
 */
void node::set_flag(flag_t f, bool v)
{
    verify_flag(f);
    f_flags[static_cast<size_t>(f)] = v;
}


/** \brief Verify that f corresponds to the node type.
 *
 * This function verifies that \p f corresponds to a valid flag according
 * to the type of this node object.
 *
 * \todo
 * Move some of the external tests (tests done by code in other
 * places like the parser) to here because some flags are
 * mutally exclusive and we should prevent such from being set
 * simultaneously.
 *
 * \exception internal_error
 * This function checks that the flag is allowed in the type of node.
 * If not, this exception is raised because that represents a compiler
 * bug.
 *
 * \param[in] f  The flag to check.
 *
 * \sa set_flag()
 * \sa get_flag()
 */
void node::verify_flag(flag_t f) const
{
    switch(f)
    {
    case flag_t::NODE_CATCH_FLAG_TYPED:
        if(f_type == node_t::NODE_CATCH)
        {
            return;
        }
        break;

    case flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES:
        if(f_type == node_t::NODE_DIRECTIVE_LIST)
        {
            return;
        }
        break;

    case flag_t::NODE_ENUM_FLAG_CLASS:
    case flag_t::NODE_ENUM_FLAG_INUSE:
        if(f_type == node_t::NODE_ENUM)
        {
            return;
        }
        break;

    case flag_t::NODE_FOR_FLAG_CONST:
    case flag_t::NODE_FOR_FLAG_FOREACH:
    case flag_t::NODE_FOR_FLAG_IN:
        if(f_type == node_t::NODE_FOR)
        {
            return;
        }
        break;

    case flag_t::NODE_FUNCTION_FLAG_GETTER:
    case flag_t::NODE_FUNCTION_FLAG_NEVER:
    case flag_t::NODE_FUNCTION_FLAG_NOPARAMS:
    case flag_t::NODE_FUNCTION_FLAG_OUT:
    case flag_t::NODE_FUNCTION_FLAG_SETTER:
    case flag_t::NODE_FUNCTION_FLAG_VOID:
        if(f_type == node_t::NODE_FUNCTION)
        {
            return;
        }
        break;

    case flag_t::NODE_FUNCTION_FLAG_OPERATOR:
        if(f_type == node_t::NODE_FUNCTION
        || f_type == node_t::NODE_CALL)
        {
            return;
        }
        break;

    case flag_t::NODE_IDENTIFIER_FLAG_OPERATOR:
        if(f_type == node_t::NODE_IDENTIFIER    // TBD: I use identifiers for the member operators but maybe that is wrong?
        || f_type == node_t::NODE_VIDENTIFIER)  // TBD: I use identifiers for the member operators but maybe that is wrong?
        {
            return;
        }
        break;

    case flag_t::NODE_IDENTIFIER_FLAG_WITH:
    case flag_t::NODE_IDENTIFIER_FLAG_TYPED:
        if(f_type == node_t::NODE_CLASS
        || f_type == node_t::NODE_IDENTIFIER
        || f_type == node_t::NODE_VIDENTIFIER
        || f_type == node_t::NODE_STRING)
        {
            return;
        }
        break;

    case flag_t::NODE_IMPORT_FLAG_IMPLEMENTS:
        if(f_type == node_t::NODE_IMPORT)
        {
            return;
        }
        break;

    case flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS:
    case flag_t::NODE_PACKAGE_FLAG_REFERENCED:
        if(f_type == node_t::NODE_PACKAGE)
        {
            return;
        }
        break;

    case flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED:
    case flag_t::NODE_PARAM_MATCH_FLAG_PROTOTYPE_UNCHECKED:
        if(f_type == node_t::NODE_PARAM_MATCH)
        {
            return;
        }
        break;

    case flag_t::NODE_PARAM_FLAG_CATCH:         // a parameter defined in a catch()
    case flag_t::NODE_PARAM_FLAG_CONST:
    case flag_t::NODE_PARAM_FLAG_IN:
    case flag_t::NODE_PARAM_FLAG_OUT:
    case flag_t::NODE_PARAM_FLAG_NAMED:
    case flag_t::NODE_PARAM_FLAG_PARAMREF:      // referenced from another parameter
    case flag_t::NODE_PARAM_FLAG_REFERENCED:    // referenced from a parameter or a variable
    case flag_t::NODE_PARAM_FLAG_REST:
    case flag_t::NODE_PARAM_FLAG_UNCHECKED:
    case flag_t::NODE_PARAM_FLAG_UNPROTOTYPED:
        if(f_type == node_t::NODE_PARAM)
        {
            return;
        }
        break;

    case flag_t::NODE_SWITCH_FLAG_DEFAULT:           // we found a 'default:' label in that switch
        if(f_type == node_t::NODE_SWITCH)
        {
            return;
        }
        break;

    case flag_t::NODE_TYPE_FLAG_MODULO:             // type ... as mod ...;
        if(f_type == node_t::NODE_TYPE)
        {
            return;
        }
        break;

    case flag_t::NODE_VARIABLE_FLAG_CONST:
    case flag_t::NODE_VARIABLE_FLAG_FINAL:
    case flag_t::NODE_VARIABLE_FLAG_LOCAL:
    case flag_t::NODE_VARIABLE_FLAG_MEMBER:
    case flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES:
    case flag_t::NODE_VARIABLE_FLAG_ENUM:                 // there is a NODE_SET and it somehow needs to be copied
    case flag_t::NODE_VARIABLE_FLAG_COMPILED:             // Expression() was called on the NODE_SET
    case flag_t::NODE_VARIABLE_FLAG_INUSE:                // this variable was referenced
    case flag_t::NODE_VARIABLE_FLAG_ATTRS:                // currently being read for attributes (to avoid loops)
    case flag_t::NODE_VARIABLE_FLAG_DEFINED:              // was already parsed
    case flag_t::NODE_VARIABLE_FLAG_DEFINING:             // currently defining, can't read
    case flag_t::NODE_VARIABLE_FLAG_TOADD:                // to be added in the directive list
    case flag_t::NODE_VARIABLE_FLAG_TEMPORARY:
    case flag_t::NODE_VARIABLE_FLAG_NOINIT:
    case flag_t::NODE_VARIABLE_FLAG_VARIABLE:
        if(f_type == node_t::NODE_VAR
        || f_type == node_t::NODE_VARIABLE
        || f_type == node_t::NODE_VAR_ATTRIBUTES)
        {
            return;
        }
        break;

    [[unlikely]] case flag_t::NODE_FLAG_max:
        break;

    // default: -- do not define so the compiler can tell us if
    //             an enumeration item is missing in this case
    }

    // since we do not use 'default' completely invalid values are not caught
    // in the switch...
    //
    std::stringstream ss;
    char const * flag_name("<out of range>");
    if(f < flag_t::NODE_FLAG_max)
    {
        flag_name = flag_to_string(f);
    }
    ss << "node_flag.cpp: node::verify_flag(): flag ("
       << flag_name
       << '/'
       << std::to_string(static_cast<int>(f))
       << ") / type mismatch ("
       << node::type_to_string(f_type)
       << ':'
       << std::to_string(static_cast<int>(f_type))
       << ") for node:\n"
       << *this
       << "\n";
    throw internal_error(ss.str());
}


/** \brief Compare a set of flags with the current flags of this node.
 *
 * This function compares the specified set of flags with the node's
 * flags. If the sets are equal, then the function returns true.
 * Otherwise the function returns false.
 *
 * This function compares all the flags, whether or not they are
 * valid for the current node type.
 *
 * \param[in] s  The set of flags to compare with.
 *
 * \return true if \p s is equal to the node flags.
 *
 * \sa get_flag()
 */
bool node::compare_all_flags(flag_set_t const& s) const
{
    return f_flags == s;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
