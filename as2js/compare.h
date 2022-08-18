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
#pragma once


namespace as2js
{


enum class compare_t
{
	COMPARE_EQUAL = 0,
	COMPARE_GREATER = 1,
	COMPARE_LESS = -1,
	COMPARE_UNORDERED = 2,
	COMPARE_ERROR = -2,
	COMPARE_UNDEFINED = -3		// not yet compared
};


namespace compare_utils
{
inline bool is_ordered(compare_t const c)
{
    return c == compare_t::COMPARE_EQUAL || c == compare_t::COMPARE_GREATER || c == compare_t::COMPARE_LESS;
}
}
// namespace compare_utils


} // namespace as2js
// vim: ts=4 sw=4 et
