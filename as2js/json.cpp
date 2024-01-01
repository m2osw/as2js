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

// self
//
#include    "as2js/json.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"
#include    "as2js/stream.h"


// C++
//
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>




/** \file
 * \brief Implementation of the json reader and writer.
 *
 * This file includes the implementation of the as2js::json class.
 *
 * The parser makes use of the lexer and an input stream.
 *
 * The writer makes use of an output stream.
 *
 * Note that our json parser supports the following extensions that
 * are NOT part of a valid json file:
 *
 * \li C-like comments using the standard slash (/) asterisk (*) to
 *     start the comment and asterisk (*) slash (/) to end it.
 * \li C++-like comments using the standard double slash (/) and ending
 *     the line with a newline character.
 * \li The NaN special value.
 * \li The +Infinity value.
 * \li The -Infinity value.
 * \li The +<number> value.
 * \li Decimal numbers are read as decimal numbers and not floating point
 *     numbers. We support full 64 bit integers.
 * \li Strings using single quote (') characters.
 * \li Strings can include \U######## characters (large Unicode, 8 digits.)
 *
 * Note that all comments are discarded while reading a json file.
 *
 * The writer, however, generates:
 *
 * \li Strings using double quotes (").
 * \li Only uses the small unicode \u#### encoding. Large Unicode characters
 *     are output as is (in the format used by your output stream.)
 * \li Does not output any comments (although you may include a comment in
 *     the header parameter.)
 *
 * However, it will:
 *
 * \li Generate integers that are 64 bit.
 * \li Output NaN for undefined numbers.
 * \li Output Infinity and -Infinity for number representing infinity.
 *
 * We may later introduce a flag to allow / disallow these values.
 */


namespace as2js
{

/** \brief Private implementation functions.
 *
 * Our json implementation makes use of functions that are defined in
 * this unnamed namespace.
 */
namespace
{

/** \brief Append a raw string to a stringified string.
 *
 * This function appends a string (str) to a stringified
 * string (result). In the process, it adds quotes to the
 * resulting string.
 *
 * \param[in,out] result  Where the output string is appended with a
 *                        valid string for a json file.
 * \param[in] str  The raw input string which needs to be stringified.
 */
void append_string(std::string & result, std::string const & str)
{
    result += '"';
    std::size_t const max_chars(str.length());
    for(std::size_t idx(0); idx < max_chars; ++idx)
    {
        switch(str[idx])
        {
        case '\b':
            result += '\\';
            result += 'b';
            break;

        case '\f':
            result += '\\';
            result += 'f';
            break;

        case '\n':
            result += '\\';
            result += 'n';
            break;

        case '\r':
            result += '\\';
            result += 'r';
            break;

        case '\t':
            result += '\\';
            result += 't';
            break;

        case '\\':
            result += '\\';
            result += '\\';
            break;

        case '"':
            result += '\\';
            result += '"';
            break;

        // Escaping a single quote (') is not valid json
        //case '\'':
        //    result += '\\';
        //    result += '\'';
        //    break;

        default:
            if(static_cast<std::uint8_t>(str[idx]) < 0x0020 || str[idx] == 0x007F)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(static_cast<std::uint8_t>(str[idx]));
                result += ss.str();
            }
            else
            {
                result += str[idx];
            }
            break;

        }
    }
    result += '"';
}

}
// no name namespace


/** \brief Initialize a json_value saving_t object.
 *
 * This function initializes a json_value saving_t object attached to
 * the specified json_value \p value.
 *
 * While saving we cannot know whether the json is currently cyclical
 * or not. We use this saving_t object to mark all the objects being
 * saved with a flag. If the flag is already set, this constructor
 * fails with an exception.
 *
 * To avoid cyclical json trees, make sure to always allocate any
 * new value that you add to your tree.
 *
 * \exception cyclical_structure
 * This exception is raised if the json_value about to be saved is
 * already marked as being saved, meaning that a child of json_value
 * points back to this json_value.
 *
 * \param[in] value  The value being marked as being saved.
 */
json::json_value::saving_t::saving_t(json_value const& value)
    : f_value(const_cast<json_value&>(value))
{
    if(f_value.f_saving)
    {
        throw cyclical_structure("JSON cannot stringify a set of objects and arrays which are cyclical.");
    }
    f_value.f_saving = true;
}


/** \brief Destroy a json_value saving_t object.
 *
 * The destructor of a json_value marks the attached json_value object
 * as saved. In other words, it allows it to be saved again.
 *
 * It is used to make sure that the same json tree can be saved
 * multiple times.
 *
 * Note that since this happens once the value is saved, if it
 * appears multiple times in the tree, but is not cyclical, the
 * save will work.
 */
json::json_value::saving_t::~saving_t()
{
    f_value.f_saving = false;
}




/** \brief Initialize a json_value object.
 *
 * The NULL constructor only accepts a position and it marks this json
 * value as a NULL value.
 *
 * The type of this json_value will be set to JSON_TYPE_NULL.
 *
 * \param[in] position  The position where this json_value was read from.
 */
json::json_value::json_value(position const &position)
    : f_type(type_t::JSON_TYPE_NULL)
    , f_position(position)
{
}


/** \brief Initialize a json_value object.
 *
 * The integer constructor accepts a position and an integer. It creates
 * an integer json_value object.
 *
 * The value cannot be modified, however, it can be retrieved using the
 * get_integer() function.
 *
 * The type of this json_value will be set to JSON_TYPE_INTEGER.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] integer  The integer to save in this json_value.
 */
json::json_value::json_value(position const &position, integer i)
    : f_type(type_t::JSON_TYPE_INTEGER)
    , f_position(position)
    , f_integer(i)
{
}


/** \brief Initialize a json_value object.
 *
 * The floating point constructor accepts a position and a floating point
 * number.
 *
 * The value cannot be modified, however, it can be retrieved using
 * the get_floating_point() function.
 *
 * The type of this json will be JSON_TYPE_FLOATING_POINT.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] floating_point  The floating point to save in the json_value.
 */
json::json_value::json_value(position const &position, floating_point f)
    : f_type(type_t::JSON_TYPE_FLOATING_POINT)
    , f_position(position)
    , f_float(f)
{
}


/** \brief Initialize a json_value object.
 *
 * The string constructor accepts a position and a string parameter.
 *
 * The value cannot be modified, however, it can be retrieved using
 * the get_string() function.
 *
 * The type of this json_value will be set to JSON_TYPE_STRING.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] s  The string to save in this json_value object.
 */
json::json_value::json_value(position const &position, std::string const & s)
    : f_type(type_t::JSON_TYPE_STRING)
    , f_position(position)
    , f_string(s)
{
}


/** \brief Initialize a json_value object.
 *
 * The Boolean constructor accepts a position and a Boolean value: true
 * or false.
 *
 * The value cannot be modified, however, it can be tested using
 * the get_type() function and check the type of object.
 *
 * The type of this json_value will be set to JSON_TYPE_TRUE when the
 * \p boolean parameter is true, and to JSON_TYPE_FALSE when the
 * \p boolean parameter is false.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] boolean  The boolean value to save in this json_value object.
 */
json::json_value::json_value(position const &position, bool boolean)
    : f_type(boolean ? type_t::JSON_TYPE_TRUE : type_t::JSON_TYPE_FALSE)
    , f_position(position)
{
}


/** \brief Initialize a json_value object.
 *
 * The array constructor accepts a position and an array as parameters.
 *
 * The array in this json_value can be modified using the set_item()
 * function. Also, it can be retrieved using the get_array() function.
 *
 * The type of this json_value will be set to JSON_TYPE_ARRAY.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] array  The array value to save in this json_value object.
 */
json::json_value::json_value(position const &position, array_t const& array)
    : f_type(type_t::JSON_TYPE_ARRAY)
    , f_position(position)
    , f_array(array)
{
}


/** \brief Initialize a json_value object.
 *
 * The object constructor accepts a position and an object.
 *
 * The object in this json_value can be modified using the set_member()
 * function. Also, it can be retrieved using the get_object() function.
 *
 * The type of this json_value will be set to JSON_TYPE_OBJECT.
 *
 * \param[in] position  The position where this json_value was read from.
 * \param[in] object  The object value to save in this json_value object.
 */
json::json_value::json_value(position const &position, object_t const& object)
    : f_type(type_t::JSON_TYPE_OBJECT)
    , f_position(position)
    , f_object(object)
{
}


/** \brief Retrieve the type of this json_value object.
 *
 * The type of a json_value cannot be modified. This value is read-only.
 *
 * The type determines what get_...() and what set_...() (if any)
 * functions can be called against this json_value object. If an invalid
 * function is called, then an exception is raised. To know which functions
 * are valid for this object, you need to check out its type.
 *
 * Note that the Boolean json_value objects do not have any getter or
 * setter functions. Their type defines their value: JSON_TYPE_TRUE and
 * JSON_TYPE_FALSE.
 *
 * The type is one of:
 *
 * \li JSON_TYPE_ARRAY
 * \li JSON_TYPE_FALSE
 * \li JSON_TYPE_FLOATING_POINT
 * \li JSON_TYPE_INTEGER
 * \li JSON_TYPE_NULL
 * \li JSON_TYPE_OBJECT
 * \li JSON_TYPE_STRING
 * \li JSON_TYPE_TRUE
 *
 * A json_value cannot have the special type JSON_TYPE_UNKNOWN.
 *
 * \return The type of this json_value.
 */
json::json_value::type_t json::json_value::get_type() const
{
    return f_type;
}


/** \brief Get the integer.
 *
 * This function is used to retrieve the integer from a JSON_TYPE_INTEGER
 * json_value.
 *
 * It is not possible to change the integer value directly. Instead you
 * have to create a new json_value with the new value and replace this
 * object with the new one.
 *
 * \exception internal_error
 * This exception is raised if the json_value object is not of type
 * JSON_TYPE_INTEGER.
 *
 * \return An integer object.
 */
integer json::json_value::get_integer() const
{
    if(f_type != type_t::JSON_TYPE_INTEGER)
    {
        throw internal_error("get_integer() called with a non-integer value type.");
    }
    return f_integer;
}


/** \brief Get the floating point.
 *
 * This function is used to retrieve the floating point from a
 * JSON_TYPE_FLOATING_POINT json_value.
 *
 * It is not possible to change the floating point value directly. Instead
 * you have to create a new json_value with the new value and replace this
 * object with the new one.
 *
 * \exception internal_error
 * This exception is raised if the json_value object is not of type
 * JSON_TYPE_FLOATING_POINT.
 *
 * \return A floating_point object.
 */
floating_point json::json_value::get_floating_point() const
{
    if(f_type != type_t::JSON_TYPE_FLOATING_POINT)
    {
        throw internal_error("get_floating_point() called with a non-floating point value type.");
    }
    return f_float;
}


/** \brief Get the string.
 *
 * This function lets you retrieve the string of a JSON_TYPE_STRING object.
 *
 * \exception internal_error
 * This exception is raised if the json_value object is not of type
 * JSON_TYPE_STRING.
 *
 * \return The string of this json_value object.
 */
std::string const & json::json_value::get_string() const
{
    if(f_type != type_t::JSON_TYPE_STRING)
    {
        throw internal_error("get_string() called with a non-string value type.");
    }
    return f_string;
}


/** \brief Get a reference to this json_value array.
 *
 * This function is used to retrieve a read-only reference to the array
 * of a JSON_TYPE_ARRAY json_value.
 *
 * You may change the array using the set_item() function. Note that if
 * you did not make a copy of the array returned by this function, you
 * will see the changes. It also means that iterators are likely not
 * going to work once a call to set_item() was made.
 *
 * \exception internal_error
 * This exception is raised if the json_value object is not of type
 * JSON_TYPE_ARRAY.
 *
 * \return A constant reference to a json_value array_t object.
 */
json::json_value::array_t const& json::json_value::get_array() const
{
    if(f_type != type_t::JSON_TYPE_ARRAY)
    {
        throw internal_error("get_array() called with a non-array value type.");
    }
    return f_array;
}


/** \brief Change the value of an array item.
 *
 * This function is used to change the value of an array item. The index
 * (\p idx) defines the position of the item to change. The \p value is
 * the new value to save at that position.
 *
 * Note that the pointer to the value cannot be set to NULL.
 *
 * The index (\p idx) can be any value between 0 and the current size of
 * the array. If idx is larger, then an exception is raised.
 *
 * When \p idx is set to the current size of the array, the \p value is
 * pushed at the end of the array (i.e. a new item is added to the existing
 * array.)
 *
 * \exception internal_error
 * If the json_value is not of type JSON_TYPE_ARRAY, then this function
 * raises this exception.
 *
 * \exception out_of_range
 * If idx is out of range (larger than the array size) then this exception
 * is raised. Note that idx is unsigned so it cannot be negative.
 *
 * \exception invalid_data
 * If the value pointer is a NULL pointer, then this exception is raised.
 *
 * \param[in] idx  The index where \p value is to be saved.
 * \param[in] value  The new value to be saved.
 */
void json::json_value::set_item(std::size_t idx, json_value::pointer_t value)
{
    if(f_type != type_t::JSON_TYPE_ARRAY)
    {
        throw internal_error("set_item() called with a non-array value type.");
    }
    if(idx > f_array.size())
    {
        throw out_of_range("json::json_value::set_item() called with an index out of range.");
    }
    if(value == nullptr)
    {
        throw invalid_data("json::json_value::set_item() called with a null pointer as the value.");
    }
    if(idx == f_array.size())
    {
        // append value
        f_array.push_back(value);
    }
    else
    {
        // replace previous value
        f_array[idx] = value;
    }
}


/** \brief Get a reference to this json_value object.
 *
 * This function is used to retrieve a read-only reference to the object
 * of a JSON_TYPE_OBJECT json_value.
 *
 * You may change the object using the set_member() function. Note that
 * if you did not make a copy of the object returned by this function,
 * you will see the changes. It also means that iterators are likely not
 * going to work once a call to set_member() was made.
 *
 * \exception internal_error
 * This exception is raised if the json_value object is not of type
 * JSON_TYPE_OBJECT.
 *
 * \return A constant reference to a json_value object_t object.
 */
json::json_value::object_t const & json::json_value::get_object() const
{
    if(f_type != type_t::JSON_TYPE_OBJECT)
    {
        throw internal_error("get_object() called with a non-object value type.");
    }
    return f_object;
}


/** \brief Change the value of an object member.
 *
 * This function is used to change the value of an object member. The
 * \p name defines the member to change. The \p value is the new value
 * to save along that name.
 *
 * The \p name can be any string except the empty string. If name is set
 * to the empty string, then an exception is raised.
 *
 * If a member with the same name already exists, it gets overwritten
 * with this new value. If the name is new, then the object is modified
 * which may affect your copy of the object, if you have one.
 *
 * In order to remove an object member, set it to a null pointer:
 *
 * \code
 *      set_member("clear_this", as2js::json::json_value::pointer_t());
 * \endcode
 *
 * \exception internal_error
 * If the json_value is not of type JSON_TYPE_OBJECT, then this function
 * raises this exception.
 *
 * \exception invalid_index
 * If name is the empty string then this exception is raised.
 *
 * \param[in] name  The name of the object field.
 * \param[in] value  The new value to be saved.
 */
void json::json_value::set_member(std::string const & name, json_value::pointer_t value)
{
    if(f_type != type_t::JSON_TYPE_OBJECT)
    {
        throw internal_error("set_member() called with a non-object value type.");
    }
    if(name.empty())
    {
        // TBD: is that really not allowed?
        throw invalid_index("json::json_value::set_member() called with an empty string as the member name.");
    }

    // this one is easy enough
    if(value != nullptr)
    {
        // add/replace
        f_object[name] = value;
    }
    else
    {
        // remove
        f_object.erase(name);
    }
}


/** \brief Get a constant reference to the json_value position.
 *
 * This function returns a constant reference to the json_value position.
 *
 * This position object is specific to this json_value so each one of
 * them can have a different position.
 *
 * The position of a json_value cannot be modified. When creating a
 * json_value, the position passed in as a parameter is copied in
 * the f_position of the json_value.
 *
 * \return The position of the json_value object in the source.
 */
position const& json::json_value::get_position() const
{
    return f_position;
}


/** \brief Get the json_value as a string.
 *
 * This function transforms a json_value object into a string.
 *
 * This is used to serialize the json_value and output it to a string.
 *
 * This function may raise an exception in the event the json_value is
 * cyclic, meaning that a child json_value points back at one of
 * its parent json_value's.
 *
 * \exception internal_error
 * This exception is raised if a json_value object is of type
 * JSON_TYPE_UNKNOWN, which should not happen.
 *
 * \return A string representing the json_value.
 */
std::string json::json_value::to_string() const
{
    std::string result;

    switch(f_type)
    {
    case type_t::JSON_TYPE_ARRAY:
        result += "[";
        if(f_array.size() > 0)
        {
            saving_t s(*this);
            result += f_array[0]->to_string(); // recursive
            size_t const max_elements(f_array.size());
            for(size_t i(1); i < max_elements; ++i)
            {
                result += ",";
                result += f_array[i]->to_string(); // recursive
            }
        }
        result += "]";
        break;

    case type_t::JSON_TYPE_FALSE:
        return "false";

    case type_t::JSON_TYPE_FLOATING_POINT:
        {
            floating_point f(f_float.get());
            if(f.is_nan())
            {
                return "NaN";
            }
            if(f.is_positive_infinity())
            {
                return "Infinity";
            }
            if(f.is_negative_infinity())
            {
                return "-Infinity";
            }

            // generate the floating point and remove unnecessary zeroes
            //
            std::string s(std::to_string(f_float.get()));
            if(s.find('.') != std::string::npos)
            {
                while(s.back() == '0')
                {
                    s.pop_back();
                }
                if(s.back() == '.')
                {
                    s.pop_back();
                }
            }
            return s;
        }

    case type_t::JSON_TYPE_INTEGER:
        return std::to_string(f_integer.get());

    case type_t::JSON_TYPE_NULL:
        return "null";

    case type_t::JSON_TYPE_OBJECT:
        result += "{";
        if(f_object.size() > 0)
        {
            saving_t s(*this);
            object_t::const_iterator obj(f_object.begin());
            append_string(result, obj->first);
            result += ":";
            result += obj->second->to_string(); // recursive
            for(++obj; obj != f_object.end(); ++obj)
            {
                result += ",";
                append_string(result, obj->first);
                result += ":";
                result += obj->second->to_string(); // recursive
            }
        }
        result += "}";
        break;

    case type_t::JSON_TYPE_STRING:
        append_string(result, f_string);
        break;

    case type_t::JSON_TYPE_TRUE:
        return "true";

    case type_t::JSON_TYPE_UNKNOWN:
        throw internal_error("json type \"Unknown\" is not valid and should never be used (it should not be possible to use it to create a json_value in the first place!)"); // LCOV_EXCL_LINE

    }

    return result;
}




json::json_value_ref::json_value_ref(json_value::pointer_t parent, std::string const & name)
    : f_parent(parent)
    , f_name(name)
{
    if(f_parent == nullptr)
    {
        throw internal_error("json_value_ref created with a nullptr.");
    }
    if(f_parent->get_type() != json_value::type_t::JSON_TYPE_OBJECT)
    {
        throw incompatible_type(
                  "json_value_ref expected an object with a named reference (instead of json_value with type "
                + std::to_string(static_cast<int>(f_parent->get_type()))
                + ").");
    }
    if(f_name.empty())
    {
        throw invalid_index("json::json_value_ref constructor called with an empty string as a member name.");
    }
}


json::json_value_ref::json_value_ref(json_value::pointer_t parent, ssize_t index)
    : f_parent(parent)
    , f_index(index)
{
    if(f_parent == nullptr)
    {
        throw internal_error("json_value_ref created with a nullptr.");
    }
    if(f_parent->get_type() != json_value::type_t::JSON_TYPE_ARRAY)
    {
        throw incompatible_type(
                  "json_value_ref expected an array with an indexed reference (instead of json_value with type "
                + std::to_string(static_cast<int>(f_parent->get_type()))
                + ").");
    }
    if(index == -1)
    {
        f_index = f_parent->get_array().size();
    }
    else if(index < 0)
    {
        throw incompatible_type("json_value_ref to an array must use an index which is positive, 0 or -1.");
    }
    else if(f_index > f_parent->get_array().size())
    {
        // this gives us the ability to create items in any order,
        // intermediates are simply set to `null`; we still make sure you
        // don't go too far by verifying at most 1,000 items are added
        //
        if(f_index - f_parent->get_array().size() > MAX_ITEMS_AT_ONCE)
        {
            throw out_of_range(
                      "json_value_ref adding too many items at once (limit "
                    + std::to_string(MAX_ITEMS_AT_ONCE)
                    + ").");
        }
        position pos;
        json_value::pointer_t value(std::make_shared<json_value>(pos));
        do
        {
            f_parent->set_item(f_parent->get_array().size(), value);
        }
        while(f_index > f_parent->get_array().size());
    }
}


json::json_value_ref::json_value_ref(json_value_ref const & ref)
    : f_parent(ref.f_parent)
    , f_name(ref.f_name)
    , f_index(ref.f_index)
{
}


json::json_value_ref & json::json_value_ref::operator = (json_value_ref const & ref)
{
    if(this != &ref)
    {
        f_parent = ref.f_parent;
        f_name = ref.f_name;
        f_index = ref.f_index;
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (std::nullptr_t)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos)); // json null value
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (integer i)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, i));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (floating_point f)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, f));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (char const * s)
{
    return operator = (std::string(s));
}


json::json_value_ref & json::json_value_ref::operator = (std::string const & s)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, s));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (bool boolean)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, boolean));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (json_value::array_t const & array)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, array));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (json_value::object_t const & object)
{
    position pos;
    json_value::pointer_t value(std::make_shared<json_value>(pos, object));
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref & json::json_value_ref::operator = (json_value::pointer_t const & value)
{
    if(f_name.empty())
    {
        f_parent->set_item(f_index, value);
    }
    else
    {
        f_parent->set_member(f_name, value);
    }
    return *this;
}


json::json_value_ref json::json_value_ref::operator [] (char const * name)
{
    return operator [] (std::string(name));
}


json::json_value_ref json::json_value_ref::operator [] (std::string const & name)
{
    position pos;
    json_value::object_t object;
    json_value::pointer_t value;
    if(f_name.empty())
    {
        json_value::array_t const & ary(f_parent->get_array());
        if(f_index >= ary.size())
        {
            value = std::make_shared<json_value>(pos, object);
            f_parent->set_item(f_index, value);
        }
        else
        {
            value = ary[f_index];
        }
    }
    else
    {
        json_value::object_t const & obj(f_parent->get_object());
        auto it(obj.find(f_name));
        if(it == obj.end())
        {
            value = std::make_shared<json_value>(pos, object);
            f_parent->set_member(f_name, value);
        }
        else
        {
            value = it->second;
        }
    }
    return json_value_ref(value, name);
}


json::json_value_ref json::json_value_ref::operator [] (ssize_t idx)
{
    position pos;
    json_value::array_t array;
    json_value::pointer_t value;
    if(f_name.empty())
    {
        json_value::array_t const & ary(f_parent->get_array());
        if(f_index >= ary.size())
        {
            value = std::make_shared<json_value>(pos, array);
            f_parent->set_item(f_index, value);
        }
        else
        {
            value = ary[f_index];
        }
    }
    else
    {
        json_value::object_t const & obj(f_parent->get_object());
        auto it(obj.find(f_name));
        if(it == obj.end())
        {
            value = std::make_shared<json_value>(pos, array);
            f_parent->set_member(f_name, value);
        }
        else
        {
            value = it->second;
        }
    }
    return json_value_ref(value, idx);
}


json::json_value_ref::operator integer () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_INTEGER)
            {
                return value->get_integer();
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_INTEGER)
            {
                return it->second->get_integer();
            }
        }
    }
    return integer();
}


json::json_value_ref::operator floating_point () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_FLOATING_POINT)
            {
                return value->get_floating_point();
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_FLOATING_POINT)
            {
                return it->second->get_floating_point();
            }
        }
    }
    return floating_point();
}


json::json_value_ref::operator std::string () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_STRING)
            {
                return value->get_string();
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_STRING)
            {
                return it->second->get_string();
            }
        }
    }
    return std::string();
}


json::json_value_ref::operator bool () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_TRUE)
            {
                return true;
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_TRUE)
            {
                return true;
            }
        }
    }
    return false;
}


json::json_value_ref::operator json_value::array_t const & () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_ARRAY)
            {
                return value->get_array();
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_ARRAY)
            {
                return it->second->get_array();
            }
        }
    }
    //return json_value::array_t();
    throw incompatible_type("This entry is not an array.");
}


json::json_value_ref::operator json_value::object_t const & () const
{
    if(f_name.empty())
    {
        json_value::array_t const & array(f_parent->get_array());
        if(f_index < array.size())
        {
            json_value::pointer_t value(array[f_index]);
            if(value->get_type() == json_value::type_t::JSON_TYPE_OBJECT)
            {
                return value->get_object();
            }
        }
    }
    else
    {
        json_value::object_t const & object(f_parent->get_object());
        auto it(object.find(f_name));
        if(it != object.end())
        {
            if(it->second->get_type() == json_value::type_t::JSON_TYPE_OBJECT)
            {
                return it->second->get_object();
            }
        }
    }
    //return json_value::object_t();
    throw incompatible_type("This entry is not an object.");
}


json::json_value::pointer_t json::json_value_ref::parent() const
{
    return f_parent;
}








/** \brief Read a json value.
 *
 * This function opens a FileInput stream, setups a default position
 * and then calls parse() to parse the file in a json tree.
 *
 * \param[in] filename  The name of a json file.
 */
json::json_value::pointer_t json::load(std::string const & filename)
{
    position pos;
    pos.set_filename(filename);

    // we could not find this module, try to load the it
    input_stream<std::ifstream>::pointer_t in(std::make_shared<input_stream<std::ifstream>>());
    in->open(filename);
    if(!in->is_open())
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, pos);
        msg << "cannot open JSON file \"" << filename << "\".";
        // should we throw here?
        return json_value::pointer_t();
    }
    in->get_position().set_filename(filename);

    return parse(in);
}


/** \brief Parse a json object.
 *
 * This function is used to read a json input stream.
 *
 * If a recoverable error occurs, the function returns with a json_value
 * smart pointer. If errors occur, then a message is created and sent,
 * but as much as possible of the input file is read in.
 *
 * Note that the resulting value may be a NULL pointer if too much failed.
 *
 * An empty file is not a valid json file. To the minimum you must have:
 *
 * \code
 * null;
 * \endcode
 *
 * \param[in] in  The input stream to be parsed.
 *
 * \return A pointer to a json_value tree, it may be a NULL pointer.
 */
json::json_value::pointer_t json::parse(base_stream::pointer_t in)
{
    // Parse the json file
    //
    // Note:
    // We do not allow external options because it does not make sense
    // (i.e. json is very simple and no additional options should affect
    // the lexer!)
    options::pointer_t o(std::make_shared<options>());
    // Make sure it is marked as json (line terminators change in this case)
    o->set_option(option_t::OPTION_JSON, 1);
    f_lexer = std::make_shared<lexer>(in, o);
    f_value = read_json_value(f_lexer->get_next_token(false));

    if(!f_value)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, in->get_position());
        msg << "could not interpret this JSON input \"" << in->get_position().get_filename() << "\".";
    }

    f_lexer.reset(); // release 'in' and 'options' pointers

    return f_value;
}


/** \brief Read only json value.
 *
 * This function transform the specified \p n node in a json_value object.
 *
 * The type of object is defined from the type of node we just received
 * from the lexer.
 *
 * \li NODE_FALSE -- create a false json_value
 * \li NODE_FLOATING_POINT -- create a floating point json_value
 * \li NODE_INTEGER -- create an integer json_value
 * \li NODE_NULL -- create a null json_value
 * \li NODE_STRING -- create a string json_value
 * \li NODE_TRUE -- create a true json_value
 *
 * If the lexer returned a NODE_SUBTRACT, then we assume we are about to
 * read an integer or a floating point. We do that and then calculate the
 * opposite and save the result as a FLOATING_POINT or INTEGER json_value.
 *
 * If the lexer returned a NODE_OPEN_SQUARE_BRACKET then the function
 * enters the mode used to read an array.
 *
 * If the lexer returned a NODE_OPEN_CURVLY_BRACKET then the function
 * enters the mode used to read an object.
 *
 * Note that the function is somewhat weak in regard to error handling.
 * If the input is not valid as per as2js json documentation, then an
 * error is emitted and the process stops early.
 *
 * \param[in] n  The node to be transform in a json value.
 */
json::json_value::pointer_t json::read_json_value(node::pointer_t n)
{
    if(n->get_type() == node_t::NODE_EOF)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_EOF, n->get_position());
        msg << "the end of the file was reached while reading JSON data.";
        return json_value::pointer_t();
    }

    switch(n->get_type())
    {
    case node_t::NODE_ADD:
        // positive number...
        n = f_lexer->get_next_token(false);
        switch(n->get_type())
        {
        case node_t::NODE_FLOATING_POINT:
            return std::make_shared<json_value>(n->get_position(), n->get_floating_point());

        case node_t::NODE_INTEGER:
            return std::make_shared<json_value>(n->get_position(), n->get_integer());

        default:
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
            msg << "unexpected token (" << n->get_type_name() << ") found after a \"+\" sign, a number was expected.";
            return json_value::pointer_t();

        }
        /*NOT_REACHED*/
        break;

    case node_t::NODE_FALSE:
        return std::make_shared<json_value>(n->get_position(), false);

    case node_t::NODE_FLOATING_POINT:
        return std::make_shared<json_value>(n->get_position(), n->get_floating_point());

    case node_t::NODE_INTEGER:
        return std::make_shared<json_value>(n->get_position(), n->get_integer());

    case node_t::NODE_NULL:
        return std::make_shared<json_value>(n->get_position());

    case node_t::NODE_OPEN_CURVLY_BRACKET: // read an object
        {
            json_value::object_t obj;

            position pos(n->get_position());
            n = f_lexer->get_next_token(false);
            if(n->get_type() != node_t::NODE_CLOSE_CURVLY_BRACKET)
            {
                for(;;)
                {
                    if(n->get_type() != node_t::NODE_STRING)
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STRING_EXPECTED, n->get_position());
                        msg << "expected a string as the JSON object member name.";
                        return json_value::pointer_t();
                    }
                    std::string name(n->get_string());
                    n = f_lexer->get_next_token(false);
                    if(n->get_type() != node_t::NODE_COLON)
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COLON_EXPECTED, n->get_position());
                        msg << "expected a colon (:) as the JSON object member name ("
                            << name
                            << ") and member value separator (invalid type is "
                            << n->get_type_name()
                            << ")";
                        return json_value::pointer_t();
                    }
                    // skip the colon
                    n = f_lexer->get_next_token(false);
                    json_value::pointer_t value(read_json_value(n)); // recursive
                    if(value == nullptr)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    if(obj.find(name) != obj.end())
                    {
                        // TBD: we should verify that json indeed forbids such
                        //      nonsense; because we may have it wrong
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_OBJECT_MEMBER_DEFINED_TWICE, n->get_position());
                        msg << "the same object member \"" << name << "\" was defined twice, which is not allowed in JSON.";
                        // continue because (1) the existing element is valid
                        // and (2) the new element is valid
                    }
                    else
                    {
                        obj[name] = value;
                    }
                    n = f_lexer->get_next_token(false);
                    if(n->get_type() == node_t::NODE_CLOSE_CURVLY_BRACKET)
                    {
                        break;
                    }
                    if(n->get_type() != node_t::NODE_COMMA)
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, n->get_position());
                        msg << "expected a comma (,) to separate two JSON object members.";
                        return json_value::pointer_t();
                    }
                    n = f_lexer->get_next_token(false);
                }
            }

            return std::make_shared<json_value>(pos, obj);
        }
        break;

    case node_t::NODE_OPEN_SQUARE_BRACKET: // read an array
        {
            json_value::array_t array;

            position pos(n->get_position());
            n = f_lexer->get_next_token(false);
            if(n->get_type() != node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                for(;;)
                {
                    json_value::pointer_t value(read_json_value(n)); // recursive
                    if(value == nullptr)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    array.push_back(value);
                    n = f_lexer->get_next_token(false);
                    if(n->get_type() == node_t::NODE_CLOSE_SQUARE_BRACKET)
                    {
                        break;
                    }
                    if(n->get_type() != node_t::NODE_COMMA)
                    {
                        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, n->get_position());
                        msg << "expected a comma (,) to separate two JSON array items.";
                        return json_value::pointer_t();
                    }
                    n = f_lexer->get_next_token(false);
                }
            }

            return std::make_shared<json_value>(pos, array);
        }
        break;

    case node_t::NODE_STRING:
        return std::make_shared<json_value>(n->get_position(), n->get_string());

    case node_t::NODE_SUBTRACT:
        // negative number...
        n = f_lexer->get_next_token(false);
        switch(n->get_type())
        {
        case node_t::NODE_FLOATING_POINT:
            {
                floating_point f(n->get_floating_point());
                if(!f.is_nan())
                {
                    f.set(-f.get());
                    n->set_floating_point(f);
                }
                // else ... should we err about this one?
            }
            return std::make_shared<json_value>(n->get_position(), n->get_floating_point());

        case node_t::NODE_INTEGER:
            {
                integer i(n->get_integer());
                i.set(-i.get());
                n->set_integer(i);
            }
            return std::make_shared<json_value>(n->get_position(), n->get_integer());

        default:
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
            msg << "unexpected token (" << n->get_type_name() << ") found after a \"-\" sign, a number was expected.";
            return json_value::pointer_t();

        }
        /*NOT_REACHED*/
        break;

    case node_t::NODE_TRUE:
        return std::make_shared<json_value>(n->get_position(), true);

    default:
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
        msg << "unexpected token (" << n->get_type_name() << ") found in a JSON input stream.";
        return json_value::pointer_t();

    }

    // somehow this is required in Sanatize mode
    //
    return json_value::pointer_t();
}


/** \brief Save the json in the specified file.
 *
 * This function is used to save this json in the specified file.
 *
 * One can also specified a header, in most cases a comment that
 * gives copyright, license information and eventually some information
 * explaining what that file is about.
 *
 * \param[in] filename  The name of the file on disk.
 * \param[in] header  A header to be saved before the json data.
 *
 * \return true if the save() succeeded.
 *
 * \sa output()
 */
bool json::save(std::string const & filename, std::string const & header) const
{
    output_stream<std::ofstream>::pointer_t out(std::make_shared<output_stream<std::ofstream>>());
    out->open(filename);
    if(!out->is_open())
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, out->get_position());
        msg << "could not open output file \"" << filename << "\".";
        return false;
    }

    return output(out, header);
}


/** \brief Output this json to the specified output.
 *
 * This function saves this json to the specified output object: \p out.
 *
 * If a header is specified (i.e. \p header is not an empty string) then
 * it gets saved before any json data.
 *
 * The output file is made a UTF-8 text file as the function first
 * saves a BOM in the file. Note that this means you should NOT
 * save anything in that file before calling this function. You
 * may, however, write more data (a footer) on return.
 *
 * \note
 * Functions called by this function may generate the cyclic exception.
 * This happens if your json tree is cyclic which means that a child
 * element points back to one of its parent.
 *
 * \exception invalid_data
 * This exception is raised in the event the json does not have
 * any data to be saved. This happens if you create a json object
 * and never load or parse a valid json or call the set_value()
 * function.
 *
 * \param[in] out  The output stream where the json is to be saved.
 * \param[in] header  A string representing a header. It should
 *                    be written in a C or C++ comment for the
 *                    parser to be able to re-read the file seamlessly.
 *
 * \return true if the data was successfully written to \p out.
 */
bool json::output(base_stream::pointer_t out, std::string const & header) const
{
    if(!f_value)
    {
        // should we instead output "null"?
        throw invalid_data("this JSON has no value to output.");
    }

    // we can't really know for sure we are writing to a file or not
    // we could have a flag, but in most cases the BOM is not required anymore
    //if(std::dynamic_pointer_cast<FileOutput>(out))
    //{
    //    // Only do this if we are outputting to a file!
    //    // start with a BOM so the file is clearly marked as being UTF-8
    //    //
    //    out->write_string(libutf8::to_u8string(libutf8::BOM_CHAR));
    //}

    if(!header.empty())
    {
        out->write_string(header);
        out->write_string("\n");
    }

    out->write_string(f_value->to_string());

    return true;
}


/** \brief Set the value of this json object.
 *
 * This function is used to define the value of this json object. This
 * is used whenever you create a json in memory and want to save it
 * on disk or send it to a client.
 *
 * \param[in] value  The json_value to save in this json object.
 */
void json::set_value(json::json_value::pointer_t value)
{
    f_value = value;
}


/** \brief Retrieve the value of the json object.
 *
 * This function returns the current value of this json object. This
 * is the function you need to call after a call to the load() or
 * parse() functions used to read a json file from an input stream.
 *
 * Note that this function gives you the root json_value object of
 * the json object. You can then read the data or modify it as
 * required. If you make changes, you may later call the save()
 * or output() functions to save the changes to a file or
 * an output stream.
 *
 * \return A pointer to the json_value of this json object.
 */
json::json_value::pointer_t json::get_value() const
{
    return f_value;
}


json::json_value_ref json::operator [] (char const * name)
{
    return operator [] (std::string(name));
}


json::json_value_ref json::operator [] (std::string const & name)
{
    if(f_value == nullptr)
    {
        position pos;
        json_value::object_t obj;
        f_value = std::make_shared<json_value>(pos, obj);
    }
    return json_value_ref(f_value, name);
}


json::json_value_ref json::operator [] (ssize_t idx)
{
    if(f_value == nullptr)
    {
        position pos;
        json_value::array_t ary;
        f_value = std::make_shared<json_value>(pos, ary);
    }
    return json_value_ref(f_value, idx);
}


/** \brief Canonicalize the JSON data found in \p js.
 *
 * This function transforms the \p js string to a JSON value and then
 * back to a string. That string is the result and it is considered
 * canonicalized.
 *
 * \param[in] js  The JSON to canonicalize.
 *
 * \return The canonicalized JSON.
 */
std::string json_canonicalize(std::string const & js)
{
    input_stream<std::stringstream>::pointer_t in(std::make_shared<input_stream<std::stringstream>>());
    *in << js;
    json p;
    json::json_value::pointer_t root(p.parse(in));
    if(root == nullptr)
    {
        throw invalid_data("parsing the input JSON failed.");
    }
    output_stream<std::stringstream>::pointer_t out(std::make_shared<output_stream<std::stringstream>>());
    if(!p.output(out, std::string()))
    {
        throw invalid_data("generating the canonicalized JSON failed.");
    }
    return out->str();
}


std::ostream & operator << (std::ostream & out, json const & js)
{
    output_stream<std::stringstream>::pointer_t output(std::make_shared<output_stream<std::stringstream>>());
    js.output(output, std::string());
    return out << output->str();
}



}
// namespace as2js
// vim: ts=4 sw=4 et
