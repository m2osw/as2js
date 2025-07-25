// Copyright (c) 2005-2025  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief This file is the actual as2js compiler/transpiler.
 *
 * The project includes a library which does 99% of the work. This is
 * the implementation of the as2js command line tool that handles
 * command line options, initializes the necessary objects to compile
 * .asj files in one of:
 *
 * \li a node tree (mainly for debug)
 * \li binary code (directly Intel/AMD instruction in binary--no specific
 *     format but it uses the Linux ABI)
 * \li assembly language (64 bit Intel/AMD)
 * \li JavaScript (this was the point at first)
 * \li C++ (create C++ classes .cpp/.h files)
 *
 * The tree is very useful for me to debug the lexer, parser, and compiler
 * work before even trying to transform said tree to something else.
 *
 * The binary code is used in our other libraries so that way we do not need
 * to have any other compiler and/or assembler installed on your server. This
 * tool and library are enough for you to create a binary file that our
 * other tools (again using this library) can use to run complex expressions
 * directly in binary.
 */

// self
//
#include    "tools/license.h"



// as2js
//
#include    <as2js/archive.h>
#include    <as2js/binary.h>
#include    <as2js/compiler.h>
#include    <as2js/exception.h>
#include    <as2js/message.h>
#include    <as2js/parser.h>
#include    <as2js/version.h>


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/string_replace_many.h>
#include    <snapdev/to_lower.h>


// C++
//
#include    <cstring>
#include    <iomanip>
#include    <set>


// last include
//
#include    <snapdev/poison.h>






/** \brief Private implementations of the as2js compiler, the actual tool.
 *
 * This namespace is used to hide all the tool private functions to
 * avoid any conflicts.
 */
namespace
{



enum class command_t
{
    COMMAND_UNDEFINED,

    COMMAND_ASSEMBLY,
    COMMAND_BINARY,
    COMMAND_BINARY_VERSION,
    COMMAND_COMPILER_TREE,
    COMMAND_CPP,
    COMMAND_CREATE_ARCHIVE,
    COMMAND_DATA_SECTION,
    COMMAND_END_SECTION,
    COMMAND_EXECUTE,
    COMMAND_EXTRACT_ARCHIVE,
    COMMAND_IS_BINARY,
    COMMAND_JAVASCRIPT,
    COMMAND_LIST_ARCHIVE,
    COMMAND_PARSER_TREE,
    COMMAND_TEXT_SECTION,
    COMMAND_VARIABLES,
};


class as2js_compiler
{
public:
    typedef std::shared_ptr<as2js_compiler>         pointer_t;

    int                         parse_command_line_options(int argc, char *argv[]);
    int                         run();

private:
    void                        license();
    void                        usage();
    void                        version();
    void                        set_output(command_t output);
    void                        set_option(
                                      as2js::option_t option
                                    , int argc
                                    , char * argv[]
                                    , int & i);
    int                         output_error_count();
    void                        compile();
    void                        generate_binary(as2js::compiler::pointer_t c);
    void                        binary_utils();
    void                        list_external_variables(
                                      std::ifstream & in
                                    , as2js::binary_header & header);
    void                        create_archive();
    void                        list_archive();
    void                        execute();

    typedef std::map<std::string, std::string>  variable_t;

    int                         f_error_count = 0;
    std::string                 f_progname = std::string();
    std::vector<std::string>    f_filenames = std::vector<std::string>();
    std::string                 f_save_to_file = std::string();
    std::string                 f_output = std::string();
    //std::string                 f_archive_path = std::string();
    variable_t                  f_variables = variable_t();
    command_t                   f_command = command_t::COMMAND_UNDEFINED;
    as2js::options::pointer_t   f_options = std::make_shared<as2js::options>();
    std::set<as2js::option_t>   f_option_defined = std::set<as2js::option_t>();
    as2js::node::pointer_t      f_root = as2js::node::pointer_t();
    bool                        f_ignore_unknown_variables = false;
    bool                        f_show_all_results = false;
    bool                        f_three_underscores_to_space = false;
};


int as2js_compiler::parse_command_line_options(int argc, char *argv[])
{
    f_progname = snapdev::pathinfo::basename(std::string(argv[0]));

    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-'
        && argv[i][1] != '\0')
        {
            if(argv[i][1] == '-')
            {
                if(strcmp(argv[i] + 2, "help") == 0)
                {
                    usage();
                    return 1;
                }
                if(strcmp(argv[i] + 2, "license") == 0)
                {
                    license();
                    return 1;
                }
                if(strcmp(argv[i] + 2, "version") == 0)
                {
                    version();
                    return 1;
                }

                if(strcmp(argv[i] + 2, "log-level") == 0)
                {
                    ++i;
                    if(i >= argc)
                    {
                        ++f_error_count;
                        std::cerr
                            << "error: the \"--log-level\" option expects one option, the level as a number or its name.\n";
                    }
                    else
                    {
                        as2js::message_level_t const level(as2js::string_to_message_level(argv[i]));
                        if(level != as2js::MESSAGE_LEVEL_INVALID)
                        {
                            set_message_level(level);
                        }
                        else
                        {
                            ++f_error_count;
                            std::cerr
                                << "error: unknown log level \""
                                << argv[i]
                                << "\".\n"; // TODO: add a command to list available levels
                        }
                    }
                }
                else if(strcmp(argv[i] + 2, "save-after-execute") == 0)
                {
                    ++i;
                    if(i >= argc)
                    {
                        ++f_error_count;
                        std::cerr
                            << "error: the \"--save-after-execute\" option expects a filename.\n";
                    }
                    else
                    {
                        f_save_to_file = argv[i];
                    }
                }
                else if(strcmp(argv[i] + 2, "binary") == 0)
                {
                    set_output(command_t::COMMAND_BINARY);
                }
                else if(strcmp(argv[i] + 2, "binary-version") == 0)
                {
                    set_output(command_t::COMMAND_BINARY_VERSION);
                }
                else if(strcmp(argv[i] + 2, "create-archive") == 0)
                {
                    set_output(command_t::COMMAND_CREATE_ARCHIVE);
                }
                else if(strcmp(argv[i] + 2, "data-section") == 0)
                {
                    set_output(command_t::COMMAND_DATA_SECTION);
                }
                else if(strcmp(argv[i] + 2, "end-section") == 0)
                {
                    set_output(command_t::COMMAND_END_SECTION);
                }
                else if(strcmp(argv[i] + 2, "execute") == 0)
                {
                    set_output(command_t::COMMAND_EXECUTE);
                }
                else if(strcmp(argv[i] + 2, "extract-archive") == 0)
                {
                    set_output(command_t::COMMAND_EXTRACT_ARCHIVE);
                }
                else if(strcmp(argv[i] + 2, "is-binary") == 0)
                {
                    set_output(command_t::COMMAND_IS_BINARY);
                }
                else if(strcmp(argv[i] + 2, "list-archive") == 0)
                {
                    set_output(command_t::COMMAND_LIST_ARCHIVE);
                }
                else if(strcmp(argv[i] + 2, "parser-tree") == 0)
                {
                    set_output(command_t::COMMAND_PARSER_TREE);
                }
                else if(strcmp(argv[i] + 2, "text-section") == 0)
                {
                    set_output(command_t::COMMAND_TEXT_SECTION);
                }
                else if(strcmp(argv[i] + 2, "compiler-tree") == 0)
                {
                    set_output(command_t::COMMAND_COMPILER_TREE);
                }
                else if(strcmp(argv[i] + 2, "variables") == 0)
                {
                    set_output(command_t::COMMAND_VARIABLES);
                }
                else if(strcmp(argv[i] + 2, "allow-with") == 0)
                {
                    set_option(as2js::option_t::OPTION_ALLOW_WITH, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "coverage") == 0)
                {
                    set_option(as2js::option_t::OPTION_COVERAGE, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "debug") == 0)
                {
                    set_option(as2js::option_t::OPTION_DEBUG, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-sequences") == 0)
                {
                    set_option(as2js::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-operators") == 0)
                {
                    set_option(as2js::option_t::OPTION_EXTENDED_OPERATORS, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-statements") == 0)
                {
                    set_option(as2js::option_t::OPTION_EXTENDED_STATEMENTS, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "json") == 0)
                {
                    set_option(as2js::option_t::OPTION_JSON, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "octal") == 0)
                {
                    set_option(as2js::option_t::OPTION_OCTAL, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "strict") == 0)
                {
                    set_option(as2js::option_t::OPTION_STRICT, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "trace") == 0)
                {
                    set_option(as2js::option_t::OPTION_TRACE, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "unsafe-math") == 0)
                {
                    set_option(as2js::option_t::OPTION_UNSAFE_MATH, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "ignore-unknown-variables") == 0)
                {
                    f_ignore_unknown_variables = true;
                }
                else if(strcmp(argv[i] + 2, "error-on-missing-variables") == 0)
                {
                    f_ignore_unknown_variables = false;
                }
                else if(strcmp(argv[i] + 2, "show-all-results") == 0)
                {
                    f_show_all_results = true;
                }
                else if(strcmp(argv[i] + 2, "hide-all-results") == 0)
                {
                    f_show_all_results = false;
                }
                else if(strcmp(argv[i] + 2, "three-underscores-to-space") == 0)
                {
                    f_three_underscores_to_space = true;
                }
                else
                {
                    ++f_error_count;
                    std::cerr
                        << "error: unknown command line option \""
                        << argv[i]
                        << "\".\n";
                }
            }
            else
            {
                std::size_t const max(strlen(argv[i]));
                for(std::size_t j(1); j < max; ++j)
                {
                    switch(argv[i][j])
                    {
                    case 'b':
                        set_output(command_t::COMMAND_BINARY);
                        break;

                    case 'h':
                        usage();
                        return 1;

                    //case 'L': -- replaced by "extern functions" table
                    //    if(!f_archive_path.empty())
                    //    {
                    //        ++f_error_count;
                    //        std::cerr
                    //            << "error: command line option \"-L\" cannot be used more than once.\n";
                    //    }
                    //    ++j;
                    //    if(j >= max)
                    //    {
                    //        ++i;
                    //        if(i >= argc)
                    //        {
                    //            ++f_error_count;
                    //            std::cerr
                    //                << "error: command line option \"-L\" is expected to be followed by one path.\n";
                    //        }
                    //        f_archive_path = argv[i];
                    //    }
                    //    else
                    //    {
                    //        f_archive_path = argv[i] + j;
                    //    }
                    //    break;

                    case 'o':
                        if(!f_output.empty())
                        {
                            ++f_error_count;
                            std::cerr
                                << "error: command line option \"-o\" cannot be used more than once.\n";
                        }
                        ++j;
                        if(j >= max)
                        {
                            ++i;
                            if(i >= argc)
                            {
                                ++f_error_count;
                                std::cerr
                                    << "error: command line option \"-o\" is expected to be followed by one filename.\n";
                            }
                            f_output = argv[i];
                        }
                        else
                        {
                            f_output = argv[i] + j;
                        }
                        break;

                    case 't':
                        set_output(command_t::COMMAND_PARSER_TREE);
                        break;

                    case 'T':
                        set_output(command_t::COMMAND_COMPILER_TREE);
                        break;

                    case 'V':
                        version();
                        return 1;

                    default:
                        ++f_error_count;
                        std::cerr
                            << "error: unknown command line option \"-"
                            << argv[i][j]
                            << "\".\n";
                        break;

                    }
                }
            }
        }
        else
        {
            char const * equal(strchr(argv[i], '='));
            if(equal == nullptr)
            {
                f_filenames.push_back(argv[i]);
            }
            else if(equal - argv[i] == 0)
            {
                ++f_error_count;
                std::cerr
                    << "error: variable name missing in \""
                    << argv[i]
                    << "\".\n";
            }
            else
            {
                std::string const name(argv[i], equal - argv[i]);
                std::string const value(equal + 1);
                f_variables[name] = value;
            }
        }
    }

    if(f_command == command_t::COMMAND_UNDEFINED)
    {
        // this is what one wants by default (transliteration from
        // Alex Script to JavaScript)
        //
        f_command = command_t::COMMAND_JAVASCRIPT;
    }

    return output_error_count();
}


int as2js_compiler::output_error_count()
{
    if(f_error_count != 0)
    {
        std::cerr
            << "error: found "
            << f_error_count 
            << " error"
            << (f_error_count == 1 ? "" : "s")
            << " in the command line options.\n";
        return 1;
    }

    return 0;
}


void as2js_compiler::license()
{
    std::cout << as2js_tools::license;
}


void as2js_compiler::usage()
{
    std::cout
        << "Usage: " << f_progname << " [-opts] <filename>.ajs | <var>=<value> ...\n"
           "where -opts is one or more of:\n"
           "Commands (one of):\n"
           "  -b | --binary          generate a binary file.\n"
           "       --binary-version  output version of the binary file.\n"
           "       --data-section    position where the data section starts.\n"
           "       --end-section     position where the end section starts.\n"
           "       --error-on-missing-variables\n"
           "                         variables defined on the command must exist.\n"
           "       --execute         execute the specified compiled script.\n"
           "  -h | --help            print out this help screen.\n"
           "       --hide-all-results\n"
           "                         do not print the external values before exiting.\n"
           "       --ignore-unknown-variables\n"
           "                         variables defined on the command that do not exist\n"
           "                         can be ignored if not defined in the script.\n"
           "       --is-binary       check whether a file is a binary file.\n"
           "       --license         print compiler's license.\n"
           "  -t | --parser-tree     output the tree of nodes.\n"
           "       --show-all-results\n"
           "                         print all the external values before exiting.\n"
           "       --text-section    position where the text section starts.\n"
           "  -T | --compiler-tree   output the tree of nodes.\n"
           "       --variables       list external variables.\n"
           "  -V | --version         print version of the compiler.\n"

           "\n"
           "Options:\n"
           "  -L <path>              path to archive libraries.\n"
    ;
}


void as2js_compiler::version()
{
    std::cout
        << f_progname << " v" << AS2JS_VERSION_STRING << std::endl
        << "libas2js v" << as2js::get_version_string() << std::endl;
}


void as2js_compiler::set_output(command_t command)
{
    if(f_command != command_t::COMMAND_UNDEFINED)
    {
        ++f_error_count;
        std::cerr << "error: output type already set. Try only one of --binary, --tree, --is-binary, etc.\n";
        return;
    }

    f_command = command;
}


void as2js_compiler::set_option(as2js::option_t option, int argc, char * argv[], int & i)
{
    // prevent duplication which will help people understand why something
    // would possibly not work
    {
        auto const unique(f_option_defined.insert(option));
        if(!unique.second)
        {
            ++f_error_count;
            std::cerr
                << "error: option \""
                << argv[i]
                << "\" was specified more than once.\n";
            return;
        }
    }

    as2js::option_value_t value(0);
    if(i + 1 < argc)
    {
        char * v(argv[i + 1]);
        if(strcmp(v, "true") == 0)
        {
            value = 1;
            ++i;
        }
        else if(strcmp(v, "false") == 0)
        {
            value = 0;
            ++i;
        }
        else if(as2js::is_integer(v, true))
        {
            value = as2js::to_integer(v);
            ++i;
        }
        else if(as2js::is_floating_point(v))
        {
            value = as2js::to_floating_point(v);
            ++i;
        }
    }
    f_options->set_option(option, value);
}


int as2js_compiler::run()
{
    switch(f_command)
    {
    case command_t::COMMAND_BINARY_VERSION:
    case command_t::COMMAND_IS_BINARY:
    case command_t::COMMAND_TEXT_SECTION:
    case command_t::COMMAND_DATA_SECTION:
    case command_t::COMMAND_END_SECTION:
    case command_t::COMMAND_VARIABLES:
        binary_utils();
        break;

    case command_t::COMMAND_CREATE_ARCHIVE:
        create_archive();
        break;

    case command_t::COMMAND_LIST_ARCHIVE:
    case command_t::COMMAND_EXTRACT_ARCHIVE:
        list_archive();
        break;

    case command_t::COMMAND_EXECUTE:
        execute();
        break;

    case command_t::COMMAND_PARSER_TREE:
    case command_t::COMMAND_COMPILER_TREE:
    case command_t::COMMAND_BINARY:
    case command_t::COMMAND_ASSEMBLY:
    case command_t::COMMAND_JAVASCRIPT:
    case command_t::COMMAND_CPP:
        compile();
        break;

    case command_t::COMMAND_UNDEFINED:
        throw as2js::internal_error("found the \"undefined\" command when we should replace it with the default \"javascript\"?");

    }

    return output_error_count();
}


void as2js_compiler::compile()
{
    if(f_filenames.empty())
    {
        ++f_error_count;
        std::cerr << "error: an input filename is required.\n";
        return;
    }

    if(f_output.empty())
    {
        f_output = "a.out";
    }

    // we are compiling a user script so mark it as such
    //
    f_options->set_option(as2js::option_t::OPTION_USER_SCRIPT, 1);

    // TODO: I'm pretty sure that the following loop would require me to
    //       load all the _programs_ (user defined files) as children of a
    //       NODE_ROOT as NODE_PROGRAM and then compile all the NODE_PROGRAM
    //       nodes at once with one call to the compile() function
    //
    for(auto const & filename : f_filenames)
    {
        as2js::reset_errors();

        // open file
        //
        as2js::base_stream::pointer_t input;
        if(filename == "-")
        {
            input = std::make_shared<as2js::cin_stream>();
            input->get_position().set_filename("-");
        }
        else
        {
            as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
            in->get_position().set_filename(filename);
            in->open(filename);
            if(!in->is_open())
            {
                ++f_error_count;
                std::cerr
                    << "error: could not open file \""
                    << filename
                    << "\".\n";
                continue;
            }
            input = in;
        }

        // parse the source
        //
        as2js::parser::pointer_t parser(std::make_shared<as2js::parser>(input, f_options));
        f_root = parser->parse();
        if(as2js::error_count() != 0)
        {
            ++f_error_count;
            std::cerr
                << "error: parsing of input file \""
                << filename
                << "\" failed.\n";
            continue;
        }

        if(f_command == command_t::COMMAND_PARSER_TREE)
        {
            // user wants to see the parser tree, show that and try next file
            //
            std::cout << *f_root << "\n";
            continue;
        }

        // run the compiler
        //
        as2js::compiler::pointer_t compiler(std::make_shared<as2js::compiler>(f_options));
        if(compiler->compile(f_root) != 0)
        {
            // there were errors, skip
            //
            ++f_error_count;
            std::cerr
                << "error: parsing of input file \""
                << filename
                << "\" failed.\n";
            continue;
        }

        switch(f_command)
        {
        case command_t::COMMAND_COMPILER_TREE:
            std::cout << *f_root << "\n";
            break;

        case command_t::COMMAND_BINARY:
            generate_binary(compiler);
            break;

        case command_t::COMMAND_ASSEMBLY:
        case command_t::COMMAND_JAVASCRIPT:
        case command_t::COMMAND_CPP:
            ++f_error_count;
            std::cerr
                << "error: output command not yet implemented.\n";
            break;

        case command_t::COMMAND_BINARY_VERSION:
        case command_t::COMMAND_CREATE_ARCHIVE:
        case command_t::COMMAND_IS_BINARY:
        case command_t::COMMAND_DATA_SECTION:
        case command_t::COMMAND_END_SECTION:
        case command_t::COMMAND_EXECUTE:
        case command_t::COMMAND_EXTRACT_ARCHIVE:
        case command_t::COMMAND_LIST_ARCHIVE:
        case command_t::COMMAND_PARSER_TREE:
        case command_t::COMMAND_TEXT_SECTION:
        case command_t::COMMAND_UNDEFINED:
        case command_t::COMMAND_VARIABLES:
            throw as2js::internal_error("these cases were checked earlier and cannot happen here."); // LCOV_EXCL_LINE

        }
    }
}


void as2js_compiler::generate_binary(as2js::compiler::pointer_t compiler)
{
    // TODO: add support for '-' (i.e. stdout)
    //
    as2js::output_stream<std::ofstream>::pointer_t output(std::make_shared<as2js::output_stream<std::ofstream>>());
    output->open(f_output);
    if(!output->is_open())
    {
        ++f_error_count;
        std::cerr
            << "error: could not open output file \""
            << f_output
            << "\".\n";
        return;
    }
    as2js::binary_assembler::pointer_t binary(
            std::make_shared<as2js::binary_assembler>(
                      output
                    , f_options
                    , compiler));
    int const errcnt(binary->output(f_root));
    if(errcnt != 0)
    {
        ++f_error_count;
        std::cerr
            << "error: "
            << errcnt
            << " errors occured while transforming the tree to binary.\n";
    }
}


void as2js_compiler::binary_utils()
{
    if(!f_output.empty())
    {
        ++f_error_count;
        std::cerr
            << "as2js:error: no output file ("
            << f_output
            << ") is supported with this command.\n";
        return;
    }

    if(f_filenames.empty())
    {
        f_filenames.push_back("a.out");
    }

    // open input file
    //
    std::ifstream in;
    in.open(f_filenames[0]);
    if(!in.is_open())
    {
        ++f_error_count;
        std::cerr
            << "as2js:error: could not open \""
            << f_filenames[0]
            << "\" as input binary file.\n";
        return;
    }

    // read the header
    //
    as2js::binary_header header;
    in.read(reinterpret_cast<char *>(&header), sizeof(header));

    if(header.f_magic[0] != as2js::BINARY_MAGIC_B0
    || header.f_magic[1] != as2js::BINARY_MAGIC_B1
    || header.f_magic[2] != as2js::BINARY_MAGIC_B2
    || header.f_magic[3] != as2js::BINARY_MAGIC_B3)
    {
        ++f_error_count;
        std::cerr << "as2js:error: file \""
            << f_filenames[0]
            << "\" does not look like an as2js binary file (invalid magic).\n";
        return;
    }

    switch(f_command)
    {
    case command_t::COMMAND_BINARY_VERSION:
        std::cout
            << static_cast<int>(header.f_version_major)
            << '.'
            << static_cast<int>(header.f_version_minor)
            << '\n';
        return;

    case command_t::COMMAND_IS_BINARY:
        // that was sufficient for this command, return now
        return;

    case command_t::COMMAND_TEXT_SECTION:
        std::cout << header.f_start << '\n';
        return;

    case command_t::COMMAND_DATA_SECTION:
        std::cout << header.f_variables << '\n';
        return;

    case command_t::COMMAND_END_SECTION:
        // this is the file size minus 4
        //
        in.seekg(0, std::ios_base::end);
        std::cout << in.tellg() - 4L << '\n';
        return;

    case command_t::COMMAND_VARIABLES:
        list_external_variables(in, header);
        return;

    default:
        throw as2js::internal_error("these cases were checked earlier and cannot happen here"); // LCOV_EXCL_LINE

    }
}


void as2js_compiler::list_external_variables(std::ifstream & in, as2js::binary_header & header)
{
    // list external variables
    //
    in.seekg(header.f_variables, std::ios_base::beg);
    as2js::binary_variable::vector_t variables(header.f_variable_count);
    in.read(reinterpret_cast<char *>(variables.data()), sizeof(as2js::binary_variable) * header.f_variable_count);
    for(std::uint16_t idx(0); idx < header.f_variable_count; ++idx)
    {
        std::string name;
        if(variables[idx].f_name_size < sizeof(variables[idx].f_name))
        {
            name = std::string(
                      reinterpret_cast<char const *>(&variables[idx].f_name)
                    , variables[idx].f_name_size);
        }
        else
        {
            in.seekg(variables[idx].f_name, std::ios_base::beg);
            name = std::string(variables[idx].f_name_size, ' ');
            in.read(name.data(), variables[idx].f_name_size);
        }
        if(idx == 0)
        {
            std::cout << "var ";
        }
        else
        {
            std::cout << ",\n    ";
        }
#if 0
// show the offset of where the variable data is defined
// (for easy verification purposes, instead of having to read the binary)
//
std::cout
    << " /* offset: "
    << (variables[idx].f_data_size > sizeof(variables[idx].f_data)
            ? variables[idx].f_data
            : header.f_variables
                + idx * sizeof(as2js::binary_variable)
                + offsetof(as2js::binary_variable, f_data))
    << " */ ";
#endif
        std::cout << name;
        switch(variables[idx].f_type)
        {
        case as2js::variable_type_t::VARIABLE_TYPE_BOOLEAN:
            std::cout << ": Boolean";
            if(variables[idx].f_data != 0)
            {
                std::cerr << " := true";
            }
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_INTEGER:
            std::cout << ": Integer";
            if(variables[idx].f_data != 0)
            {
                std::cerr << " := " << variables[idx].f_data;
            }
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_FLOATING_POINT:
            std::cout << ": Double";
            if(variables[idx].f_data != 0)
            {
                std::uint64_t const * ptr(&variables[idx].f_data);
                std::cerr << " := " << *reinterpret_cast<double const *>(ptr);
            }
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_STRING:
            std::cout << ": String";
            if(variables[idx].f_data_size > 0)
            {
                std::string value;
                if(variables[idx].f_data_size < sizeof(variables[idx].f_data))
                {
                    value = std::string(
                              reinterpret_cast<char const *>(&variables[idx].f_data)
                            , variables[idx].f_data_size);
                }
                else
                {
                    in.seekg(variables[idx].f_data, std::ios_base::beg);
                    value = std::string(variables[idx].f_data_size, ' ');
                    in.read(value.data(), variables[idx].f_data_size);
                }
                std::cout << " = " << value;
            }
            break;

        //case as2js::variable_type_t::VARIABLE_TYPE_UNKNOWN:
        default:
            std::cout << " /* unknown type */ ";
            break;

        }
    }
    std::cout << ";\n";
}


void as2js_compiler::create_archive()
{
    as2js::archive ar;
    if(!ar.create(f_filenames))
    {
        ++f_error_count;
        std::cerr
            << "error: could not create archive file.\n";
        return;
    }

    as2js::output_stream<std::ofstream>::pointer_t output(std::make_shared<as2js::output_stream<std::ofstream>>());
    output->open(f_output);
    if(!output->is_open())
    {
        ++f_error_count;
        std::cerr
            << "error: could not open archive file \""
            << f_output
            << "\" to save run-time functions.\n";
        return;
    }

    if(!ar.save(output))
    {
        ++f_error_count;
        std::cerr
            << "error: could not save archive file \""
            << f_output
            << "\".\n";
        return;
    }
}


void as2js_compiler::list_archive()
{
    if(f_filenames.size() != 1)
    {
        ++f_error_count;
        std::cerr
            << "error: expected exactly one filename with --list-archive or --extract-archive, found "
            << f_filenames.size()
            << " instead.\n";
        return;
    }

    as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
    in->open(f_filenames[0]);
    if(!in->is_open())
    {
        // TODO: test again with .oar extension
        //
        ++f_error_count;
        std::cerr
            << "error: could not open archive \""
            << f_filenames[0]
            << "\".\n";
        return;
    }

    as2js::archive ar;
    if(!ar.load(in))
    {
        ++f_error_count;
        std::cerr
            << "error: failed loading archive \""
            << f_filenames[0]
            << "\".\n";
        return;
    }

    as2js::rt_function::map_t functions(ar.get_functions());
    std::size_t name_width(10);
    for(auto const & f : functions)
    {
        name_width = std::max(f.first.length(), name_width);
    }
    std::cout
        << "     NAME" << std::setw(name_width - 4) << ' ' << "SIZE\n";
    int pos(1);
    for(auto const & f : functions)
    {
        std::cout
            << std::setw(3) << pos
            << ". " << f.first
            << std::setw(name_width - f.first.length()) << ' '
            << f.second->get_code().size()
            << "\n";
        ++pos;
    }
}


void as2js_compiler::execute()
{
    if(f_filenames.empty())
    {
        f_filenames.push_back("a.out");
    }
    else if(f_filenames.size() > 1)
    {
        ++f_error_count;
        std::cerr
            << "error: --execute expects exactly one filename.\n";
        return;
    }

    as2js::running_file script;
    if(!script.load(f_filenames[0]))
    {
        ++f_error_count;
        std::cerr
            << "error: could not load \""
            << f_filenames[0]
            << "\" to execute code.\n";
        return;
    }

    for(auto const & var : f_variables)
    {
//std::cerr << "--- var = [" << var.second << "]\n";
        // TODO: support all types of variables
        //
        std::string value(var.second);
        std::size_t pos(var.first.find(':'));
        std::string name;
        std::string type;
        if(pos != std::string::npos)
        {
            name = var.first.substr(0, pos);
            type = var.first.substr(pos + 1);
        }
        else
        {
            name = var.first;
            if(value.empty())
            {
                type = "string";
            }
            else if(value.length() >= 2
                 && value[0] == '"'
                 && value[value.length() - 1] == '"')
            {
                type = "string";
                value = value.substr(1, value.length() - 2);
            }
            else if(value.length() >= 2
                 && value[0] == '\''
                 && value[value.length() - 1] == '\'')
            {
                type = "string";
                value = value.substr(1, value.length() - 2);
            }
            else
            {
                type = "integer";
                std::size_t const max(value.length());
                std::size_t idx(0);
                if(max >= 2
                && (value[idx] == '+' || value[idx] == '-'))
                {
                    ++idx;
                }
                for(; idx < max; ++idx)
                {
                    char const c(value[idx]);
                    if(c == '.')
                    {
                        if(type == "integer")
                        {
                            type = "double";
                        }
                        else
                        {
                            type = "string";
                            break;
                        }
                    }
                    else if(c < '0' || c > '9')
                    {
                        if(value == "false"
                        || value == "true")
                        {
                            type = "boolean";
                            break;
                        }
                        type = "string";
                        break;
                    }
                    else if(max > 0 && (c == 'e' || c == 'E'))
                    {
                        if(type == "integer")
                        {
                            type = "double";
                        }
                        else if(type != "double")
                        {
                            type = "string";
                            break;
                        }
                        ++idx;
                        if(value[idx] == '+' || value[idx] == '-')
                        {
                            ++idx;
                        }
                        if(idx >= max)
                        {
                            type = "string";
                            break;
                        }
                        for(; idx < max; ++idx)
                        {
                            char const e(value[idx]);
                            if(e < '0' || e > '9')
                            {
                                type = "string";
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
        if(type == "string"
        && f_three_underscores_to_space)
        {
            value = snapdev::string_replace_many(value, {{"___", " "}});
        }
        if(name.empty())
        {
            ++f_error_count;
            std::cerr
                << "error: a variable must have a name before its ':<type>' specification.\n";
            return;
        }

        if(!script.has_variable(name))
        {
            if(!f_ignore_unknown_variables)
            {
                ++f_error_count;
                std::cerr
                    << "error: unknown variable \""
                    << name
                    << "\" in \""
                    << f_filenames[0]
                    << "\".\n";
                return;
            }
            std::cerr
                << "warning: variable \""
                << name
                << "\" not found in \""
                << f_filenames[0]
                << "\".\n";
            continue;
        }
        type = snapdev::to_lower(type);
        if(type == "integer")
        {
            std::int64_t const integer(std::stoll(value, nullptr, 0));
            script.set_variable(name, integer);
        }
        else if(type == "boolean")
        {
            if(value == "true")
            {
                script.set_variable(name, true);
            }
            else if(value == "false")
            {
                script.set_variable(name, false);
            }
            else
            {
                ++f_error_count;
                std::cerr
                    << "error: a boolean variable must be set to \"true\" or \"false\".\n";
                return;
            }
        }
        else if(type == "double")
        {
            double const floating_point(std::stod(value, nullptr));
            script.set_variable(name, floating_point);
        }
        else if(type == "string")
        {
            script.set_variable(name, value);
        }
        else
        {
            ++f_error_count;
            std::cerr
                << "error: unknown variable type \""
                << type
                << "\" for \""
                << name
                << "\".\n";
            return;
        }
    }

    as2js::binary_result result;

    script.run(result);

    if(!f_save_to_file.empty())
    {
        script.save(f_save_to_file);
    }

    switch(result.get_type())
    {
    case as2js::variable_type_t::VARIABLE_TYPE_BOOLEAN:
        std::cout << std::boolalpha << result.get_boolean() << "\n";
        break;

    case as2js::variable_type_t::VARIABLE_TYPE_INTEGER:
        std::cout << result.get_integer() << "\n";
        break;

    case as2js::variable_type_t::VARIABLE_TYPE_FLOATING_POINT:
        std::cout << result.get_floating_point() << "\n";
        break;

    case as2js::variable_type_t::VARIABLE_TYPE_STRING:
        std::cout << result.get_string() << "\n";
        break;

    default:
        std::cerr << "error: unknown result type in as2js_compiler.\n";
        break;

    }

    if(f_show_all_results)
    {
        std::size_t const count(script.variable_size());
        for(std::size_t idx(0); idx < count; ++idx)
        {
            std::string name;
            as2js::binary_variable * var(script.get_variable(idx, name));

            std::cout << name << "=";

            std::uint64_t const * data;
            if(var->f_data_size <= sizeof(var->f_data))
            {
                data = &var->f_data;
            }
            else
            {
                data = reinterpret_cast<std::uint64_t const *>(var->f_data);
            }

            switch(var->f_type)
            {
            case as2js::variable_type_t::VARIABLE_TYPE_UNKNOWN:
                std::cout << "<unknown variable type>\n";
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_BOOLEAN:
                std::cout << std::boolalpha << ((*data & 255) != 0) << '\n';
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_INTEGER:
                std::cout << static_cast<std::int64_t>(*data) << '\n';
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_FLOATING_POINT:
                std::cout << *reinterpret_cast<double const *>(data) << '\n';
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_STRING:
                if(data != nullptr)
                {
                    std::cout << std::string(reinterpret_cast<char const *>(data), var->f_data_size) << '\n';
                }
                else
                {
                    std::cout << "(null)\n";
                }
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_ARRAY:
                // TODO: implement this for() loop in a sub-function which
                //       we can call recursively
                //
                std::cout << "--- found array ---\n";
                break;

            case as2js::variable_type_t::VARIABLE_TYPE_RANGE:
                std::cout << "--- RANGE value not yet supported in as2js ---\n";
                break;

            }
        }
    }
}



} // no name namespace


int main(int argc, char *argv[])
{
    try
    {
        as2js_compiler::pointer_t c(std::make_shared<as2js_compiler>());
        if(c->parse_command_line_options(argc, argv) != 0)
        {
            return 1;
        }
        return c->run();
    }
    catch(std::exception const & e)
    {
        std::cerr << "as2js: exception: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
