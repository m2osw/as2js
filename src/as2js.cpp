/* src/as2js.cpp

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

// self
//
#include    "license.h"

// as2js lib
//
// need to change to compiler.h once it compiles
#include    "as2js/parser.h"
#include    "as2js/as2js.h"

// advgetopt lib
//
#include    <advgetopt/advgetopt.h>


/** \file
 * \brief This file is the actual as2js compiler.
 *
 * The project includes a library which does 99% of the work. This is
 * the implementation of the as2js command line tool that handles
 * command line options and initializes an Option object with those
 * before starting compiling various .js files.
 */




/** \brief Private implementations of the as2js compiler, the actual tool.
 *
 * This namespace is used to hide all the tool private functions to
 * avoid any conflicts.
 */
namespace
{


/** \brief Command line options.
 *
 * This table includes all the options supported by the compiler.
 */
constexpr advgetopt::option g_options[] =
{
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_ALIAS,
        "licence",
        "license",
        nullptr, // hide from help output
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "license",
        nullptr,
        "Print out the license of this command line tool.",
        nullptr
    },
    {
        'h',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "Show version and exit.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_DEFAULT_OPTION | advgetopt::GETOPT_FLAG_MULTIPLE,
        "filename",
        nullptr,
        nullptr, // hidden argument in --help screen
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};



// TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "as2js",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "AS2JSFLAGS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = 0,
    .f_help_header = "Usage: %p [--<opt>] <source>.as ...\n"
                     "Where --<opt> is one or more of:",
    .f_help_footer = nullptr,
    .f_version = AS2JS_VERSION,
    .f_license = nullptr,
    .f_copyright = nullptr,
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};



} // no name namespace


class as2js_compiler
{
public:
    typedef std::shared_ptr<as2js_compiler>         pointer_t;
    typedef std::shared_ptr<advgetopt::getopt>      getopt_ptr_t;

    as2js_compiler(int argc, char *argv[]);

private:
    getopt_ptr_t        f_opt = getopt_ptr_t();
};


as2js_compiler::as2js_compiler(int argc, char *argv[])
{
    // The library takes care of these possibilities with the .rc file
    //if(g_configuration_files.empty())
    //{
    //    g_configuration_files.push_back("~/.config/as2js/as2js.rc");
    //    g_configuration_files.push_back("/etc/as2js/as2js.rc");
    //}
    f_opt.reset(
        new advgetopt::getopt(g_options_environment, argc, argv)
    );

    if(f_opt->is_defined("help"))
    {
        std::cerr << f_opt->usage();
        exit(1);
    }

    if(f_opt->is_defined("license")      // English
    || f_opt->is_defined("licence"))     // French
    {
        as2js_tools::license::license();
        exit(1);
    }

    if(f_opt->is_defined("version"))
    {
        std::cout << f_opt->get_program_name() << " v" << AS2JS_VERSION << std::endl
                << "libas2js v" << as2js::as2js_library_version() << std::endl;
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    try
    {
        as2js_compiler::pointer_t c(new as2js_compiler(argc, argv));
    }
    catch(std::exception const& e)
    {
        std::cerr << "as2js: exception: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
