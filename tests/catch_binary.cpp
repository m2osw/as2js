// Copyright (c) 2011-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    <as2js/binary.h>

//#include    <as2js/exception.h>
//#include    <as2js/message.h>


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/glob_to_list.h>
#include    <snapdev/pathinfo.h>
//#include    <snapdev/tokenize_string.h>


// libutf8
//
//#include    <libutf8/iterator.h>


// C++
//
//#include    <cstring>
//#include    <algorithm>
//#include    <iomanip>


// C
//
//#include    <signal.h>
//#include    <sys/wait.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{


void run_script(std::string const & s)
{
    std::string cmd("export AS2JS_RC='");
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "' && ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/tools/as2js -b -L ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/rt -o ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/tests/a.out ";
    cmd += s;
std::cerr << "--- pwd: [";
system("pwd");
std::cerr << "]\n";
std::cerr << "--- as2js command: [" << cmd << "]\n";

    // for the script to work, the compiler must find the scripts directory
    // which is defined in the rc file
    //
    {
        std::ofstream rc("as2js/as2js.rc");
        rc << "{\"scripts\":\""
           << SNAP_CATCH2_NAMESPACE::g_source_dir()
           << "/scripts\"}\n";
    }

    // first compile the file
    //
    int const r(system(cmd.c_str()));
    CATCH_REQUIRE(r == 0);
}


typedef std::map<std::string, std::string>      variable_t;

struct meta
{
    variable_t      f_variables = variable_t();
    std::string     f_result = std::string();
};


meta load_script_meta(std::string const & s)
{
    meta result;

    std::string const filename(snapdev::pathinfo::replace_suffix(s, ".ajs", ".meta"));

    snapdev::file_contents meta(filename);
    CATCH_REQUIRE(meta.read_all());

    std::string const & str(meta.contents());

    char const * m(str.c_str());
    while(*m != '\0')
    {
        while(isspace(*m))
        {
            ++m;
        }
        if(*m == '\0')
        {
            return result;
        }
        if(*m == '#')
        {
            // skip comments
            //
            while(*m != '\n')
            {
                if(*m == '\0')
                {
                    return result;
                }
                ++m;
            }
            continue;
        }

        if(*m == '(')
        {
            // expected result
            //   (<value>)
            //
            ++m;
            CATCH_REQUIRE(result.f_result.empty());
            while(*m != ')')
            {
                CATCH_REQUIRE(*m != '\n');
                CATCH_REQUIRE(*m != '\0');
                result.f_result += *m;
                ++m;
            }
            ++m;
            while(isspace(*m))
            {
                ++m;
            }
            if(*m != '\n')
            {
                CATCH_REQUIRE(*m == '\0');
                return result;
            }
            ++m;
            continue;
        }

        if(*m == '\n')
        {
            // empty line
            //
            ++m;
            continue;
        }

        // other we have either a variable:
        //   <name>=<value>
        //
        std::string name;
        std::string value;
        while(*m != '=')
        {
            CATCH_REQUIRE(*m != '\n');
            CATCH_REQUIRE(*m != '\0');
            name += *m;
            ++m;
        }
        ++m;
        while(*m != '\n' && *m != '\0')
        {
            value += *m;
            ++m;
        }

        result.f_variables[name] = value;
    }

    return result;
}


void execute(meta const & m)
{
    std::string filename(SNAP_CATCH2_NAMESPACE::g_binary_dir());
    filename += "/tests/a.out";

    as2js::running_file script;
    CATCH_REQUIRE(script.load(filename));

    for(auto const & var : m.f_variables)
    {
        // TODO: support all types of variables
        //
        std::int64_t const value(std::stoll(var.second, nullptr, 0));
        script.set_variable(var.first, value);
    }

    as2js::binary_result result;

    script.run(result);

std::cerr << "got result! " << result.get_integer() << " == " << m.f_result << "\n";
    std::int64_t const expected_result(std::stoll(m.f_result, nullptr, 0));
    CATCH_REQUIRE(result.get_integer() == expected_result);
}



}







CATCH_TEST_CASE("binary_integer_operators", "[binary][integer]")
{
    CATCH_START_SECTION("binary_integer_operators: test binary operator for integers")
    {
        snapdev::glob_to_list<std::list<std::string>> scripts;
        CATCH_REQUIRE(scripts.read_path<snapdev::glob_to_list_flag_t::GLOB_FLAG_NONE>
                                    (SNAP_CATCH2_NAMESPACE::g_source_dir()
                                        + "/tests/binary/integer_operator_*.ajs"));
        for(auto const & s : scripts)
        {
            run_script(s);
            meta m(load_script_meta(s));
            execute(m);
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
