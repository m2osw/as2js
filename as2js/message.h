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
#pragma once

// self
//
#include    <as2js/position.h>
#include    <as2js/integer.h>
#include    <as2js/floating_point.h>


// C++
//
#include    <sstream>


namespace as2js
{


enum class message_level_t : std::uint8_t
{
    MESSAGE_LEVEL_OFF,
    MESSAGE_LEVEL_TRACE,
    MESSAGE_LEVEL_DEBUG,
    MESSAGE_LEVEL_INFO,
    MESSAGE_LEVEL_WARNING,
    MESSAGE_LEVEL_ERROR,
    MESSAGE_LEVEL_FATAL,
};


enum class err_code_t : std::uint8_t
{
    AS_ERR_NONE = 0,

    AS_ERR_ABSTRACT,
    AS_ERR_BAD_NUMERIC_TYPE,
    AS_ERR_BAD_PRAGMA,
    AS_ERR_CANNOT_COMPILE,
    AS_ERR_CANNOT_MATCH,
    AS_ERR_CANNOT_OVERLOAD,
    AS_ERR_CANNOT_OVERWRITE_CONST,
    AS_ERR_CASE_LABEL,
    AS_ERR_COLON_EXPECTED,
    AS_ERR_COMMA_EXPECTED,
    AS_ERR_CURVLY_BRACKETS_EXPECTED,
    AS_ERR_DEFAULT_LABEL,
    AS_ERR_DIVIDE_BY_ZERO,
    AS_ERR_DUPLICATES,
    AS_ERR_DYNAMIC,
    AS_ERR_EXPRESSION_EXPECTED,
    AS_ERR_FINAL,
    AS_ERR_IMPROPER_STATEMENT,
    AS_ERR_INACCESSIBLE_STATEMENT,
    AS_ERR_INCOMPATIBLE,
    AS_ERR_INCOMPATIBLE_PRAGMA_ARGUMENT,
    AS_ERR_INSTALLATION,
    AS_ERR_INSTANCE_EXPECTED,
    AS_ERR_INTERNAL_ERROR,
    AS_ERR_NATIVE,
    AS_ERR_INVALID_ARRAY_FUNCTION,
    AS_ERR_INVALID_ATTRIBUTES,
    AS_ERR_INVALID_CATCH,
    AS_ERR_INVALID_CLASS,
    AS_ERR_INVALID_CONDITIONAL,
    AS_ERR_INVALID_DEFINITION,
    AS_ERR_INVALID_DO,
    AS_ERR_INVALID_ENUM,
    AS_ERR_INVALID_EXPRESSION,
    AS_ERR_INVALID_FIELD,
    AS_ERR_INVALID_FIELD_NAME,
    AS_ERR_INVALID_FRAME,
    AS_ERR_INVALID_FUNCTION,
    AS_ERR_INVALID_GOTO,
    AS_ERR_INVALID_IMPORT,
    AS_ERR_INVALID_INPUT_STREAM,
    AS_ERR_INVALID_KEYWORD,
    AS_ERR_INVALID_LABEL,
    AS_ERR_INVALID_NAMESPACE,
    AS_ERR_INVALID_NODE,
    AS_ERR_INVALID_NUMBER,
    AS_ERR_INVALID_OPERATOR,
    AS_ERR_INVALID_PACKAGE_NAME,
    AS_ERR_INVALID_PARAMETERS,
    AS_ERR_INVALID_REST,
    AS_ERR_INVALID_RETURN_TYPE,
    AS_ERR_INVALID_SCOPE,
    AS_ERR_INVALID_TEMPLATE,
    AS_ERR_INVALID_TRY,
    AS_ERR_INVALID_TYPE,
    AS_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE,
    AS_ERR_INVALID_VARIABLE,
    AS_ERR_IO_ERROR,
    AS_ERR_LABEL_NOT_FOUND,
    AS_ERR_LOOPING_REFERENCE,
    AS_ERR_MISMATCH_FUNC_VAR,
    AS_ERR_MISSSING_VARIABLE_NAME,
    AS_ERR_NEED_CONST,
    AS_ERR_NOT_ALLOWED,
    AS_ERR_NOT_ALLOWED_IN_STRICT_MODE,
    AS_ERR_NOT_FOUND,
    AS_ERR_NOT_SUPPORTED,
    AS_ERR_OBJECT_MEMBER_DEFINED_TWICE,
    AS_ERR_PARENTHESIS_EXPECTED,
    AS_ERR_PRAGMA_FAILED,
    AS_ERR_SEMICOLON_EXPECTED,
    AS_ERR_SQUARE_BRACKETS_EXPECTED,
    AS_ERR_STRING_EXPECTED,
    AS_ERR_STATIC,
    AS_ERR_TYPE_NOT_LINKED,
    AS_ERR_UNKNOWN_ESCAPE_SEQUENCE,
    AS_ERR_UNKNOWN_OPERATOR,
    AS_ERR_UNTERMINATED_STRING,
    AS_ERR_UNEXPECTED_EOF,
    AS_ERR_UNEXPECTED_PUNCTUATION,
    AS_ERR_UNEXPECTED_TOKEN,
    AS_ERR_UNEXPECTED_DATABASE,
    AS_ERR_UNEXPECTED_RC,

    AS_ERR_max
};


class message_callback
{
public:
    virtual             ~message_callback() {}

    virtual void        output(
                              message_level_t message_level
                            , err_code_t error_code
                            , position const & pos
                            , std::string const & message) = 0;
};


// Note: avoid copies because with such you'd get the message two or more times
class message
    : public std::stringstream
{
public:
                        message(message_level_t message_level, err_code_t error_code, position const & pos);
                        message(message_level_t message_level, err_code_t error_code);
                        message(message const & rhs) = delete;
                        ~message();

    message &           operator = (message const & rhs) = delete;

    template<typename T>
    message &           operator << (T const & data)
                        {
                            static_cast<std::stringstream &>(*this) << data;
                            return *this;
                        }

    // internal types; you can add your own types with
    // message & operator << (message& os, <my-type>);
    message &           operator << (char const * s);
    message &           operator << (wchar_t const * s);
    message &           operator << (std::string const & s);
    message &           operator << (char const v);
    message &           operator << (char32_t const v);
    message &           operator << (signed char const v);
    message &           operator << (unsigned char const v);
    message &           operator << (signed short const v);
    message &           operator << (unsigned short const v);
    message &           operator << (signed int const v);
    message &           operator << (unsigned int const v);
    message &           operator << (signed long const v);
    message &           operator << (unsigned long const v);
    message &           operator << (signed long long const v);
    message &           operator << (unsigned long long const v);
    message &           operator << (integer const v);
    message &           operator << (float const v);
    message &           operator << (double const v);
    message &           operator << (floating_point const v);
    message &           operator << (bool const v);

private:
    message_level_t     f_message_level = message_level_t::MESSAGE_LEVEL_OFF;
    err_code_t          f_error_code = err_code_t::AS_ERR_NONE;
    position            f_position = position();
};



std::string     message_level_to_string(message_level_t level);
void            set_message_callback(message_callback * callback);
void            set_message_level(message_level_t min_level);
int             warning_count();
int             error_count();

} // namespace as2js
// vim: ts=4 sw=4 et
