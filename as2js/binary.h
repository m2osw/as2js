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
#pragma once

/** \brief Transforms code to binary a x86-64 can run.
 *
 * This set of classes are used to generate binary code that can be executed
 * inately on your Intel or AMD processor.
 *
 * The code is outputted to a binary file with a small header, a .text
 * section, and a .data section. These are using our own format so that
 * we can do that work without any dependency a computer wouldn't have.
 *
 * To see the assembly code, you can use the objdump tool this way:
 *
 * \code
 * dd ibs=1 skip=16 if=a.out of=b.out
 * objdump -b binary -m i386:x86-64 -D b.out | less
 * \endcode
 */

// self
//
#include    <as2js/node.h>
#include    <as2js/options.h>
#include    <as2js/output.h>
#include    <as2js/stream.h>



// versiontheca
//
#include    <versiontheca/versiontheca.h>



namespace as2js
{


// magic bytes (B0 is at offset 0, etc.)
//
constexpr std::uint32_t BINARY_MAGIC_B0 = 0xBA;
constexpr std::uint32_t BINARY_MAGIC_B1 = 0xDC;
constexpr std::uint32_t BINARY_MAGIC_B2 = 0x0D;
constexpr std::uint32_t BINARY_MAGIC_B3 = 0xE1;


// version found in the header
//
constexpr std::uint8_t  BINARY_VERSION_MAJOR = 1;
constexpr std::uint8_t  BINARY_VERSION_MINOR = 0;


typedef std::uint32_t                       offset_t;
typedef std::map<std::string, offset_t>     offset_map_t;


struct binary_header
{
    std::uint8_t        f_magic[4] = { BINARY_MAGIC_B0, BINARY_MAGIC_B1, BINARY_MAGIC_B2, BINARY_MAGIC_B3 };
    std::uint8_t        f_version_major = BINARY_VERSION_MAJOR;
    std::uint8_t        f_version_minor = BINARY_VERSION_MINOR;
    std::uint16_t       f_variable_count = 0;
    offset_t            f_variables = 0;
    offset_t            f_start = 0;
};

// the code (.text) starts right after the header and we want it aligned to
// 8 bytes so the size of the header must be a multiple of 8
//
static_assert((sizeof(binary_header) & (64 / 8 - 1)) == 0);



enum variable_type_t : std::uint16_t
{
    VARIABLE_TYPE_UNKNOWN,
    VARIABLE_TYPE_BOOLEAN,
    VARIABLE_TYPE_INTEGER,
    VARIABLE_TYPE_FLOATING_POINT,
    VARIABLE_TYPE_STRING,
    // TODO: add all the other basic types (i.e. Date, Array, etc.)
};


enum class relocation_t
{
    RELOCATION_VARIABLE_32BITS,
    RELOCATION_DATA_32BITS,
    RELOCATION_CONSTANT_32BITS,
    RELOCATION_RT_32BITS,
};


enum class register_t
{
    REGISTER_RAX = 0,
    REGISTER_RCX = 1,
    REGISTER_RDX = 2,
    REGISTER_RBX = 3,
    REGISTER_RSP = 4,
    REGISTER_RBP = 5,
    REGISTER_RSI = 6,
    REGISTER_RDI = 7,

    // uses 0x49 instead of 0x48
    REGISTER_R8  = 8,
    REGISTER_R9  = 9,
    REGISTER_R10 = 10,
    REGISTER_R11 = 11,
    REGISTER_R12 = 12,
    REGISTER_R13 = 13,
    REGISTER_R14 = 14,
    REGISTER_R15 = 15,
};


typedef std::uint16_t               variable_flags_t;

constexpr variable_flags_t const    VARIABLE_FLAG_DEFAULT   = 0x0000;
constexpr variable_flags_t const    VARIABLE_FLAG_ALLOCATED = 0x0001; // while running, we may allocate a string


struct binary_variable
{
    typedef std::vector<binary_variable>    vector_t;

    variable_type_t     f_type = VARIABLE_TYPE_UNKNOWN;
    variable_flags_t    f_flags = VARIABLE_FLAG_DEFAULT;
    std::uint16_t       f_pad = 0;
    std::uint16_t       f_name_size = 0;
    offset_t            f_name = 0;
    std::uint32_t       f_data_size = 0;
    std::uint64_t       f_data = 0;     // if f_data_size <= sizeof(f_data), it is defined here, otherwise, it is an offset to the data
};
static_assert((sizeof(binary_variable) & (64 / 8 - 1)) == 0);


class temporary_variable
{
public:
    typedef std::vector<temporary_variable>     vector_t;

                        temporary_variable(
                                  std::string const & name
                                , node_t type
                                , std::size_t size
                                , ssize_t offset);

    std::string const & get_name() const;
    node_t              get_type() const;
    std::size_t         get_size() const;
    ssize_t             get_offset() const;

private:
    std::string         f_name = std::string();
    node_t              f_type = node_t::NODE_UNKNOWN;
    std::size_t         f_size = 0ULL;
    ssize_t             f_offset = 0ULL;
};


class relocation
{
public:
    typedef std::vector<relocation>     vector_t;

                        //relocation();
                        relocation(
                                  std::string const & name
                                , relocation_t type
                                , offset_t position
                                , offset_t offset);

    std::string         get_name() const;
    relocation_t        get_relocation() const;
    offset_t            get_position() const;
    offset_t            get_offset() const;

private:
    std::string         f_name = std::string();
    relocation_t        f_relocation = relocation_t::RELOCATION_VARIABLE_32BITS;
    offset_t            f_position = 0ULL;
    offset_t            f_offset = 0ULL;
};



typedef std::vector<std::uint8_t>       text_t;



struct variable_t
{
    std::uint32_t       f_flags = VARIABLE_FLAG_DEFAULT;
    std::uint32_t       f_size = 0;
    std::uint64_t       f_string = 0;       // once loaded this becomes a pointer to char *
};



class build_file
{
public:
    void                        add_extern_variable(std::string const & name, data::pointer_t type);
    void                        add_temporary_variable(std::string const & name, data::pointer_t type);
    void                        add_private_variable(std::string const & name, data::pointer_t type);
    void                        add_constant(std::string & name, double value);
    void                        add_constant(std::string & name, std::string value);
    void                        add_rt_function(std::string const & name);

    std::size_t                 get_size_of_temporary_variables() const;
    temporary_variable *        find_temporary_variable(std::string const & name) const;

    offset_t                    get_current_text_offset() const;
    void                        add_text(std::uint8_t const * text, std::size_t size);

    void                        add_relocation(
                                          std::string const & name
                                        , relocation_t relocation
                                        , offset_t position
                                        , offset_t offset);

    void                        save(base_stream::pointer_t out);

private:
    binary_variable *           new_binary_variable(std::string const & name, variable_type_t type, std::size_t size);

    binary_header               f_header = binary_header();
    relocation::vector_t        f_relocations = relocation::vector_t();
    binary_variable::vector_t   f_extern_variables = binary_variable::vector_t();
    temporary_variable::vector_t
                                f_temporary_1byte = temporary_variable::vector_t();
    temporary_variable::vector_t
                                f_temporary_8bytes = temporary_variable::vector_t();
    std::vector<char>           f_strings = std::vector<char>();
    text_t                      f_text = text_t();
    offset_map_t                f_private_offsets = offset_map_t(); // private data is separated by size for alignment (packing) reason
    text_t                      f_bool_private = text_t();          // bool
    text_t                      f_number_private = text_t();        // int64_t/double
    text_t                      f_string_private = text_t();        // variable_t
    offset_map_t                f_rt_function_offsets = offset_map_t();
    text_t                      f_rt_functions = text_t();
    std::size_t                 f_next_const_string = 0;
    offset_t                    f_text_offset = 0;
    offset_t                    f_data_offset = 0;
    offset_t                    f_number_private_offset = 0;
    offset_t                    f_string_private_offset = 0;
    offset_t                    f_bool_private_offset = 0;
    offset_t                    f_strings_offset = 0;
    offset_t                    f_after_strings_offset = 0;
};



class binary_result
{
public:
    void                        set_type(variable_type_t type);
    variable_type_t             get_type() const;

    void                        set_boolean(bool value);
    bool                        get_boolean() const;

    void                        set_integer(std::int64_t value);
    std::int64_t                get_integer() const;

    void                        set_floating_point(double value);
    double                      get_floating_point() const;

    void                        set_string(std::string const & value);
    std::string                 get_string() const;

private:
    variable_type_t             f_type = VARIABLE_TYPE_UNKNOWN;
    std::uint64_t               f_value[2] = {};        // for dates we'll want 2 int64 once we have that (I think)
    std::string                 f_string = std::string();
};



class running_file
{
public:
                                running_file(running_file const &) = delete;
    running_file &              operator = (running_file const &) = delete;

    // load binary file
    //
    bool                        load(std::string const & filename);
    versiontheca::versiontheca  get_version() const;

    // prepare variables
    //
    void                        set_variable(std::string const & name, bool value);
    void                        set_variable(std::string const & name, std::int64_t value);
    void                        set_variable(std::string const & name, double value);
    void                        set_variable(std::string const & name, std::string const & value);

    // run the code
    //
    void                        run(binary_result & result);

private:
    text_t                      f_text = text_t();      // this is really the entire file
    binary_header *             f_header = nullptr;     // pointer at the start of f_text
    binary_variable *           f_variables = nullptr;  // pointer to variables within f_text
};



class binary_assembler
{
public:
    typedef std::shared_ptr<binary_assembler>      pointer_t;

                                binary_assembler(
                                      base_stream::pointer_t output
                                    , options::pointer_t options);

    base_stream::pointer_t      get_output();
    options::pointer_t          get_options();

    int                         output(node::pointer_t root);

private:
    void                        generate_amd64_code(flatten_nodes::pointer_t fn);
    void                        generate_align8();
    void                        generate_load(data::pointer_t d, register_t reg);
    void                        generate_store(data::pointer_t d, register_t reg);
    void                        generate_add_sub(operation::pointer_t op, bool add);
    void                        generate_bitwise_not(operation::pointer_t op);
    void                        generate_multiply(operation::pointer_t op);

    base_stream::pointer_t      f_output = base_stream::pointer_t();
    options::pointer_t          f_options = options::pointer_t();
    build_file                  f_file = build_file();
};



} // namespace as2js
// vim: ts=4 sw=4 et