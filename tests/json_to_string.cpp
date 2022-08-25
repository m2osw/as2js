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

// tools
//
#include    <tools/license.h>


// as2js
//
#include    <as2js/json.h>
#include    <as2js/exceptions.h>
#include    <as2js/message.h>
#include    <as2js/version.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/licenses.h>


// boost
//
#include    <boost/preprocessor/stringize.hpp>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>





class messages : public as2js::message_callback
{
public:
    messages()
    {
        as2js::message::set_message_callback(this);
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

    try
    {
        static const advgetopt::option options[] = {
            advgetopt::define_option(
                  advgetopt::Name("licence")
                , advgetopt::Flags(advgetopt::standalone_command_flags<advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
                , advgetopt::Alias("license")
            ),
            advgetopt::define_option(
                  advgetopt::Name("output")
                , advgetopt::ShortName(U'o')
                , advgetopt::Flags(advgetopt::command_flags<advgetopt::GETOPT_FLAG_REQUIRED>())
                , advgetopt::Help("the output filename")
            ),
            advgetopt::define_option(
                  advgetopt::Name("filename")
                , advgetopt::Flags(advgetopt::command_flags<advgetopt::GETOPT_FLAG_MULTIPLE
                                                          , advgetopt::GETOPT_FLAG_DEFAULT_OPTION>())
            ),
            advgetopt::end_options()
        };
        static const advgetopt::options_environment options_env =
        {
            .f_project_name = "as2js",
            .f_group_name = nullptr,
            .f_options = options,
            .f_options_files_directory = nullptr,
            .f_environment_variable_name = nullptr,
            .f_environment_variable_intro = nullptr,
            .f_section_variables_name = nullptr,
            .f_configuration_files = nullptr,
            .f_configuration_filename = nullptr,
            .f_configuration_directories = nullptr,
            .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
            .f_help_header = "Usage: %p [--opt] [test-name]\n"
                             "with --opt being one or more of the following:",
            .f_help_footer = "%c",
            .f_version = AS2JS_VERSION_STRING,
            .f_license = advgetopt::g_license_gpl_v2,
            .f_copyright = "Copyright (c) 2005-"
                           BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                           " by Made to Order Software Corporation, All Rights Reserved",
            //.f_build_date = UTC_BUILD_DATE,
            //.f_build_time = UTC_BUILD_TIME,
            //.f_groups     = nullptr
        };

        advgetopt::getopt opt(options_env, argc, argv);

        if(opt.is_defined("help"))
        {
            std::cerr << opt.usage(advgetopt::GETOPT_FLAG_SHOW_ALL);
            exit(1);
        }

        if(opt.is_defined("version"))
        {
            std::cout << AS2JS_VERSION_STRING << std::endl;
            exit(1);
        }

        if(opt.is_defined("license") || opt.is_defined("licence"))
        {
            std::cout << as2js_tools::license;
            exit(1);
        }

        if(!opt.is_defined("filename"))
        {
            std::cerr << "error: no filename specified." << std::endl;
            exit(1);
        }

        if(!opt.is_defined("output"))
        {
            std::cerr << "error: no output specified.\n";
            exit(1);
        }

        std::string output_filename(opt.get_string("output"));
        as2js::output_stream<std::ofstream>::pointer_t out(std::make_shared<as2js::output_stream<std::ofstream>>());
        out->open(output_filename);
        if(!out->is_open())
        {
            std::cerr
                << "error: could not open output file \""
                << output_filename
                << "\" for writing.\n";
            exit(1);
        }

        messages msg;
        int max_filenames(opt.size("filename"));
        for(int idx(0); idx < max_filenames; ++idx)
        {
            // first we use JSON to load the file, if we detect an
            // error return 1 instead of 0
            std::string filename(opt.get_string("filename", idx));
            as2js::json::pointer_t load_json(std::make_shared<as2js::json>());
            as2js::json::json_value::pointer_t loaded_value(load_json->load(filename));
            if(loaded_value)
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
                                    // skip comments
                                    str += "/";
                                    do
                                    {
                                        str += c;
                                        c = in->get();
                                    }
                                    while(c != as2js::CHAR32_EOF && c != '\n');
                                    // keep the comments, but not inside the JSON strings
                                    out->write_string(indent);
                                    out->write_string(str);
                                    if(str[str.length() - 1] == '\\')
                                    {
                                        // we add a $ when str ends with a '\'
                                        out->write_string("$");
                                    }
                                    out->write_string("\n");
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
                            if(str[str.length() - 1] == '\\')
                            {
                                str += "\\n";
                            }
                            // output this one line as a C++ string
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
            unlink(opt.get_string("output").c_str());
        }
    }
    catch(advgetopt::getopt_exit const & e)
    {
        err = e.code();
    }

    return err;
}

// vim: ts=4 sw=4 et
