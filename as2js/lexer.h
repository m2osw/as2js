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
#include    <as2js/stream.h>
#include    <as2js/options.h>
#include    <as2js/node.h>



namespace as2js
{



constexpr std::size_t       MAX_REGEXP_LENGTH = 1024;


class lexer
{
public:
    typedef std::shared_ptr<lexer>      pointer_t;

                                lexer(
                                      base_stream::pointer_t input
                                    , options::pointer_t options);

    base_stream::pointer_t      get_input();
    position                    get_position() const;

    node::pointer_t             get_new_node(node_t type);
    node::pointer_t             get_next_token(bool regexp_allowed);
    node::pointer_t             get_next_template_token();

private:
    typedef int                         char_type_t;
    typedef std::vector<char32_t>       char_buffer_t;

    static constexpr char_type_t const  CHAR_NO_FLAGS        = 0x0000;
    static constexpr char_type_t const  CHAR_LETTER          = 0x0001;
    static constexpr char_type_t const  CHAR_DIGIT           = 0x0002;
    static constexpr char_type_t const  CHAR_PUNCTUATION     = 0x0004;
    static constexpr char_type_t const  CHAR_WHITE_SPACE     = 0x0008;
    static constexpr char_type_t const  CHAR_LINE_TERMINATOR = 0x0010;
    static constexpr char_type_t const  CHAR_HEXDIGIT        = 0x0020;
    static constexpr char_type_t const  CHAR_INVALID         = 0x8000;   // such as 0xFFFE & 0xFFFF

    void                        get_token();
    char32_t                    getc();
    void                        ungetc(char32_t c);
    int64_t                     read_hex(std::uint32_t const max, bool allow_separator);
    int64_t                     read_binary(std::uint32_t const max);
    int64_t                     read_octal(char32_t c, std::uint32_t const max, bool legacy, bool allow_separator);
    char32_t                    escape_sequence(bool accept_continuation);
    char_type_t                 char_type(char32_t c);
    char32_t                    read(char32_t c, char_type_t flags, std::string & str);
    void                        read_identifier(char32_t c);
    void                        read_number(char32_t c);
    void                        read_string(char32_t quote);
    void                        read_template_start();
    bool                        has_option_set(options::option_t option) const;

    base_stream::pointer_t      f_input = base_stream::pointer_t();
    int                         f_last_byte = -1;
    char_buffer_t               f_unget = char_buffer_t();
    options::pointer_t          f_options = options::pointer_t();
    char_type_t                 f_char_type = CHAR_NO_FLAGS;    // type of the last character read
    bool                        f_regexp_allowed = true;

    node_t                      f_result_type = node_t::NODE_UNKNOWN;
    std::string                 f_result_string = std::string();
    integer                     f_result_integer = integer();
    floating_point              f_result_floating_point = floating_point();
};



} // namespace as2js
// vim: ts=4 sw=4 et
