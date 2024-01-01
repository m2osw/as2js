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
#include    "as2js/archive.h"

//#include    "as2js/exception.h"
//#include    "as2js/message.h"
//#include    "as2js/output.h"


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/pathinfo.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


namespace
{

// magic bytes (B0 is at offset 0, etc.)
//
constexpr std::uint32_t ARCHIVE_MAGIC_B0 = 0x03;    // non-ascii printable (Ctrl-C)
constexpr std::uint32_t ARCHIVE_MAGIC_B1 = 0x6F;    // [o]bject
constexpr std::uint32_t ARCHIVE_MAGIC_B2 = 0x61;    // [ar]chive
constexpr std::uint32_t ARCHIVE_MAGIC_B3 = 0x72;

// version found in the header
//
constexpr std::uint8_t  ARCHIVE_VERSION_MAJOR = 1;
constexpr std::uint8_t  ARCHIVE_VERSION_MINOR = 0;

// a complete file is composed of:
//
//   archive_header
//   archive_function[archive_header.f_count]
//   null terminated strings
//   code blocks
//
struct archive_header
{
    std::uint8_t            f_magic[4] = { ARCHIVE_MAGIC_B0, ARCHIVE_MAGIC_B1, ARCHIVE_MAGIC_B2, ARCHIVE_MAGIC_B3 };
    std::uint8_t            f_version_major = ARCHIVE_VERSION_MAJOR;
    std::uint8_t            f_version_minor = ARCHIVE_VERSION_MINOR;
    std::uint16_t           f_pad = 0;          // we may add flags/arch. info
    std::uint32_t           f_count = 0;        // number of functions
    std::uint32_t           f_names = 0;        // size of all strings (appears right after functions)
};


struct archive_function
{
    std::uint32_t           f_name = 0;         // offset to name (null terminated string)
    std::uint32_t           f_code = 0;         // offset to code
    std::uint32_t           f_size = 0;         // size of code
};


} // no name namespace



void rt_function::set_name(std::string const & name)
{
    f_name = name;
}


std::string const & rt_function::get_name() const
{
    return f_name;
}


void rt_function::set_code(rt_text_t const & code)
{
    f_code = code;
}


rt_text_t const & rt_function::get_code() const
{
    return f_code;
}


rt_function::pointer_t load_function(base_stream::pointer_t in)
{
    rt_function::pointer_t func(std::make_shared<rt_function>());
    std::string name(in->get_position().get_filename());
    name = snapdev::pathinfo::basename(name, ".*");
    if(name.length() > 3
    && name[0] =='r'
    && name[1] =='t'
    && name[2] =='_')
    {
        name = name.substr(3);
    }
    func->set_name(name);

    rt_text_t code;
    for(;;)
    {
        std::uint8_t buf[1024 * 16];
        ssize_t const size(in->read_bytes(reinterpret_cast<char *>(buf), sizeof(buf)));
        if(size == -1)
        {
            return rt_function::pointer_t();
        }
        if(size == 0)
        {
            break;
        }
        code.insert(code.end(), buf, buf + size);
    }
    func->set_code(code);

    return func;
}


rt_function::pointer_t load_function(std::string const & filename)
{
    input_stream<std::ifstream>::pointer_t in(std::make_shared<input_stream<std::ifstream>>());
    in->open(filename);
    if(in->is_open())
    {
        in->get_position().set_filename(filename);
        return load_function(in);
    }
    return rt_function::pointer_t();
}







bool archive::create(std::vector<std::string> const & patterns)
{
    f_functions.clear();

    for(auto const & p : patterns)
    {
        if(!add_from_pattern(p))
        {
            return false;
        }
    }

    return true;
}


bool archive::add_from_pattern(std::string const & pattern)
{
    snapdev::glob_to_list<std::list<std::string>> glob;
    if(!glob.read_path<
          snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
        , snapdev::glob_to_list_flag_t::GLOB_FLAG_TILDE
        , snapdev::glob_to_list_flag_t::GLOB_FLAG_BRACE>(pattern))
    {
        return false;
    }

    for(auto const & filename : glob)
    {
        rt_function::pointer_t func(load_function(filename));
        if(func == nullptr)
        {
            return false;
        }
        add_function(func);
    }

    return true;
}


bool archive::load(base_stream::pointer_t in)
{
    f_functions.clear();

    archive_header header = {};
    if(in->read_bytes(reinterpret_cast<char *>(&header), sizeof(header)) != sizeof(header))
    {
        return false;
    }
    if(header.f_magic[0] != ARCHIVE_MAGIC_B0
    || header.f_magic[1] != ARCHIVE_MAGIC_B1
    || header.f_magic[2] != ARCHIVE_MAGIC_B2
    || header.f_magic[3] != ARCHIVE_MAGIC_B3)
    {
        return false;
    }

    std::vector<archive_function> functions(header.f_count);
    if(in->read_bytes(reinterpret_cast<char *>(functions.data()), sizeof(archive_function) * header.f_count) != static_cast<ssize_t>(sizeof(archive_function) * header.f_count))
    {
        return false;
    }

    std::vector<char> names(header.f_names);
    if(in->read_bytes(reinterpret_cast<char *>(names.data()), header.f_names) != header.f_names)
    {
        return false;
    }

    std::uint32_t const name_start(sizeof(header) + sizeof(archive_function) * header.f_count);
    for(std::size_t idx(0); idx < header.f_count; ++idx)
    {
        rt_function::pointer_t func(std::make_shared<rt_function>());
        func->set_name(names.data() + functions[idx].f_name - name_start);
        rt_text_t code(functions[idx].f_size);
        if(in->read_bytes(reinterpret_cast<char *>(code.data()), functions[idx].f_size) != static_cast<ssize_t>(functions[idx].f_size))
        {
            return false;
        }
        func->set_code(code);
        add_function(func);
    }

    return true;
}


bool archive::save(base_stream::pointer_t out)
{
    archive_header header;
    header.f_count = f_functions.size();

    if(header.f_count == 0)
    {
        return false;
    }

    std::uint32_t const start_name_offset(
              sizeof(archive_header)
            + sizeof(archive_function) * header.f_count);
    std::uint32_t end_name_offset(start_name_offset);

    // we start with zero and go back through the list to adjust to
    // the correct offset once we have it
    //
    std::uint32_t end_code_offset(0);

    std::vector<archive_function> functions(header.f_count);
    std::size_t idx(0);
    for(auto const & func : f_functions)
    {
        functions[idx].f_name = end_name_offset;
        end_name_offset += func.second->get_name().length() + 1; // +1 for '\0'

        functions[idx].f_code = end_code_offset;
        std::size_t const size(func.second->get_code().size());
        functions[idx].f_size = size;
        end_code_offset += size;

        ++idx;
    }

    header.f_names = end_name_offset - start_name_offset;

    for(idx = 0; idx < header.f_count; ++idx)
    {
        functions[idx].f_code += end_name_offset;
    }

    out->write_bytes(reinterpret_cast<char const *>(&header), sizeof(header));
    out->write_bytes(reinterpret_cast<char const *>(functions.data()), sizeof(archive_function) * header.f_count);

    for(auto const & func : f_functions)
    {
        std::string const & name(func.second->get_name());
        out->write_bytes(name.c_str(), name.length() + 1);
    }

    for(auto const & func : f_functions)
    {
        auto const & code(func.second->get_code());
        out->write_bytes(reinterpret_cast<char const *>(code.data()), code.size());
    }

    return true;
}


void archive::add_function(rt_function::pointer_t const & func)
{
    f_functions[func->get_name()] = func;
}


rt_function::pointer_t archive::find_function(std::string const & name) const
{
    auto it(f_functions.find(name));
    if(it == f_functions.end())
    {
        return rt_function::pointer_t();
    }
    return it->second;
}


rt_function::map_t archive::get_functions() const
{
    return f_functions;
}



} // namespace as2js
// vim: ts=4 sw=4 et
