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
#include    "as2js/node.h"

#include    "as2js/exception.h"


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Compare two nodes against each others.
 *
 * This function is used to compare two nodes against each others. The
 * compare is expected to return a compare_t enumeration value.
 *
 * At this time, the implementation only compares basic literals (i.e.
 * integers, floating points, strings, Booleans, null, undefined.)
 *
 * \todo
 * Implement a compare of object and array literals.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE COMPARE  ***************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Compare two nodes together.
 *
 * This function returns the result of comparing two nodes against each
 * others. The result is one of the compare_t values.
 *
 * At this time, if the function is used to compare nodes that are not
 * literals, then it returns COMPARE_ERROR.
 *
 * The function may return COMPARE_UNORDERED in strict mode or when
 * comparing a value against a NaN.
 *
 * As per the ECMAScript refence, strings are compared as is in binary
 * mode. We do not make use of Unicode or take the locale in account.
 *
 * \note
 * The compare is expected to work as defined in ECMAScript 5 (see 11.8.5,
 * 11.9.3, and 11.9.6).
 *
 * The nearly equal is only used by the smart match operator. This is an
 * addition by as2js which is somewhat like the ~~ operator defined by
 * perl.
 *
 * \param[in] lhs  The left hand side node.
 * \param[in] rhs  The right hand side node.
 * \param[in] mode  Whether the compare is strict, lousy, or smart
 *                  (===, ==, ~~).
 *
 * \return One of the compare_t values representing the comparison result.
 */
compare_t node::compare(node::pointer_t const lhs, node::pointer_t const rhs, compare_mode_t const mode)
{
    if(!lhs->is_literal() || !rhs->is_literal())
    {
        // invalid left or right hand side
        return compare_t::COMPARE_ERROR;
    }

    // since we do not have a NODE_BOOLEAN, but instead have NODE_TRUE
    // and NODE_FALSE, we have to handle that case separately
    if(lhs->f_type == node_t::NODE_FALSE)
    {
        if(rhs->f_type == node_t::NODE_FALSE)
        {
            return compare_t::COMPARE_EQUAL;
        }
        if(rhs->f_type == node_t::NODE_TRUE)
        {
            return compare_t::COMPARE_LESS;
        }
    }
    else if(lhs->f_type == node_t::NODE_TRUE)
    {
        if(rhs->f_type == node_t::NODE_FALSE)
        {
            return compare_t::COMPARE_GREATER;
        }
        if(rhs->f_type == node_t::NODE_TRUE)
        {
            return compare_t::COMPARE_EQUAL;
        }
    }

    // exact same type?
    if(lhs->f_type == rhs->f_type)
    {
        switch(lhs->f_type)
        {
        case node_t::NODE_FLOATING_POINT:
            // NaN is a special case we have to take in account
            if(mode == compare_mode_t::COMPARE_SMART
            && lhs->get_floating_point().nearly_equal(rhs->get_floating_point()))
            {
                return compare_t::COMPARE_EQUAL;
            }
            return lhs->get_floating_point().compare(rhs->get_floating_point());

        case node_t::NODE_INTEGER:
            return lhs->get_integer().compare(rhs->get_integer());

        case node_t::NODE_NULL:
            return compare_t::COMPARE_EQUAL;

        case node_t::NODE_STRING:
            if(lhs->f_str == rhs->f_str)
            {
                return compare_t::COMPARE_EQUAL;
            }
            return lhs->f_str < rhs->f_str ? compare_t::COMPARE_LESS : compare_t::COMPARE_GREATER;

        case node_t::NODE_UNDEFINED:
            return compare_t::COMPARE_EQUAL;

        default: // LCOV_EXCL_LINE
            throw internal_error("comparing two literal nodes with an unknown type."); // LCOV_EXCL_LINE

        }
        /*NOTREACHED*/
    }

    // if strict mode is turned on, we cannot compare objects
    // that are not of the same type (i.e. no conversions allowed)
    if(mode == compare_mode_t::COMPARE_STRICT)
    {
        return compare_t::COMPARE_UNORDERED;
    }

    // we handle one special case here since 'undefined' is otherwise
    // converted to NaN and it would not be equal to 'null' which is
    // represented as being equal to zero
    if((lhs->f_type == node_t::NODE_NULL && rhs->f_type == node_t::NODE_UNDEFINED)
    || (lhs->f_type == node_t::NODE_UNDEFINED && rhs->f_type == node_t::NODE_NULL))
    {
        return compare_t::COMPARE_EQUAL;
    }

    // if we are here, we have go to convert both parameters to floating
    // point numbers and compare the result (remember that we do not handle
    // objects in this functon)
    floating_point lf;
    switch(lhs->f_type)
    {
    case node_t::NODE_INTEGER:
        lf.set(lhs->f_int.get());
        break;

    case node_t::NODE_FLOATING_POINT:
        lf = lhs->f_float;
        break;

    case node_t::NODE_TRUE:
        lf.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        lf.set(0.0);
        break;

    case node_t::NODE_STRING:
        lf.set(as2js::to_floating_point(lhs->f_str));
        break;

    case node_t::NODE_UNDEFINED:
        lf.set_nan();
        break;

    default: // LCOV_EXCL_LINE
        // failure (cannot convert)
        throw internal_error("could not convert a literal node to a floating point (lhs)."); // LCOV_EXCL_LINE

    }

    floating_point rf;
    switch(rhs->f_type)
    {
    case node_t::NODE_INTEGER:
        rf.set(rhs->f_int.get());
        break;

    case node_t::NODE_FLOATING_POINT:
        rf = rhs->f_float;
        break;

    case node_t::NODE_TRUE:
        rf.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        rf.set(0.0);
        break;

    case node_t::NODE_STRING:
        rf.set(as2js::to_floating_point(rhs->f_str));
        break;

    case node_t::NODE_UNDEFINED:
        rf.set_nan();
        break;

    default: // LCOV_EXCL_LINE
        // failure (cannot convert)
        throw internal_error("could not convert a literal node to a floating point (rhs)."); // LCOV_EXCL_LINE

    }

    if(mode == compare_mode_t::COMPARE_SMART
    && lf.nearly_equal(rf))
    {
        return compare_t::COMPARE_EQUAL;
    }

    return lf.compare(rf);
}


} // namespace as2js
// vim: ts=4 sw=4 et
