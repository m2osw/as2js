// Copyright (c) 2022  Made to Order Software Corp.  All Rights Reserved
// 
// https://snapwebsites.org/project/as2js
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and
// associated documentation files (the "Software"), to
// deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
// 
// The above copyright notice and this permission notice
// shall be included in all copies or substantial
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// self
//
#include    "license.h"


// as2js
//
#include    <as2js/as2js.h>
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/stream.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>


// libexcept
//
#include    <libexcept/exception.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// boost
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>


/** \file
 * \brief Implementation of a JSON tool.
 *
 * Much of the JavaScript data (and now much more) is managed using JSON.
 * This tool allows you to do various tasks with JSON data:
 *
 * * verify that it can be loaded and whether it is fully compliant
 * * output JSON as one line or beautified
 * * pick the value of a field and print it to stdout
 */




/** \brief Private implementations of the JSON tool.
 *
 * This namespace is used to hide all the tool private functions to
 * avoid conflicts.
 */
namespace
{


/** \brief Command line options.
 *
 * This table includes all the options supported by the compiler.
 */
constexpr advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("verify")
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("Verify that the input is valid JSON.")
    ),
    advgetopt::define_option(
          advgetopt::Name("--")
        , advgetopt::Flags(advgetopt::command_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_MULTIPLE>())
        , advgetopt::Help("[<input> [<output>]]")
    ),
    advgetopt::end_options()
};


advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};



// TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "json",
    .f_group_name = "as2js",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "JSONFLAGS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [--<opt>] [<input> [<output>]]\n"
                     "Where --<opt> is one or more of:",
    .f_help_footer = nullptr,
    .f_version = AS2JS_VERSION,
    .f_license = "GPL v2 or newer",
    .f_copyright = "Copyright (c) 2022-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions,
};



} // no name namespace


class json_handler
    : public as2js::MessageCallback
{
public:
    typedef std::shared_ptr<json_handler>         pointer_t;

                            json_handler(int argc, char *argv[]);

    int                     run();

    // MessageCallback implementation
    virtual void            output(
                                  as2js::message_level_t message_level
                                , as2js::err_code_t error_code
                                , as2js::Position const& pos
                                , std::string const& message) override;

private:
    advgetopt::getopt       f_opts;
};


json_handler::json_handler(int argc, char *argv[])
    : f_opts(g_options_environment, argc, argv)
{
    as2js::Message::set_message_callback(this);
}


void json_handler::output(
      as2js::message_level_t message_level
    , as2js::err_code_t error_code
    , as2js::Position const & pos
    , std::string const & message)
{
    bool is_error(false);
    std::string msg;
    switch(message_level)
    {
    case as2js::message_level_t::MESSAGE_LEVEL_OFF:
        return;

    case as2js::message_level_t::MESSAGE_LEVEL_FATAL:
        is_error = true;
        msg += "fatal";
        break;

    case as2js::message_level_t::MESSAGE_LEVEL_ERROR:
        is_error = true;
        msg += "error";
        break;

    case as2js::message_level_t::MESSAGE_LEVEL_WARNING:
        msg += "warning";
        break;

    case as2js::message_level_t::MESSAGE_LEVEL_INFO:
        msg += "info";
        break;

    case as2js::message_level_t::MESSAGE_LEVEL_DEBUG:
        msg += "debug";
        break;

    case as2js::message_level_t::MESSAGE_LEVEL_TRACE:
        msg += "trace";
        break;

    default:
        msg += "unknown-level";
        break;

    }

    msg += ':';
    msg += std::to_string(static_cast<int>(error_code));
    msg += ':';

    if(!pos.get_filename().empty())
    {
        msg += " in ";
        msg += pos.get_filename().to_utf8();
        if(pos.get_line() > 0)
        {
            msg += '(';
            msg += std::to_string(pos.get_line());
            if(pos.get_column() != as2js::Position::DEFAULT_COUNTER)
            {
                msg += ':';
                msg += std::to_string(pos.get_column());
            }
            msg += ')';
        }
        msg += ':';
    }
    else if(!pos.get_function().empty())
    {
        msg += pos.get_function().to_utf8();
        msg += "():";
        if(pos.get_line() > 0)
        {
            msg += std::to_string(pos.get_line());
            if(pos.get_column() != as2js::Position::DEFAULT_COUNTER)
            {
                msg += ':';
                msg += std::to_string(pos.get_column());
            }
            msg += ':';
        }
    }
    else if(pos.get_line() > 0)
    {
        msg += std::to_string(pos.get_line());
        if(pos.get_column() != as2js::Position::DEFAULT_COUNTER)
        {
            msg += ':';
            msg += std::to_string(pos.get_column());
        }
        msg += ':';
    }

    msg += ' ';
    msg += message;

    if(is_error)
    {
        std::cerr << msg << std::endl;
    }
    else
    {
        std::cout << msg << std::endl;
    }
}


int json_handler::run()
{
    // setup input
    //
    as2js::Input::pointer_t in;
    if(f_opts.size("--") < 1
    || f_opts.get_string("--") == "-")
    {
        in = std::make_shared<as2js::StandardInput>();
    }
    else
    {
        auto i = std::make_shared<as2js::FileInput>();
        if(!i->open(as2js::String(f_opts.get_string("--"))))
        {
            std::cerr
                << "error: could not open \""
                << f_opts.get_string("--")
                << "\".\n";
            return 1;
        }
        in = i;
    }

    // load the JSON in memory
    //
    // (one day we may have a streaming version which can read & work on
    // the JSON objects without the need of a full memory preload)
    //
    as2js::JSON json;
    as2js::JSON::JSONValue::pointer_t root(json.parse(in));

    int const error_count(as2js::Message::error_count());
    if(error_count > 0)
    {
        std::cerr << "found "
            << error_count
            << " error"
            << (error_count == 1 ? "" : "s")
            << std::endl;
        return 1;
    }

    if(root == nullptr)
    {
        std::cerr << "error: could not load input.\n";
        return 1;
    }

    int const warning_count(as2js::Message::warning_count());
    if(warning_count > 0)
    {
        std::cerr << "found "
            << warning_count
            << " warning"
            << (warning_count == 1 ? "" : "s")
            << std::endl;
        return 1;
    }

    // commands that do not require output data
    //
    if(f_opts.is_defined("verify"))
    {
        // it loaded, it's verified!
        //
        return 0;
    }

    // setup output
    //
    as2js::Output::pointer_t out;
    if(f_opts.size("--") < 2
    || f_opts.get_string("--", 1) == "-")
    {
        out = std::make_shared<as2js::StandardOutput>();
    }
    else
    {
        auto o = std::make_shared<as2js::FileOutput>();
        if(!o->open(f_opts.get_string("--", 1)))
        {
            std::cerr << "error: could not open output file \""
                << f_opts.get_string("--", 1)
                << ".\n";
            return 1;
        }
        out = o;
    }

    return 0;
}


int main(int argc, char *argv[])
{
    try
    {
        json_handler h(argc, argv);
        return h.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        // expected exit from the advgetopt (i.e. --help)
        return e.code();
    }
    catch(libexcept::exception_base_t const & e)
    {
        std::cerr
            << "json: exception: "
            << dynamic_cast<std::exception const &>(e).what()
            << std::endl;
        std::cerr
            << "json: TODO: write stack trace out..."
            << std::endl;
        return 1;
    }
    catch(std::exception const & e)
    {
        std::cerr
            << "json: exception: "
            << e.what()
            << std::endl;
        return 1;
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et
