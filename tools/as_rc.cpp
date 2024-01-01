// Copyright (c) 2005-2024  Made to Order Software Corp.  All Rights Reserved
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

// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/pathinfo.h>
#include    <snapdev/string_replace_many.h>


// C
//
#include    <string.h>



/** \file
 * \brief Tool used to convert text files to a C string.
 *
 * Often, I would like to have a "resource" built from an external text file
 * which gets compiled so the resulting library or tool has the resource
 * within its .DATA section instead of having to load a file.
 *
 * This tool coverts such text files to a .ci (C include) file with a string
 * composed of the input file converted to lines, "\\n", and also a length
 * for the string. The length is useful to create an std::string or when the
 * input may include "\\0" characters.
 *
 * If the input may include binary, use the --binary command line option and
 * all the bytes that are not ASCII will be transformed to the `\xXX` syntax.
 */


namespace
{


class as_rc
{
public:
                                as_rc(int argc, char * argv[]);
                                as_rc(as_rc const & rhs) = delete;
    as_rc &                     operator = (as_rc const & rhs) = delete;

    int                         init();
    int                         run();

private:
    void                        usage();

    int                         f_argc = 0;
    char **                     f_argv = nullptr;
    std::vector<std::string>    f_filenames = {};
    std::string                 f_output = std::string();
    std::string                 f_header = std::string();
    std::string                 f_name = std::string();
    std::string                 f_namespace = std::string();
    bool                        f_binary = false;
    bool                        f_verbose = false;
};


void as_rc::usage()
{
    std::cout << "Usage: as-rc [--opts] [--] <in1> <in2> ... <inN>\n";
    std::cout << "where [--opts] is one of more of the following:\n";
    std::cout << "   -h | --help                 print out this help screen.\n";
    std::cout << "   -o | --output <filename>    specify the output filename.\n";
    std::cout << "   -n | --name <name>          name of the final string variable.\n";
    std::cout << "   -n | --namespace <name>     place variables in a C++ namespace.\n";
    std::cout << "   -b | --binary               input is binary, not text.\n";
    std::cout << "   -v | --verbose              display messages.\n";
    std::cout << "   --                          anything after this are input filenames.\n";
}


as_rc::as_rc(int argc, char * argv[])
    : f_argc(argc)
    , f_argv(argv)
{
}


int as_rc::init()
{
    bool more_options(true);
    for(int i(1); i < f_argc; ++i)
    {
        if(more_options
        && f_argv[i][0] == '-')
        {
            if(f_argv[i][1] == '-')
            {
                // long form
                //
                if(f_argv[i][2] == '\0')
                {
                    more_options = false;
                }
                else if(strcmp(f_argv[i] + 2, "help") == 0)
                {
                    usage();
                    return 1;
                }
                else if(strcmp(f_argv[i] + 2, "name") == 0)
                {
                    ++i;
                    if(i >= f_argc)
                    {
                        std::cerr << "error:as-rc: --name expect a parameter.\n";
                        return 1;
                    }
                    if(!f_name.empty())
                    {
                        std::cerr << "error:as-rc: --name already defined.\n";
                        return 1;
                    }
                    f_name = f_argv[i];
                }
                else if(strcmp(f_argv[i] + 2, "namespace") == 0)
                {
                    ++i;
                    if(i >= f_argc)
                    {
                        std::cerr << "error:as-rc: --namespace expect a parameter.\n";
                        return 1;
                    }
                    if(!f_namespace.empty())
                    {
                        std::cerr << "error:as-rc: --namespace already defined.\n";
                        return 1;
                    }
                    f_namespace = f_argv[i];
                }
                else if(strcmp(f_argv[i] + 2, "output") == 0)
                {
                    ++i;
                    if(i >= f_argc)
                    {
                        std::cerr << "error:as-rc: --output expect a parameter.\n";
                        return 1;
                    }
                    if(!f_output.empty())
                    {
                        std::cerr << "error:as-rc: --output already defined.\n";
                        return 1;
                    }
                    f_output = f_argv[i];
                }
                else if(strcmp(f_argv[i] + 2, "verbose") == 0)
                {
                    f_verbose = true;
                }
                else
                {
                    std::cerr << "error:as-rc: unknown command line option \""
                        << f_argv[i]
                        << "\".\n";
                    return 1;
                }
            }
            else
            {
                std::size_t const len(strlen(f_argv[i]));
                for(std::size_t j(1); j < len; ++j)
                {
                    // short form
                    //
                    switch(f_argv[i][j])
                    {
                    case 'h':
                        usage();
                        return 1;
                        break;

                    case 'n':
                        if(i + 1 >= f_argc)
                        {
                            std::cerr << "error:as-rc: -o expect a parameter.\n";
                            return 1;
                        }
                        if(!f_name.empty())
                        {
                            std::cerr << "error:as-rc: -n already defined.\n";
                            return 1;
                        }
                        ++i;
                        f_name = f_argv[i];
                        break;

                    case 'o':
                        if(i + 1 >= f_argc)
                        {
                            std::cerr << "error:as-rc: -o expect a parameter.\n";
                            return 1;
                        }
                        if(!f_output.empty())
                        {
                            std::cerr << "error:as-rc: -o already defined.\n";
                            return 1;
                        }
                        ++i;
                        f_output = f_argv[i];
                        break;

                    case 'v':
                        f_verbose = true;
                        break;

                    }
                }
            }
        }
        else
        {
           f_filenames.push_back(f_argv[i]);
        }
    }

    if(f_filenames.empty())
    {
        std::cerr << "error:as-rc: at least one input filename must be specified.\n";
        return 1;
    }

    if(f_output.empty())
    {
        if(f_filenames.size() == 1)
        {
            f_output = snapdev::pathinfo::replace_suffix(f_filenames[0], ".ci");
            if(f_filenames[0] == f_output)
            {
                std::cerr << "error:as-rc: your input file is a .ci file, you must specify a --output in this case.\n";
                return 1;
            }
        }
        else
        {
            // default when not specified
            //
            //f_output = "-"; -- not supported because we generate a header as well
            std::cerr << "error:as-rc: an output file name is required.\n";
            return 1;
        }
    }

    if(std::find(f_filenames.begin(), f_filenames.end(), f_output) != f_filenames.end())
    {
        std::cerr
            << "error:as-rc: one of your input filename is the same as the output filename: \""
            << f_output
            << "\".\n";
        return 1;
    }

    if(f_output == "-")
    {
        f_verbose = false;
    }

    f_header = snapdev::pathinfo::replace_suffix(f_output, ".*", ".h");

    if(f_name.empty())
    {
        if(f_filenames.size() != 1)
        {
            std::cerr << "error:as-rc: when you have more than one filename,"
                         " you must specify a --name to define the string name.\n";
            return 1;
        }
        f_name = snapdev::pathinfo::basename(f_filenames[0]);
        if(f_name.empty())
        {
            std::cerr << "error:as-rc: could not auto-define a string name,"
                         " try again with the --name command line option.\n";
            return 1;
        }
    }

    return 0;
}


int as_rc::run()
{
    std::string input;
    for(auto const & f : f_filenames)
    {
        if(f_verbose)
        {
            std::cout
                << "as-rc:info: reading \""
                << f
                << "\".\n";
        }

        snapdev::file_contents in(f);
        if(!in.read_all())
        {
            int const e(errno);
            std::cerr << "error:as-rc: could not open \""
                << f
                << "\" for reading: "
                << e
                << " -- "
                << strerror(e)
                << "\n";
            return 1;
        }
        input += in.contents();
    }

    // we've got all the contents in memory, convert to a C literal string
    //
    std::string output(
          "/* AUTO-GENERATED FILE -- DO NOT EDIT -- see as-rc(1) for details */\n"
          "#include \"" + f_header + "\"\n"
        + (f_namespace.empty() ? "" : "namespace " + f_namespace + "{\n")
        + "size_t const " + f_name + "_size=" + std::to_string(input.length()) + ";\n"
          "char const * " + f_name + "=\n");

    if(f_binary)
    {
        unsigned pos(0);
        for(auto const & byte : input)
        {
            if((pos & 16) == 0)
            {
                if(pos > 0)
                {
                    output += "\"\n\"";
                }
                else
                {
                    output += '"';
                }
            }
            ++pos;

            if(byte == '"')
            {
                output += "\\\"";
            }
            else if(static_cast<std::uint8_t>(byte) >= ' '
                 && static_cast<std::uint8_t>(byte) <= 0x7F)
            {
                output += byte;
            }
            else
            {
                output += "\\x";
                output += snapdev::int_to_hex(byte, false, 2);
            }
        }
        if(pos == 0)
        {
            // case where the input is completely empty
            //
            output += '"';
        }
        output += "\";\n";
    }
    else
    {
        output += '"';
        output += snapdev::string_replace_many(input, {
                        // WARNING: the order is important
                        {"\"", "\\\""},
                        {"\n", "\\n\"\n\""},
                    });
        output += "\";\n";
    }
    if(!f_namespace.empty())
    {
        output += "}\n";
    }

    if(f_verbose)
    {
        std::cout
            << "as-rc:info: writing to \""
            << f_output
            << "\".\n";
    }

    std::ofstream out;
    out.open(f_output);
    if(!out)
    {
        std::cerr << "error:as-rc: could not open \""
            << f_output
            << "\" for writing the output.\n";
        return 1;
    }
    out << output;
    if(!out)
    {
        std::cerr << "error:as-rc: errors happened while writing to \""
            << f_output
            << "\".\n";
        return 1;
    }

    if(f_verbose)
    {
        std::cout
            << "as-rc:info: writing to \""
            << f_header
            << "\".\n";
    }

    std::ofstream header;
    header.open(f_header);
    if(!header)
    {
        std::cerr
            << "error:as-rc: could not open \""
            << f_header
            << "\" for writing the header.\n";
        return 1;
    }
    header
        << "/* AUTO-GENERATED FILE -- DO NOT EDIT -- see as-rc(1) for details */\n"
           "#include <stddef.h>\n"
        << (f_namespace.empty() ? "" : "namespace " + f_namespace + "{\n")
        << "extern size_t const " << f_name << "_size;\n"
           "extern char const * " << f_name << ";\n"
        << (f_namespace.empty() ? "" : "}\n");
    if(!header)
    {
        std::cerr << "error:as-rc: errors happened while writing to \""
            << f_header
            << "\".\n";
        return 1;
    }

    if(f_verbose)
    {
        std::cout
            << "as-rc:info: success.\n";
    }

    return 0;
}


} // no name namespace

int main(int argc, char * argv[])
{
    as_rc rc(argc, argv);
    int r(rc.init());
    if(r != 0)
    {
        return r;
    }
    return rc.run();
}

// vim: ts=4 sw=4 et
