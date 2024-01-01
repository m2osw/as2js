// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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

// as2js
//
#include    <as2js/version.h>


// self
//
#include    "catch_main.h"


// C++
//
#include    <cstring>
#include    <algorithm>


// last include
//
#include    <snapdev/poison.h>







CATCH_TEST_CASE("Version", "[version]")
{
    CATCH_START_SECTION("version: verify runtime vs compile time as2js version numbers")
    {
        CATCH_REQUIRE(as2js::get_major_version()   == AS2JS_VERSION_MAJOR);
        CATCH_REQUIRE(as2js::get_release_version() == AS2JS_VERSION_MINOR);
        CATCH_REQUIRE(as2js::get_patch_version()   == AS2JS_VERSION_PATCH);
        CATCH_REQUIRE(strcmp(as2js::get_version_string(), AS2JS_VERSION_STRING) == 0);
    }
    CATCH_END_SECTION()
}




// vim: ts=4 sw=4 et
