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



enum class output_t
{
    OUTPUT_UNDEFINED,

    OUTPUT_PARSER_TREE,
    OUTPUT_COMPILER_TREE,
    OUTPUT_BINARY,
    OUTPUT_ASSEMBLY,
    OUTPUT_JAVASCRIPT,
    OUTPUT_CPP,
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
    void                        set_output(output_t output);
    void                        set_option(
                                      as2js::options::option_t option
                                    , int argc
                                    , char * argv[]
                                    , int & i);
    int                         output_error_count();
    void                        generate_binary();

    int                         f_error_count = 0;
    std::string                 f_progname = std::string();
    std::vector<std::string>    f_filenames = std::vector<std::string>();
    output_t                    f_output = output_t::OUTPUT_UNDEFINED;
    as2js::options::pointer_t   f_options = std::make_shared<as2js::options>();
    std::set<as2js::options::option_t>
                                f_option_defined = std::set<as2js::options::option_t>();
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
                    set_output(output_t::OUTPUT_BINARY);
                }
                else if(strcmp(argv[i] + 2, "parser-tree") == 0)
                {
                    set_output(output_t::OUTPUT_PARSER_TREE);
                }
                else if(strcmp(argv[i] + 2, "compiler-tree") == 0)
                {
                    set_output(output_t::OUTPUT_COMPILER_TREE);
                }
                else if(strcmp(argv[i] + 2, "allow-with") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_ALLOW_WITH, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "coverage") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_COVERAGE, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "debug") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_DEBUG, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-sequences") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-operators") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_EXTENDED_OPERATORS, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "extended-escape-statements") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_EXTENDED_STATEMENTS, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "json") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_JSON, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "octal") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_OCTAL, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "strict") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_STRICT, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "trace") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_TRACE, argc, argv, i);
                }
                else if(strcmp(argv[i] + 2, "unsafe-math") == 0)
                {
                    set_option(as2js::options::option_t::OPTION_UNSAFE_MATH, argc, argv, i);
                }
                else
                {
                    ++f_error_count;
                    std::cerr
                        << "error: unknown command line option '"
                        << argv[i]
                        << "'.\n";
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
                        set_output(output_t::OUTPUT_BINARY);
                        break;

                    case 'h':
                        usage();
                        return 1;

                    case 'L':
                        license();
                        return 1;

                    case 't':
                        set_output(output_t::OUTPUT_PARSER_TREE);
                        break;

                    case 'T':
                        set_output(output_t::OUTPUT_COMPILER_TREE);
                        break;

                    case 'V':
                        version();
                        return 1;

                    default:
                        ++f_error_count;
                        std::cerr
                            << "error: unknown command line option '-"
                            << argv[i][j]
                            << "'.\n";
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

    if(f_error_count == 0
    && f_filenames.empty())
    {
        ++f_error_count;
        std::cerr << "error: an input filename is required.\n";
    }

    if(f_output == output_t::OUTPUT_UNDEFINED)
    {
        // this is what one wants by default (transliteration from
        // Alex or Advanced Script to JavaScript)
        //
        f_output = output_t::OUTPUT_JAVASCRIPT;
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
           "  -b | --binary          generate a binary file.\n"
           "  -h | --help            print out this help screen.\n"
           "  -L | --license         print compiler's license.\n"
           "  -t | --parser-tree     output the tree of nodes.\n"
           "  -T | --compiler-tree   output the tree of nodes.\n"
           "  -V | --version         print version of the compiler.\n"
    ;
}


void as2js_compiler::version()
{
    std::cout
        << f_progname << " v" << AS2JS_VERSION_STRING << std::endl
        << "libas2js v" << as2js::get_version_string() << std::endl;
}


void as2js_compiler::set_output(output_t output)
{
    if(f_output != output_t::OUTPUT_UNDEFINED)
    {
        ++f_error_count;
        std::cerr << "error: output type already set. Try only one of --binary or --tree.\n";
        return;
    }

    f_output = output;
}


void as2js_compiler::set_option(as2js::options::option_t option, int argc, char * argv[], int & i)
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

    as2js::options::option_value_t value(0);
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
    // we are compiling a user script so mark it as such
    //
    f_options->set_option(as2js::options::option_t::OPTION_USER_SCRIPT, 1);

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

        if(f_output == output_t::OUTPUT_PARSER_TREE)
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

        switch(f_output)
        {
        case output_t::OUTPUT_UNDEFINED:
        case output_t::OUTPUT_PARSER_TREE:
            throw as2js::internal_error("these cases were checked earlier and cannot happen here"); // LCOV_EXCL_LINE

        case output_t::OUTPUT_COMPILER_TREE:
            std::cout << *f_root << "\n";
            break;

        case output_t::OUTPUT_BINARY:
            generate_binary();
            break;

        case output_t::OUTPUT_ASSEMBLY:
        case output_t::OUTPUT_JAVASCRIPT:
        case output_t::OUTPUT_CPP:
            ++f_error_count;
            std::cerr
                << "error: output type not yet implemented.\n";
            break;

        }
    }

    return output_error_count();
}


void as2js_compiler::generate_binary()
{
    as2js::output_stream<std::ofstream>::pointer_t output(std::make_shared<as2js::output_stream<std::ofstream>>());
    as2js::binary::pointer_t binary(std::make_shared<as2js::binary>(output, f_options));
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
