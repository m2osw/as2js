#ifndef AS2JS_JSON_H
#define AS2JS_JSON_H
/* include/as2js/json.h

Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved

https://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "as2js/lexer.h"


namespace as2js
{

// A JSON object is a JavaScript object with field names and
// values organized in a tree of Node objects. Names may be
// strings or numbers. Values can be any type of literal
// including a node representing another list of objects.
//
// After reading a JSON object, the resulting tree is as
// optimized as possible. This means it is likely to just
// be "field name": "literal value". However, by default
// we authorize values to include complex unresolved
// expressions (non-static).
//
// The JSON class defined below allows you to gather data
// from the resulting object painlessly using a chain of
// names such as:
//
// "rc.path"
//
// To retrieve the path to the resource files from the
// global resource JSON data.


class JSON
{
public:
    typedef std::shared_ptr<JSON>       pointer_t;

    class JSONValue
    {
    public:
        typedef std::shared_ptr<JSONValue>              pointer_t;
        typedef std::vector<JSONValue::pointer_t>       array_t;
        typedef std::map<String, JSONValue::pointer_t>  object_t;

        enum class type_t
        {
            JSON_TYPE_UNKNOWN,

            JSON_TYPE_ARRAY,
            JSON_TYPE_FALSE,
            JSON_TYPE_FLOAT64,
            JSON_TYPE_INT64,
            JSON_TYPE_NULL,
            JSON_TYPE_OBJECT,
            JSON_TYPE_STRING,
            JSON_TYPE_TRUE
        };

                            JSONValue(Position const & position);  // null
                            JSONValue(Position const & position, Int64 integer);
                            JSONValue(Position const & position, Float64 floating_point);
                            JSONValue(Position const & position, String const & string);
                            JSONValue(Position const & position, bool boolean);
                            JSONValue(Position const & position, array_t const & array);
                            JSONValue(Position const & position, object_t const & object);

        type_t              get_type() const;

        Int64               get_int64() const;
        Float64             get_float64() const;
        String const &      get_string() const;
        array_t const &     get_array() const;
        void                set_item(std::size_t idx, JSONValue::pointer_t value);
        object_t const &    get_object() const;
        void                set_member(String const & name, JSONValue::pointer_t value);

        Position const&     get_position() const;

        String              to_string() const;

    private:
        class saving_t
        {
        public:
            saving_t(JSONValue const& value);
            ~saving_t();

        private:
            JSONValue &         f_value;
        };
        friend class saving_t;

        type_t const            f_type;  // no need for a default since it is a const it has to be initialized in all constructors
        Position                f_position = Position();
        bool                    f_saving = false;

        Int64                   f_integer = Int64();
        Float64                 f_float = Float64();
        String                  f_string = String();
        array_t                 f_array = array_t();
        object_t                f_object = object_t();
    };

    class JSONValueRef
    {
    public:
        static constexpr ssize_t MAX_ITEMS_AT_ONCE = 1'000;

                                JSONValueRef(
                                          JSONValue::pointer_t parent
                                        , String const & name);

                                JSONValueRef(
                                          JSONValue::pointer_t parent
                                        , ssize_t index);

        JSONValueRef &          operator = (JSONValueRef const & ref);

        JSONValueRef &          operator = (std::nullptr_t);
        JSONValueRef &          operator = (Int64 integer);
        JSONValueRef &          operator = (Float64 floating_point);
        JSONValueRef &          operator = (String const & string);
        JSONValueRef &          operator = (bool boolean);
        JSONValueRef &          operator = (JSONValue::array_t const & array);
        JSONValueRef &          operator = (JSONValue::object_t const & object);
        JSONValueRef &          operator = (JSONValue::pointer_t const & value);

        template<typename T>
        typename std::enable_if<std::is_integral<T>::value, JSONValueRef &>::type
                                operator = (T integer) { return operator = (Int64(integer)); }

        template<typename T>
        typename std::enable_if<std::is_floating_point<T>::value, JSONValueRef &>::type
                                operator = (T floating_point) { return operator = (Float64(floating_point)); }

        JSONValueRef            operator [] (char const * name);
        JSONValueRef            operator [] (String const & name);
        JSONValueRef            operator [] (ssize_t idx);

                                operator Int64 () const;
                                operator Float64 () const;
                                operator String () const;
                                operator bool () const;
                                operator JSONValue::array_t const & () const;
                                operator JSONValue::object_t const & () const;
                                operator JSONValue::pointer_t const & () const;

    private:
        JSONValue::pointer_t    f_parent = JSONValue::pointer_t();
        String                  f_name = String();
        std::size_t             f_index = 0;
    };

    JSONValue::pointer_t    load(String const & filename);
    JSONValue::pointer_t    parse(Input::pointer_t in);
    bool                    save(String const & filename, String const & header) const;
    bool                    output(Output::pointer_t out, String const & header) const;

                            operator bool () const { return f_value.operator bool(); }

    void                    set_value(JSONValue::pointer_t value);
    JSONValue::pointer_t    get_value() const;

    JSONValueRef            operator [] (char const * name);
    JSONValueRef            operator [] (String const & name);
    JSONValueRef            operator [] (ssize_t idx);

private:
    JSONValue::pointer_t    read_json_value(Node::pointer_t n);

    Lexer::pointer_t        f_lexer = Lexer::pointer_t();
    JSONValue::pointer_t    f_value = JSONValue::pointer_t();
};



}
// namespace as2js
#endif
// #ifndef AS2JS_JSON_H

// vim: ts=4 sw=4 et
