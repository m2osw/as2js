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
#include    <snapdev/to_lower.h>


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
//cmd += "gdb -ex \"catch throws\" -ex \"run\" -args ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/tools/as2js -b -L ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/rt -o ";
    cmd += SNAP_CATCH2_NAMESPACE::g_binary_dir();
    cmd += "/tests/a.out ";
    cmd += s;

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



enum class value_type_t : std::uint16_t
{
    VALUE_TYPE_UNDEFINED,
    VALUE_TYPE_BOOLEAN,
    VALUE_TYPE_INTEGER,
    VALUE_TYPE_FLOATING_POINT,
    VALUE_TYPE_STRING,
};

typedef std::uint16_t           flags_t;

constexpr flags_t const         VALUE_FLAG_IN  = 0x0000;
constexpr flags_t const         VALUE_FLAG_OUT = 0x0001;

struct value_flags
{
    std::string     f_value = std::string();
    value_type_t    f_type = value_type_t::VALUE_TYPE_UNDEFINED;
    flags_t         f_flags = 0;

    void set_type(value_type_t t)
    {
        CATCH_REQUIRE(f_type != value_type_t::VALUE_TYPE_UNDEFINED);
        f_type = t;
    }

    void set_type(std::string t)
    {
        t = snapdev::to_lower(t);
        if(t == "boolean")
        {
            set_type(value_type_t::VALUE_TYPE_BOOLEAN);
        }
        else if(t == "integer")
        {
            set_type(value_type_t::VALUE_TYPE_INTEGER);
        }
        else if(t == "double")
        {
            set_type(value_type_t::VALUE_TYPE_FLOATING_POINT);
        }
        else if(t == "string")
        {
            set_type(value_type_t::VALUE_TYPE_STRING);
        }
        else if(t == "in")
        {
            f_flags |= VALUE_FLAG_IN;
        }
        else if(t == "out")
        {
            f_flags |= VALUE_FLAG_OUT;
        }
        else
        {
            CATCH_REQUIRE(t == "");
        }
    }

    value_type_t get_type() const
    {
        return f_type == value_type_t::VALUE_TYPE_UNDEFINED
                    ? value_type_t::VALUE_TYPE_INTEGER
                    : f_type;
    }

    bool is_out() const
    {
        return (f_flags & VALUE_FLAG_OUT) != 0;
    }
};


typedef std::map<std::string, value_flags>      variable_t;

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
        while(*m == ' ' || *m == '\t')
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
            while(*m == ' ' || *m == '\t')
            {
//std::cerr << "--- result [" << result.f_result << "] -> skipping [" << static_cast<int>(*m) << "]\n";
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
        //   [<keyword>] <name>=<value>
        //
        std::string name;
        value_flags value;
        while(*m != '=')
        {
            CATCH_REQUIRE(*m != '\n');
            CATCH_REQUIRE(*m != '\0');
            if(isspace(*m))
            {
                // skip all spaces
                //
                do
                {
                    ++m;
                }
                while(*m == ' ' || *m == '\t');

                // check whether it is the last word, if so, it is taken as
                // the variable name
                //
                if(*m == '=')
                {
                    break;
                }

                // parse as a type or flag
                //
                value.set_type(name);
                name.clear();
            }
            else
            {
                name += *m;
                ++m;
            }
        }
        ++m;

        CATCH_REQUIRE_FALSE(name.empty());

        while(*m != '\n' && *m != '\0')
        {
            value.f_value += *m;
            ++m;
        }

        // when defining an "out" the name must be distinct otherwise it would
        // smash the "in"
        //
        if(value.is_out())
        {
            name = "<-" + name;
        }
//std::cerr << "--- found name [" << name << "]\n";
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
        if(!var.second.is_out())
        {
            // TODO: support all types of variables
            //
            std::int64_t const value(std::stoll(var.second.f_value, nullptr, 0));
//std::cerr << "+++ set variable \"" << var.first << "\": " << filename << "\n";
            script.set_variable(var.first, value);
        }
    }

    as2js::binary_result result;

    script.run(result);

    std::int64_t const expected_result(std::stoll(m.f_result, nullptr, 0));
    if(result.get_integer() != expected_result)
    {
        std::cerr << "--- (result) differs: " << result.get_integer() << " != " << expected_result << "\n";
    }
    CATCH_REQUIRE(result.get_integer() == expected_result);

    for(auto const & var : m.f_variables)
    {
        if(var.second.is_out())
        {
            // TODO: support all types of variables
            //
            std::int64_t const expected_value(std::stoll(var.second.f_value, nullptr, 0));
            std::int64_t returned_value(0);
            std::string name(var.first.substr(2));
            script.get_variable(name, returned_value);
            if(returned_value != expected_value)
            {
                std::cerr
                    << "--- invalid result in \""
                    << var.first
                    << "\".\n";
            }
            CATCH_REQUIRE(returned_value == expected_value);
        }
    }
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
