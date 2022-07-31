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

// self
//
#define CATCH_CONFIG_RUNNER
#include    "catch_main.h"

#include    "license.h"


// as2js
//
#include    <as2js/version.h>


// libexcept
//
#include    <libexcept/exception.h>


//// advgetopt
////
//#include    <advgetopt/advgetopt.h>
//
//
// snapdev
//
#include    <snapdev/not_used.h>


//// C
////
//#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




namespace SNAP_CATCH2_NAMESPACE
{

std::string     g_as2js_compiler;
bool            g_run_stdout_destructive = false;
bool            g_save_parser_tests = false;
bool            g_license = false;

} // namespace SNAP_CATCH2_NAMESPACE


Catch::Clara::Parser add_command_line_options(Catch::Clara::Parser const & cli)
{
    return cli
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_as2js_compiler, "as2js")
              ["--as2js"]
              ("path to the as2js compiler.")
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_run_stdout_destructive)
              ["--destructive"]
              ("also run the stdout destructive test (otherwise skip the test so we do not lose stdout).")
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_license)
              ["--license"]
              ("prints out the license of the tests.")
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_save_parser_tests)
              ["--save-parser-tests"]
              ("save the JSON used to test the parser.");
}


int init_test(Catch::Session & session)
{
    snapdev::NOT_USED(session);

    if(SNAP_CATCH2_NAMESPACE::g_license)
    {
        as2js_tools::license::license();
        return 1;
    }

    return 0;
}


int main(int argc, char * argv[])
{
    return SNAP_CATCH2_NAMESPACE::snap_catch2_main(
              "as2js"
            , AS2JS_VERSION_STRING
            , argc
            , argv
            , []() { libexcept::set_collect_stack(libexcept::collect_stack_t::COLLECT_STACK_NO); }
            , &add_command_line_options
            , &init_test
        );
}


// vim: ts=4 sw=4 et
