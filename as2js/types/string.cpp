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
#include    "as2js/string.h"

#include    "as2js/exception.h"


// libutf8
//
#include    <libutf8/base.h>
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


// C++
//
#include    <limits>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implementation of string functions.
 *
 * We use std::string for our strings. We assume that those strings are
 * composed of valid UTF-8 characters. We use the libutf8 library to
 * handle the strings seemlessly.
 *
 * The functions found here extend the basic string functionality in link
 * with AlexScript.
 */


namespace as2js
{



/** \brief Check validity of the string.
 *
 * This function checks all the characters for validity. This is based
 * on a Unicode piece of code that clearly specifies that a certain
 * number of characters just cannot be used (i.e. this includes UTF-16
 * surrogates, and any value larger than 0x10FFFF or negative numbers).
 *
 * Note that a null character '\0' is considered valid and part of
 * the string.
 *
 * \param[in] s  The string to be validated.
 *
 * \return true if the entire string is considered valid.
 *
 * \sa valid_character()
 */
bool valid(std::string const & s)
{
    for(libutf8::utf8_iterator it(s); it != s.end(); ++it)
    {
        switch(*it)
        {
        case libutf8::NOT_A_CHARACTER:
        case libutf8::EOS:
            return false;

        }
    }

    return true;
}


/** \brief Check whether a UTF-32 character is considered valid.
 *
 * The UTF-32 type is limited in the code points that can be used. This
 * function returns true if the code point of \p c is considered valid.
 *
 * Characters in UTF-32 must be defined between 0 and 0x10FFFF inclusive,
 * except for code points 0xD800 to 0xDFFF which are used as surrogate
 * in UTF-16 encoding.
 *
 * \param[in] c  The UTF-32 character to be checked.
 *
 * \return true if c is considered valid.
 *
 * \sa valid()
 */
bool valid_character(char32_t c)
{
    return libutf8::is_valid_unicode(c);
}


/** \brief Check whether this string represents a valid integer.
 *
 * This function checks the strings to see whether it represents a
 * valid integer. The function supports decimal and hexadecimal
 * numbers. Octals are not supported because JavaScript does not
 * convert numbers that start with a 0 as if these were octal
 * numbers.
 *
 * \li Decimal number: [-+]?[0-9]+
 * \li Hexadecimal number: [-+]?0[xX][0-9a-fA-F]+
 *
 * \note
 * In strict mode, hexadecimal numbers do not accept a sign.
 *
 * \param[in] s  The integer to be checked.
 * \param[in] strict  Whether we are in strict mode (hexadecimal refuse signs).
 *
 * \return true if the string represents an integer.
 *
 * \sa is_floating_point()
 * \sa to_integer()
 * \sa is_number()
 */
bool is_integer(std::string const & s, bool strict)
{
    char const *f(s.c_str());

    // sign
    //
    bool const is_signed(*f == '-' || *f == '+');
    if(is_signed)
    {
        ++f;
    }

    // handle special case of hexadecimal
    //
    if(*f == '0')
    {
        ++f;
        if(*f == 'x' || *f == 'X')
        {
            if(f[1] == '\0'
            || (strict && is_signed))
            {
                // just "0x" or "0X" is not a valid number
                //
                return false;
            }
            for(++f; isxdigit(*f); ++f);
            return *f == '\0';
        }
        // no octal support in strings
    }

    // number
    //
    for(; *f >= '0' && *f <= '9'; ++f);

    return *f == '\0';
}


/** \brief Check whether the string represents a valid floating pointer number.
 *
 * This function parses the string to see whether it represents a valid
 * floating pointer number:
 *
 * \li a sign
 * \li an integral part
 * \li a decimal part
 * \li a signed exponent
 *
 * All the elements are optional, however, to be valid, the number requires
 * at least an integral part or a decimal part.
 *
 * Note that this function returns true if the number is an integer.
 * However, it will return false for hexadecimal numbers. You may also call
 * the is_number() function to know if a string represents number whether it
 * is a decimal number or a floating point number.
 *
 * \code
 * [-+]?([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE]?[-+]?[0-9]+)?
 * \endcode
 *
 * \param[in] s  The string to check.
 *
 * \return true if the string represents a floating point number.
 *
 * \sa is_integer()
 * \sa to_floating_point()
 * \sa is_number()
 */
bool is_floating_point(std::string const & s)
{
    char const * f(s.c_str());

    // handle special case of an empty string representing 0.0
    //
    if(s.empty())
    {
        return true;
    }

    // sign
    //
    if(*f == '-' || *f == '+')
    {
        ++f;
    }

    // integral part
    //
    bool const has_integral_part(*f >= '0' && *f <= '9');
    if(has_integral_part)
    {
        for(++f; *f >= '0' && *f <= '9'; ++f);
    }

    // if '.' check for a decimal part
    //
    bool has_decimal_part(false);
    bool const has_period(*f == '.');
    if(has_period)
    {
        ++f;
        has_decimal_part = *f >= '0' && *f <= '9';
        if(has_decimal_part)
        {
            for(++f; *f >= '0' && *f <= '9'; ++f);
        }
    }

    if(has_period)
    {
        // if there is a period we must have at least one of the integral
        // or decimal parts
        //
        if(!has_integral_part
        && !has_decimal_part)
        {
            return false;
        }
    }
    else
    {
        // if there is no period, we must have an integral part
        //
        if(!has_integral_part)
        {
            return false;
        }
    }

    // if 'e' check for an exponent
    // we can have an exponent whether we have a period or not
    //
    if(*f == 'e' || *f == 'E')
    {
        ++f;
        if(*f == '+' || *f == '-')
        {
            // skip the exponent sign
            //
            ++f;
        }
        if(*f < '0' || *f > '9')
        {
            // to be valid, the exponent must include at least one digit
            //
            return false;
        }
        for(++f; *f >= '0' && *f <= '9'; ++f);
    }

    return *f == '\0';
}


/** \brief Check whether this string represents a number.
 *
 * This function checks whether this string represents a number.
 * This means it returns true in the following cases:
 *
 * \li The string represents a decimal number ([-+]?[0-9]+)
 * \li The string represents a hexadecimal number ([-+]?0[xX][0-9a-fA-F]+)
 * \li The string represents a floating point number ([-+]?[0-9]+(\.[0-9]+)?([eE]?[0-9]+)?)
 *
 * Unfortunately, JavaScript does not understand "true", "false",
 * and "null" as numbers (even though isNaN(true), isNaN(false),
 * and isNaN(null) all return true.)
 *
 * \warning
 * This function calls is_integer() and is_floating_point(). This is because
 * an integer may be written as hexadecimal and the is_floating_point()
 * function does not recognize that special case.
 *
 * \return true if this string represents a valid number
 *
 * \sa is_integer()
 * \sa is_floating_point()
 */
bool is_number(std::string const & s)
{
    return is_integer(s) || is_floating_point(s);
}


/** \brief Convert a string to an integer number.
 *
 * This function verifies that the string represents a valid integer
 * number, if so, it converts it to such and returns the result.
 *
 * If the string does not represent a valid integer, then the function
 * should return NaN. Unfortunately, there is not NaN integer. Instead
 * it will return zero (0) or it will raise an exception.
 *
 * \note
 * When used by the lexer, it should always work since the lexer reads
 * integers with the same expected syntax.
 *
 * \exception internal_error
 * The string is not empty and it does not represent what is considered
 * a valid JavaScript integer.
 *
 * \param[in] s  The string to convert to an integer.
 *
 * \return The string converted to an integer.
 */
integer::value_type to_integer(std::string const & s)
{
    if(s.empty())
    {
        return integer::value_type();
    }

    if(is_integer(s))
    {
        // Check whether it is a hexadecimal number, because if so
        // we use base 16. We want to force the base because we do
        // not support base 8 which std::stoll() could otherwise
        // switch to when we have a number that starts with zero.
        //
        char const *f(s.c_str());
        if(*f == '+' || *f == '-')
        {
            ++f;
        }
        if(f[0] == '0' && (f[1] == 'x' || f[1] == 'X'))
        {
            // the strtoll() function supports the sign
            //
            return std::stoll(s, nullptr, 16);
        }
        return std::stoll(s, nullptr, 10);
    }

    // this is invalid
    //
    throw internal_error("to_integer(std::string const & s) called with an invalid integer.");
}


/** \brief Convert a string to a floating point number.
 *
 * This function verifies that the string represents a valid floating
 * point number, if so, it converts it to such and returns the result.
 *
 * If the string does not represent a valid floating point, then the
 * function returns NaN.
 *
 * \warning
 * On an empty string, this function returns 0.0 and not NaN as expected
 * in JavaScript.
 *
 * \note
 * When used by the lexer, it should always work since the lexer reads
 * floating points with the same expected syntax.
 *
 * \param[in] s  The string to convert.
 *
 * \return The string as a floating point.
 */
floating_point::value_type to_floating_point(std::string const & s)
{
    if(s.empty())
    {
        return floating_point::value_type();
    }

    if(is_floating_point(s))
    {
        return std::stod(s, 0);
    }

    return std::numeric_limits<floating_point::value_type>::quiet_NaN();
}


/** \brief Check whether the string is considered true.
 *
 * A string that is empty is considered false. Any other string is
 * considered true.
 *
 * \return true if the string is not empty.
 */
bool is_true(std::string const & s)
{
    if(s.empty())
    {
        return false;
    }

// Not too sure where I picked that up, but the documentation clearly says
// that an empty string is false, anything else is true...
//    if(is_integer(s))
//    {
//        return to_integer(s) != 0;
//    }
//    if(is_floating_point(s))
//    {
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wfloat-equal"
//        return to_floating_point(s) != 0.0;
//#pragma GCC diagnostic pop
//    }

    return true;
}


/** \brief Make a simplified copy of the input string.
 *
 * This function makes a copy of the input string \p s while removing spaces
 * from the start, the end, and within the string keep a single space.
 *
 * If the string starts with a number, then only the number is kept.
 *
 * \note
 * This function is primarily used to compare a string using the
 * smart match operator.
 *
 * \return The simplified string.
 */
std::string simplify(std::string const & s)
{
    std::string result;

    libutf8::utf8_iterator it(s);
    for(; it != s.end(); ++it)
    {
        // TBD: should we limit the space check to spaces recognized by EMCAScript?
        if(!iswspace(*it))
        {
            break;
        }
    }

    // accept a signed number
    //
    if(it != s.end()
    && (*it == '-' || *it == '+'))
    {
        result += *it;
        ++it;
    }

    if(it != s.end())
    {
        if(*it >= '0' && *it <= '9')
        {
            // read the number, ignore the rest
            //
            result += *it;
            for(++it; it != s.end(); ++it)
            {
                if(*it < '0' || *it > '9')
                {
                    break;
                }
                result += *it;
            }
            if(it != s.end()
            && *it == '.')
            {
                result += '.';
                for(++it; it != s.end(); ++it)
                {
                    if(*it < '0' || *it > '9')
                    {
                        break;
                    }
                    result += *it;
                }
                if(it != s.end()
                && (*it == 'e' || *it == 'E'))
                {
                    std::string e;
                    e += *it;
                    ++it;
                    if(it != s.end()
                    && (*it == '+' || *it == '-'))
                    {
                        e += *it;
                        ++it;
                    }
                    if(it != s.end()
                    && *it >= '0'
                    && *it <= '9')
                    {
                        result += e;
                        result += *it;
                        for(++it; it != s.end(); ++it)
                        {
                            if(*it < '0' || *it > '9')
                            {
                                break;
                            }
                            result += *it;
                        }
                    }
                }
            }
            // ignore anything else
        }
        else
        {
            // read the string, but simplify the spaces
            //
            bool found_space(false);
            for(; it != s.end(); ++it)
            {
                if(iswspace(*it))
                {
                    found_space = true;
                }
                else
                {
                    if(found_space)
                    {
                        result += ' ';
                        found_space = false;
                    }
                    result += *it;
                }
            }
        }
    }

    if(result.empty())
    {
        // make an empty string similar to zero
        result = "0";
    }

    return result;
}


std::string convert(std::wstring const & str)
{
    return libutf8::to_u8string(str);
}



} // namespace as2js
// vim: ts=4 sw=4 et
