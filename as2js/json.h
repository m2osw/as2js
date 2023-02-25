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
#pragma once

// self
//
#include    <as2js/lexer.h>



namespace as2js
{

// A json object is a JavaScript object with field names and
// values organized in a tree of Node objects. Names may be
// strings or numbers. Values can be any type of literal
// including a node representing another list of objects.
//
// After reading a json object, the resulting tree is as
// optimized as possible. This means it is likely to just
// be "field name": "literal value". However, by default
// we authorize values to include complex unresolved
// expressions (non-static).
//
// The json class defined below allows you to gather data
// from the resulting object painlessly using a chain of
// names such as:
//
// "rc.path"
//
// To retrieve the path to the resource files from the
// global resource JSON data.


class json
{
public:
    typedef std::shared_ptr<json>       pointer_t;

    class json_value
    {
    public:
        typedef std::shared_ptr<json_value>                     pointer_t;
        typedef std::vector<json_value::pointer_t>              array_t;
        typedef std::map<std::string, json_value::pointer_t>    object_t;

        enum class type_t
        {
            JSON_TYPE_UNKNOWN,

            JSON_TYPE_ARRAY,
            JSON_TYPE_FALSE,
            JSON_TYPE_FLOATING_POINT,
            JSON_TYPE_INTEGER,
            JSON_TYPE_NULL,
            JSON_TYPE_OBJECT,
            JSON_TYPE_STRING,
            JSON_TYPE_TRUE
        };

                            json_value(position const & position);  // null
                            json_value(position const & position, integer i);
                            json_value(position const & position, floating_point f);
                            json_value(position const & position, std::string const & s);
                            json_value(position const & position, bool boolean);
                            json_value(position const & position, array_t const & array);
                            json_value(position const & position, object_t const & object);

        type_t              get_type() const;

        integer             get_integer() const;
        floating_point      get_floating_point() const;
        std::string const & get_string() const;
        array_t const &     get_array() const;
        void                set_item(std::size_t idx, json_value::pointer_t value);
        object_t const &    get_object() const;
        void                set_member(std::string const & name, json_value::pointer_t value);

        position const &    get_position() const;

        std::string         to_string() const;

    private:
        class saving_t
        {
        public:
            saving_t(json_value const& value);
            ~saving_t();

        private:
            json_value &         f_value;
        };
        friend class saving_t;

        type_t const            f_type;  // no need for a default since it is a const it has to be initialized in all constructors
        position                f_position = position();
        bool                    f_saving = false;

        integer                 f_integer = integer();
        floating_point          f_float = floating_point();
        std::string             f_string = std::string();
        array_t                 f_array = array_t();
        object_t                f_object = object_t();
    };

    class json_value_ref
    {
    public:
        static constexpr ssize_t MAX_ITEMS_AT_ONCE = 1'000;

                                json_value_ref(
                                          json_value::pointer_t parent
                                        , std::string const & name);

                                json_value_ref(
                                          json_value::pointer_t parent
                                        , ssize_t index);

                                json_value_ref(json_value_ref const & ref);

        json_value_ref &        operator = (json_value_ref const & ref);

        json_value_ref &        operator = (std::nullptr_t);
        json_value_ref &        operator = (integer i);
        json_value_ref &        operator = (floating_point f);
        json_value_ref &        operator = (char const * s);
        json_value_ref &        operator = (std::string const & s);
        json_value_ref &        operator = (bool boolean);
        json_value_ref &        operator = (json_value::array_t const & array);
        json_value_ref &        operator = (json_value::object_t const & object);
        json_value_ref &        operator = (json_value::pointer_t const & value);

        template<typename T>
        typename std::enable_if<std::is_integral<T>::value, json_value_ref &>::type
                                operator = (T i) { return operator = (integer(i)); }

        template<typename T>
        typename std::enable_if<std::is_floating_point<T>::value, json_value_ref &>::type
                                operator = (T f) { return operator = (floating_point(f)); }

        json_value_ref          operator [] (char const * name);
        json_value_ref          operator [] (std::string const & name);
        json_value_ref          operator [] (ssize_t idx);

                                operator integer () const;
                                operator floating_point () const;
                                operator std::string () const;
                                operator bool () const;
                                operator json_value::array_t const & () const;
                                operator json_value::object_t const & () const;
        json_value::pointer_t   parent() const;

    private:
        json_value::pointer_t   f_parent = json_value::pointer_t();
        std::string             f_name = std::string();
        std::size_t             f_index = 0;
    };

    json_value::pointer_t   load(std::string const & filename);
    json_value::pointer_t   parse(base_stream::pointer_t in);
    bool                    save(std::string const & filename, std::string const & header) const;
    bool                    output(base_stream::pointer_t out, std::string const & header) const;

                            operator bool () const { return f_value.operator bool(); }

    void                    set_value(json_value::pointer_t value);
    json_value::pointer_t   get_value() const;

    json_value_ref          operator [] (char const * name);
    json_value_ref          operator [] (std::string const & name);
    json_value_ref          operator [] (ssize_t idx);

private:
    json_value::pointer_t   read_json_value(node::pointer_t n);

    lexer::pointer_t        f_lexer = lexer::pointer_t();
    json_value::pointer_t   f_value = json_value::pointer_t();
};


std::ostream & operator << (std::ostream & out, json const & js);


inline std::ostream & operator << (std::ostream & out, json::json_value const & value)
{
    return out << value.to_string();
}



} // namespace as2js
// vim: ts=4 sw=4 et
