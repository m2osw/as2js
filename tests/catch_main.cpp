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



// as2js
//
#include    <as2js/version.h>


// libexcept
//
#include    <libexcept/exception.h>


// snapdev
//
#include    <snapdev/not_used.h>
#include    <snapdev/mkdir_p.h>


// C
//
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>




namespace SNAP_CATCH2_NAMESPACE
{

std::string     g_as2js_compiler;
bool            g_run_stdout_destructive = false;
bool            g_save_parser_tests = false;

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
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_save_parser_tests)
              ["--save-parser-tests"]
              ("save the JSON used to test the parser.");
}


int init_test(Catch::Session & session)
{
    snapdev::NOT_USED(session);

    // in our snapcpp environment, the default working directory for our
    // tests is the source directory (${CMAKE_SOURCE_DIR}); the as2js tests
    // want to create folders and files inside the current working directory
    // so instead we'd like to be in the temporary directory so change that
    // now at the start
    //
    std::string tmp_dir(SNAP_CATCH2_NAMESPACE::g_tmp_dir());
    if(tmp_dir.empty())
    {
        // there is a default set to:
        //    /tmp/<project-name>
        // so this should never happen
        //
        std::cerr << "error: a temporary directory must be specified.\n";
        return 1;
    }

    int const r(chdir(tmp_dir.c_str()));
    if(r != 0)
    {
        std::cerr << "error: could not change working directory to \""
            << tmp_dir
            << "\"; the directory must exist.\n";
        return 1;
    }
    char * cwd(get_current_dir_name());
    if(cwd == nullptr)
    {
        std::cerr << "error: could not retrieve the current working directory.\n";
        return 1;
    }
    SNAP_CATCH2_NAMESPACE::g_tmp_dir() = cwd;
    free(cwd);

    // update this path because otherwise the $HOME variable is going to be
    // wrong (i.e. a relative path from within said relative path is not
    // likely to work properly)
    //
    tmp_dir = SNAP_CATCH2_NAMESPACE::g_tmp_dir();

    // the snapcatch2 ensures an empty tmp directory so this should just
    // never happen ever...
    //
    struct stat st;
    if(stat("debian", &st) == 0)
    {
        std::cerr << "error: unexpected \"debian\" directory in the temporary directory;"
            " you cannot safely specify the source directory as the temporary directory.\n";
        return 1;
    }
    if(stat("as2js/CMakeLists.txt", &st) == 0)
    {
        std::cerr << "error: unexpected \"as2js/CMakeLists.txt\" file in the temporary directory;"
            " you cannot safely specify the source directory as the temporary directory.\n";
        return 1;
    }

    // move HOME to a sub-directory inside the temporary directory so that
    // way it is safe (we can change files there without the risk of
    // destroying some of the developer's files)
    //
    if(snapdev::mkdir_p("home") != 0)
    {
        std::cerr
            << "error: could not create a \"home\" directory in the temporary directory: \""
            << tmp_dir
            << "\".\n";
        return 1;
    }
    std::string const home(tmp_dir + "/home");
    setenv("HOME", home.c_str(), 1);

    // some other "modules" that require some initialization
    //
    if(SNAP_CATCH2_NAMESPACE::catch_rc_init() != 0)
    {
        return 1;
    }
    if(SNAP_CATCH2_NAMESPACE::catch_db_init() != 0)
    {
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
