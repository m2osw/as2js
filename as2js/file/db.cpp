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
#include    "db.h"  // 100% private header

#include    "as2js/exception.h"
#include    "as2js/message.h"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


namespace
{


// TODO: make this function public, after all, it could be useful somewhere
//       else
//
bool do_match(char const * name, char const * pattern)
{
    for(; *pattern != '\0'; ++pattern, ++name)
    {
        if(*pattern == '*')
        {
            // quick optimization, remove all the '*' if there are
            // multiple (although that should probably be an error!)
            do
            {
                ++pattern;
            }
            while(*pattern == '*');
            if(*pattern == '\0')
            {
                return true;
            }
            while(*name != '\0')
            {
                if(do_match(name, pattern)) // recursive call
                {
                    return true;
                }
                ++name;
            }
            return false;
        }
        if(*name != *pattern)
        {
            return false;
        }
    }

    // end of name and pattern must match if you did not
    // end the pattern with an asterisk
    //
    return *name == '\0';
}


}
// no name namespace



database::element::element(
          std::string const & element_name
        , json::json_value::pointer_t e)
    : f_element_name(element_name)
    , f_element(e)
{
    // verify the type, but we already tested before creating this object
    //
    json::json_value::type_t type(f_element->get_type());
    if(type != json::json_value::type_t::JSON_TYPE_OBJECT)
    {
        throw internal_error("an element cannot be created with a json value which has a type other than Object");
    }

    // we got a valid database element object
    //
    json::json_value::object_t const& obj(f_element->get_object());
    for(json::json_value::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        json::json_value::type_t const sub_type(it->second->get_type());
        std::string const field_name(it->first);
        if(field_name == "type")
        {
            if(sub_type != json::json_value::type_t::JSON_TYPE_STRING)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The type of an element in the database has to be a string.";
            }
            else
            {
                f_type = it->second->get_string();
            }
        }
        else if(field_name == "filename")
        {
            if(sub_type != json::json_value::type_t::JSON_TYPE_STRING)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The filename of an element in the database has to be a string.";
            }
            else
            {
                f_filename = it->second->get_string();
            }
        }
        else if(field_name == "line")
        {
            if(sub_type != json::json_value::type_t::JSON_TYPE_INTEGER)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The line of an element in the database has to be an integer.";
            }
            else
            {
                f_line = static_cast<position::counter_t>(it->second->get_integer().get());
            }
        }
        // else -- TBD: should we err on unknown fields?
    }
}


void database::element::set_type(std::string const & type)
{
    f_type = type;
    f_element->set_member("type", std::make_shared<json::json_value>(f_element->get_position(), f_type));
}


void database::element::set_filename(std::string const & filename)
{
    f_filename = filename;
    f_element->set_member("filename", std::make_shared<json::json_value>(f_element->get_position(), f_filename));
}


void database::element::set_line(position::counter_t line)
{
    f_line = line;
    integer i(f_line);
    f_element->set_member("line", std::make_shared<json::json_value>(f_element->get_position(), i));
}


std::string database::element::get_element_name() const
{
    return f_element_name;
}


std::string database::element::get_type() const
{
    return f_type;
}


std::string database::element::get_filename() const
{
    return f_filename;
}


position::counter_t database::element::get_line() const
{
    return f_line;
}







database::package::package(
          std::string const & package_name
        , json::json_value::pointer_t p)
    : f_package_name(package_name)
    , f_package(p)
{
    // verify the type, but we already tested before creatin this object
    json::json_value::type_t type(f_package->get_type());
    if(type != json::json_value::type_t::JSON_TYPE_OBJECT)
    {
        throw internal_error("a package cannot be created with a json value which has a type other than Object");
    }

    // we got a valid database package object
    json::json_value::object_t const& obj(f_package->get_object());
    for(json::json_value::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        // the only type of value that we expect are objects within the
        // main object; each one represents a package
        json::json_value::type_t const sub_type(it->second->get_type());
        if(sub_type != json::json_value::type_t::JSON_TYPE_OBJECT)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
            msg << "A database is expected to be an object of object packages composed of object elements.";
        }
        else
        {
            std::string element_name(it->first);
            element::pointer_t e(new element(element_name, it->second));
            f_elements[element_name] = e;
        }
    }
}


std::string database::package::get_package_name() const
{
    return f_package_name;
}


database::element::vector_t database::package::find_elements(std::string const & pattern) const
{
    element::vector_t found;
    for(auto it(f_elements.begin()); it != f_elements.end(); ++it)
    {
        if(match_pattern(it->first, pattern))
        {
            found.push_back(it->second);
        }
    }
    return found;
}


database::element::pointer_t database::package::get_element(std::string const & element_name) const
{
    auto it(f_elements.find(element_name));
    if(it == f_elements.end())
    {
        return element::pointer_t();
    }
    // it exists
    return it->second;
}


database::element::pointer_t database::package::add_element(std::string const & element_name)
{
    auto e(get_element(element_name));
    if(!e)
    {
        // some default position object to attach to the new objects
        position pos(f_package->get_position());

        json::json_value::object_t obj_element;
        json::json_value::pointer_t new_element(new json::json_value(pos, obj_element));
        e.reset(new element(element_name, new_element));
        f_elements[element_name] = e;

        f_package->set_member(element_name, new_element);
    }
    return e;
}






bool database::load(std::string const& filename)
{
    if(f_json)
    {
        // already loaded
        //
        return f_value.operator bool ();
    }
    f_filename = filename;
    f_json = std::make_shared<json>();

    // test whether the file exists
    //
    input_stream<std::ifstream>::pointer_t in(std::make_shared<input_stream<std::ifstream>>());
    in->open(filename);
    if(!in->is_open())
    {
        // no db yet... it is okay
        //
        position pos;
        pos.set_filename(filename);
        json::json_value::object_t obj_database;
        f_value = std::make_shared<json::json_value>(pos, obj_database);
        f_json->set_value(f_value);
        return true;
    }

    // there is a db, load it
    //
    f_value = f_json->parse(in);
    if(f_value == nullptr)
    {
        return false;
    }

    json::json_value::type_t type(f_value->get_type());

    // a 'null' is acceptable, it means the database is currently empty
    if(type == json::json_value::type_t::JSON_TYPE_NULL)
    {
        return true;
    }

    if(type != json::json_value::type_t::JSON_TYPE_OBJECT)
    {
        position pos;
        pos.set_filename(filename);
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, pos);
        msg << "A database must be defined as a json object, or set to 'null'.";
        return false;
    }

    // we found the database object
    // typedef std::map<string, json_value::pointer_t> object_t;
    json::json_value::object_t const& obj(f_value->get_object());
    for(json::json_value::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        // the only type of value that we expect are objects within the
        // main object; each one represents a package
        json::json_value::type_t sub_type(it->second->get_type());
        if(sub_type != json::json_value::type_t::JSON_TYPE_OBJECT)
        {
            position pos;
            pos.set_filename(filename);
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, pos);
            msg << "A database is expected to be an object of object packages composed of elements.";
            return false;
        }

        std::string package_name(it->first);
        package::pointer_t p(std::make_shared<package>(package_name, it->second));
        f_packages[package_name] = p;
    }

    return true;
}



void database::save() const
{
    // if it has been loaded, save it
    if(f_json)
    {
        std::string const header("// database used by the AS2JS Compiler (as2js)\n"
                            "//\n"
                            "// DO NOT EDIT UNLESS YOU KNOW WHAT YOU ARE DOING\n"
                            "// If you have a problem because of the database, just delete the file\n"
                            "// and the compiler will re-generate it.\n"
                            "//\n"
                            "// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved.\n"
                            "// This file is written in UTF-8\n"
                            "// You can safely modify it with an editor supporting UTF-8\n"
                            "// The format is json:\n"
                            "//\n"
                            "// {\n"
                            "//   \"package_name\": {\n"
                            "//     \"element_name\": {\n"
                            "//       \"filename\": \"<full path filename>\",\n"
                            "//       \"line\": <line number>,\n"
                            "//       \"type\": \"<type name>\"\n"
                            "//     },\n"
                            "//     <...other elements...>\n"
                            "//   },\n"
                            "//   <...other packages...>\n"
                            "// }\n"
                            "//");
        f_json->save(f_filename, header);
    }
}


database::package::vector_t database::find_packages(std::string const & pattern) const
{
    package::vector_t found;
    for(auto it(f_packages.begin()); it != f_packages.end(); ++it)
    {
        if(match_pattern(it->first, pattern))
        {
            found.push_back(it->second);
        }
    }
    return found;
}


database::package::pointer_t database::get_package(std::string const & package_name) const
{
    auto p(f_packages.find(package_name));
    if(p == f_packages.end())
    {
        return database::package::pointer_t();
    }
    return p->second;
}


database::package::pointer_t database::add_package(std::string const & package_name)
{
    auto p(get_package(package_name));
    if(!p)
    {
        if(!f_json)
        {
            throw internal_error("attempting to add a package to the database before the database was loaded");
        }

        // some default position object to attach to the new objects
        //
        position pos;
        pos.set_filename(f_filename);

        // create the database object if not there yet
        //
        if(!f_value)
        {
            json::json_value::object_t obj_database;
            f_value = std::make_shared<json::json_value>(pos, obj_database);
            f_json->set_value(f_value);
        }

        json::json_value::object_t obj_package;
        json::json_value::pointer_t new_package(std::make_shared<json::json_value>(pos, obj_package));
        p = std::make_shared<package>(package_name, new_package);
        f_packages[package_name] = p;

        f_value->set_member(package_name, new_package);
    }
    return p;
}


bool database::match_pattern(
      std::string const & name
    , std::string const & pattern)
{
    // we want to use a recursive function and use bare pointers
    // and it really simplifies the algorithm...
    //
    return do_match(name.c_str(), pattern.c_str());
}



} // namespace as2js
// vim: ts=4 sw=4 et
