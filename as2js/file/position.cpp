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
#include    "as2js/position.h"

#include    "as2js/exception.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/** \brief Set the filename being read.
 *
 * This function saves the name of the file being read if available.
 *
 * \todo
 * Test that the filename is valid (no '\0', mainly.)
 *
 * \param[in] filename  The name of the file being compiled.
 */
void position::set_filename(std::string const & filename)
{
    f_filename = filename;
}


/** \brief Set the function being read.
 *
 * This function saves the name of the function being read if available.
 * The compiler is capable of detecting which function is being read and
 * keeps a stack of such (since a function can be declared within another.)
 * Functions without a name are given a system name for the purpose of
 * displaying errors giving us as much information as possible.
 *
 * \param[in] function  The name of the function being compiled.
 */
void position::set_function(std::string const & function)
{
    f_function = function;
}


/** \brief Reset the counter.
 *
 * This function resets all the counters to 1 except for the line which
 * is set to the specified \p line parameter (which defaults to 1.)
 *
 * \exception exception_internal_error
 * This exception is raised if the \p line prameter is smaller than 1.
 *
 * \param[in] line  The line number to start with. Defaults to 1.
 */
void position::reset_counters(counter_t line)
{
    if(line < 1)
    {
        throw internal_error("the line parameter of the position object cannot be less than 1.");
    }

    f_page = DEFAULT_COUNTER;
    f_page_line = DEFAULT_COUNTER;
    f_paragraph = DEFAULT_COUNTER;
    f_line = line;
    f_column = DEFAULT_COUNTER;
}


/** \brief Increment the page counter by 1.
 *
 * This function increments the page counter by one, resets the page
 * line to 1 and the paragraph to 1.
 */
void position::new_page()
{
    ++f_page;
    f_page_line = DEFAULT_COUNTER;
    f_paragraph = DEFAULT_COUNTER;
}


/** \brief Increments the paragraph counter by 1.
 *
 * When the compiler detects the end of a paragraph, it calls this function
 * to increment that counter by one. Paragraphs are counted within one page.
 */
void position::new_paragraph()
{
    ++f_paragraph;
}


/** \brief Increment the line counter by 1.
 *
 * This function increases the file as a whole line counter by 1. It also
 * increments the page line counter by 1.
 */
void position::new_line()
{
    ++f_page_line;
    ++f_line;
    f_column = DEFAULT_COUNTER;
}


/** \brief Increment the column counter by 1.
 *
 * This function increases the current line column number by 1.
 */
void position::new_column()
{
    ++f_column;
}


/** \brief Retrieve the filename.
 *
 * This function returns the filename as set by the set_filename() function.
 *
 * It is possible for the filename to be empty (in case you are compiling
 * a function from memory.)
 *
 * \return The current filename.
 */
std::string position::get_filename() const
{
    return f_filename;
}


/** \brief Retrieve the function name.
 *
 * This function returns the function name as set by the set_function()
 * function.
 *
 * It is possible for the function name to be empty (before it was ever set.)
 *
 * \return The current function name.
 */
std::string position::get_function() const
{
    return f_function;
}


/** \brief Retrieve the current page counter.
 *
 * The page counter is incremented by one after X number of lines or when
 * a Ctrl-L character is found in the input stream.
 *
 * \return The page number.
 */
position::counter_t position::get_page() const
{
    return f_page;
}


/** \brief Retrieve the current page line counter.
 *
 * The page line counter is incremented by one every time a new line
 * character is found. It starts at 1. It is reset back to one each
 * time a new page is found.
 *
 * \return The page line number.
 */
position::counter_t position::get_page_line() const
{
    return f_page_line;
}


/** \brief Retrieve the current paragraph counter.
 *
 * The paragraph counter is incremented by one every time empty
 * lines are found between blocks of non empty lines. It starts at 1.
 * It is reset back to one each time a new page is found.
 *
 * \return The paragraph number.
 */
position::counter_t position::get_paragraph() const
{
    return f_paragraph;
}


/** \brief Retrieve the current line counter.
 *
 * The line counter is reset to 1 (or some other value) at the start and
 * then it increases by 1 each time a new line character is found. It
 * does not get reset on anything. It is generally useful when using a
 * text editor as it represents the line number in such an editor.
 *
 * \return The current line number.
 */
position::counter_t position::get_line() const
{
    return f_line;
}


/** \brief Retrieve the current column counter.
 *
 * The column counter is reset to 1 (or some other value) at the start of
 * each line and incremented each time a character is read.
 *
 * \return The current column number.
 */
position::counter_t position::get_column() const
{
    return f_column;
}


bool position::operator == (position const & rhs) const
{
    return f_filename == rhs.f_filename
        && f_function == rhs.f_function
        && f_page == rhs.f_page
        && f_page_line == rhs.f_page_line
        && f_paragraph == rhs.f_paragraph
        && f_line == rhs.f_line
        && f_column == rhs.f_column;
}


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
std::ostream & operator << (std::ostream & out, position const & pos)
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
