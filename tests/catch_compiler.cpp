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
#include    "catch_main.h"


// as2js
//
#include    <as2js/compiler.h>
#include    <as2js/exception.h>
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/parser.h>


// snapdev
//
#include    <snapdev/string_replace_many.h>


// C++
//
#include    <algorithm>
#include    <climits>
#include    <cstring>
#include    <iomanip>


// C
//
#include    <unistd.h>
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{

bool            g_created_files = false;




class input_retriever
    : public as2js::input_retriever
{
public:
    virtual as2js::base_stream::pointer_t retrieve(std::string const & filename) override
    {
        if(filename == "")
        {
        }

        return as2js::base_stream::pointer_t();
    }

};


std::string     g_current_working_directory;


void init_compiler(as2js::compiler & compiler)
{
    // The .rc file cannot be captured by the input retriever
    // so instead we create a file in the current directory

    // setup an input retriever which in most cases just returns nullptr
    //
    compiler.set_input_retriever(std::make_shared<input_retriever>());
}


void init_rc(bool bad_script = false)
{
    g_created_files = true;

    // we recreate because in the clean up we may end up deleting that
    // folder (even though it's already created by the catch_db_init()
    // function which happens before this call)
    //
    if(mkdir("as2js", 0700) != 0)
    {
        if(errno != EEXIST)
        {
            CATCH_REQUIRE(!"could not create directory as2js");
        }
        // else -- we already created it, that's fine
    }

    // The .rc file cannot be captured by the input retriever
    // so instead we create a file in the current directory
    //
    std::string const safe_cwd(snapdev::string_replace_many(
                              g_current_working_directory
                            , { { "'", "\\'" } }));
    char const * script_path(bad_script ? "no-scripts-here" : "scripts");
    std::ofstream out("as2js/as2js.rc");
    out << "// rc test file\n"
           "{\n"
           "  'scripts': '" << SNAP_CATCH2_NAMESPACE::g_source_dir() << '/' << script_path << "',\n"
           "  'db': '" << safe_cwd << "/test.db',\n"
           "  'temporary_variable_name': '@temp$'\n"
           "}\n";

    CATCH_REQUIRE(!!out);
}



//
// JSON data used to test the compiler, most of the work is in this table
// these are long JSON strings! It is actually generated using the
// json_to_string tool and the test_as2js_compiler_*.json source files.
//
// Note: the top entries are arrays so we can execute programs in the
//       order we define them...
//
char const g_compiler_class[] =
#include "compiler_data/class.ci"
;
char const g_compiler_expression[] =
#include "compiler_data/expression.ci"
;
char const g_compiler_enum[] =
#include "compiler_data/enum.ci"
;
//char const g_compiler_compare[] =
//#include "compiler_data/compare.ci"
//;
//char const g_compiler_conditional[] =
//#include "compiler_data/conditional.ci"
//;
//char const g_compiler_equality[] =
//#include "compiler_data/equality.ci"
//;
//char const g_compiler_logical[] =
//#include "compiler_data/logical.ci"
//;
//char const g_compiler_match[] =
//#include "compiler_data/match.ci"
//;
//char const g_compiler_multiplicative[] =
//#include "compiler_data/multiplicative.ci"
//;
//char const g_compiler_relational[] =
//#include "compiler_data/relational.ci"
//;
//char const g_compiler_statements[] =
//#include "compiler_data/statements.ci"
//;








// This function runs all the tests defined in the
// string 'data'
void run_tests(char const * input_data, char const *filename)
{
    if(SNAP_CATCH2_NAMESPACE::g_save_parser_tests)
    {
        std::ofstream json_file;
        json_file.open(filename);
        CATCH_REQUIRE(json_file.is_open());
        json_file
            << "// To properly indent this JSON you may use https://json-indent.appspot.com/\n"
            << input_data
            << '\n';
    }

    as2js::input_stream<std::stringstream>::pointer_t in(std::make_shared<as2js::input_stream<std::stringstream>>());
    in->get_position().set_filename(filename);
    *in << input_data;
    as2js::json::pointer_t json_data(std::make_shared<as2js::json>());
    as2js::json::json_value::pointer_t json(json_data->parse(in));

    // verify that the JSON parse() did not fail (internal to test)
    //
    CATCH_REQUIRE(json != nullptr);
    CATCH_REQUIRE(json->get_type() == as2js::json::json_value::type_t::JSON_TYPE_ARRAY);

    std::string const name_string("name");
    std::string const program_string("program");
    std::string const verbose_string("verbose");
    std::string const slow_string("slow");
    std::string const parser_result_string("parser result");
    std::string const compiler_result_string("compiler result");
    std::string const expected_messages_string("expected messages");

    as2js::json::json_value::array_t const& array(json->get_array());
    std::size_t const max_programs(array.size());
    for(std::size_t idx(0); idx < max_programs; ++idx)
    {
        as2js::json::json_value::pointer_t prog_obj(array[idx]);
        CATCH_REQUIRE(prog_obj->get_type() == as2js::json::json_value::type_t::JSON_TYPE_OBJECT);
        as2js::json::json_value::object_t const & prog(prog_obj->get_object());

        bool verbose(false);
        as2js::json::json_value::object_t::const_iterator verbose_it(prog.find(verbose_string));
        if(verbose_it != prog.end())
        {
            verbose = verbose_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE;
        }

        bool slow(false);
        as2js::json::json_value::object_t::const_iterator slow_it(prog.find(slow_string));
        if(slow_it != prog.end())
        {
            slow = slow_it->second->get_type() == as2js::json::json_value::type_t::JSON_TYPE_TRUE;
        }

        // got a program, try to compile it with all the possible options
        as2js::json::json_value::pointer_t name(prog.find(name_string)->second);
        std::cout << "  -- working on \"" << name->get_string() << "\" " << (slow ? "" : "...") << std::flush;

        for(std::size_t opt(0); opt < (1ULL << SNAP_CATCH2_NAMESPACE::g_options_size); ++opt)
        {
            if(slow && ((opt + 1) % 250) == 0)
            {
                std::cout << "." << std::flush;
            }
#define SHOW_OPTIONS 0
#if SHOW_OPTIONS
std::cerr << "\n***\n*** OPTIONS: [0x" << std::uppercase << std::hex << opt << std::dec << "]";
#endif
            as2js::options::pointer_t options(std::make_shared<as2js::options>());
            for(std::size_t o(0); o < SNAP_CATCH2_NAMESPACE::g_options_size; ++o)
            {
                if((opt & (1 << o)) != 0)
                {
                    options->set_option(
                              SNAP_CATCH2_NAMESPACE::g_options[o].f_option
                            , options->get_option(SNAP_CATCH2_NAMESPACE::g_options[o].f_option)
                                | SNAP_CATCH2_NAMESPACE::g_options[o].f_value);
#if SHOW_OPTIONS
std::cerr << " " << SNAP_CATCH2_NAMESPACE::g_options[o].f_name << "=" << SNAP_CATCH2_NAMESPACE::g_options[o].f_value;
#endif
                }
            }
#if SHOW_OPTIONS
std::cerr << "\n***\n";
#endif

            as2js::json::json_value::pointer_t program_value(prog.find(program_string)->second);
            std::string program_source(program_value->get_string());
//std::cerr << "--- prog = [" << program_source << "]\n";
            as2js::input_stream<std::stringstream>::pointer_t prog_text(std::make_shared<as2js::input_stream<std::stringstream>>());
            prog_text->get_position().set_filename("test/" + std::string(filename) + ": " + name->get_string());
            *prog_text << program_source;
            as2js::parser::pointer_t parser(std::make_shared<as2js::parser>(prog_text, options));

            init_rc();
            SNAP_CATCH2_NAMESPACE::test_callback parser_tc(verbose, true);

            // no errors exepected while parsing (if you want to test errors
            // in the parser, use the test_as2js_parser.cpp test instead)
            //
            as2js::node::pointer_t root(parser->parse());
//if(name->get_string() == "well defined enum")
//std::cerr << "--- parser output is:\n" << *root << "\n\n";

            // verify the parser result, that way we can make sure we are
            // testing the tree we want to test with the compiler
            //
            SNAP_CATCH2_NAMESPACE::verify_result(parser_result_string, prog.find(parser_result_string)->second, root, verbose, false);

            SNAP_CATCH2_NAMESPACE::test_callback tc(verbose, false);

            // now the compiler may end up generating messages...
            as2js::json::json_value::object_t::const_iterator expected_msg_it(prog.find(expected_messages_string));
            if(expected_msg_it != prog.end())
            {

                // the expected messages value must be an array
                as2js::json::json_value::array_t const& msg_array(expected_msg_it->second->get_array());
                std::size_t const max_msgs(msg_array.size());
                for(std::size_t j(0); j < max_msgs; ++j)
                {
                    as2js::json::json_value::pointer_t message_value(msg_array[j]);
                    as2js::json::json_value::object_t const& message(message_value->get_object());

                    bool ignore_message(false);

                    as2js::json::json_value::object_t::const_iterator const message_options_iterator(message.find("options"));
                    if(message_options_iterator != message.end())
                    {
//{
//as2js::json::json_value::object_t::const_iterator line_it(message.find("line #"));
//if(line_it != message.end())
//{
//    int64_t lines(line_it->second->get_integer().get());
//std::cerr << "_________\nLine #" << lines << "\n";
//}
//else
//std::cerr << "_________\nLine #<undefined>\n";
//}
                        std::string const message_options(message_options_iterator->second->get_string());
                        for(char const *s(message_options.c_str()), *start(s);; ++s)
                        {
                            if(*s == ',' || *s == '|' || *s == '\0')
                            {
                                std::string const opt_name(start, s - start);
                                for(std::size_t o(0); o < SNAP_CATCH2_NAMESPACE::g_options_size; ++o)
                                {
                                    if(SNAP_CATCH2_NAMESPACE::g_options[o].f_name == opt_name)
                                    {
                                        ignore_message = (opt & (1 << o)) != 0;
//std::cerr << "+++ pos option [" << opt_name << "] " << ignore_message << "\n";
                                        goto found_option;
                                    }
                                    else if(SNAP_CATCH2_NAMESPACE::g_options[o].f_neg_name == opt_name)
                                    {
                                        ignore_message = (opt & (1 << o)) == 0;
//std::cerr << "+++ neg option [" << opt_name << "] " << ignore_message << "\n";
                                        goto found_option;
                                    }
                                }
                                std::cerr << "error: Option \"" << opt_name << "\" not found in our list of valid options\n";
                                CATCH_REQUIRE("option name from JSON not found in g_options" == nullptr);

found_option:
                                if(*s == '\0')
                                {
                                    break;
                                }
                                if(*s == '|')
                                {
                                    if(ignore_message)
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    if(!ignore_message)
                                    {
                                        break;
                                    }
                                }

                                // skip commas
                                do
                                {
                                    ++s;
                                }
                                while(*s == ',' || *s == '|');
                                start = s;
                            }
                        }
                    }

                    if(!ignore_message)
                    {
                        SNAP_CATCH2_NAMESPACE::test_callback::expected_t expected;
                        expected.f_message_level = static_cast<as2js::message_level_t>(message.find("message level")->second->get_integer().get());
                        expected.f_error_code = SNAP_CATCH2_NAMESPACE::str_to_error_code(message.find("error code")->second->get_string());
                        expected.f_pos.set_filename("unknown-file");
                        as2js::json::json_value::object_t::const_iterator func_it(message.find("function name"));
                        if(func_it == message.end())
                        {
                            expected.f_pos.set_function("unknown-func");
                        }
                        else
                        {
                            expected.f_pos.set_function(func_it->second->get_string());
                        }
                        as2js::json::json_value::object_t::const_iterator line_it(message.find("line #"));
                        if(line_it != message.end())
                        {
                            std::int64_t lines(line_it->second->get_integer().get());
                            for(std::int64_t l(1); l < lines; ++l)
                            {
                                expected.f_pos.new_line();
                            }
                        }
                        expected.f_message = message.find("message")->second->get_string();
                        tc.f_expected.push_back(expected);
                    }
                }
            }

            // run the compiler
            //
            as2js::compiler compiler(options);
            compiler.compile(root);

//std::cerr << "  -- compiler root after compiling:\n" << *root << "\n\n";

            // the result is object which can have children
            // which are represented by an array of objects
            //
            SNAP_CATCH2_NAMESPACE::verify_result(compiler_result_string, prog.find(compiler_result_string)->second, root, verbose, false);

            tc.got_called();
        }

        std::cout << " OK\n";
    }

    std::cout << "\n";
}


}
// no name namespace


namespace SNAP_CATCH2_NAMESPACE
{


int catch_compiler_init()
{
    // get the current working directory as we need it in multiple places
    // that way it's cached and we do not have to duplicate this code over
    // and over again
    //
    char * cwd(get_current_dir_name());
    if(cwd == nullptr)
    {
        std::cerr << "error: could not get the current directory name.\n";
        return 1;
    }
    g_current_working_directory = cwd;
    free(cwd);

    struct stat st = {};

    // we do not want a test.db or it could conflict with this test
    //
    if(stat("test.db", &st) == 0)
    {
        std::cerr << "error: file \""
            << "test.db"
            << "\" already exists; please check it out to make sure you can delete it and try running the test again.\n";
        return 1;
    }

    // Now check that we have the scripts directories, we expect
    // the test to be run from the binary directory and this folder
    // is found in the source tree... so we have to prepend the
    // souce dir 
    //
    std::vector<std::string> script_folders
    {
        "scripts",
        "scripts/extensions",
        "scripts/native",
    };
    for(auto const & p : script_folders)
    {
        std::string filename(SNAP_CATCH2_NAMESPACE::g_source_dir());
        filename += '/';
        filename += p;
        if(stat(filename.c_str(), &st) != 0)
        {
            std::cerr << "error: file \""
                << filename
                << "\" is missing; please make sure that system scripts are accessible from this test.\n";
            return 1;
        }
    }

    return 0;
}


void catch_compiler_cleanup()
{
    if(g_created_files)
    {
        // ignore errors on these few calls
        unlink("test.db");
        unlink("as2js/as2js.rc");
        rmdir("as2js");
    }
}


} // namespace SNAP_CATCH2_NAMESPACE





CATCH_TEST_CASE("compiler_invalid_module_files", "[compiler][module][invalid]")
{
    CATCH_START_SECTION("compiler_invalid_module_files: missing as2js.rc file")
    {
        // as2js.rc checked before the options (this is not a really good
        // test I guess... as the order is only fortuitous)
        CATCH_REQUIRE_THROWS_MATCHES(
              std::make_shared<as2js::compiler>(nullptr)
            , as2js::as2js_exit
            , Catch::Matchers::ExceptionMessage(
                      "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compiler_invalid_module_files: with option, still missing as2js.rc file")
    {
        as2js::options::pointer_t options(std::make_shared<as2js::options>());
        CATCH_REQUIRE_THROWS_MATCHES(
              std::make_shared<as2js::compiler>(options)
            , as2js::as2js_exit
            , Catch::Matchers::ExceptionMessage(
                      "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compiler_invalid_module_files: invalid path to scripts")
    {
        init_rc(true);
        as2js::options::pointer_t options(std::make_shared<as2js::options>());
        CATCH_REQUIRE_THROWS_MATCHES(
              std::make_shared<as2js::compiler>(options)
            , as2js::as2js_exit
            , Catch::Matchers::ExceptionMessage(
                      "as2js_exception: cannot open module file \""
                    + SNAP_CATCH2_NAMESPACE::g_source_dir()
                    + "/no-scripts-here/native/as2js_init.js\"."));
        SNAP_CATCH2_NAMESPACE::catch_compiler_cleanup();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compiler_invalid_module_files: options pointer is required")
    {
        init_rc();

        CATCH_REQUIRE_THROWS_MATCHES(
              std::make_shared<as2js::compiler>(nullptr)
            , as2js::invalid_data
            , Catch::Matchers::ExceptionMessage(
                      "as2js_exception: the 'options' pointer cannot be null in the lexer() constructor."));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compiler_invalid_nodes", "[compiler][invalid]")
{
    CATCH_START_SECTION("compiler_invalid_nodes: empty node does nothing")
    {
        as2js::node::pointer_t node;
        SNAP_CATCH2_NAMESPACE::test_callback tc(false, false);
        as2js::options::pointer_t options(std::make_shared<as2js::options>());

        as2js::compiler compiler(options);
        init_compiler(compiler);
        CATCH_REQUIRE(compiler.compile(node) == 0);

        tc.got_called();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compiler_invalid_nodes: only ROOT and PROGRAM are valid at the top")
    {
        for(int i(-1); i < static_cast<int>(as2js::node_t::NODE_max); ++i)
        {
            as2js::node::pointer_t node;
            try
            {
                node = std::make_shared<as2js::node>(as2js::node_t::NODE_UNKNOWN);
            }
            catch(as2js::incompatible_node_type const &)
            {
                // many node types cannot be created
                // (we have gaps in our numbers)
                //
                continue;
            }

            SNAP_CATCH2_NAMESPACE::test_callback tc(false, false);
            {
                SNAP_CATCH2_NAMESPACE::test_callback::expected_t expected;
                expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
                expected.f_error_code = as2js::err_code_t::AS_ERR_INTERNAL_ERROR;
                expected.f_pos.set_filename("unknown-file");
                expected.f_pos.set_function("unknown-func");
                //expected.f_pos.new_line(); -- line 1
                expected.f_message = "the compiler::compile() function expected a root or a program node to start with.";
                tc.f_expected.push_back(expected);
            }

            as2js::options::pointer_t options(std::make_shared<as2js::options>());
            as2js::compiler compiler(options);
            CATCH_REQUIRE(compiler.compile(node) != 0);
            CATCH_REQUIRE(node->get_type() == as2js::node_t::NODE_UNKNOWN);
            CATCH_REQUIRE(node->get_children_size() == 0);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compiler_class", "[compiler][class]")
{
    CATCH_START_SECTION("compiler_class: verify class functionality")
    {
        run_tests(g_compiler_class, "compiler/class.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compiler_enum", "[compiler][enum]")
{
    CATCH_START_SECTION("compiler_enum: verify enumerations")
    {
        run_tests(g_compiler_enum, "compiler/enum.json");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compiler_expression", "[compiler][expression]")
{
    CATCH_START_SECTION("compiler_expression: verify expressions")
    {
        run_tests(g_compiler_expression, "compiler/expression.json");
    }
    CATCH_END_SECTION()
}


//void As2JsCompilerUnitTests::test_compiler_compare()
//{
//    run_tests(g_compiler_compare, "test_compiler_compare.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_conditional()
//{
//    run_tests(g_compiler_conditional, "test_compiler_conditional.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_equality()
//{
//    run_tests(g_compiler_equality, "test_compiler_equality.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_logical()
//{
//    run_tests(g_compiler_logical, "test_compiler_logical.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_match()
//{
//// regex is not well supported before 4.9.0
//#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
//    run_tests(g_compiler_match, "test_compiler_match.json");
//#else
//    std::cerr << " -- warning: test As2JsOptimizerUnitTests::test_compiler_match() skip since you are compiling with a g++ version prior to 4.9.0" << std::endl;
//#endif
//}
//
//void As2JsCompilerUnitTests::test_compiler_multiplicative()
//{
//    run_tests(g_compiler_multiplicative, "test_compiler_multiplicative.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_relational()
//{
//    run_tests(g_compiler_relational, "test_compiler_relational.json");
//}
//
//void As2JsCompilerUnitTests::test_compiler_statements()
//{
//    run_tests(g_compiler_statements, "test_compiler_statements.json");
//}








// vim: ts=4 sw=4 et
