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
#include    "as2js/binary.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"
#include    "as2js/output.h"


// snapdev
//
#include    <snapdev/not_reached.h>


// versiontheca
//
#include    <versiontheca/basic.h>


// C++
//
#include    <algorithm>
#include    <iomanip>


// C
//
#include    <string.h>
#include    <sys/mman.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{

namespace
{



constexpr char const g_end_magic[] = { 'E', 'N', 'D', '!' };



} // no name namespace



/** \class binary_file
 * \brief Manage a binary file.
 *
 * Our binary file are used for JIT compiling although in many cases we can
 * also cache the results and avoid the whole compiling side by saving the
 * resulting binary code in a file.
 *
 * This class handles that file. It includes several parts:
 *
 * \li Header -- the header includes a magic code at the start of the file
 *     and then offsets to the different data types we use in the code
 *     (i.e. external variable references)
 * \li Variables -- the user defined variables (array, size found in header)
 * \li Variable Names -- the Variables have offsets to variable names which
 *     are found here
 * \li Text Section -- the actual binary code (PIC to avoid having to relocate)
 * \li Run-Time Text -- if the code requires some of the run-time code, then
 *     it gets included too. The Text Section includes the necessary CALLs to
 *     make everything work as expected.
 * \li Data Section -- The Text section references variables defined in the
 *     data section (temporary variables, variables not marked as extern);
 *     this section also includes literals except if it could be used
 *     as immediate values (add $5, %rax -- the $5 is not going to appear
 *     in the Data Section)
 */




char const * variable_type_to_string(variable_type_t t)
{
    switch(t)
    {
    //case VARIABLE_TYPE_UNKNOWN:
    default:
        return "unknown";

    case VARIABLE_TYPE_BOOLEAN:
        return "boolean";

    case VARIABLE_TYPE_INTEGER:
        return "integer";

    case VARIABLE_TYPE_FLOATING_POINT:
        return "floating_point";

    case VARIABLE_TYPE_STRING:
        return "string";

    }
}




temporary_variable::temporary_variable(
                                  std::string const & name
                                , node_t type
                                , std::size_t size
                                , ssize_t offset)
    : f_name(name)
    , f_type(type)
    , f_size(size)
    , f_offset(offset)
{
//std::cerr << "temp [" << name << "] has offset: " << offset << "\n";
    if(f_offset >= 0)
    {
        throw internal_error(
              "all temporary variables are on the stack from rbp and use a negative offset, "
            + std::to_string(f_offset)
            + " is not valid.");
    }
}


std::string const & temporary_variable::get_name() const
{
    return f_name;
}


node_t temporary_variable::get_type() const
{
    return f_type;
}


std::size_t temporary_variable::get_size() const
{
    return f_size;
}


ssize_t temporary_variable::get_offset() const
{
    return f_offset;
}









//relocation::relocation()
//{
//}


relocation::relocation(
          std::string const & name
        , relocation_t type
        , offset_t position
        , offset_t offset)
    : f_name(name)
    , f_relocation(type)
    , f_position(position)
    , f_offset(offset)
{
}


std::string relocation::get_name() const
{
    return f_name;
}


relocation_t relocation::get_relocation() const
{
    return f_relocation;
}


offset_t relocation::get_position() const
{
    return f_position;
}


offset_t relocation::get_offset() const
{
    return f_offset;
}








binary_variable * build_file::new_binary_variable(std::string const & name, variable_type_t type, std::size_t size)
{
    binary_variable var = {};

    // type
    //
    var.f_type = type;

#ifdef _DEBUG
    if(f_extern_variables.size() > 0)
    {
        std::string previous_name;
        if(f_extern_variables[f_extern_variables.size() - 1].f_name_size <= sizeof(var.f_name))
        {
            previous_name = std::string(
                              reinterpret_cast<char const *>(&f_extern_variables[f_extern_variables.size() - 1].f_name)
                            , f_extern_variables[f_extern_variables.size() - 1].f_name_size);
        }
        else
        {
            previous_name = std::string(
                              f_strings.begin() + f_extern_variables[f_extern_variables.size() - 1].f_name
                            , f_strings.begin() + f_extern_variables[f_extern_variables.size() - 1].f_name_size);
        }
        if(previous_name >= name)
        {
            throw internal_error("binary variables are expected to be added in lexical order.");
        }
    }
#endif

    // name
    //
    var.f_name_size = name.length();
    if(var.f_name_size <= sizeof(var.f_name))
    {
        // it fits right here!
        //
        memcpy(
              reinterpret_cast<char *>(&var.f_name)
            , name.c_str()
            , var.f_name_size);
    }
    else
    {
        // the current offset is within the variable_names array of characters
        // once we're ready to save that, we compute the start offset of the
        // f_strings in the output file and add that to the f_name's
        //
        var.f_name = f_strings.size();
        f_strings.insert(f_strings.end(), name.begin(), name.end());
    }

    // size
    //
    var.f_data_size = size;

    std::size_t const index(f_extern_variables.size());
    f_extern_variables.push_back(var);
    return &f_extern_variables[index];
}


void build_file::add_extern_variable(std::string const & name, data::pointer_t type)
{
    // TODO: save the default value (SET <expr>)
    //
    node::pointer_t instance(type->get_node()->get_type_node());
    if(instance == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
        msg << "no type found for variable \""
            << name
            << "\".";
        throw internal_error(msg.str());
    }
    std::string const & type_name(instance->get_string());

    if(instance->get_type() == node_t::NODE_CLASS
    && instance->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        std::size_t size(0);
        variable_type_t var_type(VARIABLE_TYPE_UNKNOWN);
        if(type_name == "Boolean")
        {
            var_type = VARIABLE_TYPE_BOOLEAN;
            size = sizeof(bool);
        }
        else if(type_name == "Integer")
        {
            var_type = VARIABLE_TYPE_INTEGER;
            size = sizeof(std::int64_t);
        }
        else if(type_name == "Double")
        {
            var_type = VARIABLE_TYPE_FLOATING_POINT;
            size = sizeof(double);
        }
        else if(type_name == "String")
        {
            var_type = VARIABLE_TYPE_STRING;
            size = 0; // TODO
        }
        if(var_type != VARIABLE_TYPE_UNKNOWN)
        {
            binary_variable * var(new_binary_variable(name, var_type, size));

            // TODO: add default values (var x := <default value>;)
            //       only that may be a complex expression so we want
            //       to have it in the code unless it's a constant
            //
            switch(var_type)
            {
            case VARIABLE_TYPE_BOOLEAN:
                var->f_data = static_cast<std::uint64_t>(false);
                break;

            case VARIABLE_TYPE_INTEGER:
                var->f_data = static_cast<std::uint64_t>(0);
                break;

            case VARIABLE_TYPE_FLOATING_POINT:
                {
                    double default_value(0.0);

                    // use intermediate pointer to avoid the strict aliasing issue
                    //
                    double const * value_ptr(&default_value);
                    var->f_data = *reinterpret_cast<std::uint64_t const *>(value_ptr);
                }
                break;

            case VARIABLE_TYPE_STRING:
                {
                    // something to do with this (see f_name handling in new_binary_variable() above)
                    //var.f_name_size = name.length();
                    //var.f_name = f_strings.size();
                    //f_strings.insert(f_strings.end(), name.begin(), name.end());
                    var->f_data = 0;
                }
                break;

            case VARIABLE_TYPE_UNKNOWN:
                snapdev::NOT_REACHED();

            }

            return;
        }
    }

    // TBD: I think this can happen if you re-define native types (for fun)
    //
    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
    msg << "unsupported node type \""
        << type_name
        << "\" for a temporary variable (maybe try add_data() instead).";
    throw internal_error(msg.str());
}


void build_file::add_temporary_variable(std::string const & name, data::pointer_t type)
{
    node::pointer_t instance(type->get_node()->get_type_node());
    if(instance == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
        msg << "no type found for a temporary variable.";
        throw internal_error(msg.str());
    }
    std::string const & type_name(instance->get_string());

    if(instance->get_type() == node_t::NODE_CLASS
    && instance->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        if(type_name == "Boolean")
        {
            f_temporary_1byte.push_back(temporary_variable(
                          name
                        , node_t::NODE_BOOLEAN
                        , 1
                        , (f_temporary_1byte.size() + 1LL) * -1LL));
            return;
        }
        else if(type_name == "Integer")
        {
            f_temporary_8bytes.push_back(temporary_variable(
                          name
                        , node_t::NODE_INTEGER
                        , 8
                        , (f_temporary_8bytes.size() + 1LL) * -8LL));
            return;
        }
        else if(type_name == "Double")
        {
            f_temporary_8bytes.push_back(temporary_variable(
                          name
                        , node_t::NODE_DOUBLE
                        , 8
                        , (f_temporary_8bytes.size() + 1LL) * -8LL));
            return;
        }
    }

    // TBD: I think this can happen if you re-define native types (for fun)
    //
    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
    msg << "unsupported node type \""
        << type_name
        << "\" for a temporary variable (maybe try add_data() instead).";
    throw internal_error(msg.str());
}


void build_file::add_private_variable(std::string const & name, data::pointer_t type)
{
    // TODO: save the default value (SET <expr>)
    //
    node::pointer_t instance(type->get_node()->get_type_node());
    if(instance == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
        msg << "no type found for variable \""
            << name
            << "\".";
        throw internal_error(msg.str());
    }
    std::string const & type_name(instance->get_string());

    if(instance->get_type() == node_t::NODE_CLASS
    && instance->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        binary_variable var = {};

        var.f_name_size = name.length();
        var.f_name = f_strings.size();
        f_strings.insert(f_strings.end(), name.begin(), name.end());

        // TODO: add default values (var x := <default value>;)
        //
        if(type_name == "Boolean")
        {
            f_private_offsets[name] = f_bool_private.size();
            bool value(0);                  // TODO: <- use SET expr if it exists and is constant
            f_bool_private.insert(f_bool_private.end(), &value, &value + 1);
            return;
        }
        else if(type_name == "Integer"
             || type_name == "Double")
        {
            f_private_offsets[name] = f_bool_private.size();
            std::int64_t value(0);          // TODO: <- use SET expr if it exists and is constant
            f_number_private.insert(f_number_private.end(), &value, &value + 1);
            return;
        }
        else if(type_name == "String")
        {
            f_private_offsets[name] = f_bool_private.size();
            variable_t value = {};          // TODO: <- use SET expr if it exists and is constant
            f_string_private.insert(
                      f_string_private.end()
                    , reinterpret_cast<std::uint8_t const *>(&value)
                    , reinterpret_cast<std::uint8_t const *>(&value + 1));
            return;
        }
    }

    // TBD: I think this can happen if you re-define native types (for fun)
    //
    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
    msg << "unsupported node type \""
        << type_name
        << "\" for a temporary variable (maybe try add_data() instead).";
    throw internal_error(msg.str());
}


void build_file::add_constant(std::string & name, double value)
{
    // constant are saved along non-constant private variables
    //
    name = "@";
    name += std::to_string(static_cast<std::uint64_t>(value));
    auto it(f_private_offsets.find(name));
    if(it == f_private_offsets.end())
    {
        // not yet defined, add anew
        //
        f_private_offsets[name] = f_number_private.size();
        f_number_private.insert(f_number_private.end(), &value, &value + 1);
    }
    //else -- already here, just return the existing name
}


void build_file::add_constant(std::string & name, std::string value)
{
    for(auto const & it : f_private_offsets)
    {
        if(it.first.substr(0, 4) == "@str")
        {
            variable_t * str(reinterpret_cast<variable_t *>(f_string_private.data() + it.second));
            if(str->f_size == value.length()
            && memcmp(f_strings.data() + str->f_string, value.c_str(), str->f_size) == 0)
            {
                // found the exact same string, reuse it
                //
                name = it.first;
                return;
            }
        }
    }

    // string was not found, add new string now
    //
    variable_t s = {};
    s.f_size = value.length();
    s.f_string = f_strings.size();
    f_strings.insert(f_strings.end(), name.begin(), name.end());

    ++f_next_const_string;
    name = "@str";
    name += std::to_string(static_cast<std::uint64_t>(f_next_const_string));

    f_private_offsets[name] = f_string_private.size();
    f_string_private.insert(
              f_string_private.end()
            , reinterpret_cast<std::uint8_t const *>(&s)
            , reinterpret_cast<std::uint8_t const *>(&s + 1));
}


void build_file::add_rt_function(
      std::string const & path
    , std::string const & name)
{
    // already added?
    //
    if(f_rt_function_offsets.find(name) != f_rt_function_offsets.end())
    {
        return;
    }

    // save the offset
    //
    f_rt_function_offsets[name] = f_rt_functions.size();

    // load the function
    //
    if(f_archive.get_functions().empty())
    {
        std::string filename(path + "/rt.oar");
        as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
        in->get_position().set_filename(filename);
        in->open(filename);
        if(!in->is_open())
        {
            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
            msg << "could not open run-time object archive \""
                << filename
                << "\".";
            throw cannot_open_file(msg.str());
        }
        f_archive.load(in);
    }
    rt_function::pointer_t func(f_archive.find_function(name));
    if(func == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "could not find run-time function \""
            << name
            << "\" from run-time archive (it may not exist in older versions).";
        throw not_implemented(msg.str());
    }
    f_rt_functions.insert(f_rt_functions.end(), func->get_code().begin(), func->get_code().end());
}


offset_t build_file::get_current_text_offset() const
{
    return f_text.size();
}


void build_file::add_text(std::uint8_t const * text, std::size_t size)
{
    f_text.insert(f_text.end(), text, text + size);
}


void build_file::add_relocation(std::string const & name, relocation_t type, offset_t position, offset_t offset)
{
    f_relocations.push_back(relocation(name, type, position, offset));
}


void build_file::save(base_stream::pointer_t out)
{
    // compute offsets / rellocations
    //
    f_text_offset = sizeof(binary_header);

    // note: f_text and each run-time function added are already aligned to
    //       a multiple of 8 bytes (see the generate_align8() call after the
    //       frame restoration) so no need to adjust here
    //
    f_data_offset = sizeof(binary_header)
                  + f_text.size()
                  + f_rt_functions.size();

    // save the data types with the largest alignment requirements first
    //
    f_number_private_offset = f_data_offset + f_extern_variables.size() * sizeof(binary_variable);
    f_string_private_offset = f_number_private_offset + f_number_private.size();
    f_bool_private_offset = f_string_private_offset + f_string_private.size();
    f_strings_offset = f_bool_private_offset + f_bool_private.size();
    f_after_strings_offset = f_strings_offset + f_strings.size();

    for(auto const & r : f_relocations)
    {
        switch(r.get_relocation())
        {
        case relocation_t::RELOCATION_VARIABLE_32BITS:
            {
                // TODO: variables are expected to be in order so we should use
                //       a binary search instead
                //
                auto it(std::find_if(
                      f_extern_variables.begin()
                    , f_extern_variables.end()
                    , [&r, this](auto const & var)
                    {
                        char const * s(nullptr);
                        if(var.f_name_size <= sizeof(var.f_name))
                        {
                            s = reinterpret_cast<char const *>(&var.f_name);
                        }
                        else
                        {
                            s = f_strings.data() + var.f_name;
                        }
                        std::string const name(s, var.f_name_size);
                        return r.get_name() == name;
                    }));
                if(it == f_extern_variables.end())
                {
                    // this is an external variable
                    //
                    throw internal_error(
                              "could not find variable for relocation named \""
                            + r.get_name()
                            + "\".");
                }

                offset_t offset(it->f_data_size <= sizeof(it->f_data)
                        ? f_data_offset
                            + sizeof(binary_variable) * (it - f_extern_variables.begin())
                            + offsetof(binary_variable, f_data)
                            - f_text_offset
                        : it->f_data);

                // subtract position of rip at the time this offset is used
                //
                offset -= r.get_offset();

                // save the result in f_text
                //
                offset_t const idx(r.get_position());
                f_text[idx + 0] = offset >>  0;
                f_text[idx + 1] = offset >>  8;
                f_text[idx + 2] = offset >> 16;
                f_text[idx + 3] = offset >> 24;
            }
            break;

        case relocation_t::RELOCATION_RT_32BITS:
            {
                // TODO: variables are expected to be in order so we should use
                //       a binary search instead
                //
                auto it(f_rt_function_offsets.find(r.get_name()));
                if(it == f_rt_function_offsets.end())
                {
                    // this is an external variable
                    //
                    throw internal_error(
                              "could not find run-time function for relocation named \""
                            + r.get_name()
                            + "\".");
                }

                offset_t offset(f_text.size()
                                - r.get_offset()
                                + it->second);

                // save the result in f_text
                //
                offset_t const idx(r.get_position());
                f_text[idx + 0] = offset >>  0;
                f_text[idx + 1] = offset >>  8;
                f_text[idx + 2] = offset >> 16;
                f_text[idx + 3] = offset >> 24;
            }
            break;

        default:
            throw not_implemented("this relocation type is not yet implemented.");

        }
    }

    for(std::size_t idx(0); idx < f_extern_variables.size(); ++idx)
    {
        if(f_extern_variables[idx].f_name_size > sizeof(f_extern_variables[0].f_name))
        {
            f_extern_variables[idx].f_name += f_strings_offset;
        }
    }

    // save header (badc0de1)
    //
    f_header.f_variable_count = f_extern_variables.size();
    f_header.f_variables = f_data_offset; // variables are saved first
    f_header.f_start = f_text_offset;
    f_header.f_file_size = ((f_after_strings_offset + 3) & -4) + sizeof(char) * 4;
    out->write_bytes(reinterpret_cast<char const *>(&f_header), sizeof(f_header));

    // .text (i.e. binary code we want to execute)
    //
    out->write_bytes(
              reinterpret_cast<char const *>(f_text.data())
            , f_text.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_rt_functions.data())
            , f_rt_functions.size());

    // .data
    //
    out->write_bytes(
              reinterpret_cast<char const *>(f_extern_variables.data())
            , f_extern_variables.size() * sizeof(binary_variable));
    out->write_bytes(
              reinterpret_cast<char const *>(f_number_private.data())
            , f_number_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_string_private.data())
            , f_string_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_bool_private.data())
            , f_bool_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_strings.data())
            , f_strings.size());

    // clearly mark the end of the file
    //
    offset_t const adjust(4 - (f_after_strings_offset & 3));
    if(adjust != 4)
    {
        char buf[4] = {};
        out->write_bytes(buf, adjust);
    }
    out->write_bytes(g_end_magic, 4);;
}


std::size_t build_file::get_size_of_temporary_variables() const
{
    // one byte per boolean, but rounded up to the next 8 bytes
    // plus 8 bytes per integer or double
    //
    // this is what we subtract from the rsp pointer to allocate our
    // local variables
    //
    return ((f_temporary_1byte.size() + 7) & -8)
         + f_temporary_8bytes.size() * 8;
}


temporary_variable * build_file::find_temporary_variable(std::string const & name) const
{
    auto const it8(std::find_if(
          f_temporary_8bytes.begin()
        , f_temporary_8bytes.end()
        , [name](auto const & t)
        {
            return t.get_name() == name;
        }));
    if(it8 != f_temporary_8bytes.end())
    {
        return const_cast<temporary_variable *>(&*it8);
    }

    auto const it1(std::find_if(
          f_temporary_1byte.begin()
        , f_temporary_1byte.end()
        , [name](auto const & t)
        {
            return t.get_name() == name;
        }));
    if(it1 != f_temporary_1byte.end())
    {
        return const_cast<temporary_variable *>(&*it1);
    }

    return nullptr;
}








running_file::running_file()
{
}


running_file::~running_file()
{
    clean();
}


void running_file::clean()
{
    if(f_file != nullptr)
    {
        // first free variables that were allocated
        //
        for(std::size_t idx(0); idx < f_header->f_variable_count; ++idx)
        {
            binary_variable * v(f_variables + idx);
            if(v->f_type == VARIABLE_TYPE_STRING
            && (v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
            {
                free(reinterpret_cast<void *>(v->f_data));
            }
        }

        free(f_file);
    }

    // the other pointers points inside f_file, nothing to free
    // however, we still want to clear the values
    //
    f_size = 0;
    f_file = nullptr;
    f_header = nullptr;
    f_variables = nullptr;
    f_text = nullptr;
    f_protected = false;
}


bool running_file::load(std::string const & filename)
{
    clean();

    as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
    in->get_position().set_filename(filename);
    in->open(filename);
    if(!in->is_open())
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "could not open binary file \""
            << filename
            << "\".";
        throw cannot_open_file(msg.str());
    }
    return load(in);
}


bool running_file::load(base_stream::pointer_t in)
{
    clean();

    binary_header header;
    ssize_t size(in->read_bytes(reinterpret_cast<char *>(&header), sizeof(header)));
    if(size != sizeof(header))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND, in->get_position());
        msg << "could not read header.";
        return false;
    }

    long const sc_page_size(sysconf(_SC_PAGESIZE));
    f_size = (header.f_file_size + sc_page_size - 1) & -sc_page_size;
    if(posix_memalign(
          reinterpret_cast<void **>(&f_file)
        , sc_page_size
        , f_size) != 0)
    {
        throw std::bad_alloc();
    }

    memcpy(f_file, &header, sizeof(header));

    size = header.f_file_size - sizeof(header);
    if(in->read_bytes(reinterpret_cast<char *>(f_file + sizeof(header)), size) != size)
    {
        free(f_file);
        f_file = nullptr;
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND, in->get_position());
        msg << "could not read text and variables.";
        return false;
    }

    f_header = reinterpret_cast<binary_header *>(f_file);
    f_text = reinterpret_cast<std::uint8_t *>(f_header + 1);
    f_variables = reinterpret_cast<binary_variable *>(f_file + f_header->f_variables);

    return true;
}


versiontheca::versiontheca::pointer_t running_file::get_version() const
{
    versiontheca::basic::pointer_t t(std::make_shared<versiontheca::basic>());
    versiontheca::versiontheca::pointer_t version(std::make_shared<versiontheca::versiontheca>(t));
    if(f_file != nullptr)
    {
        version->set_major(f_header->f_version_major);
        version->set_minor(f_header->f_version_minor);
    }

    return version;
}


binary_variable * running_file::find_variable(std::string const & name) const
{
    if(f_variables == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "no variables defined, running_file::load() was not called or failed.";
        throw invalid_data(msg.str());
    }
    auto it(std::lower_bound(
              f_variables
            , f_variables + f_header->f_variable_count
            , name
            , [&](binary_variable const & v, std::string const & n)
            {
                char const * s(v.f_name_size <= sizeof(v.f_name)
                    ? reinterpret_cast<char const *>(&v.f_name)
                    : reinterpret_cast<char const *>(f_file + v.f_name));
                return n < std::string(s, v.f_name_size);
            }));
    if(it == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "could not find variable \""
            << name
            << "\".";
        throw invalid_data(msg.str());
    }
    return it;
}


void running_file::set_variable(std::string const & name, bool value)
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_BOOLEAN)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to set variable \""
            << name
            << "\" to a boolean value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    v->f_data_size = sizeof(value);
    v->f_data = static_cast<std::int64_t>(value);
}


void running_file::get_variable(std::string const & name, bool & value) const
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_BOOLEAN)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to get variable \""
            << name
            << "\" as a boolean value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    if(v->f_data_size != sizeof(value))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_SUPPORTED);
        msg << "variable \""
            << name
            << "\" is not set as expected (size: "
            << v->f_data_size
            << ").";
        throw incompatible_type(msg.str());
    }
    value = static_cast<bool>(v->f_data);
}


void running_file::set_variable(std::string const & name, std::int64_t value)
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_INTEGER)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to set variable \""
            << name
            << "\" to an integer value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    v->f_data_size = sizeof(value);
    v->f_data = value;
}


void running_file::get_variable(std::string const & name, std::int64_t & value) const
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_INTEGER)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to get variable \""
            << name
            << "\" as an integer value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    if(v->f_data_size != sizeof(value))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_SUPPORTED);
        msg << "variable \""
            << name
            << "\" is not set as expected (size: "
            << v->f_data_size
            << ").";
        throw incompatible_type(msg.str());
    }
    value = v->f_data;
}


void running_file::set_variable(std::string const & name, double value)
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_INTEGER)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to set variable \""
            << name
            << "\" to a double value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    v->f_data_size = sizeof(value);
    v->f_data = *reinterpret_cast<double const *>(&value);
}


void running_file::get_variable(std::string const & name, double & value) const
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_FLOATING_POINT)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to get variable \""
            << name
            << "\" as a floating point value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    if(v->f_data_size != sizeof(value))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_SUPPORTED);
        msg << "variable \""
            << name
            << "\" is not set as expected (size: "
            << v->f_data_size
            << ").";
        throw incompatible_type(msg.str());
    }

    // use intermediate pointer to avoid the strict aliasing issue
    //
    std::uint64_t const * data_ptr(&v->f_data);
    value = *reinterpret_cast<double const *>(data_ptr);
}


void running_file::set_variable(std::string const & name, std::string const & value)
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_STRING)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to set variable \""
            << name
            << "\" to a string value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0
    && v->f_data != reinterpret_cast<std::uint64_t>(nullptr))
    {
        // use intermediate pointer to avoid the strict aliasing issue
        //
        std::uint64_t * data_ptr(&v->f_data);
        free(*reinterpret_cast<void **>(data_ptr));
        v->f_flags &= ~VARIABLE_FLAG_ALLOCATED;
    }
    v->f_data_size = value.length();
    if(v->f_data_size <= sizeof(v->f_data))
    {
        memcpy(&v->f_data, value.c_str(), v->f_data_size);
    }
    else
    {
        void * str(malloc(value.length()));
        v->f_data = reinterpret_cast<std::uint64_t>(str);
        memcpy(str, value.c_str(), value.length());
        v->f_flags |= VARIABLE_FLAG_ALLOCATED;
    }
}


void running_file::get_variable(std::string const & name, std::string & value) const
{
    binary_variable * v(find_variable(name));
    if(v->f_type != VARIABLE_TYPE_STRING)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "trying to get variable \""
            << name
            << "\" as a string value when the variable is of type: \""
            << variable_type_to_string(v->f_type)
            << "\".";
        throw incompatible_type(msg.str());
    }
    if(v->f_data_size != sizeof(value))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_SUPPORTED);
        msg << "variable \""
            << name
            << "\" is not set as expected (size: "
            << v->f_data_size
            << ").";
        throw incompatible_type(msg.str());
    }
    if(v->f_data_size <= sizeof(v->f_data))
    {
        value = std::string(reinterpret_cast<char const *>(&v->f_data), v->f_data_size);
    }
    else if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
    {
        value = std::string(v->f_data, v->f_data_size);
    }
    else
    {
        value = std::string(reinterpret_cast<char const *>(f_file + v->f_data), v->f_data_size);
    }
}


std::size_t running_file::variable_size() const
{
    if(f_header == nullptr)
    {
        throw invalid_data("running_file has no data.");
    }
    return f_header->f_variable_count;
}


binary_variable * running_file::get_variable(int index, std::string & name) const
{
    name.clear();
    if(index < 0)
    {
        throw out_of_range("running_file::get_variable() called with a negative index.");
    }
    if(f_header == nullptr)
    {
        throw invalid_data("running_file has no data.");
    }
    if(index >= f_header->f_variable_count)
    {
        return nullptr;
    }
    binary_variable * v(f_variables + index);
    char const * s(v->f_name_size <= sizeof(v->f_name)
                    ? reinterpret_cast<char const *>(&v->f_name)
                    : reinterpret_cast<char const *>(f_file + v->f_name));
    name = std::string(s, v->f_name_size);
    return v;
}


void running_file::run(binary_result & result)
{
    typedef long long (*entry_point)();

    if(f_header == nullptr)
    {
        throw invalid_data("running_file has no data.");
    }

    if(!f_protected)
    {
        int const r(mprotect(f_file, f_size, PROT_READ | PROT_WRITE | PROT_EXEC));
        if(r != 0)
        {
            throw execution_error("the file could not be protected for execution.");
        }
        f_protected = true;
    }

    // TODO: determine type at time of saving and switch over that type here
    //
    result.set_integer(reinterpret_cast<entry_point>(f_text)());
    result.set_type(VARIABLE_TYPE_INTEGER);
}








void binary_result::set_type(variable_type_t type)
{
    f_type = type;
}


variable_type_t binary_result::get_type() const
{
    return f_type;
}


void binary_result::set_boolean(bool value)
{
    f_type = VARIABLE_TYPE_BOOLEAN;
    f_value[0] = static_cast<std::uint64_t>(value);
}


bool binary_result::get_boolean() const
{
    return static_cast<bool>(f_value[0]);
}


void binary_result::set_integer(std::int64_t value)
{
    f_type = VARIABLE_TYPE_INTEGER;
    f_value[0] = value;
}


std::int64_t binary_result::get_integer() const
{
    return f_value[0];
}


void binary_result::set_floating_point(double value)
{
    f_type = VARIABLE_TYPE_FLOATING_POINT;

    // use intermediate pointer to avoid the strict aliasing issue
    //
    double const * value_ptr(&value);
    f_value[0] = *reinterpret_cast<std::uint64_t const *>(value_ptr);
}


double binary_result::get_floating_point() const
{
    std::uint64_t const * value_ptr(f_value + 0);
    return *reinterpret_cast<double const *>(value_ptr);
}


void binary_result::set_string(std::string const & value)
{
    f_type = VARIABLE_TYPE_STRING;
    f_string = value;
}


std::string binary_result::get_string() const
{
    return f_string;
}















binary_assembler::binary_assembler(
          base_stream::pointer_t output
        , options::pointer_t options
        , std::string const & rt_functions_oar)
    : f_output(output)
    , f_options(options)
{
    if(!rt_functions_oar.empty())
    {
        f_rt_functions_oar = rt_functions_oar;
    }
}


base_stream::pointer_t binary_assembler::get_output()
{
    return f_output;
}


options::pointer_t binary_assembler::get_options()
{
    return f_options;
}


int binary_assembler::output(node::pointer_t root)
{
    int const save_errcnt(error_count());

std::cerr << "----- start flattening...\n";
    flatten_nodes::pointer_t fn(flatten(root));
std::cerr << "----- end flattening... (" << fn->get_operations().size() << ")\n";

    if(fn != nullptr)
    {
        // generate binary output
        //
std::cerr << "----- start generating... (" << fn->get_operations().size() << ")\n";
        generate_amd64_code(fn);
std::cerr << "----- end generating... (" << fn->get_operations().size() << ")\n";

        // it worked, save the results
        //
std::cerr << "----- start saving... (" << fn->get_operations().size() << ")\n";
        f_file.save(f_output);
std::cerr << "----- end saving... (" << fn->get_operations().size() << ")\n";
    }

    return error_count() - save_errcnt;
}


void binary_assembler::generate_amd64_code(flatten_nodes::pointer_t fn)
{
    // clear the existing file
    //
    f_file = {};

    // on entry setup esp & rbp
    //
    std::uint8_t setup_frame[] = {
        0x55,           // PUSH rpb
        0x48,           // MOV rbp := rsp
        0x89,
        0xE5,
    };
    f_file.add_text(setup_frame, sizeof(setup_frame));

    for(auto const & it : fn->get_variables())
    {
        if(it.second->is_temporary())
        {
            // at this point, a temporary variable does not have a value,
            // only a type and a name; temporaries automatically have a
            // STORE before a LOAD
            //
            f_file.add_temporary_variable(it.first, it.second);
        }
        else if(it.second->is_extern())
        {
            f_file.add_extern_variable(it.first, it.second);
        }
        else
        {
            f_file.add_private_variable(it.first, it.second);
        }
    }

    offset_t const temp_size(f_file.get_size_of_temporary_variables());
    if(temp_size > 0)
    {
        // we need to reserve the space for our temporary variables
        //
        if(temp_size < 128)
        {
            std::uint8_t buf[] = {
                0x48,
                0x83,
                0xEC,
                static_cast<std::uint8_t>(temp_size),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        else
        {
            std::uint8_t buf[] = {
                0x48,
                0x81,
                0xEC,
                static_cast<std::uint8_t>(temp_size >>  0),
                static_cast<std::uint8_t>(temp_size >>  8),
                static_cast<std::uint8_t>(temp_size >> 16),
                static_cast<std::uint8_t>(temp_size >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
    }

    for(auto const & it : fn->get_operations())
    {
std::cerr << "  ++  " << it->to_string() << "\n";
        switch(it->get_operation())
        {
        case node_t::NODE_ADD:
            generate_add_sub(it, true);
            break;

        case node_t::NODE_BITWISE_NOT:
            generate_bitwise_not(it);
            break;

        case node_t::NODE_MULTIPLY:
            generate_multiply(it);
            break;

        case node_t::NODE_POWER:
            generate_power(it);
            break;

        case node_t::NODE_SUBTRACT:
            generate_add_sub(it, false);
            break;

        default:
            throw not_implemented("operation was not yet implemented");

        }
    }

    // on exit, restore frame and return
    //
    if(temp_size > 0)
    {
        // we need to restore the stack pointer first
        //
        if(temp_size < 128)
        {
            std::uint8_t buf[] = {
                0x48,
                0x83,
                0xC4,
                static_cast<std::uint8_t>(temp_size),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        else
        {
            std::uint8_t buf[] = {
                0x48,
                0x81,
                0xC4,
                static_cast<std::uint8_t>(temp_size >>  0),
                static_cast<std::uint8_t>(temp_size >>  8),
                static_cast<std::uint8_t>(temp_size >> 16),
                static_cast<std::uint8_t>(temp_size >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
    }
    std::uint8_t restore_frame[] = {  // POP rbp
        0x5D,
        0xC3,
    };
    f_file.add_text(restore_frame, sizeof(restore_frame));

    generate_align8();
}


void binary_assembler::generate_align8()
{
    switch(f_file.get_current_text_offset() & 7)
    {
    case 0:
        // already aligned
        break;

    case 8 - 1:
        {
            std::uint8_t nop[] = {
                0x90,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 2:
        {
            std::uint8_t nop[] = {
                0x66,
                0x90,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 3:
        {
            std::uint8_t nop[] = {
                0x0F,
                0x1F,
                0x00,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 4:
        {
            std::uint8_t nop[] = {
                0x0F,
                0x1F,
                0x40,
                0x00,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 5:
        {
            std::uint8_t nop[] = {
                0x0F,
                0x1F,
                0x44,
                0x00,
                0x00,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 6:
        {
            std::uint8_t nop[] = {
                0x66,
                0x0F,
                0x1F,
                0x44,
                0x00,
                0x00,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    case 8 - 7:
        {
            std::uint8_t nop[] = {
                0x0F,
                0x1F,
                0x80,
                0x00,
                0x00,
                0x00,
                0x00,
            };
            f_file.add_text(nop, sizeof(nop));
        }
        break;

    }
}




void binary_assembler::generate_load(data::pointer_t d, register_t reg)
{
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_INTEGER:
        {
            std::int64_t value(d->get_node()->get_integer().get());
            switch(get_smallest_size(value))
            {
            case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            case integer_size_t::INTEGER_SIZE_64BITS:
                {
                    std::uint8_t buf[] = {
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),       // 64 bits
                        static_cast<std::uint8_t>(0xB8 | (static_cast<int>(reg) & 7)),   // MOV r := imm64
                        static_cast<std::uint8_t>(value >>  0),
                        static_cast<std::uint8_t>(value >>  8),
                        static_cast<std::uint8_t>(value >> 16),
                        static_cast<std::uint8_t>(value >> 24),
                        static_cast<std::uint8_t>(value >> 32),
                        static_cast<std::uint8_t>(value >> 40),
                        static_cast<std::uint8_t>(value >> 48),
                        static_cast<std::uint8_t>(value >> 56),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }
                break;

            default:
                // in 64bit the sign extend works with 32 bits so all others
                // can make use of the 32 bit value
                //
                // on the other hand, the 16 & 8 bit immediate do not get
                // extended; the upper part of the register remains unchanged
                //
                // Note: the MOVSX/MOVZX do not work with immediate values
                {
                    std::uint8_t buf[] = {
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),       // 64 bits
                        0xC7,
                        static_cast<std::uint8_t>(0xC0 | (static_cast<int>(reg) & 7)),   // MOV r := imm32
                        static_cast<std::uint8_t>(value >>  0),
                        static_cast<std::uint8_t>(value >>  8),
                        static_cast<std::uint8_t>(value >> 16),
                        static_cast<std::uint8_t>(value >> 24),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }
                break;

            }
        }
        break;

    case node_t::NODE_VARIABLE:
        if(d->is_temporary())
        {
            std::string const name(n->get_string());
            temporary_variable * temp_var(f_file.find_temporary_variable(name));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary not found in generate_load()");
            }
            switch(temp_var->get_size())
            {
            case 8: // Integer / Double
                {
                    ssize_t const offset(temp_var->get_offset());
                    integer_size_t const offset_size(get_smallest_size(offset));
                    switch(offset_size)
                    {
                    case INTEGER_SIZE_1BIT:
                    case INTEGER_SIZE_8BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                                            // 64 bits
                                0x8B,                       // MOV r := m
                                static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                                            // 'r' and disp(rbp) (r/m)
                                static_cast<std::uint8_t>(offset),
                                                            // 8 bit offset
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    case INTEGER_SIZE_8BITS_UNSIGNED:
                    case INTEGER_SIZE_16BITS_SIGNED:
                    case INTEGER_SIZE_16BITS_UNSIGNED:
                    case INTEGER_SIZE_32BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                                            // 64 bits
                                0x8B,                       // MOV r := m
                                static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                                            // 'r' and disp(rbp) (r/m)
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
                                                            // 32 bit offset
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    default:
                        // x86-64 only supports disp8 and disp32
                        //
                        // for larger offsets we would need to use an
                        // index register; but we should never go over
                        // disp32 on the stack anyway since it's only 2Mb
                        //
                        throw not_implemented("offset size not yet supported for \""
                            + temp_var->get_name()
                            + "\" (type: "
                            + std::to_string(static_cast<int>(offset_size))
                            + " for size: "
                            + std::to_string(offset)
                            + ").");

                    }
                }
                break;

            default:
                throw not_implemented("temporary size not yet supported in generate_load()");

            }
        }
        else if(d->is_extern())
        {
            std::size_t const pos(f_file.get_current_text_offset());
            std::uint8_t buf[] = {
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                            // 64 bits
                0x8B,                       // MOV r := m
                static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                                            // 'r' and disp(rip) (r/m)
                0x00,                       // 32 bit offset
                0x00,
                0x00,
                0x00,
            };
            f_file.add_text(buf, sizeof(buf));
            f_file.add_relocation(
                      d->get_string()
                    , relocation_t::RELOCATION_VARIABLE_32BITS
                    , pos + 3
                    , f_file.get_current_text_offset());
        }
        else
        {
std::cerr << "--- WARNING: generate_load() hit a variable type not yet implemented...\n";
        }
        break;

    default:
std::cerr << "--- WARNING: generate_load() hit a data type other than already implemented...\n";
        break;

    }
}


void binary_assembler::generate_store(data::pointer_t d, register_t reg)
{
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_VARIABLE:
        {
            std::string const name(n->get_string());
            if(d->is_temporary())
            {
                // TODO: add a map so we can find these at once instead of
                //       having a search through all those vars.
                //
                temporary_variable * temp_var(f_file.find_temporary_variable(name));
                if(temp_var == nullptr)
                {
                    throw internal_error("temporary not found in generate_store()");
                }
                switch(temp_var->get_size())
                {
                case 8: // Integer / Double
                    {
                        ssize_t const offset(temp_var->get_offset());
                        integer_size_t const offset_size(get_smallest_size(offset));
                        switch(offset_size)
                        {
                        case INTEGER_SIZE_1BIT:
                        case INTEGER_SIZE_8BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                                                // 64 bits
                                    0x89,                       // MOV m := r
                                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                                                // 'r' and disp(rbp) (r/m)
                                    static_cast<std::uint8_t>(offset),
                                                                // 8 bit offset
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        case INTEGER_SIZE_8BITS_UNSIGNED:
                        case INTEGER_SIZE_16BITS_SIGNED:
                        case INTEGER_SIZE_16BITS_UNSIGNED:
                        case INTEGER_SIZE_32BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                                                // 64 bits
                                    0x89,                       // MOV m := r
                                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                                                // 'r' and disp(rbp) (r/m)
                                    static_cast<std::uint8_t>(offset >>  0),
                                    static_cast<std::uint8_t>(offset >>  8),
                                    static_cast<std::uint8_t>(offset >> 16),
                                    static_cast<std::uint8_t>(offset >> 24),
                                                                // 32 bit offset
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        default:
                            // x86-64 only supports disp8 and disp32
                            //
                            // for larger offsets we would need to use an
                            // index register; but we should never go over
                            // disp32 on the stack anyway since it's only 2Mb
                            //
                            throw not_implemented("offset size not supported yet in "
                                + temp_var->get_name()
                                + " (type: "
                                + std::to_string(static_cast<int>(offset_size))
                                + " for size: "
                                + std::to_string(offset)
                                + ").");

                        }
                    }
                    break;

                default:
                    throw not_implemented("temporary size not yet supported in generate_store()");

                }
            }
            else
            {
std::cerr << "--- WARNING: generate_store() unhandled variable type...\n";
            }
        }
        break;

    default:
std::cerr << "--- WARNING: generate_store() hit a data type other than already implemented...\n";
        break;

    }
}


void binary_assembler::generate_add_sub(operation::pointer_t op, bool add)
{
    data::pointer_t lhs(op->get_left_handside());
    generate_load(lhs, register_t::REGISTER_RAX);

    data::pointer_t rhs(op->get_right_handside());
    switch(rhs->get_integer_size())
    {
    case integer_size_t::INTEGER_SIZE_1BIT:
    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,       // 64 bits
                0x83,       // ADD or SUB r64 +/-= imm8
                static_cast<std::uint8_t>(add ? 0xC0 : 0xE8),
                            // r/m
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get()),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:        // there is no r64 + imm16, use r64 + imm32 instead
    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,       // 64 bits
                static_cast<std::uint8_t>(add ? 0x05 : 0x2D),
                            // ADD or SUB rax +/-= imm32
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >>  0),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >>  8),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >> 16),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_64BITS:
        {
            generate_load(rhs, register_t::REGISTER_RDX);
            std::uint8_t buf[] = {
                0x48,       // ADD or SUB rax +/-= rdx
                static_cast<std::uint8_t>(add ? 0x01 : 0x29),
                0xD0,
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    default:
        throw not_implemented("integer size not yet implementedd in generate_add_sub().");

    }

    generate_store(op->get_result(), register_t::REGISTER_RAX);
}


void binary_assembler::generate_bitwise_not(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    generate_load(lhs, register_t::REGISTER_RAX);

    std::uint8_t buf[] = {
        0x48,       // 64 bits
        0xF7,       // ADD or SUB r64 +/-= imm8
        0xD0,
    };
    f_file.add_text(buf, sizeof(buf));

    generate_store(op->get_result(), register_t::REGISTER_RAX);
}


void binary_assembler::generate_multiply(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    generate_load(lhs, register_t::REGISTER_RAX);

    data::pointer_t rhs(op->get_right_handside());
    switch(rhs->get_integer_size())
    {
    case integer_size_t::INTEGER_SIZE_1BIT:
    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,       // 64 bits
                0x6B,       // IMUL r64 *= imm8
                0xC0,       // r/m
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get()),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:        // there is no r64 + imm16, use r64 + imm32 instead
    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,       // 64 bits
                0x69,       // IMUL r64 *= imm32
                0xC0,       // r/m
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >>  0),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >>  8),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >> 16),
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get() >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_64BITS:
        {
            generate_load(rhs, register_t::REGISTER_RDX);
            std::uint8_t buf[] = {
                0x48,       // IMUL rax *= rdx
                0x0F,
                0xAF,
                0xC2,
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_UNKNOWN:
        // this is not an integer
        //
        switch(rhs->get_data_type())
        {
        case node_t::NODE_VARIABLE:
            {
                generate_load(rhs, register_t::REGISTER_RDX);
                std::uint8_t buf[] = {
                    0x48,       // IMUL rax *= rdx
                    0x0F,
                    0xAF,
                    0xC2,
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        default:
            throw not_implemented("non-integer node not yet handled in generate_multiply().");

        }
        break;

    default:
        throw not_implemented("integer size not yet implemented in generate_multiply().");

    }

    generate_store(op->get_result(), register_t::REGISTER_RAX);
}


void binary_assembler::generate_power(operation::pointer_t op)
{
    f_file.add_rt_function(f_rt_functions_oar, "power");

    data::pointer_t lhs(op->get_left_handside());
    generate_load(lhs, register_t::REGISTER_RDI);

    data::pointer_t rhs(op->get_right_handside());
    generate_load(rhs, register_t::REGISTER_RSI);

    std::size_t const pos(f_file.get_current_text_offset());
    std::uint8_t buf[] = {
        0xE8,       // CALL disp32(rip)
        0x00,
        0x00,
        0x00,
        0x00,
    };
    f_file.add_text(buf, sizeof(buf));
    f_file.add_relocation(
              "power"
            , relocation_t::RELOCATION_RT_32BITS
            , pos + 1
            , f_file.get_current_text_offset());

    generate_store(op->get_result(), register_t::REGISTER_RAX);
}



} // namespace as2js
// vim: ts=4 sw=4 et
