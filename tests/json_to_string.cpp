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
#include    <tests/license.h>


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/stringize.h>


// as2js
//
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/version.h>


// C
//
#include    <string.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>





class messages
    : public as2js::message_callback
{
public:
    messages()
    {
        as2js::set_message_callback(this);
    }

    // implementation of the output
    virtual void output(
          as2js::message_level_t message_level
        , as2js::err_code_t error_code
        , as2js::position const & pos
        , std::string const & message)
    {
        std::cerr << "error:" << static_cast<int>(message_level)
                       << ":" << static_cast<int>(error_code)
                       << ":" << pos
                       << ":" << message << std::endl;
    }
};



#pragma GCC diagnostic ignored "-Wpedantic"
int main(int argc, char **argv)
{
    int err(0);
    bool newlines(false);
    bool keep_comments(false);
    std::string const progname(snapdev::pathinfo::basename(std::string(argv[0])));
    std::string output_filename;
    std::vector<std::string> input_filenames;

    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--license") == 0)
        {
            std::cout << as2js_tools::license;
            return 1;
        }
        if(strcmp(argv[i], "--help") == 0
        || strcmp(argv[i], "-h") == 0)
        {
            std::cout << "Usage: " << progname << " [--opt ...] <input> ...\n"
                "where --opt is one of:\n"
                "  --copyright               print this tool copyright notice\n"
                "  --help | -h               print out the help screen\n"
                "  --license                 show the license\n"
                "  --keep-comments           keep comments in output\n"
                "  --newlines                insert newlines in the output\n"
                "  --version                 print the version of the as2js project\n"
                "  --output | -o <filename>  the name of the output file\n"
                ;
            return 1;
        }
        if(strcmp(argv[i], "--version") == 0)
        {
            std::cout << AS2JS_VERSION_STRING << std::endl;
            return 1;
        }
        if(strcmp(argv[i], "--copyright") == 0)
        {
            std::cout << "Copyright (c) 2005-"
                           SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                           " by Made to Order Software Corporation, All Rights Reserved";
            return 1;
        }

        if(strcmp(argv[i], "--newlines") == 0)
        {
            newlines = true;
        }
        else if(strcmp(argv[i], "--keep-comments") == 0)
        {
            keep_comments = true;
        }
        else if(strcmp(argv[i], "--output") == 0
        || strcmp(argv[i], "-o") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "json-to-string:error: --output must be followed by a filename.\n";
                return 1;
            }
            if(!output_filename.empty())
            {
                std::cerr << "json-to-string:error: --output can only be used once.\n";
                return 1;
            }
            output_filename = argv[i];
        }
        else if(argv[i][0] == '-')
        {
            std::cerr
                << "json-to-string:error: unknown command line option \""
                << argv[i]
                << "\".\n";
            return 1;
        }
        else if(argv[i][0] == '\0')
        {
            std::cerr << "json-to-string:error: a filename must be specified (an empty parameter is not acceptable).\n";
            return 1;
        }
        else
        {
            input_filenames.push_back(argv[i]);
        }
    }

    as2js::output_stream<std::ofstream>::pointer_t out(std::make_shared<as2js::output_stream<std::ofstream>>());
    out->open(output_filename);
    if(!out->is_open())
    {
        std::cerr
            << "error: could not open output file \""
            << output_filename
            << "\" for writing.\n";
        return 1;
    }

    messages msg;
    std::size_t max_filenames(input_filenames.size());
    for(std::size_t idx(0); idx < max_filenames; ++idx)
    {
        // first we use JSON to load the file, if we detect an
        // error return 1 instead of 0
        //
        std::string filename(input_filenames[idx]);
        as2js::json::pointer_t load_json(std::make_shared<as2js::json>());
        as2js::json::json_value::pointer_t loaded_value(load_json->load(filename));
        if(loaded_value != nullptr)
        {
            as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
            in->open(filename);
            if(in->is_open())
            {
                char32_t c(0);
                while(c != as2js::CHAR32_EOF)
                {
                    // read one line of JSON
                    std::string str;
                    std::string indent;
                    for(;;)
                    {
                        c = in->get();
                        if(c == as2js::CHAR32_EOF || c == '\n')
                        {
                            break;
                        }
                        if((c == ' ' || c == '\t') && str.empty())
                        {
                            // left trim
                            indent += c;
                            continue;
                        }
                        if(str.empty() && c == '/')
                        {
                            c = in->get();
                            if(c == '/')
                            {
                                // read comments
                                str += "/";
                                do
                                {
                                    str += c;
                                    c = in->get();
                                }
                                while(c != as2js::CHAR32_EOF && c != '\n');
                                if(keep_comments)
                                {
                                    // keep the comments, but not inside the JSON strings
                                    out->write_string(indent);
                                    out->write_string(str);
                                    if(str[str.length() - 1] == '\\')
                                    {
                                        // we add a $ when str ends with a '\'
                                        out->write_string("$");
                                    }
                                    out->write_string("\n");
                                }
                                indent.clear();
                                str.clear();
                                continue;
                            }
                        }
                        if(c == '"')
                        {
                            // add 1 '\' characters in front of the '"'
                            str += "\\\"";
                        }
                        else if(c == '\\')
                        {
                            // add 2 '\' character for each '\'
                            str += "\\\\";
                        }
                        else
                        {
                            str += c;
                        }
                    }
                    if(!str.empty())
                    {
                        // if string ends with "\" then we need to add a "\n"
                        if(newlines
                        || str[str.length() - 1] == '\\')
                        {
                            str += "\\n";
                        }

                        // output this one line as a C++ string
                        //
                        out->write_string(indent);
                        out->write_string("\"");
                        out->write_string(str);
                        out->write_string("\"\n");
                    }
                }
            }
            else
            {
                as2js::message err_msg(as2js::message_level_t::MESSAGE_LEVEL_FATAL, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, loaded_value->get_position());
                err_msg << "could not re-open this JSON input file \"" << filename << "\".";
                err = 1;
            }
        }
        else
        {
            err = 1;
        }
    }

    if(err == 1)
    {
        // on error make sure to delete because otherwise cmake thinks
        // that the target is all good.
        //
        unlink(output_filename.c_str());
    }

    return err;
}

// vim: ts=4 sw=4 et
