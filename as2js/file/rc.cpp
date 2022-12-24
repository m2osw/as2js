// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "rc.h"

#include    "as2js/exception.h"
#include    "as2js/json.h"
#include    "as2js/message.h"


// C++
//
#include    <cstring>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{

namespace
{

char const * g_rc_directories[] =
{
    // check user defined variable
    "$AS2JS_RC",
    // try locally first (assuming you are a heavy JS developer, you'd
    // probably start with your local files)
    "as2js",
    // try your user "global" installation directory
    "~/.config/as2js",
    // try the system directory
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
rc_t::rc_t()
{
    reset();
}


/** \brief Reset the resources to internal defaults.
 *
 * This function resets all the rc_t variables to internal defaults:
 *
 * \li scripts -- "as2js/scripts"
 * \li db -- "/tmp/as2js_packages.db"
 * \li temporary_variable_name -- "@temp"
 *
 * This function is called on construction and when calling init_rc().
 *
 * Note that does not reset the home parameter which has no internal
 * default and is managed differently.
 */
void rc_t::reset()
{
    // internal defaults
    f_scripts = "as2js/scripts";
    f_db = "/tmp/as2js_packages.db";
    f_temporary_variable_name = "@temp";
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
void rc_t::init_rc(bool const accept_if_missing)
{
    reset();

    // first try to find a place with a .rc file
    //
    input_stream<std::ifstream>::pointer_t in(std::make_shared<input_stream<std::ifstream>>());
    std::string rcfilename;
    for(char const ** dir = g_rc_directories; *dir != nullptr; ++dir)
    {
        std::stringstream buffer;
        if(**dir == '$')
        {
            std::string env_defined(getenv(*dir + 1));
            if(env_defined.empty())
            {
                continue;
            }
            buffer << env_defined << "/as2js.rc";
        }
        else if(**dir == '~' && (*dir)[1] == '/')
        {
            std::string home(get_home());
            if(home.empty())
            {
                // no valid $HOME variable
                continue;
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
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INSTALLATION);
            msg << "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
            throw as2js_exit("cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc", 1);
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
                message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_UNEXPECTED_RC, root->get_position());
                msg << "A resource file (.rc) must be defined as a JSON object, or set to 'null'.";
                throw as2js_exit("A resource file (.rc) must be defined as a JSON object, or set to 'null'.", 1);
            }

            json::json_value::object_t const& obj(root->get_object());
            for(json::json_value::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
            {
                // the only type of values in the resource files are strings
                //
                json::json_value::type_t sub_type(it->second->get_type());
                if(sub_type != json::json_value::type_t::JSON_TYPE_STRING)
                {
                    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_UNEXPECTED_RC, it->second->get_position());
                    msg << "A resource file is expected to be an object of string elements.";
                    throw as2js_exit("A resource file is expected to be an object of string elements.", 1);
                }

                std::string const parameter_name(it->first);
                std::string const parameter_value(it->second->get_string());

                if(parameter_name == "scripts")
                {
                    f_scripts = parameter_value;
                }
                else if(parameter_name == "db")
                {
                    f_db = parameter_value;
                }
                else if(parameter_name == "temporary_variable_name")
                {
                    f_temporary_variable_name = parameter_value;
                }
            }
        }
    }
}


std::string const & rc_t::get_scripts() const
{
    return f_scripts;
}


std::string const & rc_t::get_db() const
{
    return f_db;
}


std::string const & rc_t::get_temporary_variable_name() const
{
    return f_temporary_variable_name;
}


std::string const & rc_t::get_home()
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
