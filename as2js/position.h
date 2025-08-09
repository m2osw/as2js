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

// self
//
#include    <as2js/string.h>


namespace as2js
{



class position
{
public:
    typedef int32_t         counter_t;
    static counter_t const  DEFAULT_COUNTER = 1;

    void                set_filename(std::string const & filename);
    void                set_function(std::string const & function);
    void                reset_counters(counter_t line = DEFAULT_COUNTER);
    void                new_page();
    void                new_paragraph();
    void                new_line();
    void                new_column();

    std::string         get_filename() const;
    std::string         get_function() const;
    counter_t           get_page() const;
    counter_t           get_page_line() const;
    counter_t           get_paragraph() const;
    counter_t           get_line() const;
    counter_t           get_column() const;

    bool                operator == (position const & rhs) const;

private:
    std::string         f_filename = std::string();
    std::string         f_function = std::string();
    counter_t           f_page = DEFAULT_COUNTER;
    counter_t           f_page_line = DEFAULT_COUNTER;
    counter_t           f_paragraph = DEFAULT_COUNTER;
    counter_t           f_line = DEFAULT_COUNTER;
    counter_t           f_column = DEFAULT_COUNTER;
};


/** \brief Print this position in the \p out stream.
 *
 * This function prints out this position in the \p out stream. We limit
 * the printing to the filename and the line number as most compilers
 * do. The other information is available for you to print additional
 * data if required.
 *
 * \code
 * <filename>:<line>:
 * \endcode
 *
 * \param[in,out] out  The stream to print to.
 * \param[in] pos  The position to print in the output.
 *
 * \return A reference to this stream.
 */
template<class _CharT, class _Traits>
inline std::basic_ostream<_CharT, _Traits> & operator << (std::basic_ostream<_CharT, _Traits> & out, position const & pos)
{
    if(pos.get_filename().empty())
    {
        out << "line " << pos.get_line() << ":";
    }
    else
    {
        out << pos.get_filename() << ":" << pos.get_line() << ":";
    }
    if(pos.get_column() != position::DEFAULT_COUNTER)
    {
        out << pos.get_column() << ":";
    }

    return out;
}


} // namespace as2js
// vim: ts=4 sw=4 et
