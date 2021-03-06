#ifndef AS2JS_LICENSE_H
#define AS2JS_LICENSE_H
/* src/license.h

Copyright (c) 2005-2019  Made to Order Software Corp.  All Rights Reserved

https://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/** \file
 * \brief Declaration of the license function.
 *
 * This header defines the license function so all our tools can display
 * the same license without the need for us to redefine the license in each
 * tool.
 */

namespace as2js_tools
{
namespace license
{

void license();

}
// namespace license
}
// namespace as2js_tools

#endif
// #ifndef AS2JS_LICENSE_H
// vim: ts=4 sw=4 et
