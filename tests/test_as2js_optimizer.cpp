/* tests/test_as2js_optimizer.cpp

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

#include    "test_as2js_optimizer.h"
#include    "test_as2js_main.h"

#include    "as2js/optimizer.h"
#include    "as2js/parser.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"
#include    "as2js/json.h"

#include    <unistd.h>
#include    <sys/stat.h>

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsOptimizerUnitTests );

namespace
{


struct err_to_string_t
{
    as2js::err_code_t       f_code;
    char const *            f_name;
    int                     f_line;
};

#define    TO_STR_sub(s)            #s
#define    ERROR_NAME(err)     { as2js::err_code_t::AS_ERR_##err, TO_STR_sub(err), __LINE__ }

err_to_string_t const g_error_table[] =
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
    ERROR_NAME(UNTERMINATED_STRING),
    ERROR_NAME(UNEXPECTED_EOF),
    ERROR_NAME(UNEXPECTED_PUNCTUATION),
    ERROR_NAME(UNEXPECTED_TOKEN),
    ERROR_NAME(UNEXPECTED_DATABASE),
    ERROR_NAME(UNEXPECTED_RC)
};
size_t const g_error_table_size = sizeof(g_error_table) / sizeof(g_error_table[0]);


as2js::err_code_t str_to_error_code(as2js::String const& error_name)
{
    for(size_t idx(0); idx < g_error_table_size; ++idx)
    {
        if(error_name == g_error_table[idx].f_name)
        {
            return g_error_table[idx].f_code;
        }
    }
    std::cerr << "Error name \"" << error_name << "\" not found.\n";
    CPPUNIT_ASSERT(!"error name not found, test_as2js_optimizer.cpp bug");
    return as2js::err_code_t::AS_ERR_NONE;
}


char const *error_code_to_str(as2js::err_code_t const& error_code)
{
    for(size_t idx(0); idx < g_error_table_size; ++idx)
    {
        if(error_code == g_error_table[idx].f_code)
        {
            return g_error_table[idx].f_name;
        }
    }
    std::cerr << "Error code \"" << static_cast<int>(error_code) << "\" not found.\n";
    CPPUNIT_ASSERT(!"error code not found, test_as2js_optimizer.cpp bug");
    return "unknown";
}



class test_callback : public as2js::MessageCallback
{
public:
    test_callback(bool verbose)
        : f_verbose(verbose)
    {
        as2js::Message::set_message_callback(this);
        g_warning_count = as2js::Message::warning_count();
        g_error_count = as2js::Message::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::Message::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {
        // skip trace messages which happen all the time because of the
        // lexer debug option
        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_TRACE)
        {
            return;
        }

        if(f_expected.empty())
        {
            std::cerr << "\n*** STILL NECESSARY ***\n";
            std::cerr << "filename = " << pos.get_filename() << "\n";
            std::cerr << "msg = " << message << "\n";
            std::cerr << "page = " << pos.get_page() << "\n";
            std::cerr << "line = " << pos.get_line() << "\n";
            std::cerr << "error_code = " << static_cast<int>(error_code) << " (" << error_code_to_str(error_code) << ")\n";
        }

        CPPUNIT_ASSERT(!f_expected.empty());

        if(f_verbose)
        {
            std::cerr << "filename = " << pos.get_filename() << " (node) / " << f_expected[0].f_pos.get_filename() << " (JSON)\n";
            std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << "\n";
            std::cerr << "page line = " << pos.get_page_line() << " / " << f_expected[0].f_pos.get_page_line() << "\n";
            std::cerr << "error_code = " << static_cast<int>(error_code) << " (" << error_code_to_str(error_code) << ") / " << static_cast<int>(f_expected[0].f_error_code) << " (" << error_code_to_str(f_expected[0].f_error_code) << ")\n";
        }

        CPPUNIT_ASSERT(f_expected[0].f_call);
        CPPUNIT_ASSERT(message_level == f_expected[0].f_message_level);
        CPPUNIT_ASSERT(error_code == f_expected[0].f_error_code);
        CPPUNIT_ASSERT(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CPPUNIT_ASSERT(pos.get_function() == f_expected[0].f_pos.get_function());
        CPPUNIT_ASSERT(pos.get_page() == f_expected[0].f_pos.get_page());
        CPPUNIT_ASSERT(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CPPUNIT_ASSERT(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CPPUNIT_ASSERT(pos.get_line() == f_expected[0].f_pos.get_line());
        CPPUNIT_ASSERT(message == f_expected[0].f_message.to_utf8());

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CPPUNIT_ASSERT(g_warning_count == as2js::Message::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::Message::error_count() << "\n";
            CPPUNIT_ASSERT(g_error_count == as2js::Message::error_count());
        }

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        if(!f_expected.empty())
        {
            std::cerr << "\n*** STILL " << f_expected.size() << " EXPECTED ***\n";
            std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
            std::cerr << "msg = " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "error_code = " << static_cast<int>(f_expected[0].f_error_code) << "\n";
        }
        CPPUNIT_ASSERT(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::Position             f_pos = as2js::Position();
        as2js::String               f_message = as2js::String(); // UTF-8 string
    };

    std::vector<expected_t>     f_expected = std::vector<expected_t>();
    bool                        f_verbose = false;

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;



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
struct named_options
{
    as2js::Options::option_t    f_option;
    char const *                f_name;
    char const *                f_neg_name;
    int                         f_value;
};
named_options const g_options[] =
{
    {
        as2js::Options::option_t::OPTION_ALLOW_WITH,
        "allow_with",
        "no_allow_with",
        1
    },
    {
        as2js::Options::option_t::OPTION_BINARY,
        "binary",
        "no_binary",
        1
    },
    {
        as2js::Options::option_t::OPTION_COVERAGE,
        "coverage",
        "no_coverage",
        1
    },
    {
        as2js::Options::option_t::OPTION_DEBUG,
        "debug",
        "no_debug",
        1
    },
    {
        as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES,
        "extended_escape_sequences",
        "no_extended_escape_sequences",
        1
    },
    {
        as2js::Options::option_t::OPTION_EXTENDED_OPERATORS,
        "extended_operators",
        "no_extended_operators",
        1
    },
    {
        as2js::Options::option_t::OPTION_EXTENDED_OPERATORS,
        "extended_operators_safe",
        "no_extended_operators_safe",
        2
    },
    {
        as2js::Options::option_t::OPTION_EXTENDED_STATEMENTS,
        "extended_statements",
        "no_extended_statements",
        1
    },
    {
        as2js::Options::option_t::OPTION_EXTENDED_STATEMENTS,
        "extended_statements_safe",
        "no_extended_statements_safe",
        2
    },
    //{ -- this one does not make sense here
    //    as2js::Options::option_t::OPTION_JSON,
    //    "json",
    //    "no_json"
    //},
    {
        as2js::Options::option_t::OPTION_OCTAL,
        "octal",
        "no_octal",
        1
    },
    {
        as2js::Options::option_t::OPTION_STRICT,
        "strict",
        "no_strict",
        1
    },
    {
        as2js::Options::option_t::OPTION_TRACE,
        "trace",
        "no_trace",
        1
    },
    {
        as2js::Options::option_t::OPTION_UNSAFE_MATH,
        "unsafe_math",
        "no_unsafe_math",
        1
    }
};
size_t const g_options_size(sizeof(g_options) / sizeof(g_options[0]));


struct flg_to_string_t
{
    as2js::Node::flag_t     f_flag;
    char const *            f_name;
    int                     f_line;
};

#define    FLAG_NAME(flg)     { as2js::Node::flag_t::NODE_##flg, TO_STR_sub(flg), __LINE__ }

flg_to_string_t const g_flag_table[] =
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
size_t const g_flag_table_size = sizeof(g_flag_table) / sizeof(g_flag_table[0]);


as2js::Node::flag_t str_to_flag_code(as2js::String const& flag_name)
{
    for(size_t idx(0); idx < g_flag_table_size; ++idx)
    {
        if(flag_name == g_flag_table[idx].f_name)
        {
            return g_flag_table[idx].f_flag;
        }
    }
    CPPUNIT_ASSERT(!"flag code not found, test_as2js_optimizer.cpp bug");
    return as2js::Node::flag_t::NODE_FLAG_max;
}


as2js::String flag_to_str(as2js::Node::flag_t& flg)
{
    for(size_t idx(0); idx < g_flag_table_size; ++idx)
    {
        if(flg == g_flag_table[idx].f_flag)
        {
            return g_flag_table[idx].f_name;
        }
    }
    CPPUNIT_ASSERT(!"flag code not found, test_as2js_optimizer.cpp bug");
    return "";
}




void verify_flags(as2js::Node::pointer_t node, as2js::String const& flags_set, bool verbose)
{
    // list of flags that have to be set
    std::vector<as2js::Node::flag_t> flgs;
    as2js::as_char_t const *f(flags_set.c_str());
    as2js::as_char_t const *s(f);
    for(;;)
    {
        if(*f == ',' || *f == '\0')
        {
            if(s == f)
            {
                break;
            }
            as2js::String name(s, f - s);
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
    std::vector<as2js::Node::flag_t> flgs_to_check;
    switch(node->get_type())
    {
    case as2js::Node::node_t::NODE_CATCH:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_CATCH_FLAG_TYPED);
        break;

    case as2js::Node::node_t::NODE_DIRECTIVE_LIST:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES);
        break;

    case as2js::Node::node_t::NODE_ENUM:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_ENUM_FLAG_CLASS);
        break;

    case as2js::Node::node_t::NODE_FOR:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FOR_FLAG_CONST);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FOR_FLAG_FOREACH);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FOR_FLAG_IN);
        break;

    case as2js::Node::node_t::NODE_FUNCTION:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_GETTER);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_NEVER);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_NOPARAMS);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_OUT);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_SETTER);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_FUNCTION_FLAG_VOID);
        break;

    case as2js::Node::node_t::NODE_IDENTIFIER:
    case as2js::Node::node_t::NODE_VIDENTIFIER:
    case as2js::Node::node_t::NODE_STRING:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_IDENTIFIER_FLAG_WITH);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_IDENTIFIER_FLAG_TYPED);
        break;

    case as2js::Node::node_t::NODE_IMPORT:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_IMPORT_FLAG_IMPLEMENTS);
        break;

    case as2js::Node::node_t::NODE_PACKAGE:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED);
        break;

    case as2js::Node::node_t::NODE_PARAM_MATCH:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED);
        break;

    case as2js::Node::node_t::NODE_PARAM:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_CATCH);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_CONST);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_IN);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_OUT);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_NAMED);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_PARAMREF);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_REFERENCED);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_REST);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_UNCHECKED);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_PARAM_FLAG_UNPROTOTYPED);
        break;

    case as2js::Node::node_t::NODE_SWITCH:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_SWITCH_FLAG_DEFAULT);
        break;

    case as2js::Node::node_t::NODE_TYPE:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_TYPE_FLAG_MODULO);
        break;

    case as2js::Node::node_t::NODE_VARIABLE:
    case as2js::Node::node_t::NODE_VAR_ATTRIBUTES:
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_CONST);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_FINAL);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_LOCAL);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_MEMBER);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_ENUM);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_COMPILED);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_INUSE);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_ATTRS);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_DEFINED);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_DEFINING);
        flgs_to_check.push_back(as2js::Node::flag_t::NODE_VARIABLE_FLAG_TOADD);
        break;

    default:
        // no flags supported
        break;

    }

    CPPUNIT_ASSERT(flgs.size() <= flgs_to_check.size());

    for(size_t idx(0); idx < flgs_to_check.size(); ++idx)
    {
        as2js::Node::flag_t flg(flgs_to_check[idx]);
        std::vector<as2js::Node::flag_t>::iterator it(std::find(flgs.begin(), flgs.end(), flg));
        if(it == flgs.end())
        {
            // expected to be unset
            if(verbose && node->get_flag(flg))
            {
                std::cerr << "*** Comparing flags " << flag_to_str(flg) << " (should not be set)\n";
            }
            CPPUNIT_ASSERT(!node->get_flag(flg));
        }
        else
        {
            // expected to be set
            flgs.erase(it);
            if(verbose && !node->get_flag(flg))
            {
                std::cerr << "*** Comparing flags " << flag_to_str(flg) << " (it should be set in this case)\n";
            }
            CPPUNIT_ASSERT(node->get_flag(flg));
        }
    }

    CPPUNIT_ASSERT(flgs.empty());
}




struct attr_to_string_t
{
    as2js::Node::attribute_t    f_attribute;
    char const *                f_name;
    int                         f_line;
};

#define    ATTRIBUTE_NAME(attr)      { as2js::Node::attribute_t::NODE_ATTR_##attr, TO_STR_sub(attr), __LINE__ }

attr_to_string_t const g_attribute_table[] =
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
size_t const g_attribute_table_size = sizeof(g_attribute_table) / sizeof(g_attribute_table[0]);


as2js::Node::attribute_t str_to_attribute_code(as2js::String const& attr_name)
{
    for(size_t idx(0); idx < g_attribute_table_size; ++idx)
    {
        if(attr_name == g_attribute_table[idx].f_name)
        {
            return g_attribute_table[idx].f_attribute;
        }
    }
    CPPUNIT_ASSERT(!"attribute code not found, test_as2js_optimizer.cpp bug");
    return as2js::Node::attribute_t::NODE_ATTR_max;
}


as2js::String attribute_to_str(as2js::Node::attribute_t& attr)
{
    for(size_t idx(0); idx < g_attribute_table_size; ++idx)
    {
        if(attr == g_attribute_table[idx].f_attribute)
        {
            return g_attribute_table[idx].f_name;
        }
    }
    CPPUNIT_ASSERT(!"attribute code not found, test_as2js_optimizer.cpp bug");
    return "";
}




void verify_attributes(as2js::Node::pointer_t node, as2js::String const& attributes_set, bool verbose)
{
    // list of attributes that have to be set
    std::vector<as2js::Node::attribute_t> attrs;
    as2js::as_char_t const *a(attributes_set.c_str());
    as2js::as_char_t const *s(a);
    for(;;)
    {
        if(*a == ',' || *a == '\0')
        {
            if(s == a)
            {
                break;
            }
            as2js::String name(s, a - s);
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
    std::vector<as2js::Node::attribute_t> attrs_to_check;

    if(node->get_type() != as2js::Node::node_t::NODE_PROGRAM)
    {
        // except for PROGRAM, all attributes always apply
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_PUBLIC);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_PRIVATE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_PROTECTED);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_INTERNAL);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_TRANSIENT);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_VOLATILE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_STATIC);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_ABSTRACT);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_VIRTUAL);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_ARRAY);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_REQUIRE_ELSE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_ENSURE_THEN);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_NATIVE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_DEPRECATED);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_UNSAFE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_CONSTRUCTOR);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_FINAL);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_ENUMERABLE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_TRUE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_FALSE);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_UNUSED);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_DYNAMIC);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_FOREACH);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_NOBREAK);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_AUTOBREAK);
        attrs_to_check.push_back(as2js::Node::attribute_t::NODE_ATTR_DEFINED);
    }

    CPPUNIT_ASSERT(attrs.size() <= attrs_to_check.size());

    for(size_t idx(0); idx < attrs_to_check.size(); ++idx)
    {
        as2js::Node::attribute_t attr(attrs_to_check[idx]);
        std::vector<as2js::Node::attribute_t>::iterator it(std::find(attrs.begin(), attrs.end(), attr));
        if(it == attrs.end())
        {
            // expected to be unset
            if(verbose && node->get_attribute(attr))
            {
                std::cerr << "*** Comparing attributes " << attribute_to_str(attr) << " (should not be set)\n";
            }
            CPPUNIT_ASSERT(!node->get_attribute(attr));
        }
        else
        {
            // expected to be set
            attrs.erase(it);
            if(verbose && !node->get_attribute(attr))
            {
                std::cerr << "*** Comparing attributes " << attribute_to_str(attr) << " (it should be set in this case)\n";
            }
            CPPUNIT_ASSERT(node->get_attribute(attr));
        }
    }

    CPPUNIT_ASSERT(attrs.empty());
}




void verify_result(as2js::JSON::JSONValue::pointer_t expected, as2js::Node::pointer_t node, bool verbose)
{
    as2js::String node_type_string;
    node_type_string.from_utf8("node type");
    as2js::String children_string;
    children_string.from_utf8("children");
    //as2js::String link_strings[static_cast<int>(as2js::Node::link_t::LINK_max)];
    //link_strings[0].from_utf8("link instance");
    //link_strings[1].from_utf8("link type");
    //link_strings[2].from_utf8("link attributes");
    //link_strings[3].from_utf8("link goto exit");
    //link_strings[4].from_utf8("link goto enter");
    as2js::String label_string;
    label_string.from_utf8("label");
    as2js::String flags_string;
    flags_string.from_utf8("flags");
    as2js::String attributes_string;
    attributes_string.from_utf8("attributes");
    as2js::String integer_string;
    integer_string.from_utf8("integer");
    as2js::String float_string;
    float_string.from_utf8("float");

    CPPUNIT_ASSERT(expected->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
    as2js::JSON::JSONValue::object_t const& child_object(expected->get_object());

    as2js::JSON::JSONValue::object_t::const_iterator it_node_type(child_object.find(node_type_string));
    if(it_node_type == child_object.end())
    {
        std::cerr << "\nerror: \"node type\" is mandatory in your JSON.\n";
        exit(1);
    }
    as2js::JSON::JSONValue::pointer_t node_type_value(it_node_type->second);
    if(verbose || node->get_type_name() != node_type_value->get_string())
    {
        std::cerr << "*** Comparing " << node->get_type_name() << " (node) vs " << node_type_value->get_string() << " (JSON)\n";
    }
    CPPUNIT_ASSERT(node->get_type_name() == node_type_value->get_string());

    as2js::JSON::JSONValue::object_t::const_iterator it_label(child_object.find(label_string));
    if(it_label != child_object.end())
    {
        // we expect a string in this object
        if(node->get_string() != it_label->second->get_string())
        {
            std::cerr << "   Expecting string \"" << it_label->second->get_string() << "\", node has \"" << node->get_string() << "\"\n";
        }
        CPPUNIT_ASSERT(node->get_string() == it_label->second->get_string());
    }
    else
    {
        // the node cannot have a string otherwise, so we expect a throw
        CPPUNIT_ASSERT_THROW(node->get_string(), as2js::exception_internal_error);
    }

    as2js::JSON::JSONValue::object_t::const_iterator it_flags(child_object.find(flags_string));
    if(it_flags != child_object.end())
    {
        // the tester declared as set of flags that are expected to be set
        verify_flags(node, it_flags->second->get_string(), verbose);
    }
    else
    {
        // all flags must be unset
        verify_flags(node, "", verbose);
    }

    // WARNING: these attributes are what we call IMMEDIATE ATTRIBUTES in case
    //          of the parser because the parser also makes use of a
    //          LINK_ATTRIBUTES which represents a list of attributes
    as2js::JSON::JSONValue::object_t::const_iterator it_attributes(child_object.find(attributes_string));
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

    as2js::JSON::JSONValue::object_t::const_iterator it_integer(child_object.find(integer_string));
    if(it_integer != child_object.end())
    {
        // we expect an integer in this object
        if(node->get_int64().get() != it_integer->second->get_int64().get())
        {
            std::cerr << "   Expecting " << it_integer->second->get_int64().get() << ", got " << node->get_int64().get() << " in the node\n";
        }
        CPPUNIT_ASSERT(node->get_int64().get() == it_integer->second->get_int64().get());
    }
    else
    {
        // the node cannot have an integer otherwise, so we expect a throw
        CPPUNIT_ASSERT_THROW(node->get_int64(), as2js::exception_internal_error);
    }

    as2js::JSON::JSONValue::object_t::const_iterator it_float(child_object.find(float_string));
    if(it_float != child_object.end())
    {
        // if we expect a NaN we have to compare specifically
        // because (NaN == NaN) always returns false
        if(it_float->second->get_float64().is_NaN())
        {
            CPPUNIT_ASSERT(node->get_float64().is_NaN());
        }
        else if(it_float->second->get_float64().is_positive_infinity())
        {
            CPPUNIT_ASSERT(node->get_float64().is_positive_infinity());
        }
        else if(it_float->second->get_float64().is_negative_infinity())
        {
            CPPUNIT_ASSERT(node->get_float64().is_negative_infinity());
        }
        else
        {
            // we expect a floating point in this object
            if(node->get_float64().get() - it_float->second->get_float64().get() > 0.0001)
            {
                std::cerr << "   Expecting " << it_float->second->get_float64().get() << ", got " << node->get_float64().get() << " in the node\n";
            }
            CPPUNIT_ASSERT(node->get_float64().get() - it_float->second->get_float64().get() <= 0.0001);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            // further, if the float is zero, it may be +0.0 or -0.0
            if(it_float->second->get_float64().get() == 0.0)
            {
                CPPUNIT_ASSERT(std::signbit(node->get_float64().get()) == std::signbit(it_float->second->get_float64().get()));
            }
#pragma GCC diagnostic pop
        }
    }
    else
    {
        // the node cannot have a float otherwise, so we expect a throw
        CPPUNIT_ASSERT_THROW(node->get_float64(), as2js::exception_internal_error);
    }

    // List of links are tested just like children, only the list starts somewhere else
    //for(int link_idx(0); link_idx < static_cast<int>(as2js::Node::link_t::LINK_max); ++link_idx)
    //{
    //    as2js::String link_name;
    //    switch(static_cast<as2js::Node::link_t>(link_idx))
    //    {
    //    case as2js::Node::link_t::LINK_INSTANCE:
    //        link_name = "instance";
    //        break;

    //    case as2js::Node::link_t::LINK_TYPE:
    //        link_name = "type";
    //        break;

    //    case as2js::Node::link_t::LINK_ATTRIBUTES:
    //        link_name = "attributes";
    //        break;

    //    case as2js::Node::link_t::LINK_GOTO_EXIT:
    //        link_name = "goto-exit";
    //        break;

    //    case as2js::Node::link_t::LINK_GOTO_ENTER:
    //        link_name = "goto-enter";
    //        break;

    //    case as2js::Node::link_t::LINK_max:
    //        CPPUNIT_ASSERT(!"LINK_max reached when getting the link type");
    //        break;

    //    }
    //    as2js::JSON::JSONValue::object_t::const_iterator it_link(child_object.find(link_strings[link_idx]));
    //    as2js::Node::pointer_t link_node(node->get_link(static_cast<as2js::Node::link_t>(link_idx)));
    //    if(link_node)
    //    {
    //        // make sure root node is of the right type
    //        switch(static_cast<as2js::Node::link_t>(link_idx))
    //        {
    //        case as2js::Node::link_t::LINK_INSTANCE:
    //            CPPUNIT_ASSERT(!"optimizer does not use LINK_INSTANCE");
    //            break;

    //        case as2js::Node::link_t::LINK_TYPE:
    //            CPPUNIT_ASSERT(!"optimizer does not use LINK_TYPE");
    //            break;

    //        case as2js::Node::link_t::LINK_ATTRIBUTES:
    //            CPPUNIT_ASSERT(link_node->get_type() == as2js::Node::node_t::NODE_ATTRIBUTES);
    //            break;

    //        case as2js::Node::link_t::LINK_GOTO_EXIT:
    //            CPPUNIT_ASSERT(!"optimizer does not use LINK_GOTO_EXIT");
    //            break;

    //        case as2js::Node::link_t::LINK_GOTO_ENTER:
    //            CPPUNIT_ASSERT(!"optimizer does not use LINK_GOTO_ENTER");
    //            break;

    //        case as2js::Node::link_t::LINK_max:
    //            CPPUNIT_ASSERT(!"LINK_max reached when testing the link_node type");
    //            break;

    //        }
    //    }
    //    if(it_link != child_object.end())
    //    {
    //        // the children value must be an array
    //        as2js::JSON::JSONValue::array_t const& array(it_link->second->get_array());
    //        size_t const max_links(array.size());
    //        if(link_node)
    //        {
    //            if(verbose && max_links != link_node->get_children_size())
    //            {
    //                std::cerr << "   Expecting " << max_links << " " << link_name << ", we have " << link_node->get_children_size() << " in the node\n";
    //            }
    //            CPPUNIT_ASSERT(max_links == link_node->get_children_size());
    //            for(size_t idx(0); idx < max_links; ++idx)
    //            {
    //                as2js::JSON::JSONValue::pointer_t link_value(array[idx]);
    //                verify_result(link_value, link_node->get_child(idx), verbose); // recursive
    //            }
    //        }
    //        else
    //        {
    //            if(verbose && max_links != 0)
    //            {
    //                std::cerr << "   Expecting " << max_links << " " << link_name << ", we have no " << link_name << " at all in the node\n";
    //            }
    //            CPPUNIT_ASSERT(max_links == 0);
    //        }
    //    }
    //    else
    //    {
    //        // no children defined in the JSON, no children expected in the node
    //        if(verbose && link_node && link_node->get_children_size() != 0)
    //        {
    //            std::cerr << "   Expecting no " << link_name << " list, we have " << link_node->get_children_size() << " " << link_name << " in the node\n";
    //        }
    //        CPPUNIT_ASSERT(!link_node || link_node->get_children_size() == 0);
    //    }
    //}

    as2js::JSON::JSONValue::object_t::const_iterator it_children(child_object.find(children_string));
    if(it_children != child_object.end())
    {
        // the children value must be an array
        as2js::JSON::JSONValue::array_t const& array(it_children->second->get_array());
        size_t const max_children(array.size());
        if(verbose && max_children != node->get_children_size())
        {
            std::cerr << "   Expecting " << max_children << " children, we have " << node->get_children_size() << " in the node\n";
        }
        CPPUNIT_ASSERT(max_children == node->get_children_size());
        for(size_t idx(0); idx < max_children; ++idx)
        {
            as2js::JSON::JSONValue::pointer_t children_value(array[idx]);
            verify_result(children_value, node->get_child(idx), verbose); // recursive
        }
    }
    else
    {
        // no children defined in the JSON, no children expected in the node
        if(verbose && node->get_children_size() != 0)
        {
            std::cerr << "   Expecting no children, we have " << node->get_children_size() << " in the node\n";
        }
        CPPUNIT_ASSERT(node->get_children_size() == 0);
    }
}


//
// JSON data used to test the optimizer, most of the work is in this table
// these are long JSON strings! It is actually generated using the
// json_to_string tool and the test_as2js_optimizer_*.json source files.
//
// Note: the top entries are arrays so we can execute programs in the
//       order we define them...
//
char const g_optimizer_additive[] =
#include "test_as2js_optimizer_additive.ci"
;
char const g_optimizer_assignments[] =
#include "test_as2js_optimizer_assignments.ci"
;
char const g_optimizer_bitwise[] =
#include "test_as2js_optimizer_bitwise.ci"
;
char const g_optimizer_compare[] =
#include "test_as2js_optimizer_compare.ci"
;
char const g_optimizer_conditional[] =
#include "test_as2js_optimizer_conditional.ci"
;
char const g_optimizer_equality[] =
#include "test_as2js_optimizer_equality.ci"
;
char const g_optimizer_logical[] =
#include "test_as2js_optimizer_logical.ci"
;
char const g_optimizer_match[] =
#include "test_as2js_optimizer_match.ci"
;
char const g_optimizer_multiplicative[] =
#include "test_as2js_optimizer_multiplicative.ci"
;
char const g_optimizer_relational[] =
#include "test_as2js_optimizer_relational.ci"
;
char const g_optimizer_statements[] =
#include "test_as2js_optimizer_statements.ci"
;








// This function runs all the tests defined in the
// string 'data'
void run_tests(char const *data, char const *filename)
{
    as2js::String input_data;
    input_data.from_utf8(data);

    if(as2js_test::g_save_parser_tests)
    {
        std::ofstream json_file;
        json_file.open(filename);
        CPPUNIT_ASSERT(json_file.is_open());
        json_file << "// To properly indent this JSON you may use http://json-indent.appspot.com/"
                << std::endl << data << std::endl;
    }

    as2js::StringInput::pointer_t in(new as2js::StringInput(input_data));
    as2js::JSON::pointer_t json_data(new as2js::JSON);
    as2js::JSON::JSONValue::pointer_t json(json_data->parse(in));

    // verify that the optimizer() did not fail
    CPPUNIT_ASSERT(!!json);
    CPPUNIT_ASSERT(json->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);

    as2js::String name_string;
    name_string.from_utf8("name");
    as2js::String program_string;
    program_string.from_utf8("program");
    as2js::String verbose_string;
    verbose_string.from_utf8("verbose");
    as2js::String slow_string;
    slow_string.from_utf8("slow");
    as2js::String parser_result_string;
    parser_result_string.from_utf8("parser result");
    as2js::String optimizer_result_string;
    optimizer_result_string.from_utf8("optimizer result");
    as2js::String expected_messages_string;
    expected_messages_string.from_utf8("expected messages");

    std::cout << "\n";

    as2js::JSON::JSONValue::array_t const& array(json->get_array());
    size_t const max_programs(array.size());
    for(size_t idx(0); idx < max_programs; ++idx)
    {
        as2js::JSON::JSONValue::pointer_t prog_obj(array[idx]);
        CPPUNIT_ASSERT(prog_obj->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        as2js::JSON::JSONValue::object_t const& prog(prog_obj->get_object());

        bool verbose(false);
        as2js::JSON::JSONValue::object_t::const_iterator verbose_it(prog.find(verbose_string));
        if(verbose_it != prog.end())
        {
            verbose = verbose_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_TRUE;
        }

        bool slow(false);
        as2js::JSON::JSONValue::object_t::const_iterator slow_it(prog.find(slow_string));
        if(slow_it != prog.end())
        {
            slow = slow_it->second->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_TRUE;
        }

        // got a program, try to compile it with all the possible options
        as2js::JSON::JSONValue::pointer_t name(prog.find(name_string)->second);
        std::cout << "  -- working on \"" << name->get_string() << "\" " << (slow ? "" : "...") << std::flush;

        {
            as2js::JSON::JSONValue::pointer_t program_value(prog.find(program_string)->second);
            as2js::String program_source(program_value->get_string());
//std::cerr << "prog = [" << program_source << "]\n";
            as2js::StringInput::pointer_t prog_text(new as2js::StringInput(program_source));
            as2js::Options::pointer_t options(new as2js::Options);
            as2js::Parser::pointer_t parser(new as2js::Parser(prog_text, options));

            test_callback tc(verbose);

            // no errors exepected while parsing (if you want to test errors
            // in the parser, use the test_as2js_parser.cpp test instead)
            as2js::Node::pointer_t root(parser->parse());

            // verify the parser result, that way we can make sure we are
            // testing the tree we want to test in the optimizer
            verify_result(prog.find(parser_result_string)->second, root, verbose);

            // now the optimizer may end up generating messages...
            // (there are not many, mainly things like division by zero
            // and illegal operation.)
            as2js::JSON::JSONValue::object_t::const_iterator expected_msg_it(prog.find(expected_messages_string));
            if(expected_msg_it != prog.end())
            {

                // the expected messages value must be an array
                as2js::JSON::JSONValue::array_t const& msg_array(expected_msg_it->second->get_array());
                size_t const max_msgs(msg_array.size());
                for(size_t j(0); j < max_msgs; ++j)
                {
                    as2js::JSON::JSONValue::pointer_t message_value(msg_array[j]);
                    as2js::JSON::JSONValue::object_t const& message(message_value->get_object());

                    as2js::JSON::JSONValue::object_t::const_iterator const message_options_iterator(message.find("options"));
                    if(message_options_iterator != message.end())
                    {
//{
//as2js::JSON::JSONValue::object_t::const_iterator line_it(message.find("line #"));
//if(line_it != message.end())
//{
//    int64_t lines(line_it->second->get_int64().get());
//std::cerr << "_________\nLine #" << lines << "\n";
//}
//else
//std::cerr << "_________\nLine #<undefined>\n";
//}
                        test_callback::expected_t expected;
                        expected.f_message_level = static_cast<as2js::message_level_t>(message.find("message level")->second->get_int64().get());
                        expected.f_error_code = str_to_error_code(message.find("error code")->second->get_string());
                        expected.f_pos.set_filename("unknown-file");
                        as2js::JSON::JSONValue::object_t::const_iterator func_it(message.find("function name"));
                        if(func_it == message.end())
                        {
                            expected.f_pos.set_function("unknown-func");
                        }
                        else
                        {
                            expected.f_pos.set_function(func_it->second->get_string());
                        }
                        as2js::JSON::JSONValue::object_t::const_iterator line_it(message.find("line #"));
                        if(line_it != message.end())
                        {
                            int64_t lines(line_it->second->get_int64().get());
                            for(int64_t l(1); l < lines; ++l)
                            {
                                expected.f_pos.new_line();
                            }
                        }
                        expected.f_message = message.find("message")->second->get_string();
                        tc.f_expected.push_back(expected);
                    }
                }
            }

            // run the optimizer
            as2js::Optimizer::optimize(root);

            tc.got_called();

            // the result is object which can have children
            // which are represented by an array of objects
            verify_result(prog.find(optimizer_result_string)->second, root, verbose);
        }

        std::cout << " OK\n";
    }
}


}
// no name namespace





void As2JsOptimizerUnitTests::test_optimizer_invalid_nodes()
{
    // empty node does nothing, return false
    {
        as2js::Node::pointer_t node;
        CPPUNIT_ASSERT(!as2js::Optimizer::optimize(node));
    }

    // unknown node does nothing, return false
    {
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_UNKNOWN));
        CPPUNIT_ASSERT(!as2js::Optimizer::optimize(node));
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_UNKNOWN);
        CPPUNIT_ASSERT(node->get_children_size() == 0);
    }

    // a special case where an optimization occurs on a node without a parent
    // (something that should not occur in a real tree)
    {
        // ADD
        //   INT64 = 3
        //   INT64 = 20
        as2js::Node::pointer_t node_add(new as2js::Node(as2js::Node::node_t::NODE_ADD));

        as2js::Node::pointer_t node_three(new as2js::Node(as2js::Node::node_t::NODE_INT64));
        as2js::Int64 three;
        three.set(3);
        node_three->set_int64(three);
        node_add->append_child(node_three);

        as2js::Node::pointer_t node_twenty(new as2js::Node(as2js::Node::node_t::NODE_INT64));
        as2js::Int64 twenty;
        twenty.set(20);
        node_twenty->set_int64(twenty);
        node_add->append_child(node_twenty);

        // optimization does not happen
        CPPUNIT_ASSERT_THROW(!as2js::Optimizer::optimize(node_add), as2js::exception_internal_error);

        // verify that nothing changed
        CPPUNIT_ASSERT(node_add->get_type() == as2js::Node::node_t::NODE_ADD);
        CPPUNIT_ASSERT(node_add->get_children_size() == 2);
        CPPUNIT_ASSERT(node_three->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(node_three->get_children_size() == 0);
        CPPUNIT_ASSERT(node_three->get_int64().compare(three) == as2js::compare_t::COMPARE_EQUAL);
        CPPUNIT_ASSERT(node_twenty->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(node_twenty->get_children_size() == 0);
        CPPUNIT_ASSERT(node_twenty->get_int64().compare(twenty) == as2js::compare_t::COMPARE_EQUAL);
    }
}


void As2JsOptimizerUnitTests::test_optimizer_additive()
{
    run_tests(g_optimizer_additive, "test_optimizer_additive.json");
}

void As2JsOptimizerUnitTests::test_optimizer_assignments()
{
    run_tests(g_optimizer_assignments, "test_optimizer_assignments.json");
}

void As2JsOptimizerUnitTests::test_optimizer_bitwise()
{
    run_tests(g_optimizer_bitwise, "test_optimizer_bitwise.json");
}

void As2JsOptimizerUnitTests::test_optimizer_compare()
{
    run_tests(g_optimizer_compare, "test_optimizer_compare.json");
}

void As2JsOptimizerUnitTests::test_optimizer_conditional()
{
    run_tests(g_optimizer_conditional, "test_optimizer_conditional.json");
}

void As2JsOptimizerUnitTests::test_optimizer_equality()
{
    run_tests(g_optimizer_equality, "test_optimizer_equality.json");
}

void As2JsOptimizerUnitTests::test_optimizer_logical()
{
    run_tests(g_optimizer_logical, "test_optimizer_logical.json");
}

void As2JsOptimizerUnitTests::test_optimizer_match()
{
// regex is not well supported before 4.9.0
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    run_tests(g_optimizer_match, "test_optimizer_match.json");
#else
    std::cerr << " -- warning: test As2JsOptimizerUnitTests::test_optimizer_match() skip since you are compiling with a g++ version prior to 4.9.0" << std::endl;
#endif
}

void As2JsOptimizerUnitTests::test_optimizer_multiplicative()
{
    run_tests(g_optimizer_multiplicative, "test_optimizer_multiplicative.json");
}

void As2JsOptimizerUnitTests::test_optimizer_relational()
{
    run_tests(g_optimizer_relational, "test_optimizer_relational.json");
}

void As2JsOptimizerUnitTests::test_optimizer_statements()
{
    run_tests(g_optimizer_statements, "test_optimizer_statements.json");
}








// vim: ts=4 sw=4 et
