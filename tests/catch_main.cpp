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

// self
//
#define CATCH_CONFIG_RUNNER
#include    "catch_main.h"



// as2js
//
#include    <as2js/exception.h>
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



#define    TO_STR_sub(s)    #s



// command line flags
//
std::string     g_as2js_compiler;
bool            g_run_destructive = false;
bool            g_save_parser_tests = false;





// class used to capture error messages

test_callback::test_callback(bool verbose, bool parser)
    : f_verbose(verbose)
    , f_parser(parser)
{
    as2js::set_message_callback(this);
    fix_counters();
}


test_callback::~test_callback()
{
    // make sure the pointer gets reset!
    //
    as2js::set_message_callback(nullptr);
}


void test_callback::fix_counters()
{
    g_warning_count = as2js::warning_count();
    g_error_count = as2js::error_count();
}


// implementation of the output
void test_callback::output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::position const & pos, std::string const& message)
{
    // skip trace messages
    //
    if(message_level == as2js::message_level_t::MESSAGE_LEVEL_TRACE)
    {
        return;
    }
    ++f_position;

    if(f_expected.empty())
    {
        std::cerr << "\n*** STILL NECESSARY *** (#" << f_position << ")\n";
        std::cerr << "filename = " << pos.get_filename() << "\n";
        std::cerr << "message level = " << static_cast<int>(message_level) << " (" << message_level << ")\n";
        std::cerr << "msg = " << message << "\n";
        std::cerr << "page = " << pos.get_page() << "\n";
        std::cerr << "line = " << pos.get_line() << "\n";
        std::cerr << "error code = " << static_cast<int>(error_code) << " (" << error_code_to_str(error_code) << ")\n";
    }

    CATCH_REQUIRE(!f_expected.empty());

    // the compiler uses this flag to generate the following warning
    //
    if(f_parser)
    {
        std::cerr << "\n                 >>> WARNING <<<\n"
                     "  >>> You got an error from the parser. These should not happen here.\n"
                     "  >>> If you need to test something in the parser, move your test to the\n"
                     "  >>> tests/parser_data/*.json files instead.\n\n";
    }

    bool const error(!f_expected[0].f_call
              || message_level != f_expected[0].f_message_level
              || error_code != f_expected[0].f_error_code
              || pos.get_filename() != f_expected[0].f_pos.get_filename()
              || pos.get_function() != f_expected[0].f_pos.get_function()
              || pos.get_page() != f_expected[0].f_pos.get_page()
              || pos.get_page_line() != f_expected[0].f_pos.get_page_line()
              || pos.get_paragraph() != f_expected[0].f_pos.get_paragraph()
              || pos.get_line() != f_expected[0].f_pos.get_line()
              || message != f_expected[0].f_message);
    bool const verbose(f_verbose || error);
    if(verbose)
    {
        std::cerr << "\n";
        if(error)
        {
            std::cerr << "*** FAILED TEST *** (#" << f_position << ")\n";
        }
        else
        {
            std::cerr << "*** TEST MESSAGE *** (#" << f_position << ")\n";
        }
        std::cerr << "filename = " << pos.get_filename() << " (node) / " << f_expected[0].f_pos.get_filename() << " (JSON)\n";
        std::cerr << "message level = " << static_cast<int>(message_level) << " (" << message_level
                  << ") / " << static_cast<int>(f_expected[0].f_message_level) << " (" << f_expected[0].f_message_level << ")\n";
        std::cerr << "msg = " << message << '\n'
                  << "    / " << f_expected[0].f_message << '\n';
        std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << '\n';
        std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << '\n';
        std::cerr << "page line = " << pos.get_page_line() << " / " << f_expected[0].f_pos.get_page_line() << '\n';
        std::cerr << "error code = " << static_cast<int>(error_code) << " (" << error_code_to_str(error_code)
                  << ") / " << static_cast<int>(f_expected[0].f_error_code)
                  << " (" << error_code_to_str(f_expected[0].f_error_code) << ")\n";
    }

    CATCH_REQUIRE(f_expected[0].f_call);
    CATCH_REQUIRE(message_level == f_expected[0].f_message_level);
    CATCH_REQUIRE(error_code == f_expected[0].f_error_code);
    CATCH_REQUIRE(pos.get_filename() == f_expected[0].f_pos.get_filename());
    CATCH_REQUIRE(pos.get_function() == f_expected[0].f_pos.get_function());
    CATCH_REQUIRE(pos.get_page() == f_expected[0].f_pos.get_page());
    CATCH_REQUIRE(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
    CATCH_REQUIRE(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
    CATCH_REQUIRE(pos.get_line() == f_expected[0].f_pos.get_line());
    CATCH_REQUIRE(message == f_expected[0].f_message);

    if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
    {
        ++g_warning_count;
        CATCH_REQUIRE(g_warning_count == as2js::warning_count());
    }

    if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
    || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
    {
        ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::error_count() << "\n";
        CATCH_REQUIRE(g_error_count == as2js::error_count());
    }

    f_expected.erase(f_expected.begin());
}

void test_callback::got_called()
{
    if(!f_expected.empty())
    {
        std::cerr << "\n*** STILL " << f_expected.size() << " EXPECTED *** (#" << f_position << ")\n";
        std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
        std::cerr << "message level = " << static_cast<int>(f_expected[0].f_message_level)
                        << " (" << f_expected[0].f_message_level << ")\n";
        std::cerr << "msg = " << f_expected[0].f_message << "\n";
        std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
        std::cerr << "line = " << f_expected[0].f_pos.get_line() << "\n";
        std::cerr << "error code = " << static_cast<int>(f_expected[0].f_error_code)
                        << " (" << error_code_to_str(f_expected[0].f_error_code) << ")\n";
    }
    CATCH_REQUIRE(f_expected.empty());
}

std::int32_t    test_callback::g_warning_count = 0;
std::int32_t    test_callback::g_error_count = 0;





// functions to convert error codes to/from strings

struct err_to_string_t
{
    as2js::err_code_t       f_code;
    char const *            f_name;
    int                     f_line;
};

#define    ERROR_NAME(err)  { as2js::err_code_t::AS_ERR_##err, TO_STR_sub(err), __LINE__ }

constexpr err_to_string_t const g_error_table[] =
{
    ERROR_NAME(NONE),
    ERROR_NAME(ABSTRACT),
    ERROR_NAME(BAD_NUMERIC_TYPE),
    ERROR_NAME(BAD_PRAGMA),
    ERROR_NAME(CANNOT_COMPILE),
    ERROR_NAME(CANNOT_MATCH),
    ERROR_NAME(CANNOT_OVERLOAD),
    ERROR_NAME(CANNOT_OVERWRITE_CONST),
    ERROR_NAME(CASE_LABEL),
    ERROR_NAME(COLON_EXPECTED),
    ERROR_NAME(COMMA_EXPECTED),
    ERROR_NAME(CURVLY_BRACKETS_EXPECTED),
    ERROR_NAME(DEFAULT_LABEL),
    ERROR_NAME(DIVIDE_BY_ZERO),
    ERROR_NAME(DUPLICATES),
    ERROR_NAME(DYNAMIC),
    ERROR_NAME(EXPRESSION_EXPECTED),
    ERROR_NAME(FINAL),
    ERROR_NAME(IMPROPER_STATEMENT),
    ERROR_NAME(INACCESSIBLE_STATEMENT),
    ERROR_NAME(INCOMPATIBLE),
    ERROR_NAME(INCOMPATIBLE_PRAGMA_ARGUMENT),
    ERROR_NAME(INSTALLATION),
    ERROR_NAME(INSTANCE_EXPECTED),
    ERROR_NAME(INTERNAL_ERROR),
    ERROR_NAME(NATIVE),
    ERROR_NAME(INVALID_ARRAY_FUNCTION),
    ERROR_NAME(INVALID_ATTRIBUTES),
    ERROR_NAME(INVALID_CATCH),
    ERROR_NAME(INVALID_CLASS),
    ERROR_NAME(INVALID_CONDITIONAL),
    ERROR_NAME(INVALID_DEFINITION),
    ERROR_NAME(INVALID_DO),
    ERROR_NAME(INVALID_ENUM),
    ERROR_NAME(INVALID_EXPRESSION),
    ERROR_NAME(INVALID_FIELD),
    ERROR_NAME(INVALID_FIELD_NAME),
    ERROR_NAME(INVALID_FRAME),
    ERROR_NAME(INVALID_FUNCTION),
    ERROR_NAME(INVALID_GOTO),
    ERROR_NAME(INVALID_IMPORT),
    ERROR_NAME(INVALID_INPUT_STREAM),
    ERROR_NAME(INVALID_KEYWORD),
    ERROR_NAME(INVALID_LABEL),
    ERROR_NAME(INVALID_NAMESPACE),
    ERROR_NAME(INVALID_NODE),
    ERROR_NAME(INVALID_NUMBER),
    ERROR_NAME(INVALID_OPERATOR),
    ERROR_NAME(INVALID_PACKAGE_NAME),
    ERROR_NAME(INVALID_PARAMETERS),
    ERROR_NAME(INVALID_REST),
    ERROR_NAME(INVALID_RETURN_TYPE),
    ERROR_NAME(INVALID_SCOPE),
    ERROR_NAME(INVALID_TEMPLATE),
    ERROR_NAME(INVALID_TRY),
    ERROR_NAME(INVALID_TYPE),
    ERROR_NAME(INVALID_UNICODE_ESCAPE_SEQUENCE),
    ERROR_NAME(INVALID_VARIABLE),
    ERROR_NAME(IO_ERROR),
    ERROR_NAME(LABEL_NOT_FOUND),
    ERROR_NAME(LOOPING_REFERENCE),
    ERROR_NAME(MISMATCH_FUNC_VAR),
    ERROR_NAME(MISSSING_VARIABLE_NAME),
    ERROR_NAME(NEED_CONST),
    ERROR_NAME(NOT_ALLOWED),
    ERROR_NAME(NOT_ALLOWED_IN_STRICT_MODE),
    ERROR_NAME(NOT_FOUND),
    ERROR_NAME(NOT_SUPPORTED),
    ERROR_NAME(OBJECT_MEMBER_DEFINED_TWICE),
    ERROR_NAME(PARENTHESIS_EXPECTED),
    ERROR_NAME(PRAGMA_FAILED),
    ERROR_NAME(SEMICOLON_EXPECTED),
    ERROR_NAME(SQUARE_BRACKETS_EXPECTED),
    ERROR_NAME(STRING_EXPECTED),
    ERROR_NAME(STATIC),
    ERROR_NAME(TYPE_NOT_LINKED),
    ERROR_NAME(UNKNOWN_ESCAPE_SEQUENCE),
    ERROR_NAME(UNKNOWN_OPERATOR),
    ERROR_NAME(UNKNOWN_PRAGMA),
    ERROR_NAME(UNTERMINATED_STRING),
    ERROR_NAME(UNEXPECTED_EOF),
    ERROR_NAME(UNEXPECTED_PUNCTUATION),
    ERROR_NAME(UNEXPECTED_TOKEN),
    ERROR_NAME(UNEXPECTED_DATABASE),
    ERROR_NAME(UNEXPECTED_RC)
};
constexpr std::size_t const g_error_table_size = sizeof(g_error_table) / sizeof(g_error_table[0]);


as2js::err_code_t str_to_error_code(std::string const & error_name)
{
    for(size_t idx(0); idx < g_error_table_size; ++idx)
    {
        if(error_name == g_error_table[idx].f_name)
        {
            return g_error_table[idx].f_code;
        }
    }
    std::cerr << "Error name \"" << error_name << "\" not found.\n";
    CATCH_REQUIRE(!"error name not found, catch_parser.cpp bug");
    return as2js::err_code_t::AS_ERR_NONE;
}


char const * error_code_to_str(as2js::err_code_t const error_code)
{
    // TODO: the error code should be in order
    //       1. verify that the order is indeed correct
    //       2. use a binary search instead
    //
    for(std::size_t idx(0); idx < g_error_table_size; ++idx)
    {
        if(error_code == g_error_table[idx].f_code)
        {
            return g_error_table[idx].f_name;
        }
    }
    std::cerr << "Error code \"" << static_cast<int>(error_code) << "\" not found.\n";
    CATCH_REQUIRE(!"error code not found, catch_parser.cpp bug");
    return "unknown";
}






// Options

//
// we have two special pragmas that accept 0, 1, 2, or 3
// namely, those are:
//
//  . OPTION_EXTENDED_STATEMENTS -- force '{' ... '}' in
//    blocks for: if, while, do, for, with...
//
//  . OPTION_EXTENDED_OPERATORS -- force ':=' instead of '='
//
// for this reason we support and f_value which is viewed
// as a set of flags
//
named_options const g_options[] =
{
    {
        as2js::option_t::OPTION_ALLOW_WITH,
        "allow_with",
        "no_allow_with",
        1
    },
    {
        as2js::option_t::OPTION_COVERAGE,
        "coverage",
        "no_coverage",
        1
    },
    {
        as2js::option_t::OPTION_DEBUG,
        "debug",
        "no_debug",
        1
    },
    {
        as2js::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES,
        "extended_escape_sequences",
        "no_extended_escape_sequences",
        1
    },
    {
        as2js::option_t::OPTION_EXTENDED_OPERATORS,
        "extended_operators",
        "no_extended_operators",
        1
    },
    {
        as2js::option_t::OPTION_EXTENDED_OPERATORS,
        "extended_operators_safe",
        "no_extended_operators_safe",
        2
    },
    {
        as2js::option_t::OPTION_EXTENDED_STATEMENTS,
        "extended_statements",
        "no_extended_statements",
        1
    },
    {
        as2js::option_t::OPTION_EXTENDED_STATEMENTS,
        "extended_statements_safe",
        "no_extended_statements_safe",
        2
    },
    //{ -- this one does not make sense here
    //    as2js::option_t::OPTION_JSON,
    //    "json",
    //    "no_json"
    //},
    {
        as2js::option_t::OPTION_OCTAL,
        "octal",
        "no_octal",
        1
    },
    {
        as2js::option_t::OPTION_STRICT,
        "strict",
        "no_strict",
        1
    },
    {
        as2js::option_t::OPTION_TRACE,
        "trace",
        "no_trace",
        1
    },
    {
        as2js::option_t::OPTION_UNSAFE_MATH,
        "unsafe_math",
        "no_unsafe_math",
        1
    }
};
std::size_t const g_options_size = sizeof(g_options) / sizeof(g_options[0]);






// Flags

struct flg_to_string_t
{
    as2js::flag_t           f_flag;
    char const *            f_name;
    int                     f_line;
};

#define    FLAG_NAME(flg)     { as2js::flag_t::NODE_##flg, TO_STR_sub(flg), __LINE__ }

constexpr flg_to_string_t const g_flag_table[] =
{
    FLAG_NAME(CATCH_FLAG_TYPED),
    FLAG_NAME(DIRECTIVE_LIST_FLAG_NEW_VARIABLES),
    FLAG_NAME(ENUM_FLAG_CLASS),
    FLAG_NAME(FOR_FLAG_CONST),
    FLAG_NAME(FOR_FLAG_FOREACH),
    FLAG_NAME(FOR_FLAG_IN),
    FLAG_NAME(FUNCTION_FLAG_GETTER),
    FLAG_NAME(FUNCTION_FLAG_SETTER),
    FLAG_NAME(FUNCTION_FLAG_OUT),
    FLAG_NAME(FUNCTION_FLAG_VOID),
    FLAG_NAME(FUNCTION_FLAG_NEVER),
    FLAG_NAME(FUNCTION_FLAG_NOPARAMS),
    FLAG_NAME(FUNCTION_FLAG_OPERATOR),
    FLAG_NAME(IDENTIFIER_FLAG_WITH),
    FLAG_NAME(IDENTIFIER_FLAG_TYPED),
    FLAG_NAME(IMPORT_FLAG_IMPLEMENTS),
    FLAG_NAME(PACKAGE_FLAG_FOUND_LABELS),
    FLAG_NAME(PACKAGE_FLAG_REFERENCED),
    FLAG_NAME(PARAM_FLAG_CONST),
    FLAG_NAME(PARAM_FLAG_IN),
    FLAG_NAME(PARAM_FLAG_OUT),
    FLAG_NAME(PARAM_FLAG_NAMED),
    FLAG_NAME(PARAM_FLAG_REST),
    FLAG_NAME(PARAM_FLAG_UNCHECKED),
    FLAG_NAME(PARAM_FLAG_UNPROTOTYPED),
    FLAG_NAME(PARAM_FLAG_REFERENCED),
    FLAG_NAME(PARAM_FLAG_PARAMREF),
    FLAG_NAME(PARAM_FLAG_CATCH),
    FLAG_NAME(PARAM_MATCH_FLAG_UNPROTOTYPED),
    FLAG_NAME(SWITCH_FLAG_DEFAULT),
    FLAG_NAME(TYPE_FLAG_MODULO),
    FLAG_NAME(VARIABLE_FLAG_CONST),
    FLAG_NAME(VARIABLE_FLAG_FINAL),
    FLAG_NAME(VARIABLE_FLAG_LOCAL),
    FLAG_NAME(VARIABLE_FLAG_MEMBER),
    FLAG_NAME(VARIABLE_FLAG_ATTRIBUTES),
    FLAG_NAME(VARIABLE_FLAG_ENUM),
    FLAG_NAME(VARIABLE_FLAG_COMPILED),
    FLAG_NAME(VARIABLE_FLAG_INUSE),
    FLAG_NAME(VARIABLE_FLAG_ATTRS),
    FLAG_NAME(VARIABLE_FLAG_DEFINED),
    FLAG_NAME(VARIABLE_FLAG_DEFINING),
    FLAG_NAME(VARIABLE_FLAG_TOADD)
};
constexpr std::size_t const g_flag_table_size = sizeof(g_flag_table) / sizeof(g_flag_table[0]);


as2js::flag_t str_to_flag_code(std::string const & flag_name)
{
    for(size_t idx(0); idx < g_flag_table_size; ++idx)
    {
        if(flag_name == g_flag_table[idx].f_name)
        {
            return g_flag_table[idx].f_flag;
        }
    }
    //CATCH_REQUIRE(!"flag code not found, catch_parser.cpp bug");
    CATCH_REQUIRE(flag_name == "unknown flag");
    return as2js::flag_t::NODE_FLAG_max;
}


std::string flag_to_str(as2js::flag_t const flg)
{
    for(size_t idx(0); idx < g_flag_table_size; ++idx)
    {
        if(flg == g_flag_table[idx].f_flag)
        {
            return g_flag_table[idx].f_name;
        }
    }
    CATCH_REQUIRE(!"flag code not found, catch_parser.cpp bug");
    return "";
}




void verify_flags(as2js::node::pointer_t node, std::string const & flags_set, bool verbose)
{
    // list of flags that have to be set
    //
    std::vector<as2js::flag_t> flgs;
    char const * f(flags_set.c_str());
    char const * s(f);
    for(;;)
    {
        if(*f == ',' || *f == '\0')
        {
            if(s == f)
            {
                break;
            }
            std::string name(s, f - s);
//std::cerr << "Checking " << name << " -> " << static_cast<int>(str_to_flag_code(name)) << "\n";
            flgs.push_back(str_to_flag_code(name));
            if(*f == '\0')
            {
                break;
            }
            do // skip commas
            {
                ++f;
            }
            while(*f == ',');
            s = f;
        }
        else
        {
            ++f;
        }
    }

    // list of flags that must be checked
    std::vector<as2js::flag_t> flgs_to_check;
    switch(node->get_type())
    {
    case as2js::node_t::NODE_CATCH:
        flgs_to_check.push_back(as2js::flag_t::NODE_CATCH_FLAG_TYPED);
        break;

    case as2js::node_t::NODE_DIRECTIVE_LIST:
        flgs_to_check.push_back(as2js::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES);
        break;

    case as2js::node_t::NODE_ENUM:
        flgs_to_check.push_back(as2js::flag_t::NODE_ENUM_FLAG_CLASS);
        break;

    case as2js::node_t::NODE_FOR:
        flgs_to_check.push_back(as2js::flag_t::NODE_FOR_FLAG_CONST);
        flgs_to_check.push_back(as2js::flag_t::NODE_FOR_FLAG_FOREACH);
        flgs_to_check.push_back(as2js::flag_t::NODE_FOR_FLAG_IN);
        break;

    case as2js::node_t::NODE_FUNCTION:
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_GETTER);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_NEVER);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_NOPARAMS);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_OPERATOR);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_OUT);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_SETTER);
        flgs_to_check.push_back(as2js::flag_t::NODE_FUNCTION_FLAG_VOID);
        break;

    case as2js::node_t::NODE_IDENTIFIER:
    case as2js::node_t::NODE_VIDENTIFIER:
    case as2js::node_t::NODE_STRING:
        flgs_to_check.push_back(as2js::flag_t::NODE_IDENTIFIER_FLAG_WITH);
        flgs_to_check.push_back(as2js::flag_t::NODE_IDENTIFIER_FLAG_TYPED);
        break;

    case as2js::node_t::NODE_IMPORT:
        flgs_to_check.push_back(as2js::flag_t::NODE_IMPORT_FLAG_IMPLEMENTS);
        break;

    case as2js::node_t::NODE_PACKAGE:
        flgs_to_check.push_back(as2js::flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS);
        flgs_to_check.push_back(as2js::flag_t::NODE_PACKAGE_FLAG_REFERENCED);
        break;

    case as2js::node_t::NODE_PARAM_MATCH:
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED);
        break;

    case as2js::node_t::NODE_PARAM:
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_CATCH);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_CONST);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_IN);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_OUT);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_NAMED);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_PARAMREF);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_REFERENCED);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_REST);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_UNCHECKED);
        flgs_to_check.push_back(as2js::flag_t::NODE_PARAM_FLAG_UNPROTOTYPED);
        break;

    case as2js::node_t::NODE_SWITCH:
        flgs_to_check.push_back(as2js::flag_t::NODE_SWITCH_FLAG_DEFAULT);
        break;

    case as2js::node_t::NODE_TYPE:
        flgs_to_check.push_back(as2js::flag_t::NODE_TYPE_FLAG_MODULO);
        break;

    case as2js::node_t::NODE_VARIABLE:
    case as2js::node_t::NODE_VAR_ATTRIBUTES:
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_CONST);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_FINAL);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_LOCAL);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_MEMBER);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_ENUM);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_COMPILED);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_INUSE);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_ATTRS);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_DEFINED);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_DEFINING);
        flgs_to_check.push_back(as2js::flag_t::NODE_VARIABLE_FLAG_TOADD);
        break;

    default:
        // no flags supported
        break;

    }

    CATCH_REQUIRE(flgs.size() <= flgs_to_check.size());

    for(size_t idx(0); idx < flgs_to_check.size(); ++idx)
    {
        as2js::flag_t flg(flgs_to_check[idx]);
        std::vector<as2js::flag_t>::iterator it(std::find(flgs.begin(), flgs.end(), flg));
        if(it == flgs.end())
        {
            // expected to be unset
            if(verbose && node->get_flag(flg))
            {
                std::cerr
                    << "\n*** Comparing flags "
                    << flag_to_str(flg)
                    << " (should not be set):\n"
                    << *node
                    << "\n";
            }
            CATCH_REQUIRE(!node->get_flag(flg));
        }
        else
        {
            // expected to be set
            flgs.erase(it);
            if(verbose && !node->get_flag(flg))
            {
                std::cerr
                    << "\n*** Comparing flags "
                    << flag_to_str(flg)
                    << " (it should be set in this case):\n"
                    << *node
                    << "\n";
            }
            CATCH_REQUIRE(node->get_flag(flg));
        }
    }

    CATCH_REQUIRE(flgs.empty());
}






// Attributes

struct attr_to_string_t
{
    as2js::attribute_t      f_attribute;
    char const *            f_name;
    int                     f_line;
};

#define    ATTRIBUTE_NAME(attr)      { as2js::attribute_t::NODE_ATTR_##attr, TO_STR_sub(attr), __LINE__ }

constexpr attr_to_string_t const g_attribute_table[] =
{
    ATTRIBUTE_NAME(PUBLIC),
    ATTRIBUTE_NAME(PRIVATE),
    ATTRIBUTE_NAME(PROTECTED),
    ATTRIBUTE_NAME(INTERNAL),
    ATTRIBUTE_NAME(TRANSIENT),
    ATTRIBUTE_NAME(VOLATILE),
    ATTRIBUTE_NAME(STATIC),
    ATTRIBUTE_NAME(ABSTRACT),
    ATTRIBUTE_NAME(VIRTUAL),
    ATTRIBUTE_NAME(ARRAY),
    ATTRIBUTE_NAME(REQUIRE_ELSE),
    ATTRIBUTE_NAME(ENSURE_THEN),
    ATTRIBUTE_NAME(NATIVE),
    ATTRIBUTE_NAME(DEPRECATED),
    ATTRIBUTE_NAME(UNSAFE),
    ATTRIBUTE_NAME(CONSTRUCTOR),
    ATTRIBUTE_NAME(FINAL),
    ATTRIBUTE_NAME(ENUMERABLE),
    ATTRIBUTE_NAME(TRUE),
    ATTRIBUTE_NAME(FALSE),
    ATTRIBUTE_NAME(UNUSED),
    ATTRIBUTE_NAME(DYNAMIC),
    ATTRIBUTE_NAME(FOREACH),
    ATTRIBUTE_NAME(NOBREAK),
    ATTRIBUTE_NAME(AUTOBREAK),
    ATTRIBUTE_NAME(DEFINED)
};
constexpr std::size_t const g_attribute_table_size = sizeof(g_attribute_table) / sizeof(g_attribute_table[0]);


as2js::attribute_t str_to_attribute_code(std::string const & attr_name)
{
    for(std::size_t idx(0); idx < g_attribute_table_size; ++idx)
    {
        if(attr_name == g_attribute_table[idx].f_name)
        {
            return g_attribute_table[idx].f_attribute;
        }
    }
    CATCH_REQUIRE(!"attribute code not found, catch_parser.cpp bug");
    return as2js::attribute_t::NODE_ATTR_max;
}


std::string attribute_to_str(as2js::attribute_t const attr)
{
    for(std::size_t idx(0); idx < g_attribute_table_size; ++idx)
    {
        if(attr == g_attribute_table[idx].f_attribute)
        {
            return g_attribute_table[idx].f_name;
        }
    }
    CATCH_REQUIRE(!"attribute code not found, catch_parser.cpp bug");
    return std::string();
}




void verify_attributes(as2js::node::pointer_t n, std::string const & attributes_set, bool verbose)
{
    // list of attributes that have to be set
    //
    std::vector<as2js::attribute_t> attrs;
    char const * a(attributes_set.c_str());
    char const * s(a);
    for(;;)
    {
        if(*a == ',' || *a == '\0')
        {
            if(s == a)
            {
                break;
            }
            std::string const name(s, a - s);
            attrs.push_back(str_to_attribute_code(name));
            if(*a == '\0')
            {
                break;
            }
            do // skip commas
            {
                ++a;
            }
            while(*a == ',');
            s = a;
        }
        else
        {
            ++a;
        }
    }

    // list of attributes that must be checked
    std::vector<as2js::attribute_t> attrs_to_check;

    if(n->get_type() != as2js::node_t::NODE_PROGRAM)
    {
        // except for PROGRAM, all attributes always apply
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_PUBLIC);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_PRIVATE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_PROTECTED);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_INTERNAL);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_TRANSIENT);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_VOLATILE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_STATIC);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_ABSTRACT);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_VIRTUAL);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_ARRAY);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_REQUIRE_ELSE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_ENSURE_THEN);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_NATIVE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_DEPRECATED);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_UNSAFE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_CONSTRUCTOR);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_FINAL);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_ENUMERABLE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_TRUE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_FALSE);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_UNUSED);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_DYNAMIC);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_FOREACH);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_NOBREAK);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_AUTOBREAK);
        attrs_to_check.push_back(as2js::attribute_t::NODE_ATTR_DEFINED);
    }

    CATCH_REQUIRE(attrs.size() <= attrs_to_check.size());

    for(size_t idx(0); idx < attrs_to_check.size(); ++idx)
    {
        as2js::attribute_t attr(attrs_to_check[idx]);
        std::vector<as2js::attribute_t>::iterator it(std::find(attrs.begin(), attrs.end(), attr));
        if(it == attrs.end())
        {
            // expected to be unset
            //
            if(verbose && n->get_attribute(attr))
            {
                std::cerr
                    << "*** Comparing attributes "
                    << attribute_to_str(attr)
                    << " (should not be set)\n"
                    << *n
                    << '\n';
            }
            CATCH_REQUIRE(!n->get_attribute(attr));
        }
        else
        {
            // expected to be set
            //
            attrs.erase(it);
            if(verbose && !n->get_attribute(attr))
            {
                std::cerr
                    << "*** Comparing attributes "
                    << attribute_to_str(attr)
                    << " (it should be set in this case)\n"
                    << *n
                    << '\n';
            }
            CATCH_REQUIRE(n->get_attribute(attr));
        }
    }

    CATCH_REQUIRE(attrs.empty());
}



void verify_child_node(
      std::string const & result_name
    , as2js::json::json_value::pointer_t expected
    , as2js::json::json_value::object_t const & json_object
    , as2js::node::pointer_t node
    , as2js::node::pointer_t link_node
    , char const * link_name
    , bool direct
    , bool verbose)
{
    as2js::json::json_value::object_t::const_iterator it_link(json_object.find(link_name));
    if(it_link != json_object.end())
    {
        // the children value must be an array
        //
        as2js::json::json_value::array_t const & array(it_link->second->get_array());
        std::size_t const max_links(array.size());
        if(link_node != nullptr)
        {
            if(direct)
            {
                if(verbose && max_links != 1)
                {
                    std::cerr
                        << "   Expecting "
                        << max_links
                        << " "
                        << link_name
                        << ", we always have 1 in the node (direct)\n";
                }
                CATCH_REQUIRE(max_links == 1);
                as2js::json::json_value::pointer_t link_value(array[0]);
                verify_result(result_name, link_value, link_node, verbose, true); // recursive
            }
            else
            {
                if(verbose && max_links != link_node->get_children_size())
                {
                    std::cerr
                        << "   Expecting "
                        << max_links
                        << " "
                        << link_name
                        << ", we have "
                        << link_node->get_children_size()
                        << " in the node\n";
                }
                CATCH_REQUIRE(max_links == link_node->get_children_size());
                for(std::size_t idx(0); idx < max_links; ++idx)
                {
                    as2js::json::json_value::pointer_t link_value(array[idx]);
                    verify_result(result_name, link_value, link_node->get_child(idx), verbose, false); // recursive
                }
            }
        }
        else
        {
            if(verbose && max_links != 0)
            {
                std::cerr
                    << "   "
                    << result_name
                    << ": Expecting "
                    << max_links
                    << ' '
                    << link_name
                    << ", we have no "
                    << link_name
                    << " at all in the node\n";
            }
            CATCH_REQUIRE(max_links == 0);
        }
    }
    else
    {
        // no children defined in the JSON, no children expected in the node
        //
        if(verbose
        && link_node != nullptr
        && link_node->get_children_size() != 0)
        {
            std::cerr
                << "   Expecting no \""
                << link_name
                << "\" list, we have "
                << link_node->get_children_size()
                << " "
                << link_name
                << " in the node:\n"
                << *node
                << "JSON position: "
                << expected->get_position()
                << "\nComparing against link node:\n"
                << *link_node;
        }
        bool const valid_link(link_node == nullptr || link_node->get_children_size() == 0);
        CATCH_REQUIRE(valid_link);
    }
}


void verify_result(
      std::string const & result_name
    , as2js::json::json_value::pointer_t expected
    , as2js::node::pointer_t node
    , bool verbose
    , bool ignore_children)
{
    std::string const node_type_string("node type");
    std::string const children_string("children");
    //std::string link_strings[static_cast<int>(as2js::link_t::LINK_max)];
    //link_strings[0].from_utf8("link instance");
    //link_strings[1].from_utf8("link type");
    //link_strings[2].from_utf8("link attributes");
    //link_strings[3].from_utf8("link goto exit");
    //link_strings[4].from_utf8("link goto enter");
    std::string const label_string("label");
    std::string const flags_string("flags");;
    std::string const attributes_string("attributes");
    std::string const integer_string("integer");
    std::string const float_string("float");

    CATCH_REQUIRE(expected->get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);
    as2js::json::json_value::object_t const & child_object(expected->get_object());

    as2js::json::json_value::object_t::const_iterator it_node_type(child_object.find(node_type_string));
    if(it_node_type == child_object.end())
    {
        std::cerr
            << "\nerror: \"node type\" is mandatory in your JSON:\n"
            << *expected
            << "\n";
        exit(1);
    }
    as2js::json::json_value::pointer_t node_type_value(it_node_type->second);
    if(verbose && node->get_type_name() != node_type_value->get_string())
    {
        std::cerr
            << "*** Comparing "
            << node->get_type_name()
            << " (node) vs "
            << node_type_value->get_string()
            << " (JSON) -- pos: "
            << expected->get_position()
            << " -- Node:\n"
            << *node
            << "JSON:\n"
            << *node_type_value;
        switch(node->get_type())
        {
        case as2js::node_t::NODE_IDENTIFIER:
            std::cerr << " \"" << node->get_string() << "\"";
            break;

        default:
            // no details for this node type
            break;

        }
        std::cerr << "\n";
    }
    CATCH_REQUIRE(node->get_type_name() == node_type_value->get_string());

    as2js::json::json_value::object_t::const_iterator it_label(child_object.find(label_string));
    if(it_label != child_object.end())
    {
        // we expect a string in this object
        if(verbose && node->get_string() != it_label->second->get_string())
        {
            std::cerr << "   Expecting string \"" << it_label->second->get_string() << "\", node has \"" << node->get_string() << "\"\n";
        }
        CATCH_REQUIRE(node->get_string() == it_label->second->get_string());
//std::cerr << "  -- labels are a match! [" << node->get_string() << "]\n";
    }
    else
    {
        // the node cannot have a string otherwise, so we expect a throw
        CATCH_REQUIRE_THROWS_MATCHES(
              node->get_string()
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: get_string() called with non-string node type: \""
                    + std::string(node->get_type_name())
                    + "\"."));
    }

    as2js::json::json_value::object_t::const_iterator it_flags(child_object.find(flags_string));
    if(it_flags != child_object.end())
    {
        // the tester declared as set of flags that are expected to be set
        //
        verify_flags(node, it_flags->second->get_string(), verbose);
    }
    else
    {
        // all flags must be unset
        //
        verify_flags(node, "", verbose);
    }

    // WARNING: these attributes are what we call IMMEDIATE ATTRIBUTES in case
    //          of the parser because the parser also makes use of a
    //          LINK_ATTRIBUTES which represents a list of attributes
    as2js::json::json_value::object_t::const_iterator it_attributes(child_object.find(attributes_string));
    if(it_attributes != child_object.end())
    {
        // the tester declared as set of attributes that are expected to be set
        verify_attributes(node, it_attributes->second->get_string(), verbose);
    }
    else
    {
        // all attributes must be unset
        verify_attributes(node, "", verbose);
    }

    as2js::json::json_value::object_t::const_iterator it_integer(child_object.find(integer_string));
    if(it_integer != child_object.end())
    {
        // we expect an integer in this object
        if(node->get_integer().get() != it_integer->second->get_integer().get())
        {
            std::cerr << "   Expecting " << it_integer->second->get_integer().get() << ", got " << node->get_integer().get() << " in the node\n";
        }
        CATCH_REQUIRE(node->get_integer().get() == it_integer->second->get_integer().get());
    }
    else
    {
        // the node cannot have an integer otherwise, so we expect a throw
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              node->get_integer()
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: get_integer() called with a non-integer node type."));
    }

    as2js::json::json_value::object_t::const_iterator it_float(child_object.find(float_string));
    if(it_float != child_object.end())
    {
        // if we expect a NaN we have to compare specifically
        // because (NaN == NaN) always returns false
        //
        if(it_float->second->get_floating_point().is_nan())
        {
            CATCH_REQUIRE(node->get_floating_point().is_nan());
        }
        else if(it_float->second->get_floating_point().is_positive_infinity())
        {
            CATCH_REQUIRE(node->get_floating_point().is_positive_infinity());
        }
        else if(it_float->second->get_floating_point().is_negative_infinity())
        {
            CATCH_REQUIRE(node->get_floating_point().is_negative_infinity());
        }
        else
        {
            // we expect a floating point number in this object
            if(node->get_floating_point().get() - it_float->second->get_floating_point().get() > 0.0001)
            {
                std::cerr << "   Expecting " << it_float->second->get_floating_point().get() << ", got " << node->get_floating_point().get() << " in the node\n";
            }
            CATCH_REQUIRE(node->get_floating_point().get() - it_float->second->get_floating_point().get() <= 0.0001);

            // further, if the float is zero, it may be +0.0 or -0.0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(it_float->second->get_floating_point().get() == 0.0)
            {
                CATCH_REQUIRE(std::signbit(node->get_floating_point().get()) == std::signbit(it_float->second->get_floating_point().get()));
            }
#pragma GCC diagnostic pop
        }
    }
    else
    {
        // the node cannot have a floating point otherwise, so we expect a throw
        CATCH_REQUIRE_THROWS_MATCHES(
              node->get_floating_point()
            , as2js::internal_error
            , Catch::Matchers::ExceptionMessage(
                      "internal_error: get_floating_point() called with a non-floating point node type."));
    }

// COMPILER / OPTIMIZER SPECIFIC?
    // certain links asks us to ignore the links and children because
    // we do not want to duplicate the whole type classes a hundred times...
    if(!ignore_children)
    {
//std::cerr << "Node is [" << *node << "]\n";

        // verify the links
        verify_child_node(result_name, expected, child_object, node, node->get_instance(),        "instance",       true,  verbose);
        verify_child_node(result_name, expected, child_object, node, node->get_type_node(),       "type node",      true,  verbose);
        verify_child_node(result_name, expected, child_object, node, node->get_attribute_node(),  "attribute node", false, verbose);
        verify_child_node(result_name, expected, child_object, node, node->get_goto_exit(),       "goto exit",      false, verbose);
        verify_child_node(result_name, expected, child_object, node, node->get_goto_enter(),      "goto enter",     false, verbose);

//        // List of links are tested just like children, only the list starts somewhere else
//        for(int link_idx(0); link_idx < static_cast<int>(as2js::Node::link_t::LINK_max); ++link_idx)
//        {
//            as2js::String link_name;
//            switch(static_cast<as2js::Node::link_t>(link_idx))
//            {
//            case as2js::Node::link_t::LINK_INSTANCE:
//                link_name = "instance";
//                break;
//
//            case as2js::Node::link_t::LINK_TYPE:
//                link_name = "type";
//                break;
//
//            case as2js::Node::link_t::LINK_ATTRIBUTES:
//                link_name = "attributes";
//                break;
//
//            case as2js::Node::link_t::LINK_GOTO_EXIT:
//                link_name = "goto-exit";
//                break;
//
//            case as2js::Node::link_t::LINK_GOTO_ENTER:
//                link_name = "goto-enter";
//                break;
//
//            case as2js::Node::link_t::LINK_max:
//                CATCH_REQUIRE(!"LINK_max reached when getting the link type");
//                break;
//
//            }
//            bool direct(false);
//            as2js::Node::pointer_t link_node(node->get_link(static_cast<as2js::Node::link_t>(link_idx)));
//            if(link_node)
//            {
//                // make sure root node is of the right type
//                // Why did I write this that way? The types from the time
//                // we created the tree in the parser are still around...
//                switch(static_cast<as2js::Node::link_t>(link_idx))
//                {
//                case as2js::Node::link_t::LINK_INSTANCE:
//                    direct = true;
////std::cerr << "Instance [" << *link_node << "]\n";
//                    //CATCH_REQUIRE(!"compiler does not use LINK_INSTANCE");
//                    break;
//
//                case as2js::Node::link_t::LINK_TYPE:
//                    direct = true;
////std::cerr << "Type [" << *link_node << "]\n";
//                    //CATCH_REQUIRE(!"compiler does not use LINK_TYPE");
//                    break;
//
//                case as2js::Node::link_t::LINK_ATTRIBUTES:
//                    direct = false;
//                    CATCH_REQUIRE(link_node->get_type() == as2js::Node::node_t::NODE_ATTRIBUTES);
//                    break;
//
//                case as2js::Node::link_t::LINK_GOTO_EXIT:
//                    CATCH_REQUIRE(!"compiler does not use LINK_GOTO_EXIT");
//                    break;
//
//                case as2js::Node::link_t::LINK_GOTO_ENTER:
//                    CATCH_REQUIRE(!"compiler does not use LINK_GOTO_ENTER");
//                    break;
//
//                case as2js::Node::link_t::LINK_max:
//                    CATCH_REQUIRE(!"LINK_max reached when testing the link_node type");
//                    break;
//
//                }
//            }
//            as2js::JSON::JSONValue::object_t::const_iterator it_link(child_object.find(link_strings[link_idx]));
//            if(it_link != child_object.end())
//            {
//                // the children value must be an array
//                as2js::JSON::JSONValue::array_t const& array(it_link->second->get_array());
//                size_t const max_links(array.size());
//                if(link_node)
//                {
//                    if(direct)
//                    {
//                        if(verbose && max_links != 1)
//                        {
//                            std::cerr << "   Expecting " << max_links << " " << link_name << ", we always have 1 in the node (direct)\n";
//                        }
//                        CATCH_REQUIRE(max_links == 1);
//                        as2js::JSON::JSONValue::pointer_t link_value(array[0]);
//                        verify_result(result_name, link_value, link_node, verbose, true); // recursive
//                    }
//                    else
//                    {
//                        if(verbose && max_links != link_node->get_children_size())
//                        {
//                            std::cerr << "   Expecting " << max_links << " " << link_name << ", we have " << link_node->get_children_size() << " in the node\n";
//                        }
//                        CATCH_REQUIRE(max_links == link_node->get_children_size());
//                        for(size_t idx(0); idx < max_links; ++idx)
//                        {
//                            as2js::JSON::JSONValue::pointer_t link_value(array[idx]);
//                            verify_result(result_name, link_value, link_node->get_child(idx), verbose, false); // recursive
//                        }
//                    }
//                }
//                else
//                {
//                    if(verbose && max_links != 0)
//                    {
//                        std::cerr << "   Expecting " << max_links << " " << link_name << ", we have no " << link_name << " at all in the node\n";
//                    }
//                    CATCH_REQUIRE(max_links == 0);
//                }
//            }
//            else
//            {
//                // no children defined in the JSON, no children expected in the node
//                if(verbose && link_node && link_node->get_children_size() != 0)
//                {
//                    std::cerr << "   Expecting no " << link_name << " list, we have " << link_node->get_children_size() << " " << link_name << " in the node\n";
//                }
//                CATCH_REQUIRE(!link_node || link_node->get_children_size() == 0);
//            }
//        }

        as2js::json::json_value::object_t::const_iterator it_children(child_object.find(children_string));
        if(it_children != child_object.end())
        {
            // the children value must be an array
            as2js::json::json_value::array_t const& array(it_children->second->get_array());
            size_t const max_children(array.size());
            if(verbose && max_children != node->get_children_size())
            {
                std::cerr
                    << "   Expecting "
                    << max_children
                    << " children, we have "
                    << node->get_children_size()
                    << " in the node:\n"
                    << *node;
            }
            CATCH_REQUIRE(max_children == node->get_children_size());
            for(size_t idx(0); idx < max_children; ++idx)
            {
                as2js::json::json_value::pointer_t children_value(array[idx]);
                verify_result(result_name, children_value, node->get_child(idx), verbose, ignore_children); // recursive
            }
        }
        else
        {
            // no children defined in the JSON, no children expected in the node
            if(verbose && node->get_children_size() != 0)
            {
                std::cerr
                    << "   Expecting no children, we have "
                    << node->get_children_size()
                    << " in the node:\n"
                    << *node
                    << "\n";
            }
            CATCH_REQUIRE(node->get_children_size() == 0);
        }
    }
}


// Parser verification
void verify_parser_result(
      std::string const & result_name
    , as2js::json::json_value::pointer_t expected
    , as2js::node::pointer_t node
    , bool verbose
    , bool ignore_children)
{
    verify_result(result_name, expected, node, verbose, ignore_children);

    // the parser does not define these so we expect them all to be null pointers
    CATCH_REQUIRE(node->get_instance() == nullptr);
    CATCH_REQUIRE(node->get_type_node() == nullptr);
    CATCH_REQUIRE(node->get_goto_exit() == nullptr);
    CATCH_REQUIRE(node->get_goto_enter() == nullptr);

    as2js::json::json_value::object_t const & child_object(expected->get_object());
    as2js::json::json_value::object_t::const_iterator it_attribute(child_object.find("attribute node"));
    as2js::node::pointer_t attribute_node(node->get_attribute_node());
    if(attribute_node != nullptr)
    {
        // if it exists it must be a NODE_ATTRIBUTES type
        CATCH_REQUIRE(attribute_node->get_type() == as2js::node_t::NODE_ATTRIBUTES);

        if(it_attribute == child_object.end())
        {
            std::size_t const count(attribute_node->get_children_size());
            if(verbose && count > 0)
            {
                std::cerr
                    << "   Expecting no \"attributes\", we have "
                    << count
                    << " in the node\n";
            }
            CATCH_REQUIRE(count == 0);
        }
        else
        {
            // the children value must be an array
            as2js::json::json_value::array_t const & array(it_attribute->second->get_array());
            size_t const max_links(array.size());
            if(verbose && max_links != attribute_node->get_children_size())
            {
                std::cerr
                    << "   Expecting "
                    << max_links
                    << " instance, we have "
                    << attribute_node->get_children_size()
                    << " in the node\n";
            }
            CATCH_REQUIRE(max_links == attribute_node->get_children_size());
            for(size_t idx(0); idx < max_links; ++idx)
            {
                as2js::json::json_value::pointer_t attribute_value(array[idx]);
                verify_result(result_name, attribute_value, attribute_node->get_child(idx), verbose, false); // recursive
            }
        }
    }
    else
    {
        // no attributes in the node, no children expected in the JSON
        if(verbose && it_attribute != child_object.end())
        {
            size_t const count(it_attribute->second->get_array().size());
            std::cerr << "   Expecting " << count << " \"attributes\", we have none in the node\n";
        }
        CATCH_REQUIRE(it_attribute == child_object.end());
    }
}



} // namespace SNAP_CATCH2_NAMESPACE


Catch::Clara::Parser add_command_line_options(Catch::Clara::Parser const & cli)
{
    return cli
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_as2js_compiler, "as2js")
              ["--as2js"]
              ("path to the as2js compiler.")
         | Catch::Clara::Opt(SNAP_CATCH2_NAMESPACE::g_run_destructive)
              ["--destructive"]
              ("also run the various destructive/problematic tests that can run on their own but probably not along others (if not specified, skip those tests).")
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
    if(SNAP_CATCH2_NAMESPACE::catch_compiler_init() != 0)
    {
        return 1;
    }

    return 0;
}


void cleanup_test()
{
    SNAP_CATCH2_NAMESPACE::catch_compiler_cleanup();
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
            , &cleanup_test
        );
}


// vim: ts=4 sw=4 et
