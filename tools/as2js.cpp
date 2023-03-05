// Copyright (c) 2005-2023  Made to Order Software Corp.  All Rights Reserved
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



// as2js lib
//
#include    <as2js/binary.h>
#include    <as2js/compiler.h>
#include    <as2js/exception.h>
#include    <as2js/message.h>
#include    <as2js/parser.h>
#include    <as2js/version.h>


// snapdev
//
#include    <snapdev/pathinfo.h>


// C++
//
#include    <cstring>
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
    COMMAND_DATA_SECTION,
    COMMAND_END_SECTION,
    COMMAND_IS_BINARY,
    COMMAND_JAVASCRIPT,
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
    void                        generate_binary();
    void                        binary_utils();

    int                         f_error_count = 0;
    std::string                 f_progname = std::string();
    std::vector<std::string>    f_filenames = std::vector<std::string>();
    std::string                 f_output = std::string();
    command_t                   f_command = command_t::COMMAND_UNDEFINED;
    as2js::options::pointer_t   f_options = std::make_shared<as2js::options>();
    std::set<as2js::option_t>   f_option_defined = std::set<as2js::option_t>();
    as2js::node::pointer_t      f_root = as2js::node::pointer_t();
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
                else if(strcmp(argv[i] + 2, "binary") == 0)
                {
                    set_output(command_t::COMMAND_BINARY);
                }
                else if(strcmp(argv[i] + 2, "binary-version") == 0)
                {
                    set_output(command_t::COMMAND_BINARY_VERSION);
                }
                else if(strcmp(argv[i] + 2, "data-section") == 0)
                {
                    set_output(command_t::COMMAND_DATA_SECTION);
                }
                else if(strcmp(argv[i] + 2, "end-section") == 0)
                {
                    set_output(command_t::COMMAND_END_SECTION);
                }
                else if(strcmp(argv[i] + 2, "is-binary") == 0)
                {
                    set_output(command_t::COMMAND_IS_BINARY);
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

                    case 'L':
                        license();
                        return 1;

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
            f_filenames.push_back(argv[i]);
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
        << "Usage: " << f_progname << " [-opts] <filename>.ajs ...\n"
           "where -opts is one or more of:\n"
           "Commands (one of):\n"
           "  -b | --binary          generate a binary file.\n"
           "       --binary-version  output version of the binary file.\n"
           "       --data-section    position where the data section starts.\n"
           "       --end-section     position where the end section starts.\n"
           "  -h | --help            print out this help screen.\n"
           "       --is-binary       check whether a file is a binary file.\n"
           "  -L | --license         print compiler's license.\n"
           "  -t | --parser-tree     output the tree of nodes.\n"
           "       --text-section    position where the text section starts.\n"
           "  -T | --compiler-tree   output the tree of nodes.\n"
           "       --variables       list external variables.\n"
           "  -V | --version         print version of the compiler.\n"
           "\n"
           "Options:\n"
           "  none just yet\n"
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
        as2js::compiler compiler(f_options);
        if(compiler.compile(f_root) != 0)
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
            generate_binary();
            break;

        case command_t::COMMAND_ASSEMBLY:
        case command_t::COMMAND_JAVASCRIPT:
        case command_t::COMMAND_CPP:
            ++f_error_count;
            std::cerr
                << "error: output command not yet implemented.\n";
            break;

        case command_t::COMMAND_BINARY_VERSION:
        case command_t::COMMAND_IS_BINARY:
        case command_t::COMMAND_DATA_SECTION:
        case command_t::COMMAND_END_SECTION:
        case command_t::COMMAND_PARSER_TREE:
        case command_t::COMMAND_TEXT_SECTION:
        case command_t::COMMAND_UNDEFINED:
        case command_t::COMMAND_VARIABLES:
            throw as2js::internal_error("these cases were checked earlier and cannot happen here."); // LCOV_EXCL_LINE

        }
    }
}


void as2js_compiler::generate_binary()
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
    as2js::binary_assembler::pointer_t binary(std::make_shared<as2js::binary_assembler>(output, f_options));
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
        break;

    default:
        throw as2js::internal_error("these cases were checked earlier and cannot happen here"); // LCOV_EXCL_LINE

    }

    // list external variables
    //
    in.seekg(header.f_variables, std::ios_base::beg);
    as2js::binary_variable::vector_t variables(header.f_variable_count);
    in.read(reinterpret_cast<char *>(variables.data()), sizeof(as2js::binary_variable) * header.f_variable_count);
    for(std::uint16_t idx(0); idx < header.f_variable_count; ++idx)
    {
        in.seekg(variables[idx].f_name, std::ios_base::beg);
        std::string name(variables[idx].f_name_size, ' ');
        in.read(name.data(), variables[idx].f_name_size);
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
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_INTEGER:
            std::cout << ": Integer";
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_FLOATING_POINT:
            std::cout << ": Double";
            break;

        case as2js::variable_type_t::VARIABLE_TYPE_STRING:
            std::cout << ": String";
            break;

        //case as2js::variable_type_t::VARIABLE_TYPE_UNKNOWN:
        default:
            std::cout << " /* unknown type */ ";
            break;

        }
    }
    std::cout << ";\n";
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
    catch(std::exception const& e)
    {
        std::cerr << "as2js: exception: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
