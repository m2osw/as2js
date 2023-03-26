// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief The version of the as2js at compile time.
 *
 * This file records the as2js library version at compile time.
 *
 * The macros give you the library version at the time you are compiling.
 * The functions allow you to retrieve the version of a dynamically linked
 * library.
 */

// self
//
#include    "as2js/version.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{




/** \brief Get the major version of the library
 *
 * This function returns the major version of the running library (the
 * one you are linked against at runtime).
 *
 * \return The major version.
 */
int get_major_version()
{
    return AS2JS_VERSION_MAJOR;
}


/** \brief Get the minor version of the library.
 *
 * This function returns the minor version of the running library
 * (the one you are linked against at runtime).
 *
 * \return The release version.
 */
int get_release_version()
{
    return AS2JS_VERSION_MINOR;
}


/** \brief Get the patch version of the library.
 *
 * This function returns the patch version of the running library
 * (the one you are linked against at runtime).
 *
 * \return The patch version.
 */
int get_patch_version()
{
    return AS2JS_VERSION_PATCH;
}


/** \brief Get the full version of the library as a string.
 *
 * This function returns the major, minor, and patch versions of the
 * running library (the one you are linked against at runtime) in the
 * form of a string.
 *
 * The build version is not made available. In most cases we change
 * the build version only to run a new build, so not code will have
 * changed (some documentation and non-code files may changed between
 * build versions; but the code will work exactly the same way.)
 *
 * \return The library version.
 */
char const * get_version_string()
{
    return AS2JS_VERSION_STRING;
}


/** \brief JavaScript version this as2js instance is compatible with.
 *
 * This function returns a version of JavaScript that this as2js
 * implementation is compatible with. This means you should be able
 * to generate JavaScript of that version or below with this instance.
 *
 * If the function returns -1, then it is still considered to not be
 * that compatible just yet.
 *
 * \return The major JavaScript version (5, 6, 7...) or -1 if not compatible.
 */
int get_js_compatibility_major_version()
{
    return JS_COMPATIBILITY_VERSION_MAJOR;
}


/** \brief Return the JavaScript compatibility version as a string.
 *
 * This function returned the JavaScript compatibility version as a string.
 * This is otherwise the same version as the
 * get_js_compatibility_major_version() function returns.
 *
 * \return The JavaScript compatibility version in a string.
 */
char const * get_js_compatibility_version_string()
{
    return JS_COMPATIBILITY_VERSION_STRING;
}


} // namespace ed
// vim: ts=4 sw=4 et
