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


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <memory>
#include    <vector>
#include    <fstream>



namespace as2js
{


typedef std::uint8_t                        byte_t;
typedef std::vector<byte_t>                 byte_vector_t;
typedef std::shared_ptr<std::istream>       istream_pointer_t;
typedef std::shared_ptr<std::ostream>       ostream_pointer_t;


// when creating a stream with which you want to have a position, create
// a sub-class like so:
//     class foo : public stream, public <os-stream-type {};
// then we'll have access to a position and the OS stream
//


class base_stream
{
public:
    typedef std::shared_ptr<base_stream>    pointer_t;

    virtual                 ~base_stream() {}

    position &              get_position() { return f_position; }
    position const &        get_position() const { return f_position; }

    std::int32_t            read_char();
    virtual int             get_byte() { return EOF; }
    virtual ssize_t         write_bytes(char const * s, std::streamsize count) { snapdev::NOT_USED(s, count); return -1; }
    virtual ssize_t         write_string(std::string const & s) { return write_bytes(s.c_str(), s.length()); }

private:
    static constexpr std::int32_t       NO_LAST_CHAR = -1;

    position                f_position = position();
    std::int32_t            f_last_byte = NO_LAST_CHAR;
};


template<typename S>
class input_stream
    : public base_stream
    , public S
{
public:
    typedef std::shared_ptr<input_stream<S>>    pointer_t;

    virtual int             get_byte() { return S::get(); }
};


class cin_stream
    : public base_stream
{
public:
    typedef std::shared_ptr<cin_stream>         pointer_t;

                            cin_stream()
                            {
                                get_position().set_filename("-");
                            }

    virtual int             get_byte() { return std::cin.get(); }
};


template<typename S>
class output_stream
    : public base_stream
    , public S
{
public:
    typedef std::shared_ptr<output_stream<S>>   pointer_t;

    virtual ssize_t         write_bytes(char const * s, std::streamsize count) { S::write(s, count); return !*this ? -1 : count; }
};


class cout_stream
    : public base_stream
{
public:
    typedef std::shared_ptr<cout_stream>   pointer_t;

    virtual ssize_t         write_bytes(char const * s, std::streamsize count) { std::cout.write(s, count); return !std::cout ? -1 : count; }
};


// In order to support different types of file systems, the
// compiler supports a file retriever. Any time a file is
// opened, it calls the retriever (if defined) and uses
// that file. If no retriever was defined, the default is
// used: attempt to open the file with std::ifstream.
// In particular, this is used to handle the external definitions.
//
class input_retriever
{
public:
    typedef std::shared_ptr<input_retriever>
                                pointer_t;

    virtual                     ~input_retriever() {}

    virtual base_stream::pointer_t
                                retrieve(std::string const & filename) = 0;
};



} // namespace as2js
// vim: ts=4 sw=4 et
