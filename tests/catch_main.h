// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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


// catch2
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include    <catch2/snapcatch2.hpp>
#pragma GCC diagnostic pop


// C++
//
#include    <string>
#include    <cstring>
#include    <cstdlib>
#include    <cmath>
#include    <limits>


namespace SNAP_CATCH2_NAMESPACE
{

extern  std::string     g_as2js_compiler;
extern  bool            g_run_stdout_destructive;
extern  bool            g_save_parser_tests;


extern int              catch_rc_init();
extern int              catch_db_init();

// use snapdev::transparent_setenv instead (same functionality)
//class obj_setenv
//{
//public:
//    obj_setenv(const std::string& var)
//        : f_copy(strdup(var.c_str()))
//    {
//        putenv(f_copy);
//        std::string::size_type p(var.find_first_of('='));
//        f_name = var.substr(0, p);
//    }
//
//    obj_setenv(obj_setenv const & rhs) = delete;
//
//    ~obj_setenv()
//    {
//        putenv(strdup((f_name + "=").c_str()));
//        free(f_copy);
//    }
//
//    obj_setenv & operator = (obj_setenv const & rhs) = delete;
//
//private:
//    char *      f_copy = nullptr;
//    std::string f_name = std::string();
//};

}
// namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
