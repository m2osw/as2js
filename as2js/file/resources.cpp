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

// self
//
// this is private
#include    "resources.h"

#include    "as2js/exception.h"
#include    "as2js/json.h"
#include    "as2js/message.h"


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/remove_duplicates.h>
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <cstring>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{

namespace
{

constexpr char const * const g_rc_directories[] =
{
    // check user defined variable
    //
    "$AS2JS_RC",

    // try locally first (assuming you are a heavy JS developer, you'd
    // probably start with your local files)
    //
    "as2js",

    // try your user "global" installation directory
    //
    "~/.config/as2js",

    // try the system directory
    //
    "/etc/as2js",

    nullptr
};


bool                        g_home_initialized = false;
std::string                 g_home;

}
// no name namespace


/** \brief Initialize the resources with defaults.
 *
 * The constructor calls the reset() function to initialize the
 * variable resource parameters to internal defaults.
 */
resources::resources()
{
    reset();
}


/** \brief Reset the resources to internal defaults.
 *
 * This function resets all the resources variables to internal defaults:
 *
 * \li scripts -- "as2js/scripts"
 * \li db -- "/tmp/as2js_packages.db"
 * \li temporary_variable_name -- "@temp"
 *
 * This function is called on construction and when calling init_rc().
 *
 * Note that does not reset the home parameter which has no internal
 * default and is managed differently.
 *
 * \todo
 * Fix the default temporary directory which right now is not going to
 * work in a concurrent environment (i.e. two instances of the compiler
 * running in parallel).
 */
void resources::reset()
{
    // internal defaults
    //
    set_scripts("as2js/scripts:/usr/lib/as2js/scripts");
    set_db("/tmp/as2js_packages.db");
    set_temporary_variable_name("@temp");
}


/** \brief Find the resource file.
 *
 * This function tries to find a resource file.
 *
 * The resource file defines two paths where we can find the system
 * definitions and user imports.
 *
 * \param[in] accept_if_missing  Whether an error is generated (false)
 *                               if the file cannot be found.
 */
void resources::init(bool const accept_if_missing)
{
    reset();

    // first try to find a place with a .rc file
    //
    input_stream<std::ifstream>::pointer_t in(std::make_shared<input_stream<std::ifstream>>());
    std::string rcfilename;
    for(char const * const * dir = g_rc_directories; *dir != nullptr; ++dir)
    {
        std::stringstream buffer;
        if(**dir == '$')
        {
            char const * const env(getenv(*dir + 1));
            if(env == nullptr)
            {
                continue;
            }
            buffer << env << "/as2js.rc";
        }
        else if(**dir == '~' && (*dir)[1] == '/')
        {
            std::string const home(get_home());
            if(home.empty())
            {
                // no valid $HOME variable
                //
                continue;  // LCOV_EXCL_LINE   (this is tested specifically, but the normal coverage doesn't catch this line)
            }
            buffer << home << "/" << (*dir + 2) << "/as2js.rc";
        }
        else
        {
            buffer << *dir << "/as2js.rc";
        }
        rcfilename = buffer.str();
        if(!rcfilename.empty())
        {
            in->open(rcfilename);
            if(in->is_open())
            {
                // it worked, we are done
                //
                in->get_position().set_filename(rcfilename);
                break;
            }
            rcfilename.clear();
        }
    }

    if(rcfilename.empty())
    {
        if(!accept_if_missing)
        {
            // no position in this case...
            //
            message msg(
                  message_level_t::MESSAGE_LEVEL_FATAL
                , err_code_t::AS_ERR_INSTALLATION);
            msg << "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            throw as2js_exit(msg.str(), 1);
        }

        // nothing to load in this case...
    }
    else
    {
        json::pointer_t json(std::make_shared<json>());
        json::json_value::pointer_t root(json->parse(in));
        json::json_value::type_t type(root->get_type());
        // null is accepted, in which case we keep the defaults
        if(type != json::json_value::type_t::JSON_TYPE_NULL)
        {
            if(type != json::json_value::type_t::JSON_TYPE_OBJECT)
            {
                message msg(
                      message_level_t::MESSAGE_LEVEL_FATAL
                    , err_code_t::AS_ERR_UNEXPECTED_RC
                    , root->get_position());
                msg << rcfilename
                    << ": a resource file (.rc) must be defined as a JSON object, or set to \"null\".";
                throw as2js_exit(msg.str(), 1);
            }

            json::json_value::object_t const & obj(root->get_object());
            for(json::json_value::object_t::const_iterator it(obj.begin());
                                                           it != obj.end();
                                                           ++it)
            {
                // the only type of values in the resource files are strings
                //
                json::json_value::type_t sub_type(it->second->get_type());
                if(sub_type != json::json_value::type_t::JSON_TYPE_STRING)
                {
                    message msg(
                          message_level_t::MESSAGE_LEVEL_FATAL
                        , err_code_t::AS_ERR_UNEXPECTED_RC
                        , it->second->get_position());
                    msg << "a resource file is expected to be an object of string elements.";
                    throw as2js_exit(msg.str(), 1);
                }

                std::string const & parameter_name(it->first);
                std::string const & parameter_value(it->second->get_string());

                if(parameter_name == "scripts")
                {
                    set_scripts(parameter_value);
                }
                else if(parameter_name == "db")
                {
                    set_db(parameter_value);
                }
                else if(parameter_name == "temporary_variable_name")
                {
                    set_temporary_variable_name(parameter_value);
                }
                // else -- warn on unknown parameters?
            }
        }
    }
}


resources::script_paths_t const & resources::get_scripts() const
{
    return f_scripts;
}


void resources::set_scripts(std::string const & scripts, bool warning_about_invalid)
{
    script_paths_t paths;
    snapdev::tokenize_string(
          paths
        , scripts
        , ":"
        , true);

    f_scripts.clear();
    for(auto s : paths)
    {
        std::string e;
        std::string const canonicalized(snapdev::pathinfo::realpath(s, e));
        if(!e.empty())
        {
            if(warning_about_invalid)
            {
                message msg(
                      message_level_t::MESSAGE_LEVEL_WARNING
                    , err_code_t::AS_ERR_INSTALLATION);
                msg << "scripts path \""
                    << s
                    << "\" is not accessible ("
                    << e
                    << ").";
            }
        }
        else
        {
            f_scripts.push_back(canonicalized.empty() ? "." : canonicalized);
        }
    }

    // TODO: determine whether this can be done or not...
    //       right now it prevents our json-to-string command from working
    //
    //if(f_scripts.empty())
    //{
    //    message msg(
    //          message_level_t::MESSAGE_LEVEL_FATAL
    //        , err_code_t::AS_ERR_INSTALLATION);
    //    msg << "the list of paths to imported scripts cannot be empty.";
    //    throw as2js_exit(msg.str(), 1);
    //}

    // this is a great optimization since that way we avoid looking at the
    // same folder more than once
    //
    snapdev::unsorted_remove_duplicates(f_scripts);
}


std::string const & resources::get_db() const
{
    return f_db;
}


void resources::set_db(std::string const & db)
{
    if(db.empty())
    {
        message msg(
              message_level_t::MESSAGE_LEVEL_FATAL
            , err_code_t::AS_ERR_INSTALLATION);
        msg << "db path cannot be empty.";
        throw as2js_exit(msg.str(), 1);
    }

    f_db = db;
}


std::string const & resources::get_temporary_variable_name() const
{
    return f_temporary_variable_name;
}


void resources::set_temporary_variable_name(std::string const & name)
{
    if(name.empty())
    {
        message msg(
              message_level_t::MESSAGE_LEVEL_FATAL
            , err_code_t::AS_ERR_INSTALLATION);
        msg << "temporary variable name cannot be empty.";
        throw as2js_exit(msg.str(), 1);
    }

    f_temporary_variable_name = name;
}


std::string const & resources::get_home()
{
    if(!g_home_initialized)
    {
        g_home_initialized = true;
        char const * home(getenv("HOME"));
        if(home != nullptr)
        {
            g_home = home;
        }
    }

    return g_home;
}



} // namespace as2js
// vim: ts=4 sw=4 et
