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
#pragma once

// self
//
#include    <as2js/integer.h>
#include    <as2js/floating_point.h>


// C++
//
#include    <iostream>
#include    <string>



namespace as2js
{



// represents a continuation character (i.e. '\' + LineTerminatorSequence)
//
constexpr char32_t          STRING_CONTINUATION = -2;

// represents the end of a file
//
constexpr char32_t          CHAR32_EOF = static_cast<char32_t>(EOF);


bool                        valid(std::string const & s);
bool                        valid_character(char32_t c);
bool                        is_integer(std::string const & s, bool strict = false);
bool                        is_floating_point(std::string const & s);
bool                        is_number(std::string const & s);
integer::value_type         to_integer(std::string const & s);
floating_point::value_type  to_floating_point(std::string const & s);
bool                        is_true(std::string const & s);
//ssize_t                     utf8_length() const; -- use libutf8 funcs:  libutf8::u8length(s);
std::string                 simplify(std::string const & s);
std::string                 convert(std::wstring const & s);


//// Our String type is based on std::string and uses the libutf8 to retrieve
//// the UTF-32 characters (char32_t)
////
//class string
//    : public std::string
//{
//public:
//                            string();
//                            string(char const * str, int len = -1);
//                            string(std::string const & str);
//
//    string &                operator = (char const * str);
//    string &                operator = (std::string const & str);
//
//    bool                    operator == (char const * str) const;
//    friend bool             operator == (char const * str, string const & s);
//    bool                    operator != (char const * str) const;
//    friend bool             operator != (char const * str, string const & s);
//
//    string &                operator += (char const * str);
//    string &                operator += (std::string const & str);
//
//    string &                operator += (char32_t const c);
//    string &                operator += (char const c);
//
//    //String                  operator + (char const * str);
//    //String                  operator + (std::string const & str);
//
//    bool                    valid() const;
//    static bool             valid_character(char32_t c);
//
//    bool                    is_integer() const;
//    bool                    is_floating_point() const;
//    bool                    is_number() const;
//    integer::value_type     to_integer() const;
//    floating_point::value_type
//                            to_floating_point() const;
//    bool                    is_true() const;
//
//    ssize_t                 utf8_length() const;
//
//    string                  simplified() const;
//};

//std::ostream & operator << (std::ostream & out, string const & str);


} // namespace as2js
// vim: ts=4 sw=4 et
