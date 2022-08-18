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
#include    "as2js/os_raii.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


/** \class raii_stream_flags
 * \brief A class used to safely handle stream flags, width, and precision.
 *
 * Create an object of this type on your stack, and the flags, width,
 * and precision of your standard streams will be safe-guarded.
 *
 * See the constructor for an example.
 */


/** \brief Save the current format flags, width, and precision of a stream.
 *
 * This function saves the flags, precision, and width of a
 * stream inside this object so as to restore them later.
 *
 * The destructor will automatically restore the flags. The
 * restore() function can also be called early, although that
 * will eventually break the RAII feature since restore only
 * restores the flags once. Further calls to the restore()
 * function do nothing.
 *
 * To use:
 *
 * \code
 *   {
 *      as2js::raii_stream_flags stream_flags(std::cout);
 *      ...
 *      // this changes the flags to write numbers in hexadecimal
 *      std::cout << std::hex << 123 << ...;
 *      ...
 *   } // here all flags, width, precision get restored automatically
 * \endcode
 *
 * \bug
 * This class does not know about the fill character.
 *
 * \param[in] s  The stream of which flags are to be saved.
 */
raii_stream_flags::raii_stream_flags(std::ios_base & s)
    : f_stream(&s)
    , f_flags(s.flags())
    , f_precision(s.precision())
    , f_width(s.width())
{
}


/** \brief Restore the flags, width, and precision of a stream.
 *
 * The destructor automatically restores the stream flags, width,
 * and precision when called. Putting such an object on the stack
 * is the safest way to make sure that your function does not leak
 * the stream flags, width, and precision.
 *
 * This function calls the restore() function. Note that restore()
 * has no effect when called more than once.
 */
raii_stream_flags::~raii_stream_flags()
{
    restore();
}


/** \brief The restore function copies the flags, width, and precision
 *         back in the stream.
 *
 * This function restores the flags, width, and precision of the stream
 * as they were when the object was passed to the constructor of this
 * object.
 *
 * The function can be called any number of time, however, it only
 * restores the flags, width, and precision the first time it is called.
 *
 * In most cases, you want to let your raii_stream_flags object
 * destructor call this restore() function automatically, although
 * you may need to restore the format early once in a while.
 */
void raii_stream_flags::restore()
{
    if(f_stream)
    {
        f_stream->flags(f_flags);
        f_stream->precision(f_precision);
        f_stream->width(f_width);
        f_stream = nullptr;
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
