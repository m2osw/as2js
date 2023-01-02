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
//#include    <iostream>
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




// We can use the std::[i|o]stream instead of this complex stuff and assume
// all input is UTF-8; if you get files that are not yet UTF-8, convert them
// yourself with a tool such as iconv


//class DecodingFilter
//{
//public:
//    typedef std::shared_ptr<DecodingFilter>     pointer_t;
//
//    virtual                 ~DecodingFilter();
//
//    void                    put(byte_t c);
//    byte_t                  get();
//
//protected:
//    virtual byte_t          get_byte() = 0;
//
//    byte_vector_t           f_buffer = byte_vector_t();
//};
//
//
//class DecodingFilterISO88591
//    : public DecodingFilter
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterUTF8
//    : public DecodingFilter
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterUTF16
//    : public DecodingFilter
//{
//protected:
//    as_char_t               next_char(as_char_t c);
//
//private:
//    as_char_t               f_lead_surrogate = 0;
//};
//
//
//class DecodingFilterUTF16LE
//    : public DecodingFilterUTF16
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterUTF16BE
//    : public DecodingFilterUTF16
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterUTF32LE
//    : public DecodingFilter
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterUTF32BE
//    : public DecodingFilter
//{
//protected:
//    virtual as_char_t       get_char();
//};
//
//
//class DecodingFilterDetect
//    : public DecodingFilter
//{
//protected:
//    virtual as_char_t       get_char();
//
//private:
//    DecodingFilter::pointer_t   f_filter = DecodingFilter::pointer_t();
//};
//
//
//
//// I/O interface that YOU have to derive from so the
//// parser can read the input data from somewhere
//// You need to implement the GetC() function. You can
//// also overload the Error() function so it prints
//// the errors in a console of your choice.
//// The GetFilename() is used by the default Error()
//// function. It is used to generate an error like gcc.
//// That function returns "asc" by default.
////
//// Two examples are available below. One reads a USC-4
//// formatted file and the other reads a string.
//class Input
//{
//public:
//    typedef std::shared_ptr<Input>                  pointer_t;
//    typedef as_char_t                               char_t;
//
//    static char_t const     INPUT_EOF = -1;  // end of file
//    static char_t const     INPUT_NAC = -2;  // not a character (filter requires more input)
//    static char_t const     INPUT_ERR = -3;  // stream error
//
//    Position &              get_position();
//    Position const &        get_position() const;
//
//    char_t                  getc();
//    void                    ungetc(char_t c);
//
//protected:
//                            Input(DecodingFilter::pointer_t filter = DecodingFilter::pointer_t(new DecodingFilterDetect));
//    virtual                 ~Input() {}
//
//    virtual char_t          filter_getc();
//    virtual char_t          get_byte(); // get_byte() is not abstract because the deriving class may instead redefine filter_getc()
//
//private:
//    DecodingFilter::pointer_t   f_filter = DecodingFilter::pointer_t();
//    Position                    f_position = Position();
//    std::vector<char_t>         f_unget = std::vector<char_t>();
//};
//
//
//
//
//
//class StandardInput
//    : public Input
//{
//public:
//    typedef std::shared_ptr<StandardInput>          pointer_t;
//
//                            StandardInput();
//
//protected:
//    virtual char_t          get_byte();
//};
//
//
//
//
//
//class FileInput
//    : public Input
//{
//public:
//    typedef std::shared_ptr<FileInput>              pointer_t;
//
//    bool                    open(String const& filename);
//
//protected:
//    virtual char_t          get_byte();
//
//    std::ifstream           f_file = std::ifstream();
//};
//
//
//
//
//
//class StringInput
//    : public Input
//{
//public:
//    typedef std::shared_ptr<StringInput>            pointer_t;
//
//                            StringInput(String const & str, Position::counter_t line = 1);
//
//protected:
//    virtual char_t          filter_getc();
//
//private:
//    String                  f_str = String();
//    String::size_type       f_pos = 0;
//};
//
//
//
//class Output
//{
//public:
//    typedef std::shared_ptr<Output>             pointer_t;
//
//    virtual                 ~Output() {}
//
//    Position&               get_position();
//    Position const&         get_position() const;
//
//    void                    write(String const& data);
//
//protected:
//    virtual void            internal_write(String const& data) = 0;
//
//    Position                f_position = Position();
//};
//
//
//class StandardOutput
//    : public Output
//{
//public:
//                            StandardOutput();
//
//protected:
//    typedef std::shared_ptr<StandardOutput>     pointer_t;
//
//    virtual void            internal_write(String const& data);
//};
//
//
//class FileOutput
//    : public Output
//{
//public:
//    typedef std::shared_ptr<FileOutput>         pointer_t;
//
//    bool                    open(String const& filename);
//
//protected:
//    virtual void            internal_write(String const& data);
//
//    std::ofstream           f_file = std::ofstream();
//};
//
//
//class StringOutput
//    : public Output
//{
//public:
//    typedef std::shared_ptr<StringOutput>       pointer_t;
//
//    String const&           get_string() const;
//
//private:
//    virtual void            internal_write(String const& data);
//
//    String                  f_string = String();
//};


} // namespace as2js
// vim: ts=4 sw=4 et
