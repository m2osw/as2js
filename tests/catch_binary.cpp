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



// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/glob_to_list.h>
#include    <snapdev/pathinfo.h>
#include    <snapdev/to_lower.h>


// C++
//
#include    <iomanip>


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
    cmd += "/tools/as2js -b -o ";
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
    std::cout
        << "--- compile script to binary with command \""
        << cmd
        << "\".\n";
    int const r(system(cmd.c_str()));
std::cerr << "-------------- system() called returned from binary test (" << r << ")\n";
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
        CATCH_REQUIRE(f_type == value_type_t::VALUE_TYPE_UNDEFINED); // trying to set the type more than once?
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
    value_flags     f_result = value_flags();
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

        if(*m == '\n')
        {
            // empty line
            //
            ++m;
            continue;
        }

        // we have either a variable or a result preceeded by keywords:
        //
        //   [<keyword>] (<result>)
        //   [<keyword>] <name>=<value>
        //
        std::string name;
        value_flags value;
        while(*m != '=' && *m != '(')
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

                // check whether it is the last word before the '=', if so,
                // it is taken as the variable name and not a <keyword>
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

        if(*m == '(')
        {
            CATCH_REQUIRE(name.empty());
            result.f_result = value;

            // expected result
            //   (<value>)
            //
            ++m;
            CATCH_REQUIRE(result.f_result.f_value.empty()); // prevent double definition
            while(*m != ')')
            {
                CATCH_REQUIRE(*m != '\n');
                CATCH_REQUIRE(*m != '\0');
                result.f_result.f_value += *m;
                ++m;
            }
            ++m;

            if(result.f_result.f_value.length() >= 2
            && (result.f_result.f_value[0] == '"' || result.f_result.f_value[0] == '\'')
            && result.f_result.f_value[0] == result.f_result.f_value.back()
            && result.f_result.get_type() == value_type_t::VALUE_TYPE_INTEGER)
            {
                // default to INTEGER, unless value is surrounded by quotes
                //
                result.f_result.set_type(value_type_t::VALUE_TYPE_STRING);
            }

            if(result.f_result.get_type() == value_type_t::VALUE_TYPE_STRING
            && result.f_result.f_value.length() >= 2
            && (result.f_result.f_value[0] == '"' || result.f_result.f_value[0] == '\'')
            && result.f_result.f_value[0] == result.f_result.f_value.back())
            {
                // remove quotes around string
                //
                result.f_result.f_value = result.f_result.f_value.substr(1, result.f_result.f_value.length() - 2);
            }

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
        ++m;    // skip '='

        CATCH_REQUIRE_FALSE(name.empty());

        while(*m != '\n' && *m != '\0')
        {
            value.f_value += *m;
            ++m;
        }

        // TODO: add support for spaces/tabs after the closing quotation
        //
        if(value.f_value.length() >= 2
        && (value.f_value[0] == '"' || value.f_value[0] == '\'')
        && value.f_value[0] == value.f_value.back()
        && value.get_type() == value_type_t::VALUE_TYPE_INTEGER)
        {
            // default to INTEGER, unless value is surrounded by quotes
            //
            value.set_type(value_type_t::VALUE_TYPE_STRING);
        }

        if(value.get_type() == value_type_t::VALUE_TYPE_STRING
        && value.f_value.length() >= 2
        && (value.f_value[0] == '"' || value.f_value[0] == '\'')
        && value.f_value[0] == value.f_value.back())
        {
            // remove quotes around string
            //
            value.f_value = value.f_value.substr(1, value.f_value.length() - 2);
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
            switch(var.second.get_type())
            {
            case value_type_t::VALUE_TYPE_BOOLEAN:
                {
                    bool value(false);
                    if(var.second.f_value == "true")
                    {
                        value = true;
                    }
                    else
                    {
                        // value must be "true" or "false"
                        //
                        CATCH_REQUIRE(var.second.f_value == "false");
                    }
                    script.set_variable(var.first, value);
                }
                break;

            case value_type_t::VALUE_TYPE_INTEGER:
                {
                    std::int64_t const value(std::stoll(var.second.f_value, nullptr, 0));
                    script.set_variable(var.first, value);
                }
                break;

            case value_type_t::VALUE_TYPE_FLOATING_POINT:
                {
                    double value(0.0);
                    if(m.f_result.f_value == "MIN_VALUE")
                    {
                        value = std::numeric_limits<double>::min();
                    }
                    else if(m.f_result.f_value == "MAX_VALUE")
                    {
                        value = std::numeric_limits<double>::max();
                    }
                    else if(var.second.f_value == "POSITIVE_INFINITY")
                    {
                        value = std::numeric_limits<double>::infinity();
                    }
                    else if(var.second.f_value == "NEGATIVE_INFINITY")
                    {
                        value = -std::numeric_limits<double>::infinity();
                    }
                    else if(var.second.f_value == "EPSILON")
                    {
                        value = 2.220446049250313e-16;
                    }
                    else
                    {
                        value = std::stod(var.second.f_value, nullptr);
                    }
                    script.set_variable(var.first, value);
                }
                break;

            case value_type_t::VALUE_TYPE_STRING:
                script.set_variable(var.first, var.second.f_value);
                break;

            //case value_type_t::VALUE_TYPE_UNDEFINED:
            default:
                CATCH_REQUIRE(!"variable type not yet implemented or somehow set to UNDEFINED.");
                break;

            }
        }
    }

    as2js::binary_result result;

    script.run(result);

    switch(m.f_result.get_type())
    {
    case value_type_t::VALUE_TYPE_BOOLEAN:
        {
            bool expected_result(false);
            if(m.f_result.f_value == "true")
            {
                expected_result = true;
            }
            else
            {
                // value must be "true" or "false"
                //
                CATCH_REQUIRE(m.f_result.f_value == "false");
            }
            CATCH_REQUIRE(result.get_boolean() == expected_result);
        }
        break;

    case value_type_t::VALUE_TYPE_INTEGER:
        {
            std::int64_t const expected_result(std::stoll(m.f_result.f_value, nullptr, 0));
            if(result.get_integer() != expected_result)
            {
                std::cerr << "--- (integer result) differs: " << result.get_integer() << " != " << expected_result << "\n";
            }
            CATCH_REQUIRE(result.get_integer() == expected_result);
        }
        break;

    case value_type_t::VALUE_TYPE_FLOATING_POINT:
        {
            double expected_result(0.0);
            if(m.f_result.f_value == "MIN_VALUE")
            {
                expected_result = std::numeric_limits<double>::min();
            }
            else if(m.f_result.f_value == "MAX_VALUE")
            {
                expected_result = std::numeric_limits<double>::max();
            }
            else if(m.f_result.f_value == "POSITIVE_INFINITY")
            {
                expected_result = std::numeric_limits<double>::infinity();
            }
            else if(m.f_result.f_value == "NEGATIVE_INFINITY")
            {
                expected_result = -std::numeric_limits<double>::infinity();
            }
            else if(m.f_result.f_value == "EPSILON")
            {
                expected_result = 2.220446049250313e-16;
            }
            else
            {
                expected_result = std::stod(m.f_result.f_value, nullptr);
            }
            double const final_result(result.get_floating_point());
            if(!SNAP_CATCH2_NAMESPACE::nearly_equal(final_result, expected_result))
            {
                double const * result_ptr(&final_result);
                double const * expected_ptr(&expected_result);
                std::cerr
                    << "--- (double result) differs: "
                    << final_result
                    << " != "
                    << expected_result
                    << " (0x"
                    << std::setw(16) << std::setfill('0') << std::hex
                    << *reinterpret_cast<std::uint64_t const *>(result_ptr)
                    << " != 0x"
                    << std::setw(16)
                    << *reinterpret_cast<std::uint64_t const *>(expected_ptr)
                    << std::dec
                    << ")\n";
            }
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(final_result, expected_result));
        }
        break;

    case value_type_t::VALUE_TYPE_STRING:
        {
            std::string const expected_result(m.f_result.f_value);
            if(result.get_string() != expected_result)
            {
                std::cerr << "--- (string result) differs: " << result.get_string() << " != " << expected_result << "\n";
            }
            CATCH_REQUIRE(result.get_string() == expected_result);
        }
        break;

    //case value_type_t::VALUE_TYPE_UNDEFINED:
    default:
        CATCH_REQUIRE(!"variable type not yet implemented or somehow set to UNDEFINED.");
        break;

    }

    for(auto const & var : m.f_variables)
    {
        if(var.second.is_out())
        {
            // TODO: support all types of variables
            //
            std::string const name(var.first.substr(2));
            switch(var.second.get_type())
            {
            case value_type_t::VALUE_TYPE_BOOLEAN:
                {
                    bool expected_value(false);
                    if(var.second.f_value == "true")
                    {
                        expected_value = true;
                    }
                    else
                    {
                        // value must be "true" or "false"
                        //
                        CATCH_REQUIRE(var.second.f_value == "false");
                    }
                    bool returned_value(0);
                    script.get_variable(name, returned_value);
                    if(returned_value != expected_value)
                    {
                        std::cerr
                            << "--- invalid boolean result in \""
                            << var.first
                            << "\".\n";
                    }
                    CATCH_REQUIRE(returned_value == expected_value);
                }
                break;

            case value_type_t::VALUE_TYPE_INTEGER:
                {
                    std::int64_t const expected_value(std::stoll(var.second.f_value, nullptr, 0));
                    std::int64_t returned_value(0);
                    script.get_variable(name, returned_value);
                    if(returned_value != expected_value)
                    {
                        std::cerr
                            << "--- invalid integer result in \""
                            << var.first
                            << "\".\n";
                    }
                    CATCH_REQUIRE(returned_value == expected_value);
                }
                break;

            case value_type_t::VALUE_TYPE_FLOATING_POINT:
                {
                    double returned_value(0.0);
                    script.get_variable(name, returned_value);
                    if(var.second.f_value == "NaN")
                    {
                        CATCH_REQUIRE(std::isnan(returned_value));
                    }
                    else
                    {
                        double epsilon(0.0000000000000033);
                        double expected_result(0.0);
                        if(var.second.f_value == "MIN_VALUE")
                        {
                            epsilon = 0.0;
                            expected_result = std::numeric_limits<double>::min();
                        }
                        else if(var.second.f_value == "MAX_VALUE")
                        {
                            epsilon = 0.0;
                            expected_result = std::numeric_limits<double>::max();
                        }
                        else if(var.second.f_value == "POSITIVE_INFINITY")
                        {
                            epsilon = 0.0;
                            expected_result = std::numeric_limits<double>::infinity();
                        }
                        else if(var.second.f_value == "NEGATIVE_INFINITY")
                        {
                            epsilon = 0.0;
                            expected_result = -std::numeric_limits<double>::infinity();
                        }
                        else if(var.second.f_value == "EPSILON")
                        {
                            epsilon = 0.0;
                            expected_result = 2.220446049250313e-16;
                        }
                        else
                        {
                            expected_result = std::stod(var.second.f_value, nullptr);
                        }
                        if(!SNAP_CATCH2_NAMESPACE::nearly_equal(returned_value, expected_result, epsilon))
                        {
                            double const * value_ptr(&returned_value);
                            double const * expected_ptr(&expected_result);
                            std::cerr
                                << "--- invalid floating point result in \""
                                << var.first
                                << "\" -- "
                                << std::setprecision(20)
                                << returned_value
                                << " != "
                                << expected_result
                                << " (0x"
                                << std::setw(16) << std::setfill('0') << std::hex
                                << *reinterpret_cast<std::uint64_t const *>(value_ptr)
                                << " != 0x"
                                << std::setw(16)
                                << *reinterpret_cast<std::uint64_t const *>(expected_ptr)
                                << std::dec
                                << ")\n";
                        }
                        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(returned_value, expected_result, epsilon));
                    }
                }
                break;

            case value_type_t::VALUE_TYPE_STRING:
                {
                    std::string const expected_value(var.second.f_value);
                    std::string returned_value;
                    script.get_variable(name, returned_value);
                    if(returned_value != expected_value)
                    {
                        std::cerr
                            << "--- invalid string result in \""
                            << var.first
                            << "\".\n";
                    }
                    CATCH_REQUIRE(returned_value == expected_value);
                }
                break;

            //case value_type_t::VALUE_TYPE_UNDEFINED,
            default:
                std::cerr
                    << "variable type "
                    << static_cast<int>(var.second.get_type())
                    << " not yet implemented in catch_binary.cpp ("
                    << __FILE__
                    << ':'
                    << __LINE__
                    << ")\n";
                CATCH_REQUIRE(!"variable type not yet implemented or somehow set to UNDEFINED.");
                break;

            }
        }
    }
}



}







CATCH_TEST_CASE("binary_integer_operators", "[binary][integer]")
{
    CATCH_START_SECTION("binary_integer_operators: test binary operators for integers")
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

CATCH_TEST_CASE("binary_double_operators", "[binary][double][floating_point]")
{
    CATCH_START_SECTION("binary_double_operators: test binary operators for doubles")
    {
        snapdev::glob_to_list<std::list<std::string>> scripts;
        CATCH_REQUIRE(scripts.read_path<snapdev::glob_to_list_flag_t::GLOB_FLAG_NONE>
                                    (SNAP_CATCH2_NAMESPACE::g_source_dir()
                                        + "/tests/binary/double_operator_*.ajs"));
        for(auto const & s : scripts)
        {
            run_script(s);
            meta m(load_script_meta(s));
            execute(m);
        }
    }
    CATCH_END_SECTION()
}

CATCH_TEST_CASE("binary_boolean_operators", "[binary][boolean]")
{
    CATCH_START_SECTION("binary_boolean_operators: test binary operators for booleans")
    {
        snapdev::glob_to_list<std::list<std::string>> scripts;
        CATCH_REQUIRE(scripts.read_path<snapdev::glob_to_list_flag_t::GLOB_FLAG_NONE>
                                    (SNAP_CATCH2_NAMESPACE::g_source_dir()
                                        + "/tests/binary/boolean_operator_*.ajs"));
        for(auto const & s : scripts)
        {
            run_script(s);
            meta m(load_script_meta(s));
            execute(m);
        }
    }
    CATCH_END_SECTION()
}

CATCH_TEST_CASE("binary_string_operators", "[binary][binary_string]")
{
    CATCH_START_SECTION("binary_string_operators: test binary operators for strings")
    {
        snapdev::glob_to_list<std::list<std::string>> scripts;
        CATCH_REQUIRE(scripts.read_path<snapdev::glob_to_list_flag_t::GLOB_FLAG_NONE>
                                    (SNAP_CATCH2_NAMESPACE::g_source_dir()
                                        + "/tests/binary/string_operator_*.ajs"));
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
