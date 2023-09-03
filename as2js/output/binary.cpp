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
#include    <snapdev/join_strings.h>
#include    <snapdev/math.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/number_to_string.h>
#include    <snapdev/safe_object.h>
#include    <snapdev/version.h>


// libutf8
//
#include    <libutf8/base.h>
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


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



/** \file
 * \brief Encode our script directly to text that can be executed.
 *
 * This file encodes the nodes directly to text (i.e. binary code). The
 * result is executable code that an x86-64 CPU can execute directly.
 *
 * The instruction encoding of x86-64 is as following:
 *
 * \code
 *     [prefixes]     zero or more prefixes (LOCK, REPcc, REX.n, SIMD, etc.)
 *     [VEX]          the vex prefix for extended AVX instructions (0xC4/0xC5)
 *     OPCODE         the instruction, one, two, or three bytes
 *     [Mod R/M]      zero or one Mode, Register, Memory
 *     [SIB]          zero or one S/Index/Base
 *     [DISP]         displacement of 0, 1, 2, or 4 bytes
 *     [IMM]          zero to eight bytes (0, 1, 2, 4, or 8)
 * \endcode
 *
 * The number of bytes supported by the various entries depends on the
 * prefixes and opcode.
 *
 * Call ABI for AMD64 is found in "mpx-linux64-abi.pdf".
 */


namespace as2js
{

namespace
{



constexpr char const g_end_magic[] = { 'E', 'N', 'D', '!' };


void display_binary_variable(binary_variable const * v, int indent = 0)
{
    auto show_flags = [&v]()
    {
        if(v->f_flags != 0)
        {
            std::list<std::string> flags;
            if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
            {
                flags.push_back("ALLOCATED");
            }
            std::cout << " (" << snapdev::join_strings(flags, ", ") << ")";
        }

        std::cout << "\n";
    };

    std::string left_indent(indent * 2, ' ');
    switch(v->f_type)
    {
    case VARIABLE_TYPE_UNKNOWN:
        std::cerr << "error: found UNKNOWN binary variable.\n";
        return;

    case VARIABLE_TYPE_BOOLEAN:
        std::cout
            << left_indent
            << "* BOOLEAN: "
            << (v->f_data != 0 ? "true" : "false");
        if(v->f_data_size != sizeof(bool))
        {
            std::cout << " [WRONG SIZE]";
        }
        break;

    case VARIABLE_TYPE_INTEGER:
        std::cout
            << left_indent
            << "* INTEGER: "
            << v->f_data;
        if(v->f_data_size != sizeof(std::int64_t))
        {
            std::cout << " [WRONG SIZE]";
        }
        break;

    case VARIABLE_TYPE_FLOATING_POINT:
        {
            std::uint64_t const * value(&v->f_data);
            std::cout
                << left_indent
                << "* FLOATING POINT: "
                << *reinterpret_cast<double const *>(value);
            if(v->f_data_size != sizeof(double))
            {
                std::cout << " [WRONG SIZE]";
            }
        }
        break;

    case VARIABLE_TYPE_STRING:
        std::cout
            << left_indent
            << "* STRING: ";
        if(v->f_data_size <= sizeof(v->f_data))
        {
            std::cout << std::string(reinterpret_cast<char const *>(&v->f_data), v->f_data_size);
        }
        else
        {
            std::cout << std::string(reinterpret_cast<char const *>(v->f_data), v->f_data_size);
        }
        break;

    case VARIABLE_TYPE_RANGE:
        std::cerr << "error: found RANGE binary variable, which is not yet fully supported.\n";
        return;

    case VARIABLE_TYPE_ARRAY:
        {
            binary_variable::vector_of_pointers_t const * items(reinterpret_cast<binary_variable::vector_of_pointers_t const *>(v->f_data));
            std::cout
                << "* ARRAY: "
                << items->size()
                << " items";
            if(v->f_data_size != sizeof(binary_variable::vector_of_pointers_t const *))
            {
                std::cout << " [WRONG SIZE: " << v->f_data_size << " instead of " << sizeof(binary_variable::vector_of_pointers_t const *) << "]";
            }
            show_flags();

            ++indent;
            for(auto i : *items)
            {
                display_binary_variable(i, indent);
            }
        }
        return;

    }

    show_flags();
}


extern "C" {
std::int64_t ipow(std::int64_t x, std::int64_t y) noexcept
{
    return snapdev::pow(x, y);
}


void delete_buffer(char * ptr)
{
    free(ptr);
}


void strings_initialize(binary_variable * v)
{
    v->f_type = VARIABLE_TYPE_STRING;
    v->f_flags = VARIABLE_FLAG_DEFAULT;
    v->f_name = 0;              // TODO: add the name (for debug purposes)
    v->f_name_size = 0;
    v->f_data = 0;
    v->f_data_size = 0;
}


void strings_free(binary_variable * v)
{
#ifdef _DEBUG
    if(v->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type(
              "v is expected to be a string in strings_free(), found \""
            + std::string(variable_type_to_string(v->f_type))
            + "\" instead.");
    }
#endif

    if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
    {
        v->f_flags &= ~VARIABLE_FLAG_ALLOCATED;

//std::cerr << "--- strings_free() called with an allocated string of "
//<< v->f_data_size << " chars at " << reinterpret_cast<void *>(v->f_data) << "\n";
//char * ptr(reinterpret_cast<char *>(v->f_data));
//for(std::uint64_t idx(0); idx < v->f_data_size; ++idx)
//{
//std::cerr << "--- char at " << idx << " is '" << (ptr[idx] >= 0x20 && ptr[idx] < 0x7F ? ptr[idx] : '?')
//<< "' -- 0x" << std::hex << static_cast<int>(ptr[idx]) << std::dec << "\n";
//}

        free(reinterpret_cast<void *>(v->f_data));
        v->f_data = 0; // a.k.a. "nullptr"
        v->f_data_size = 0;
    }
}


void strings_copy(binary_variable * d, binary_variable const * s)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type(
              "d is expected to be a STRING in strings_copy(), not "
            + std::to_string(static_cast<int>(d->f_type)));
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_copy()");
    }
#endif

    if(d == s)
    {
        return;
    }
    strings_free(d);

    if((s->f_flags & VARIABLE_FLAG_ALLOCATED) == 0)
    {
        // not allocated, we can copy as is
        //
        d->f_flags &= ~VARIABLE_FLAG_ALLOCATED;
        d->f_data_size = s->f_data_size;
        d->f_data = s->f_data;
    }
    else if(s->f_data_size <= sizeof(s->f_data))
    {
        // TODO: this is probably a bug? s should not be allocated if
        //       the string fits in f_data
        d->f_flags &= ~VARIABLE_FLAG_ALLOCATED;
        d->f_data_size = s->f_data_size;
        memcpy(&d->f_data, reinterpret_cast<char const *>(s->f_data), s->f_data_size);
    }
    else
    {
        // the source is allocated, we need to duplicate the buffer
        // TODO: implement references
        //
        char * str(static_cast<char *>(malloc(s->f_data_size)));
        if(str == nullptr)
        {
            throw std::bad_alloc();
        }
        memcpy(str, reinterpret_cast<char const *>(s->f_data), s->f_data_size);

        d->f_type = VARIABLE_TYPE_STRING;
        d->f_flags = VARIABLE_FLAG_ALLOCATED;
        d->f_data_size = s->f_data_size;
        d->f_data = reinterpret_cast<std::uint64_t>(str);
    }
}


std::int64_t strings_compare(binary_variable const * s1, binary_variable const * s2, node_t op)
{
    //case node_t::NODE_SMART_MATCH: -- TBD how do we handle this one? if I'm correct this means rhs is of type regexp which we do not yet support here

#ifdef _DEBUG
    if(s1->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s1 is expected to be a string in strings_compare()");
    }
    if(s2->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s2 is expected to be a string in strings_compare()");
    }
#endif

    if(op == node_t::NODE_ALMOST_EQUAL)
    {
        throw not_implemented("string almost equal require a libutf-8 uppercase transformation which we don't have yet, use == instead");
    }
    else
    {
        // JavaScript compares strings using UTF-16 which is important
        // if we want to keep the same sort order as JavaScript (which
        // at this point we do want...) so this is rather slow since it
        // first converts the strings to UTF-16 then does the compare
        //
        std::u16string lhs;
        if(s1->f_data_size <= sizeof(s1->f_data))
        {
            lhs = libutf8::to_u16string(std::string(reinterpret_cast<char const *>(&s1->f_data), s1->f_data_size));
        }
        else
        {
            lhs = libutf8::to_u16string(std::string(reinterpret_cast<char const *>(s1->f_data), s1->f_data_size));
        }

        std::u16string rhs;
        if(s2->f_data_size <= sizeof(s2->f_data))
        {
            rhs = libutf8::to_u16string(std::string(reinterpret_cast<char const *>(&s2->f_data), s2->f_data_size));
        }
        else
        {
            rhs = libutf8::to_u16string(std::string(reinterpret_cast<char const *>(s2->f_data), s2->f_data_size));
        }

        std::int32_t r(memcmp(lhs.c_str(), rhs.c_str(), std::min(lhs.length(), rhs.length())));
        if(r == 0)
        {
            r = lhs.length() - rhs.length();
            r = r < 0 ? -1 : (r > 0 ? 1 : 0);
        }
        else
        {
            r = r < 0 ? -1 : 1;
        }
        switch(op)
        {
        case node_t::NODE_COMPARE:
            // r is what we need to return in this case
            return r;

        case node_t::NODE_EQUAL:
        case node_t::NODE_STRICTLY_EQUAL:
            return r == 0;

        case node_t::NODE_LESS:
            return r < 0;

        case node_t::NODE_LESS_EQUAL:
            return r <= 0;

        case node_t::NODE_GREATER:
            return r > 0;

        case node_t::NODE_GREATER_EQUAL:
            return r >= 0;

        case node_t::NODE_NOT_EQUAL:
        case node_t::NODE_STRICTLY_NOT_EQUAL:
            return r != 0;

        default:
            throw not_implemented("string_compare(): called with a string comparison operator not yet implemented.");

        }
    }
}


void strings_concat(binary_variable * d, binary_variable const * s1, binary_variable const * s2)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_concat()");
    }
    if(s1->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s1 is expected to be a string in strings_concat()");
    }
    if(s2->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s2 is expected to be a string in strings_concat()");
    }
#endif

    if(d != s1
    && d != s2)
    {
        strings_free(d);
    }

    // the "allocated" flag has no bearing if the inputs are empty
    //
    if(s1->f_data_size == 0
    && s2->f_data_size == 0)
    {
        d->f_type = VARIABLE_TYPE_STRING;
        d->f_flags = VARIABLE_FLAG_DEFAULT;
        d->f_data_size = 0;
        d->f_data = 0;
        return;
    }

    if((s1->f_flags & VARIABLE_FLAG_ALLOCATED) == 0
    && s2->f_data_size == 0)
    {
        strings_free(d);
        *d = *s1;
        return;
    }

    if((s2->f_flags & VARIABLE_FLAG_ALLOCATED) == 0
    && s1->f_data_size == 0)
    {
        strings_free(d);
        *d = *s2;
        return;
    }

    std::size_t const concat_size(s1->f_data_size + s2->f_data_size);
    if(concat_size <= sizeof(d->f_data))
    {
        strings_free(d);
        d->f_type = VARIABLE_TYPE_STRING;
        d->f_flags = VARIABLE_FLAG_DEFAULT;
        memcpy(&d->f_data, &s1->f_data, s1->f_data_size);
        memcpy(reinterpret_cast<char *>(&d->f_data) + s1->f_data_size, &s2->f_data, s2->f_data_size);
        d->f_data_size = concat_size;
        return;
    }

    char * str(static_cast<char *>(malloc(concat_size)));
    if(str == nullptr)
    {
        throw std::bad_alloc();
    }
    if(s1->f_data_size <= sizeof(s1->f_data))
    {
        memcpy(str, &s1->f_data, s1->f_data_size);
    }
    else
    {
        memcpy(str, reinterpret_cast<char const *>(s1->f_data), s1->f_data_size);
    }
    if(s2->f_data_size <= sizeof(s2->f_data))
    {
        memcpy(str + s1->f_data_size, &s2->f_data, s2->f_data_size);
    }
    else
    {
        memcpy(str + s1->f_data_size, reinterpret_cast<char const *>(s2->f_data), s2->f_data_size);
    }
    if(d == s1)
    {
        strings_free(d);
    }
    else if(d == s2)
    {
        strings_free(d);
    }
    d->f_data = reinterpret_cast<std::uint64_t>(str);
    d->f_data_size = concat_size;
    d->f_type = VARIABLE_TYPE_STRING;
    d->f_flags = VARIABLE_FLAG_ALLOCATED;
}


void strings_concat_params(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_concat_params()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_concat_params()");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_concat_params().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));

    std::size_t const max(v->size());

    // first compute the total size
    //
    std::size_t size(s->f_data_size);
    for(std::size_t idx(0); idx < max; ++idx)
    {
        binary_variable * p(v[0][idx]);
        if(p->f_type != VARIABLE_TYPE_STRING)
        {
            throw internal_error("concat() called with a parameter which is not a string.");
        }
        size += p->f_data_size;
    }

    // `d` could be one of the sources, so we cannot free that pointer;
    // instead allocate a new "floating" pointer here and handle the
    // save later
    //
    snapdev::safe_object<char *, delete_buffer> safe_buffer;
    char * ptr(static_cast<char *>(malloc(size)));
    if(ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    safe_buffer.make_safe(ptr);

    // now copy the inputs to the destination buffer
    //
    char const * src;
    char * dst(ptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    memcpy(dst, src, s->f_data_size);
    dst += s->f_data_size;
    for(std::size_t idx(0); idx < max; ++idx)
    {
        binary_variable * p(v[0][idx]);
        if(p->f_data_size <= sizeof(p->f_data))
        {
            src = reinterpret_cast<char const *>(&p->f_data);
        }
        else
        {
            src = reinterpret_cast<char const *>(p->f_data);
        }
        memcpy(dst, src, p->f_data_size);
        dst += p->f_data_size;
    }

    strings_free(d);

    if(size <= sizeof(d->f_data))
    {
        // safe_buffer will automatically free() the dst buffer for us
        //
        memcpy(&d->f_data, ptr, size);
    }
    else
    {
        safe_buffer.release();

        d->f_data = reinterpret_cast<std::int64_t>(ptr);
        d->f_flags |= VARIABLE_FLAG_ALLOCATED;
    }
    d->f_data_size = size;
}


void strings_unconcat(binary_variable * d, binary_variable const * s1, binary_variable const * s2)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_unconcat()");
    }
    if(s1->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s1 is expected to be a string in strings_unconcat()");
    }
    if(s2->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s2 is expected to be a string in strings_unconcat()");
    }
#endif

    if(s1->f_data_size == 0)
    {
        // instant optimization
        //
        strings_free(d);
        return;
    }

    char const * p1;
    if(s1->f_data_size <= sizeof(s1->f_data))
    {
        p1 = reinterpret_cast<char const *>(&s1->f_data);
    }
    else
    {
        p1 = reinterpret_cast<char const *>(s1->f_data);
    }
    char const * p2;
    if(s2->f_data_size <= sizeof(s2->f_data))
    {
        p2 = reinterpret_cast<char const *>(&s2->f_data);
    }
    else
    {
        p2 = reinterpret_cast<char const *>(s2->f_data);
    }
    std::size_t unconcat_size(s1->f_data_size);
    if(s1->f_data_size >= s2->f_data_size)
    {
        if(memcmp(p1 + s1->f_data_size - s2->f_data_size, p2, s2->f_data_size) == 0)
        {
            unconcat_size -= s2->f_data_size;
        }
    }

    if(s1 == d)
    {
        d->f_data_size = unconcat_size;
        return;
    }

    strings_free(d);

    if(unconcat_size <= sizeof(d->f_data))
    {
        d->f_flags = VARIABLE_FLAG_DEFAULT;
        d->f_data_size = unconcat_size;
        memcpy(&d->f_data, p1, unconcat_size);
        return;
    }

    char * str(static_cast<char *>(malloc(unconcat_size)));
    if(str == nullptr)
    {
        throw std::bad_alloc();
    }
    memcpy(str, p1, unconcat_size);
    d->f_flags |= VARIABLE_FLAG_ALLOCATED;
    d->f_data_size = unconcat_size;
    d->f_data = reinterpret_cast<std::uint64_t>(str);
}


void strings_shift(binary_variable * d, binary_variable const * s, int64_t count, node_t op)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_shift()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_shift()");
    }
#endif

    if(d == s)
    {
        throw not_implemented("strings_shift() does not support being called with s and d set to the same variable.");
    }
    strings_free(d);

    if(s->f_data_size == 0)
    {
        return;
    }

    // get the string
    //
    char const * str;
    if(s->f_data_size <= sizeof(s->f_data))
    {
        str = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        str = reinterpret_cast<char const *>(s->f_data);
    }

    if(count < 0)
    {
        // Note: the assignment operators are managed 100% the same as the
        //       non-assignments at this location (the assignment is
        //       performed after this call) so we can switch the operator
        //       to a non-assignment operator
        //
        count = -count;
        switch(op)
        {
        case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
        case node_t::NODE_ROTATE_LEFT:
            op = node_t::NODE_ROTATE_RIGHT;
            break;

        case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
        case node_t::NODE_ROTATE_RIGHT:
            op = node_t::NODE_ROTATE_LEFT;
            break;

        case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
        case node_t::NODE_SHIFT_LEFT:
            op = node_t::NODE_SHIFT_RIGHT;
            break;

        case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
        case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
        case node_t::NODE_SHIFT_RIGHT:
        case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
            op = node_t::NODE_SHIFT_LEFT;
            break;

        default:
            throw internal_error("strings_shift() called with an unsupported operation.");

        }
    }

    // "random" limit to avoid really large string shifts
    //
    switch(op)
    {
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
        count %= s->f_data_size;
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_SHIFT_LEFT:
        count &= 63;
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
        // no real need for a limit on that one, if "too large" we get ""
        break;

    default:
        throw not_implemented("strings_shift(): called with a string shift operator not yet implemented (1).");

    }

    char * dst(nullptr);
    switch(op)
    {
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ROTATE_RIGHT:
        if(count > 0)
        {
            count = s->f_data_size - count;
        }
        [[fallthrough]];
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ROTATE_LEFT:
        {
            d->f_data_size = s->f_data_size;
            if(d->f_data_size <= sizeof(d->f_data))
            {
                dst = reinterpret_cast<char *>(&d->f_data);
            }
            else
            {
                d->f_data = reinterpret_cast<std::int64_t>(malloc(d->f_data_size));
                if(d->f_data == 0)
                {
                    d->f_data_size = 0;
                    throw std::bad_alloc();
                }
                d->f_flags |= VARIABLE_FLAG_ALLOCATED;
                dst = reinterpret_cast<char *>(d->f_data);
            }
            std::size_t const rotate_length(d->f_data_size - count);
            memcpy(dst, str + count, rotate_length);
            memcpy(dst + rotate_length, str, count);
        }
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_SHIFT_LEFT:
        d->f_data_size = s->f_data_size + count;
        if(d->f_data_size <= sizeof(d->f_data))
        {
            dst = reinterpret_cast<char *>(&d->f_data);
        }
        else
        {
            d->f_data = reinterpret_cast<std::int64_t>(malloc(d->f_data_size));
            if(d->f_data == 0)
            {
                d->f_data_size = 0;
                throw std::bad_alloc();
            }
            d->f_flags |= VARIABLE_FLAG_ALLOCATED;
            dst = reinterpret_cast<char *>(d->f_data);
        }
        memcpy(dst, str, s->f_data_size);
        memset(dst + s->f_data_size, ' ', count);
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
        // if count >= s->f_data_size then output is going to be ""
        //
        if(count < static_cast<std::int64_t>(s->f_data_size))
        {
            d->f_data_size = s->f_data_size - count;
            if(d->f_data_size <= sizeof(d->f_data))
            {
                dst = reinterpret_cast<char *>(&d->f_data);
            }
            else
            {
                d->f_data = reinterpret_cast<std::int64_t>(malloc(d->f_data_size));
                if(d->f_data == 0)
                {
                    d->f_data_size = 0;
                    throw std::bad_alloc();
                }
                d->f_flags |= VARIABLE_FLAG_ALLOCATED;
                dst = reinterpret_cast<char *>(d->f_data);
            }
            memcpy(dst, str, d->f_data_size);
        }
        break;

    default:
        throw not_implemented("string_shift(): called with a string shift operator not yet implemented (2).");

    }
}


void strings_flip_case(binary_variable * d, binary_variable const * s)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_flip_case()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_flip_case()");
    }
#endif

    // TODO: use libutf8 to flip the case
    //       note that at that point the output string may have a different
    //       length as a few characters have upper lower which are encoded
    //       in different planes
    //
    if(d != s)
    {
        strings_free(d);
        if(s->f_data_size > sizeof(d->f_data))
        {
            d->f_data = reinterpret_cast<std::int64_t>(malloc(s->f_data_size));
            if(d->f_data == 0)
            {
                throw std::bad_alloc();
            }
            d->f_flags |= VARIABLE_FLAG_ALLOCATED;
        }
        d->f_data_size = s->f_data_size;
    }

    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    char * dst(nullptr);
    if(d->f_data_size <= sizeof(d->f_data))
    {
        dst = reinterpret_cast<char *>(&d->f_data);
    }
    else
    {
        dst = reinterpret_cast<char *>(d->f_data);
    }
    for(std::uint32_t idx(0); idx < d->f_data_size; ++idx)
    {
        char c(src[idx]);
        if((c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z'))
        {
            // TODO: use proper UTF-8 upper/lower functions
            //
            c ^= 0x20;
        }
        dst[idx] = c;
    }
}


void strings_multiply(binary_variable * d, binary_variable const * s, std::int64_t n)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_multiply()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_multiply()");
    }
#endif

    if(d == s)
    {
        throw not_implemented("strings_multiply() does not support being called with s and d set to the same variable.");
    }
    if(n < 0)
    {
        // TODO: this needs to be a script "raise" instead
        //
        throw incompatible_data("strings_multiply() does not support being called with s and d set to the same variable.");
    }
    strings_free(d);

    d->f_data_size = s->f_data_size * n;
    if(d->f_data_size > sizeof(d->f_data))
    {
        d->f_data = reinterpret_cast<std::int64_t>(malloc(d->f_data_size));
        if(d->f_data == 0)
        {
            d->f_data_size = 0;
            throw std::bad_alloc();
        }
        d->f_flags |= VARIABLE_FLAG_ALLOCATED;
    }

    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    char * dst(nullptr);
    if(d->f_data_size <= sizeof(d->f_data))
    {
        dst = reinterpret_cast<char *>(&d->f_data);
    }
    else
    {
        dst = reinterpret_cast<char *>(d->f_data);
    }
    int pos(0);
    for(std::int64_t idx(0); idx < n; ++idx, pos += s->f_data_size)
    {
        memcpy(dst + pos, src, s->f_data_size);
    }
}


void strings_minmax(binary_variable * d, binary_variable const * s1, binary_variable const * s2, std::int8_t minmax)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_minmax()");
    }
    if(s1->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s1 is expected to be a string in strings_minmax()");
    }
    if(s2->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s2 is expected to be a string in strings_minmax()");
    }
#endif

    std::int64_t const r(strings_compare(s1, s2, node_t::NODE_COMPARE) * minmax);
    if(r < 0)
    {
        s1 = s2;
    }
    strings_copy(d, s1);
}


void strings_at(binary_variable * d, binary_variable const * s, std::int64_t index)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_at()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_at()");
    }
#endif

    if(s->f_data_size == 0)
    {
        strings_free(d);
        return;
    }

    // we have UTF-8 strings, so the index doesn't work as is on our
    // strings, instead we have to scan the string (arg!)
    //
    // also, compared to JavaScript, we ignore the fact that the characters
    // are UTF-16 in JavaScript (that way we do not have to deal with
    // surrogates)
    //
    // TODO: see that the libutf8 offers an iterator on `char [const] *`
    //
    std::string src;
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = std::string(reinterpret_cast<char const *>(&s->f_data), s->f_data_size);
    }
    else
    {
        src = std::string(reinterpret_cast<char const *>(s->f_data), s->f_data_size);
    }

    char32_t c(libutf8::EOS);
    std::int64_t count(labs(index));
    if(index >= 0)
    {
        ++count;
    }

    // WARNING: do not test with `it != s.end()` because doing so would give
    //          you the last character instead of "" if index is too large
    //
    if(index >= 0)
    {
        for(libutf8::utf8_iterator it(src); count > 0; ++it, --count)
        {
            c = *it;
            if(c == libutf8::EOS)
            {
                break;
            }
        }
    }
    else
    {
        for(libutf8::utf8_iterator it(src, true); count > 0; --count)
        {
            --it;
            c = *it;
            if(c == libutf8::EOS)
            {
                break;
            }
        }
    }

    strings_free(d);
    if(c != libutf8::EOS)
    {
        d->f_data_size = libutf8::wctombs(reinterpret_cast<char *>(&d->f_data), c, sizeof(d->f_data));
    }
}


// WARNING: our substr() has a "start" and "end" instead of "start" and "length"
void strings_substr(binary_variable * d, binary_variable const * s, std::int64_t start, std::int64_t end)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_substr()");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_substr()");
    }
#endif

    if(s->f_data_size == 0
    || start > end
    || start < 0
    || end < 0)
    {
        strings_free(d);
        return;
    }

    // we have UTF-8 strings, so the index doesn't work as is on our
    // strings, instead we have to scan the string (arg!)
    //
    // also, compared to JavaScript, we ignore the fact that the characters
    // are UTF-16 in JavaScript (that way we do not have to deal with
    // surrogates)
    //
    std::string src;
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = std::string(reinterpret_cast<char const *>(&s->f_data), s->f_data_size);
    }
    else
    {
        src = std::string(reinterpret_cast<char const *>(s->f_data), s->f_data_size);
    }

    std::int64_t count(0);

    libutf8::utf8_iterator::difference_type idx_start(src.end() - src.begin());
    libutf8::utf8_iterator::difference_type idx_end(idx_start);
    for(libutf8::utf8_iterator it(src); it != src.end(); ++it, ++count)
    {
        if(count == start)
        {
            idx_start = it - src.begin();
        }
        if(count == end)
        {
            idx_end = it - src.begin();
            break; // no need to go further
        }
    }

    strings_free(d);
    d->f_data_size = idx_end - idx_start;
    if(d->f_data_size > 0)
    {
        if(d->f_data_size <= sizeof(d->f_data))
        {
            memcpy(&d->f_data, src.c_str() + idx_start, d->f_data_size);
        }
        else
        {
            memcpy(reinterpret_cast<char *>(d->f_data), src.c_str() + idx_start, d->f_data_size);
        }
    }
}


void strings_char_at(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_char_at().");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_char_at().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_char_at().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() != 1)
    {
        throw internal_error("charAt() expects exactly one parameter.");
    }
    std::int64_t pos(-1);
    binary_variable * p(v[0][0]);
    if(p->f_type != VARIABLE_TYPE_INTEGER)
    {
        if(p->f_type != VARIABLE_TYPE_FLOATING_POINT)
        {
            throw internal_error("charAt() called with a non-numeric parameter.");
        }
        std::uint64_t const * ptr(&p->f_data);
        pos = *reinterpret_cast<double const *>(ptr);
    }
    else
    {
        pos = p->f_data;
    }
    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    std::string const input(std::string(src, s->f_data_size));
    libutf8::utf8_iterator::value_type wc(libutf8::NOT_A_CHARACTER);
    libutf8::utf8_iterator it(input);
    for(int idx(0); idx <= pos; ++idx, ++it)
    {
        wc = *it;
        if(wc == libutf8::EOS)
        {
            // TODO: this needs to be a script throw, not a C++ throw...
            //
            throw out_of_range("position out of range for String.charAt(). (1)");
        }
    }
    if(wc == libutf8::NOT_A_CHARACTER)
    {
        throw out_of_range("position out of range for String.charAt(). (2)");
    }

    strings_free(d);
    char * dst(reinterpret_cast<char *>(&d->f_data));
    d->f_data_size = libutf8::wctombs(dst, wc, sizeof(d->f_data));
}


void strings_char_code_at(std::int64_t * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_char_code_at().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_char_code_at().");
    }
#endif
 
    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() != 1)
    {
        throw internal_error("charCodeAt() expects exactly one parameter.");
    }
    std::int64_t pos(-1);
    binary_variable * p(v[0][0]);
    if(p->f_type != VARIABLE_TYPE_INTEGER)
    {
        if(p->f_type != VARIABLE_TYPE_FLOATING_POINT)
        {
            throw internal_error("charCodeAt() called with a non-numeric parameter.");
        }
        std::uint64_t const * ptr(&p->f_data);
        pos = *reinterpret_cast<double const *>(ptr);
    }
    else
    {
        pos = p->f_data;
    }
    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    std::string const input(std::string(src, s->f_data_size));
    libutf8::utf8_iterator::value_type wc(libutf8::NOT_A_CHARACTER);
    libutf8::utf8_iterator it(input);
    for(int idx(0); idx <= pos; ++idx, ++it)
    {
        wc = *it;
        if(wc == libutf8::EOS)
        {
            // TODO: this needs to be a script throw, not a C++ throw...
            //
            throw out_of_range("position out of range for String.charCodeAt(). (1)");
        }
    }
    if(wc == libutf8::NOT_A_CHARACTER)
    {
        throw out_of_range("position out of range for String.charCodeAt(). (2)");
    }

    *d = static_cast<std::uint64_t>(wc);
}


void strings_index_of(std::int64_t * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_index_of().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_index_of().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() != 1
    && v->size() != 2)
    {
        throw internal_error("indexOf() expects one or two parameters.");
    }
    std::int64_t pos(0);
    if(v->size() == 2)
    {
        binary_variable * p2(v[0][1]);
        if(p2->f_type != VARIABLE_TYPE_INTEGER)
        {
            if(p2->f_type != VARIABLE_TYPE_FLOATING_POINT)
            {
                throw internal_error("indexOf() called with a non-numeric parameter as its second parameter.");
            }
            std::uint64_t const * ptr(&p2->f_data);
            pos = *reinterpret_cast<double const *>(ptr);
        }
        else
        {
            pos = p2->f_data;
        }
        if(pos < 0)
        {
            pos = 0;
        }
    }

    binary_variable * p1(v[0][0]);
    if(p1->f_type != VARIABLE_TYPE_STRING)
    {
        throw internal_error("indexOf() called with a non-string parameter as its first parameter.");
    }
    char const * search_string(nullptr);
    if(p1->f_data_size <= sizeof(p1->f_data))
    {
        search_string = reinterpret_cast<char const *>(&p1->f_data);
    }
    else
    {
        search_string = reinterpret_cast<char const *>(p1->f_data);
    }

    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }

    *d = -1;

    // the strings are UTF-8 so we need to iterate them with libutf8
    //
    std::string const haystack(src, s->f_data_size);
    std::string const needle(search_string, p1->f_data_size);

    libutf8::utf8_iterator it(haystack);

    // skip 'pos' characters at the start
    //
    for(std::int64_t idx(0); idx < pos; ++idx, ++it)
    {
        if(*it == libutf8::EOS)
        {
            // input is too small
            //
            if(needle.empty())
            {
                // here is a special case in JavaScript for the empty string
                //
                *d = idx;
            }
            return;
        }
    }

    // now check whether 'haystack[0 .. needle.size() - 1]' == 'needle'
    // if not, try skipping one more character from haystack and try again
    //
    for(; *it != libutf8::EOS; ++pos, ++it)
    {
        libutf8::utf8_iterator pt(it);
        for(libutf8::utf8_iterator nt(needle);; ++nt, ++pt)
        {
            if(*nt == libutf8::EOS)
            {
                // found it, we're done
                //
                *d = pos;
                return;
            }
            if(*pt == libutf8::EOS)
            {
                // needle.length > haystack.length, we can return now
                //
                return;
            }
            if(*nt != *pt)
            {
                break;
            }
        }
    }
}


void strings_last_index_of(std::int64_t * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_last_index_of().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_last_index_of().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() != 1
    && v->size() != 2)
    {
        throw internal_error("lastIndexOf() expects one or two parameters.");
    }
    std::int64_t pos(std::numeric_limits<std::int64_t>::max());
    if(v->size() == 2)
    {
        binary_variable * p2(v[0][1]);
        if(p2->f_type != VARIABLE_TYPE_INTEGER)
        {
            if(p2->f_type != VARIABLE_TYPE_FLOATING_POINT)
            {
                throw internal_error("lastIndexOf() called with a non-numeric parameter as its second parameter.");
            }
            std::uint64_t const * ptr(&p2->f_data);
            pos = *reinterpret_cast<double const *>(ptr);
        }
        else
        {
            pos = p2->f_data;
        }
        if(pos < 0)
        {
            pos = 0;
        }
    }

    binary_variable * p1(v[0][0]);
    if(p1->f_type != VARIABLE_TYPE_STRING)
    {
        throw internal_error("lastIndexOf() called with a non-string parameter as its first parameter.");
    }
    char const * search_string(nullptr);
    if(p1->f_data_size <= sizeof(p1->f_data))
    {
        search_string = reinterpret_cast<char const *>(&p1->f_data);
    }
    else
    {
        search_string = reinterpret_cast<char const *>(p1->f_data);
    }

    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }

    *d = -1;

    // the strings are UTF-8 so we need to iterate them with libutf8
    //
    std::string const haystack(src, s->f_data_size);
    std::string const needle(search_string, p1->f_data_size);

    // if needle is empty, we return the size of the string or pos
    // whichever is smaller
    //
    if(needle.empty())
    {
        // here is a special case in JavaScript for the empty string
        //
        *d = std::min(pos, static_cast<std::int64_t>(libutf8::u8length(haystack)));
        return;
    }

    // now check whether 'haystack[0 .. needle.size() - 1]' == 'needle'
    // if not, try skipping one more character from haystack and try again
    //
    // IMPORTANT NOTE: since we're dealing with UTF-8, we start from the
    //                 beginning of the string and save the position of
    //                 the last occurence instead of trying to go backward
    //                 (TODO: fix because I think this is fixable? but 'pos'
    //                 is a big issue here...)
    //
    libutf8::utf8_iterator it(haystack);
    for(std::int64_t idx(0); *it != libutf8::EOS && idx <= pos; ++idx, ++it)
    {
        libutf8::utf8_iterator pt(it);
        for(libutf8::utf8_iterator nt(needle);; ++nt, ++pt)
        {
            if(*nt == libutf8::EOS)
            {
                // found an occurence, save/replace the value
                //
                *d = idx;
                break;
            }
            if(*pt == libutf8::EOS)
            {
                // needle.length > haystack.length, we can return now
                //
                return;
            }
            if(*nt != *pt)
            {
                break;
            }
        }
    }
}


void strings_save(binary_variable * d, std::string const & result)
{
    strings_free(d);
    d->f_data_size = result.length();
    if(d->f_data_size > 0)
    {
        if(d->f_data_size <= sizeof(d->f_data))
        {
            memcpy(&d->f_data, result.c_str(), d->f_data_size);
        }
        else
        {
            d->f_data = reinterpret_cast<std::uint64_t>(malloc(d->f_data_size));
            if(d->f_data == 0)
            {
                throw std::bad_alloc();
            }
            d->f_flags |= VARIABLE_FLAG_ALLOCATED;
            memcpy(reinterpret_cast<char *>(d->f_data), result.c_str(), d->f_data_size);
        }
    }
}


void strings_replace_apply(binary_variable * d, binary_variable const * s, binary_variable const * params, bool all)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_replace_apply().");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_replace_apply().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_replace_apply().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() != 2)
    {
        throw internal_error("replace() and replaceAll() expect two parameters.");
    }
    binary_variable * search(v[0][0]);
    if(search->f_type != VARIABLE_TYPE_STRING)
    {
        // TODO: add support for REGEXP
        //
        throw internal_error("replace() and replaceAll() expect a String as their first parameter.");
    }
    binary_variable * replace(v[0][1]);
    if(replace->f_type != VARIABLE_TYPE_STRING)
    {
        throw internal_error("replace() and replaceAll() expect a String as their second parameter.");
    }

    char const * search_string(nullptr);
    if(search->f_data_size <= sizeof(search->f_data))
    {
        search_string = reinterpret_cast<char const *>(&search->f_data);
    }
    else
    {
        search_string = reinterpret_cast<char const *>(search->f_data);
    }

    char const * replace_string(nullptr);
    if(replace->f_data_size <= sizeof(replace->f_data))
    {
        replace_string = reinterpret_cast<char const *>(&replace->f_data);
    }
    else
    {
        replace_string = reinterpret_cast<char const *>(replace->f_data);
    }

    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }

    // if needle is empty, just prepend replace_string
    //
    if(search->f_data_size == 0)
    {
        all = false;
    }

    // we assume proper strings in the following loop
    //
    std::string result;
    if(s->f_data_size >= search->f_data_size)
    {
        std::uint32_t max(s->f_data_size - search->f_data_size);
        for(std::uint32_t idx(0); idx <= max; ++idx)
        {
            if(search->f_data_size == 0
            || memcmp(src + idx, search_string, search->f_data_size) == 0)
            {
                for(std::uint32_t j(0); j < replace->f_data_size; ++j)
                {
                    if(replace_string[j] == '$')
                    {
                        ++j;
                        if(j < replace->f_data_size)
                        {
                            switch(replace_string[j])
                            {
                            case '$':
                                result += '$';
                                break;

                            case '&':
                                result += std::string(search_string, search->f_data_size);
                                break;

                            case '`':
                                result += std::string(src, idx);
                                break;

                            case '\'':
                                {
                                    std::size_t offset(idx + search->f_data_size);
                                    result += std::string(src + offset, s->f_data_size - offset);
                                }
                                break;

                            // the following two are only for RegExp,
                            // otherwise it is totally ignored
                            //
                            //case '0':
                            //case '1':
                            //case '2':
                            //case '3':
                            //case '4':
                            //case '5':
                            //case '6':
                            //case '7':
                            //case '8':
                            //case '9':
                            //    {
                            //        std::size_t n(replace_string[j] - '0');
                            //        ++j;
                            //        if(j < replace->f_data_size
                            //        && replace_string[j] >= '0'
                            //        && replace_string[j] <= '9')
                            //        {
                            //            n = n * 10 + replace_string[j] - '0';
                            //        }
                            //        else
                            //        {
                            //            --j;
                            //        }
                            //        // for strings (opposed to regexp) do nothing
                            //        // with this replacement paramerter
                            //    }
                            //    break;

                            //case '<':
                            //    {
                            //        std::string name;
                            //        for(++j; j < replace->f_data_size && replace_string[j] != '>'; ++j)
                            //        {
                            //            name += replace_string[j];
                            //        }
                            //        if(replace_string[j] == '>')
                            //        {
                            //            // for strings (opposed to regexp) do nothing
                            //            // with this replacement parameter
                            //        }
                            //        else
                            //        {
                            //            // TBD: name not closed, what to do is TBD at the moment
                            //        }
                            //    }
                            //    break;

                            default:
                                // not a valid replacement, just keep the '$'
                                //
                                result += '$';
                                --j;
                                break;

                            }
                        }
                        else
                        {
                            result += '$';
                        }
                    }
                    else
                    {
                        result += replace_string[j];
                    }
                }
                if(!all)
                {
                    // only replace first occurance when `!all`
                    //
                    // still add the end of the input string to result
                    //
                    std::uint32_t const offset(idx + search->f_data_size);
                    result += std::string(src + offset, s->f_data_size - offset);
                    break;
                }
            }
            else
            {
                result += src[idx];
            }
        }
    }

    strings_save(d, result);
}


void strings_replace(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
    strings_replace_apply(d, s, params, false);
}


void strings_replace_all(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
    strings_replace_apply(d, s, params, true);
}


void strings_slice(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_slice().");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_slice().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_slice().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() < 1
    || v->size() > 2)
    {
        throw internal_error("slice() expects one or two parameters.");
    }

    std::int64_t start(0);
    std::int64_t end(0);

    binary_variable * start_var(v[0][0]);
    if(start_var->f_type != VARIABLE_TYPE_INTEGER)
    {
        // TODO: add support for REGEXP
        //
        throw internal_error("slice() expects an Integer as their first parameter.");
    }
    start = start_var->f_data;

    if(v->size() == 2)
    {
        binary_variable * end_var(v[0][1]);
        if(end_var->f_type != VARIABLE_TYPE_INTEGER)
        {
            // TODO: add support for REGEXP
            //
            throw internal_error("slice() expects an Integer as their second parameter.");
        }
        end = end_var->f_data;
    }
    else
    {
        // the exact end would be the u8length(), but that function is slow
        // and f_data_size is already computed; however, f_data_size may be
        // larger than the number of characters which the strings_substr()
        // manages as expected
        //
        //end = libutf8::u8length(std::string(s->f_data, s->f_data_size));

        end = s->f_data_size;
    }

    strings_substr(d, s, start, end);
}


void strings_substring(binary_variable * d, binary_variable const * s, binary_variable const * params)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in strings_substring().");
    }
    if(s->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("s is expected to be a string in strings_substring().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_substring().");
    }
#endif

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(params->f_data));
    if(v->size() < 1
    || v->size() > 2)
    {
        throw internal_error("slice() expects one or two parameters.");
    }

    std::int64_t start(0);
    std::int64_t end(0);

    binary_variable * start_var(v[0][0]);
    if(start_var->f_type != VARIABLE_TYPE_INTEGER)
    {
        // TODO: add support for REGEXP
        //
        throw internal_error("slice() expects an Integer as their first parameter.");
    }
    start = start_var->f_data;
    if(start < 0)
    {
        start = 0;
    }

    if(v->size() == 2)
    {
        binary_variable * end_var(v[0][1]);
        if(end_var->f_type != VARIABLE_TYPE_INTEGER)
        {
            // TODO: add support for REGEXP
            //
            throw internal_error("slice() expects an Integer as their second parameter.");
        }
        end = end_var->f_data;
        if(end < 0)
        {
            end = 0;
        }
    }
    else
    {
        // the exact end would be the u8length(), but that function is slow
        // and f_data_size is already computed; however, f_data_size may be
        // larger than the number of characters which the strings_substr()
        // manages as expected
        //
        //end = libutf8::u8length(std::string(s->f_data, s->f_data_size));

        end = s->f_data_size;
    }
    if(end < start)
    {
        std::swap(start, end);
    }

    strings_substr(d, s, start, end);
}


void strings_to_lowercase(binary_variable * d, binary_variable const * s)
{
    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    std::string const in(src, s->f_data_size);

    std::string result;
    for(libutf8::utf8_iterator it(in);; ++it)
    {
        char32_t wc(*it);
        if(wc == libutf8::NOT_A_CHARACTER
        || wc == libutf8::EOS)
        {
            break;
        }
        wc = towlower(wc);
        result += libutf8::to_u8string(wc);
    }

    strings_save(d, result);
}


void strings_to_uppercase(binary_variable * d, binary_variable const * s)
{
    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    std::string const in(src, s->f_data_size);

    std::string result;
    for(libutf8::utf8_iterator it(in);; ++it)
    {
        char32_t wc(*it);
        if(wc == libutf8::NOT_A_CHARACTER
        || wc == libutf8::EOS)
        {
            break;
        }
        wc = towupper(wc);
        result += libutf8::to_u8string(wc);
    }

    strings_save(d, result);
}


bool strings_is_white_space(char32_t wc)
{
    switch(wc)
    {
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x0020:
    case 0x00A0:
    case 0x1680:
    case 0x2000:
    case 0x2001:
    case 0x2002:
    case 0x2003:
    case 0x2004:
    case 0x2005:
    case 0x2006:
    case 0x2007:
    case 0x2008:
    case 0x2009:
    case 0x200A:
    case 0x2028:
    case 0x2029:
    case 0x202F:
    case 0x205F:
    case 0x3000:
    case 0xFEFF:
        return true;

    default:
        return false;

    }
    snapdev::NOT_REACHED();
}


void strings_trim(binary_variable * d, binary_variable const * s, bool trim_start, bool trim_end)
{
    char const * src(nullptr);
    if(s->f_data_size <= sizeof(s->f_data))
    {
        src = reinterpret_cast<char const *>(&s->f_data);
    }
    else
    {
        src = reinterpret_cast<char const *>(s->f_data);
    }
    std::string const in(src, s->f_data_size);

    std::string result;
    std::string inside_white_spaces;
    for(libutf8::utf8_iterator it(in);; ++it)
    {
        char32_t const wc(*it);
        if(wc == libutf8::NOT_A_CHARACTER
        || wc == libutf8::EOS)
        {
            break;
        }
//std::cerr << "--- wc = " << static_cast<int>(wc)
//<< " is white? " << strings_is_white_space(wc)
//<< " not a character= " << static_cast<int>(libutf8::NOT_A_CHARACTER)
//<< " EOS= " << static_cast<int>(libutf8::EOS)
//<< "\n";
        if(strings_is_white_space(wc))
        {
            if(!trim_start)
            {
                inside_white_spaces += libutf8::to_u8string(wc);
            }
        }
        else
        {
            trim_start = false;
            result += inside_white_spaces;
            inside_white_spaces.clear();
            result += libutf8::to_u8string(wc);
        }
    }
    if(!trim_end)
    {
        result += inside_white_spaces;
    }

    strings_save(d, result);
}


void strings_trim_both(binary_variable * d, binary_variable const * s)
{
    strings_trim(d, s, true, true);
}


void strings_trim_start(binary_variable * d, binary_variable const * s)
{
    strings_trim(d, s, true, false);
}


void strings_trim_end(binary_variable * d, binary_variable const * s)
{
    strings_trim(d, s, false, true);
}


void booleans_to_string(binary_variable * d, bool b)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in booleans_to_string().");
    }
#endif

    strings_save(d, b ? "true" : "false");
}


void integers_to_string(binary_variable * d, std::int64_t value, binary_variable const * params)
{
std::cerr << "--- integers_to_string()\n";

#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in integers_to_string().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in integers_to_string().");
    }
#endif

    std::int64_t base(10);
    binary_variable::vector_of_pointers_t const * p(reinterpret_cast<binary_variable::vector_of_pointers_t const *>(params->f_data));
    if(p->size() == 1)
    {
        if(p[0][0]->f_type == VARIABLE_TYPE_INTEGER)
        {
            base = p[0][0]->f_data;
            if(base < 2 || base > 36)
            {
                throw incompatible_type("integers_to_string() base must be between 2 and 36 inclusive.");
            }
        }
        else
        {
            throw incompatible_type("integers_to_string() must be called with 0 or 1 parameter; parameter must be integer.");
        }
    }

std::cerr << "--- integer to string [" << value << "] == [" << base << "]\n";
    strings_save(d, snapdev::integer_to_string(value, base));
}


void floating_points_to_string(binary_variable * d, double number, binary_variable const * params)
{
#ifdef _DEBUG
    if(d->f_type != VARIABLE_TYPE_STRING)
    {
        throw incompatible_type("d is expected to be a string in floating_points_to_string().");
    }
    if(params->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("params is expected to be an array in strings_substring().");
    }
#endif

    std::int64_t base(10);
    binary_variable::vector_of_pointers_t const * p(reinterpret_cast<binary_variable::vector_of_pointers_t const *>(params->f_data));
    if(p->size() == 1)
    {
        if(p[0][0]->f_type == VARIABLE_TYPE_INTEGER)
        {
            base = p[0][0]->f_data;
            if(base < 2 || base > 36)
            {
                throw incompatible_type("floating_points_to_string() base must be between 2 and 36 inclusive.");
            }
        }
        else
        {
            throw incompatible_type("floating_points_to_string() must be called with 0 or 1 parameter; parameter must be integer.");
        }
    }

    // TODO: the output of a double in JavaScript is quite different from
    //       most other languages; here is a good post about it although
    //       it is documented in ECMA
    //
    // https://stackoverflow.com/questions/56179272/javascript-seems-to-be-doing-floating-point-wrong-compared-to-c
    //
    std::string v;
    if(base != 10)
    {
        // TODO: implement double_to_string()
        //
        v = snapdev::integer_to_string(static_cast<std::int64_t>(number), base);
    }
    else
    {
        v = std::to_string(number);
    }
    std::string::size_type const pos(v.find('.'));
    if(pos != std::string::npos)
    {
        while(v.back() == '0')
        {
            v = v.substr(0, v.length() - 1);
        }
        if(v.back() == '.')
        {
            v = v.substr(0, v.length() - 1);
        }
    }

    strings_save(d, v);
}


void array_initialize(binary_variable * v)
{
    v->f_type = VARIABLE_TYPE_ARRAY;
    v->f_flags = VARIABLE_FLAG_ALLOCATED;
    v->f_name = 0;              // TODO: add a name (for debug purposes)
    v->f_name_size = 0;
    v->f_data_size = sizeof(binary_variable::vector_of_pointers_t *);
    v->f_data = reinterpret_cast<std::int64_t>(new binary_variable::vector_of_pointers_t);
}


void array_free(binary_variable * v)
{
#ifdef _DEBUG
    if(v->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("v is expected to be an array in array_free()");
    }
#endif

    if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
    {
        v->f_flags &= ~VARIABLE_FLAG_ALLOCATED;
        delete reinterpret_cast<binary_variable::vector_of_pointers_t *>(v->f_data);
        v->f_data = 0; // a.k.a. "nullptr"
        v->f_data_size = 0;
    }
}


void array_push(binary_variable * array, binary_variable * item)
{
#ifdef _DEBUG
    if(array->f_type != VARIABLE_TYPE_ARRAY)
    {
        throw incompatible_type("array is expected to be an array variable in array_push()");
    }
#endif

    if(array->f_data == 0
    || (array->f_flags & VARIABLE_FLAG_ALLOCATED) == 0)
    {
        throw incompatible_type("array in array_push() is not allocated");
    }

    binary_variable::vector_of_pointers_t * v(reinterpret_cast<binary_variable::vector_of_pointers_t *>(array->f_data));
    v->push_back(item);
}



}

typedef std::int64_t (*func_pointer_t)();

#pragma GCC diagnostic push
// the cast-function-type is not yet available in 7.5.0, but the version when
// it becomes necessary to have this diagnostic may be more recent than 11.3.0
#if SNAPDEV_CHECK_GCC_VERSION(11, 3, 0)
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#pragma GCC diagnostic ignored "-Wpedantic"
#define EXTERN_FUNCTION_ADD(index, func)    [index] = reinterpret_cast<func_pointer_t>(func)
typedef func_pointer_t const    extern_functions_t[];
func_pointer_t const g_extern_functions[] =
{
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_IPOW,                      ipow),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_POW,                       ::pow),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_FMOD,                      ::fmod),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_INITIALIZE,        strings_initialize),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_FREE,              strings_free),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_COPY,              strings_copy),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_COMPARE,           strings_compare),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_CONCAT,            strings_concat),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_CONCAT_PARAMS,     strings_concat_params),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_UNCONCAT,          strings_unconcat),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_SHIFT,             strings_shift),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_FLIP_CASE,         strings_flip_case),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_MULTIPLY,          strings_multiply),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_MINMAX,            strings_minmax),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_AT,                strings_at),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_SUBSTR,            strings_substr),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_CHAR_AT,           strings_char_at),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_CHAR_CODE_AT,      strings_char_code_at),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_INDEX_OF,          strings_index_of),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_LAST_INDEX_OF,     strings_last_index_of),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_REPLACE,           strings_replace),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_REPLACE_ALL,       strings_replace_all),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_SLICE,             strings_slice),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_SUBSTRING,         strings_substring),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_TO_LOWERCASE,      strings_to_lowercase),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_TO_UPPERCASE,      strings_to_uppercase),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_TRIM,              strings_trim_both),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_TRIM_START,        strings_trim_start),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_STRINGS_TRIM_END,          strings_trim_end),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_BOOLEANS_TO_STRING,        booleans_to_string),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_INTEGERS_TO_STRING,        integers_to_string),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_FLOATING_POINTS_TO_STRING, floating_points_to_string),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_ARRAY_INITIALIZE,          array_initialize),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_ARRAY_FREE,                array_free),
    EXTERN_FUNCTION_ADD(EXTERNAL_FUNCTION_ARRAY_PUSH,                array_push),
};
#pragma GCC diagnostic pop



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


void temporary_variable::adjust_offset(ssize_t const offset)
{
    f_offset += offset;
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


void relocation::adjust_offset(int offset)
{
    f_offset += offset;
}








void build_file::set_return_type(variable_type_t type)
{
    f_header.f_return_type = type;
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
        if(f_extern_variables.back().f_name_size <= sizeof(var.f_name))
        {
            previous_name = std::string(
                              reinterpret_cast<char const *>(&f_extern_variables.back().f_name)
                            , f_extern_variables.back().f_name_size);
        }
        else
        {
            previous_name = std::string(
                              f_strings.data() + f_extern_variables.back().f_name
                            , f_extern_variables.back().f_name_size);
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
    node::pointer_t type_node(type->get_node()->get_type_node());
    if(type_node == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
        msg << "no type found for variable \""
            << name
            << "\".";
        throw internal_error(msg.str());
    }
    std::string const & type_name(type_node->get_string());

    if(type_node->get_type() == node_t::NODE_CLASS
    && type_node->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        std::size_t size(0);
        variable_type_t var_type(VARIABLE_TYPE_UNKNOWN);
        if(type_name == "Boolean")
        {
            var_type = VARIABLE_TYPE_BOOLEAN;
            size = sizeof(bool);
        }
        else if(type_name == "Integer" || type_name == "CompareResult")
        {
            var_type = VARIABLE_TYPE_INTEGER;
            size = sizeof(std::int64_t);
        }
        else if(type_name == "Double" || type_name == "Number")
        {
            var_type = VARIABLE_TYPE_FLOATING_POINT;
            size = sizeof(double);
        }
        else if(type_name == "String")
        {
            var_type = VARIABLE_TYPE_STRING;
            size = 0; // TODO
        }
//std::cerr << " --- add " << type_name << " variable \"" << name << "\" -> size=" << size << "\n";
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

            case VARIABLE_TYPE_RANGE:
                {
                    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
                    msg << "add_extern_variable(): RANGE type not yet handled.";
                    throw not_implemented(msg.str());
                }

            case VARIABLE_TYPE_ARRAY:
                {
                    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
                    msg << "add_extern_variable(): an external variable cannot be of type VECTOR.";
                    throw incompatible_type(msg.str());
                }

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
        << "\" for a temporary variable -- add_external_variable().";
    throw internal_error(msg.str());
}


void build_file::add_temporary_variable(std::string const & name, data::pointer_t var)
{
    node::pointer_t n(var->get_node());
    node::pointer_t type(n->get_type_node());
    if(type == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, n->get_position());
        msg << "no type found for temporary variable \""
            << n->get_string()
            << "\".";
        throw internal_error(msg.str());
    }
    std::string const & type_name(type->get_string());

std::cerr << "--- type of var \"" << name << "\" is " << type->get_type_name() << " and name [" << type_name << "]\n";
    if((type->get_type() == node_t::NODE_CLASS
        || type->get_type() == node_t::NODE_ENUM)
    && type->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        bool const use_binary_variable(n->get_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE));
        if(type_name == "Boolean")
        {
            if(use_binary_variable)
            {
                add_temporary_variable_8bytes(name, node_t::NODE_BOOLEAN, sizeof(binary_variable));
            }
            else
            {
                add_temporary_variable_1byte(name, node_t::NODE_BOOLEAN, sizeof(bool));
            }
            return;
        }
        else if(type_name == "Integer" || type_name == "CompareResult")
        {
            if(use_binary_variable)
            {
                add_temporary_variable_8bytes(name, node_t::NODE_INTEGER, sizeof(binary_variable));
            }
            else
            {
                add_temporary_variable_8bytes(name, node_t::NODE_INTEGER, sizeof(std::int64_t));
            }
            return;
        }
        else if(type_name == "Double" || type_name == "Number")
        {
            if(use_binary_variable)
            {
                add_temporary_variable_8bytes(name, node_t::NODE_DOUBLE, sizeof(binary_variable));
            }
            else
            {
                add_temporary_variable_8bytes(name, node_t::NODE_DOUBLE, sizeof(double));
            }
            return;
        }
        else if(type_name == "String")
        {
            // binary_variable is 24 bytes, but it does not need to be
            // aligned to 24 or 32 bytes, it is enough to be aligned to 8
            //
            add_temporary_variable_8bytes(name, node_t::NODE_STRING, sizeof(binary_variable));
            return;
        }
        else if(type_name == "Array")
        {
            add_temporary_variable_8bytes(name, node_t::NODE_ARRAY, sizeof(binary_variable));
            return;
        }
    }

    // TBD: I think this can happen if you re-define native types (for fun)
    //
    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, n->get_position());
    msg << "unsupported node type \""
        << type_name
        << "\" for a temporary variable -- add_temporary_variable().";
    throw internal_error(msg.str());
}


void build_file::add_temporary_variable_1byte(
      std::string const & name
    , node_t type
    , std::size_t size)
{
    f_temporary_1byte_offset -= size;
    f_temporary_1byte.push_back(temporary_variable(
                          name
                        , type
                        , size
                        , f_temporary_1byte_offset));
}


void build_file::add_temporary_variable_8bytes(
      std::string const & name
    , node_t type
    , std::size_t size)
{
    f_temporary_8bytes_offset -= size;
    f_temporary_8bytes.push_back(temporary_variable(
                          name
                        , type
                        , size
                        , f_temporary_8bytes_offset));
}


void build_file::adjust_temporary_offset_1byte()
{
    for(auto & temp : f_temporary_1byte)
    {
        temp.adjust_offset(f_temporary_8bytes_offset);
    }
}


void build_file::add_private_variable(std::string const & name, data::pointer_t type)
{
    // TODO: save the default value (SET <expr>)
    //
    node::pointer_t instance(type->get_node()->get_type_node());
    if(instance == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, type->get_node()->get_position());
        msg << "no type found for private variable \""
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
            f_bool_private.insert(
                      f_bool_private.end()
                    , reinterpret_cast<std::uint8_t const *>(&value)
                    , reinterpret_cast<std::uint8_t const *>(&value + 1));
            return;
        }
        else if(type_name == "Integer"
             || type_name == "Double"
             || type_name == "Number"
             || type_name == "CompareResult")
        {
            f_private_offsets[name] = f_number_private.size();
            std::int64_t value(0);          // TODO: <- use SET expr if it exists and is constant
            f_number_private.insert(
                      f_number_private.end()
                    , reinterpret_cast<std::uint8_t const *>(&value)
                    , reinterpret_cast<std::uint8_t const *>(&value + 1));
            return;
        }
        else if(type_name == "String")
        {
            f_private_variable_offsets[name] = f_string_private.size();
            binary_variable value = {};          // TODO: <- use SET expr if it exists and is constant
            value.f_type = VARIABLE_TYPE_STRING;
            value.f_flags = VARIABLE_FLAG_DEFAULT;
            value.f_name_size = 0;  // name is not required here
            value.f_name = 0;
            value.f_data_size = 0;
            value.f_data = 0;

            // what string? (the SET?)
            //f_strings.insert(f_strings.end(), name.begin(), name.end());

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
        << "\" for a temporary variable.";
    throw internal_error(msg.str());
}


void build_file::add_constant(double const value, std::string & /*out*/ name)
{
    // constant are saved along non-constant private variables
    //
    name = "@";
    double const * value_ptr(&value);
    name += std::to_string(*reinterpret_cast<std::uint64_t const *>(value_ptr));
    auto it(f_private_offsets.find(name));
    if(it == f_private_offsets.end())
    {
        // not yet defined, add anew
        //
        f_private_offsets[name] = f_number_private.size();
        f_number_private.insert(
                      f_number_private.end()
                    , reinterpret_cast<std::uint8_t const *>(&value)
                    , reinterpret_cast<std::uint8_t const *>(&value + 1));
    }
    //else -- already here, just return the existing name
}


void build_file::add_constant(std::string const value, std::string & /*out*/ name)
{
    // TODO: constant strings should be accessible directly, we have two
    //       concerned at the moment: string length and many duplicates for
    //       all the string commands (i.e. instead of just binary_variable
    //       parameters, we need to support binary_variable & char const *)
    //
    for(auto const & it : f_private_variable_offsets)
    {
        if(it.first[0] == '@'
        && it.first[1] == 's')
        {
            binary_variable const * str(reinterpret_cast<binary_variable const *>(f_string_private.data() + it.second));
            if(str->f_data_size == value.length())
            {
                bool found(false);
                if(str->f_data_size <= sizeof(str->f_data))
                {
                    found = memcmp(&str->f_data, value.c_str(), str->f_data_size) == 0;
                }
                else
                {
                    found = memcmp(f_strings.data() + str->f_data, value.c_str(), str->f_data_size) == 0;
                }
                if(found)
                {
                    // found the exact same string, reuse it
                    //
                    name = it.first;
                    return;
                }
            }
        }
    }

    ++f_next_const_string;
    name = "@s";
    name += std::to_string(static_cast<std::uint64_t>(f_next_const_string));

    // string was not found, add new string now
    //
    binary_variable s = {};
    s.f_type = VARIABLE_TYPE_STRING;
    s.f_flags = VARIABLE_FLAG_DEFAULT;
    s.f_name_size = name.length();
    if(s.f_name_size <= sizeof(s.f_name))
    {
        memcpy(&s.f_name, name.c_str(), s.f_name_size);
    }
    else
    {
        s.f_name = f_strings.size();
        f_strings.insert(f_strings.end(), name.begin(), name.end());
    }
    s.f_data_size = value.length();
    if(s.f_data_size <= sizeof(s.f_data))
    {
        // "inline" (small enough)
        //
        memcpy(&s.f_data, value.c_str(), s.f_data_size);
    }
    else
    {
        // reference to "long" string
        //
        s.f_data = f_strings.size();
        f_strings.insert(f_strings.end(), value.begin(), value.end());
    }

    f_private_variable_offsets[name] = f_string_private.size();
    f_string_private.insert(
              f_string_private.end()
            , reinterpret_cast<std::uint8_t const *>(&s)
            , reinterpret_cast<std::uint8_t const *>(&s + 1));
}


offset_t build_file::get_constant_offset(std::string const & name) const
{
    auto it(f_private_offsets.find(name));
    if(it == f_private_offsets.end())
    {
        it = f_private_variable_offsets.find(name);
        if(it == f_private_variable_offsets.end())
        {
            throw internal_error(
                      "constant \""
                    + name
                    + "\" not found in get_constant_offset()");
        }
    }
    return it->second;
}


void build_file::add_label(std::string const & name)
{
    f_label_offsets[name] = get_current_text_offset();
}


//void build_file::add_rt_function(
//      std::string const & path
//    , std::string const & name)
//{
//    // already added?
//    //
//    if(f_rt_function_offsets.find(name) != f_rt_function_offsets.end())
//    {
//        return;
//    }
//
//    // save the offset
//    //
//    f_rt_function_offsets[name] = f_rt_functions.size();
//
//    // load the function
//    //
//    if(f_archive.get_functions().empty())
//    {
//        std::string filename(path + "/rt.oar");
//        as2js::input_stream<std::ifstream>::pointer_t in(std::make_shared<as2js::input_stream<std::ifstream>>());
//        in->get_position().set_filename(filename);
//        in->open(filename);
//        if(!in->is_open())
//        {
//            message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
//            msg << "could not open run-time object archive \""
//                << filename
//                << "\".";
//            throw cannot_open_file(msg.str());
//        }
//        f_archive.load(in);
//    }
//    rt_function::pointer_t func(f_archive.find_function(name));
//    if(func == nullptr)
//    {
//        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
//        msg << "could not find run-time function \""
//            << name
//            << "\" from run-time archive (it may not exist in older versions).";
//        throw not_implemented(msg.str());
//    }
//    f_rt_functions.insert(f_rt_functions.end(), func->get_code().begin(), func->get_code().end());
//
//    // TODO: make it so we have a single function which can happen to rt or text
//    //
//    switch(f_rt_functions.size() & 7)
//    {
//    case 0:
//        // already aligned
//        break;
//
//    case 8 - 1:
//        {
//            std::uint8_t nop[] = {
//                0x90,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 2:
//        {
//            std::uint8_t nop[] = {
//                0x66,
//                0x90,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 3:
//        {
//            std::uint8_t nop[] = {
//                0x0F,
//                0x1F,
//                0x00,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 4:
//        {
//            std::uint8_t nop[] = {
//                0x0F,
//                0x1F,
//                0x40,
//                0x00,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 5:
//        {
//            std::uint8_t nop[] = {
//                0x0F,
//                0x1F,
//                0x44,
//                0x00,
//                0x00,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 6:
//        {
//            std::uint8_t nop[] = {
//                0x66,
//                0x0F,
//                0x1F,
//                0x44,
//                0x00,
//                0x00,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    case 8 - 7:
//        {
//            std::uint8_t nop[] = {
//                0x0F,
//                0x1F,
//                0x80,
//                0x00,
//                0x00,
//                0x00,
//                0x00,
//            };
//            f_rt_functions.insert(f_rt_functions.end(), nop, nop + sizeof(nop));
//        }
//        break;
//
//    }
//}


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


void build_file::adjust_relocation_offset(int offset)
{
    f_relocations.back().adjust_offset(offset);
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
                  + f_text.size();
                  //+ f_rt_functions.size();

    // save the data types with the largest alignment requirements first
    //
    f_string_private_offset = f_data_offset + f_extern_variables.size() * sizeof(binary_variable);
    f_number_private_offset = f_string_private_offset + f_string_private.size();
    f_bool_private_offset = f_number_private_offset + f_number_private.size();
    f_strings_offset = f_bool_private_offset + f_bool_private.size();
    f_after_strings_offset = f_strings_offset + f_strings.size();

    for(auto const & r : f_relocations)
    {
        switch(r.get_relocation())
        {
        case relocation_t::RELOCATION_VARIABLE_32BITS_DATA:
        case relocation_t::RELOCATION_VARIABLE_32BITS_DATA_SIZE:
        case relocation_t::RELOCATION_VARIABLE_32BITS:
            {
                // TODO: variables are expected to be in order so we should use
                //       a binary search instead (see std::lower_bound() call)
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

                offset_t extra_offset(0);
                if(r.get_relocation() == relocation_t::RELOCATION_VARIABLE_32BITS_DATA)
                {
                    extra_offset = offsetof(binary_variable, f_data);
                }
                else if(r.get_relocation() == relocation_t::RELOCATION_VARIABLE_32BITS_DATA_SIZE)
                {
                    extra_offset = offsetof(binary_variable, f_data_size);
                }
                offset_t offset(it->f_data_size <= sizeof(it->f_data)
                        ? f_data_offset
                            + sizeof(binary_variable) * (it - f_extern_variables.begin())
                            + extra_offset
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

        case relocation_t::RELOCATION_CONSTANT_32BITS:
            {
                auto it(f_private_offsets.find(r.get_name()));
                if(it != f_private_offsets.end())
                {
                    offset_t offset(f_number_private_offset
                                        - f_text_offset
                                        + it->second);

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

                    break;
                }
            }

            {
                auto it(f_private_variable_offsets.find(r.get_name()));
                if(it != f_private_variable_offsets.end())
                {
                    offset_t offset(f_string_private_offset
                                        - f_text_offset
                                        + it->second);

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

                    break;
                }
            }

            // we added that relocation and now we cannot not find it?!
            //
            throw internal_error(
                      "could not find private variable or constant for relocation named \""
                    + r.get_name()
                    + "\".");
            break;

        //case relocation_t::RELOCATION_RT_32BITS:
        //    {
        //        auto it(f_rt_function_offsets.find(r.get_name()));
        //        if(it == f_rt_function_offsets.end())
        //        {
        //            // we added that run-time function, we just cannot
        //            // not find it
        //            //
        //            throw internal_error(
        //                      "could not find run-time function for relocation named \""
        //                    + r.get_name()
        //                    + "\".");
        //        }

        //        offset_t const offset(f_text.size()
        //                                - r.get_offset()
        //                                + it->second);

        //        // save the result in f_text
        //        //
        //        offset_t const idx(r.get_position());
        //        f_text[idx + 0] = offset >>  0;
        //        f_text[idx + 1] = offset >>  8;
        //        f_text[idx + 2] = offset >> 16;
        //        f_text[idx + 3] = offset >> 24;
        //    }
        //    break;

        case relocation_t::RELOCATION_LABEL_32BITS:
            {
                auto it(f_label_offsets.find(r.get_name()));
                if(it == f_label_offsets.end())
                {
                    // TBD: we probably have user defined labels that need
                    //      to be supported here?
                    //
                    throw internal_error(
                              "could not find label for relocation named \""
                            + r.get_name()
                            + "\".");
                }

                offset_t const offset(it->second - r.get_offset());

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
            // relocate from start of file
            //
            f_extern_variables[idx].f_name += f_strings_offset;
        }
    }

    for(std::size_t offset(0); offset < f_string_private.size(); offset += sizeof(binary_variable))
    {
        binary_variable * var(reinterpret_cast<binary_variable *>(f_string_private.data() + offset));
        if(var->f_name_size > sizeof(var->f_name))
        {
            // relocate from start of file
            //
            var->f_name += f_strings_offset;
        }
        if(var->f_data_size > sizeof(var->f_data))
        {
            // relocate from start of file
            //
            var->f_data += f_strings_offset;
        }
    }

    // save header (badc0de1)
    //
    f_header.f_variable_count = f_extern_variables.size();
    f_header.f_private_variable_count = f_private_variable_offsets.size();
    f_header.f_variables = f_data_offset; // variables are saved first
    f_header.f_start = f_text_offset;
    f_header.f_file_size = ((f_after_strings_offset + 3) & -4) + sizeof(char) * 4;
    out->write_bytes(reinterpret_cast<char const *>(&f_header), sizeof(f_header));

    // .text (i.e. binary code we want to execute)
    //
    out->write_bytes(
              reinterpret_cast<char const *>(f_text.data())
            , f_text.size());
    //out->write_bytes(
    //          reinterpret_cast<char const *>(f_rt_functions.data())
    //        , f_rt_functions.size());

    // .data
    //
    out->write_bytes(
              reinterpret_cast<char const *>(f_extern_variables.data())
            , f_extern_variables.size() * sizeof(binary_variable));
    out->write_bytes(
              reinterpret_cast<char const *>(f_string_private.data())
            , f_string_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_number_private.data())
            , f_number_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_bool_private.data())
            , f_bool_private.size());
    out->write_bytes(
              reinterpret_cast<char const *>(f_strings.data())
            , f_strings.size());

    // clearly mark the end of the file aligned to 4 bytes
    //
    offset_t const adjust(4 - (f_after_strings_offset & 3));
    if(adjust != 4)
    {
        char buf[4] = {};
        out->write_bytes(buf, adjust);
    }
    out->write_bytes(g_end_magic, 4);;
}


/** \brief Search variable at build time.
 *
 * This function searches for the named variable at the time the binary
 * is being built.
 *
 * \warning
 * The pointer being returned changes with additional variables get added.
 * You cannot save it anywhere, just make use of it and drop it.
 *
 * \return the variable or nullptr.
 */
binary_variable const * build_file::get_extern_variable(std::string const & name) const
{
    auto it(std::lower_bound(
              f_extern_variables.begin()
            , f_extern_variables.end()
            , name
            , [&](binary_variable const & v, std::string const & n)
            {
                char const * s(v.f_name_size <= sizeof(v.f_name)
                    ? reinterpret_cast<char const *>(&v.f_name)
                    : reinterpret_cast<char const *>(f_strings.data() + v.f_name));
                return std::string(s, v.f_name_size) < n;
            }));
    bool found(it != f_extern_variables.end());
    if(found)
    {
        // lower bound returns the "next variable" if the exact variable
        // does not exist, so here we have to make sure we've got the
        // actual variable and not the next one
        //
        if(it->f_name_size == name.length())
        {
            char const * s(it->f_name_size <= sizeof(it->f_name)
                ? reinterpret_cast<char const *>(&it->f_name)
                : reinterpret_cast<char const *>(f_strings.data() + it->f_name));
            found = memcmp(s, name.c_str(), it->f_name_size) == 0;
        }
        else
        {
            found = false;
        }
    }
    if(!found)
    {
        return nullptr;
    }
    return &*it;
}


std::size_t build_file::get_size_of_temporary_variables() const
{
    // one byte per boolean, but rounded up to the next 8 bytes
    // plus 8 bytes per integer or double
    //
    // this is what we subtract from the rsp pointer to allocate our
    // local variables
    //
    return ((-f_temporary_1byte_offset + 7) & -8)
         + -f_temporary_8bytes_offset;
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
        msg << "could not read text and variables from binary file (size: "
            << size
            << ").";
        return false;
    }

    f_header = reinterpret_cast<binary_header *>(f_file);
    f_text = reinterpret_cast<std::uint8_t *>(f_header + 1);
    f_variables = reinterpret_cast<binary_variable *>(f_file + f_header->f_variables);

    // variable data need to be relocated
    //
    // Note: at the moment the f_text buffer is using %rip to access data
    //       so there is no need for relocation in the code
    //
    std::size_t const var_count(f_header->f_variable_count + f_header->f_private_variable_count);
    for(std::uint16_t idx(0); idx < var_count; ++idx)
    {
// the name is 32 bit and thus does not support a full pointer
//        if(f_variables[idx].f_name_size > sizeof(f_variables[idx].f_name))
//        {
//            f_variables[idx].f_name += reinterpret_cast<std::uint64_t>(f_file);
//        }
        if(f_variables[idx].f_data_size > sizeof(f_variables[idx].f_data))
        {
            f_variables[idx].f_data += reinterpret_cast<std::uint64_t>(f_file);
        }
    }

    return true;
}


/** \brief Save the file buffer to disk.
 *
 * This is expected to be used only to debug the processes.
 *
 * Here is an example saving the file to "c.out":
 *
 * \code
 * std::cerr << "--- save to c.out now...\n";
 * system("pwd");
 * const_cast<running_file *>(this)->save("c.out");
 * \endcode
 *
 * \param[in] filename  The name of the file to save to.
 */
void running_file::save(std::string const & filename)
{
    std::ofstream out;
    out.open(filename);
    if(!out.is_open())
    {
        std::cerr << "error: could not open output file \""
            << filename
            << "\".\n";
        return;
    }

    out.write(reinterpret_cast<char const *>(f_file), f_size);
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
                return std::string(s, v.f_name_size) < n;
            }));
    bool found(it != f_variables + f_header->f_variable_count);
    if(found)
    {
        // lower bound returns the "next variable" if the exact variable
        // does not exist, so here we have to make sure we've got the
        // actual variable and not the next one
        //
        char const * s(it->f_name_size <= sizeof(it->f_name)
            ? reinterpret_cast<char const *>(&it->f_name)
            : reinterpret_cast<char const *>(f_file + it->f_name));
        found = std::string(s, it->f_name_size) == name;
    }
    if(!found)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND);
        msg << "could not find variable \""
            << name
            << "\".";
        throw invalid_data(msg.str());
    }
    return it;
}


bool running_file::has_variable(std::string const & name) const
{
    if(f_variables == nullptr)
    {
        return false;
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
                return std::string(s, v.f_name_size) < n;
            }));
    if(it == f_variables + f_header->f_variable_count)
    {
        return false;
    }

    char const * s(it->f_name_size <= sizeof(it->f_name)
        ? reinterpret_cast<char const *>(&it->f_name)
        : reinterpret_cast<char const *>(f_file + it->f_name));
    return std::string(s, it->f_name_size) == name;
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
    if(v->f_type != VARIABLE_TYPE_FLOATING_POINT)
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
    double const * value_ptr(&value);
    v->f_data = *reinterpret_cast<std::uint64_t const *>(value_ptr);
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
    if(v->f_data_size <= sizeof(v->f_data))
    {
        value = std::string(reinterpret_cast<char const *>(&v->f_data), v->f_data_size);
    }
    else if((v->f_flags & VARIABLE_FLAG_ALLOCATED) != 0)
    {
        value = std::string(reinterpret_cast<char const *>(v->f_data), v->f_data_size);
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_SUPPORTED);
        msg << "string variable named \""
            << name
            << "\" is not small ("
            << v->f_data_size
            << ") and not allocated.";
        throw incompatible_type(msg.str());
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
    if(f_header == nullptr)
    {
        throw invalid_data("running_file has no data.");
    }

    if(!f_protected)
    {
        // TODO: consider placing variables on the following 4K page
        //       so we can remove PROT_WRITE here
        //
        int const r(mprotect(f_file, f_size, PROT_READ | PROT_WRITE | PROT_EXEC));
        if(r != 0)
        {
            throw execution_error("the file could not be protected for execution.");
        }
        f_protected = true;
    }
    binary_variable * var(find_variable("%result"));

std::cerr << "--- run with return type: " << static_cast<int>(f_header->f_return_type) << "\n";
    typedef void (*entry_point)(extern_functions_t);
    reinterpret_cast<entry_point>(f_text)(g_extern_functions);

    switch(f_header->f_return_type)
    {
    case VARIABLE_TYPE_BOOLEAN:
        result.set_boolean(var->f_data != 0);
        break;

    case VARIABLE_TYPE_INTEGER:
        result.set_integer(var->f_data);
        break;

    case VARIABLE_TYPE_FLOATING_POINT:
        {
            std::uint64_t * ptr(&var->f_data);
            result.set_floating_point(*reinterpret_cast<double *>(ptr));
        }
        break;

    case VARIABLE_TYPE_STRING:
        if(var->f_data_size <= sizeof(var->f_data))
        {
            result.set_string(std::string(reinterpret_cast<char const *>(&var->f_data), var->f_data_size));
        }
        else
        {
            result.set_string(std::string(reinterpret_cast<char const *>(var->f_data), var->f_data_size));
        }
        break;

    default:
        result.set_type(VARIABLE_TYPE_UNKNOWN);
        break;

    }
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
        , options::pointer_t o
        , compiler::pointer_t c)
    : f_output(output)
    , f_options(o)
    , f_compiler(c)
{
    //if(!rt_functions_oar.empty())
    //{
    //    f_rt_functions_oar = rt_functions_oar;
    //}
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
    flatten_nodes::pointer_t fn(flatten(root, f_compiler));
std::cerr << "----- end flattening... (";
if(fn == nullptr)
{
std::cerr << "<nullptr>";
}
else
{
std::cerr << fn->get_operations().size();
}
std::cerr << ")\n";

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


variable_type_t binary_assembler::get_type_of_node(node::pointer_t n)
{
    node::pointer_t type_node(n->get_type_node());
    if(type_node == nullptr)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INVALID_TYPE, n->get_position());
        msg << "no type found for node of type: \""
            << n->get_type_name()
            << "\".";
        throw internal_error(msg.str());
    }

    if(type_node->get_type() != node_t::NODE_CLASS
    || !type_node->get_attribute(attribute_t::NODE_ATTR_NATIVE))
    {
        return VARIABLE_TYPE_UNKNOWN;
    }
    std::string const & type_name(type_node->get_string());

    if(type_name == "Boolean")
    {
        return VARIABLE_TYPE_BOOLEAN;
    }

    if(type_name == "Integer" || type_name == "CompareResult")
    {
        return VARIABLE_TYPE_INTEGER;
    }

    if(type_name == "Double" || type_name == "Number")
    {
        return VARIABLE_TYPE_FLOATING_POINT;
    }

    if(type_name == "String")
    {
        return VARIABLE_TYPE_STRING;
    }

    if(type_name == "Range")
    {
        return VARIABLE_TYPE_RANGE;
    }

    return VARIABLE_TYPE_UNKNOWN;
}


void binary_assembler::generate_amd64_code(flatten_nodes::pointer_t fn)
{
    if(fn->get_operations().empty())
    {
        throw out_of_range("the code in generate_amd64_code() expects a non-empty set of operations.");
    }

    // clear the existing file
    //
    f_file = {};

    // on entry setup rsp & rbp
    //
    std::uint8_t setup_frame[] = {
        0x55,           // PUSH %rbp

        0x48,           // MOV %rsp, %rbp
        0x89,
        0xE5,
    };
    f_file.add_text(setup_frame, sizeof(setup_frame));

    // we do not have a node for the pointer to the external function table,
    // so create one here -- it will get added automatically since it is
    // part of the temporary table
    //
    // WARNING: the type node inside the node object is a weak pointer
    //          so we need to keep a hold of it in this function
    //
    // TODO: we can optimize this out if no function gets called
    //
    node::pointer_t type_class(std::make_shared<node>(node_t::NODE_CLASS));
    type_class->set_string("Integer");
    type_class->set_attribute(attribute_t::NODE_ATTR_NATIVE, true);
    {
        node::pointer_t var(std::make_shared<node>(node_t::NODE_VARIABLE));
        var->set_flag(flag_t::NODE_VARIABLE_FLAG_TEMPORARY, true);
        var->set_string("%extern_functions");

        // TODO: call "resolve_internal_type()" instead...
        //
        var->set_type_node(type_class); // weak pointer

        f_extern_functions = std::make_shared<data>(var);
        fn->add_variable(f_extern_functions);
    }

//for(auto const & it : fn->get_operations())
//{
//std::cerr << "  --  " << it->to_string() << "\n";
//}

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

    f_file.adjust_temporary_offset_1byte();

    for(auto & it : fn->get_data())
    {
        std::string name;
        switch(it->get_data_type())
        {
        case node_t::NODE_FLOATING_POINT:
            f_file.add_constant(it->get_floating_point().get(), name);
            break;

        case node_t::NODE_STRING:
            f_file.add_constant(it->get_string(), name);
            break;

        default:
            throw not_implemented("trying to add a constant with a data type which is not yet implemented.");

        }
        it->set_data_name(name);
    }

    offset_t temp_size(f_file.get_size_of_temporary_variables());

    if(temp_size > 0)
    {
        // we need to reserve the space for our temporary variables
        //
        // WARNING: the stack must be properly aligned to 16 bytes when
        //          calling a function (not considering the function pointer
        //          that is pushed by the CALL instruction, which gets fixed
        //          by the `PUSH %rbp` anyway)
        //
        temp_size = (temp_size + 15) & -16;
        if(temp_size < 128)
        {
            std::uint8_t buf[] = { // SUB imm7, %rsp
                0x48,
                0x83,
                0xEC,
                static_cast<std::uint8_t>(temp_size),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        else
        {
            std::uint8_t buf[] = { // SUB imm32, %rsp
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

    generate_store_integer(f_extern_functions, register_t::REGISTER_RDI);

    // the temporary strings need to be initialized
    //
    for(auto const & it : fn->get_variables())
    {
        if(it.second->is_temporary()
        && !it.second->no_init())
        {
            // at this point, a temporary variable does not have a value,
            // only a type and a name; temporaries automatically have a
            // STORE before a LOAD
            //
            //generate_reg_mem_string(<...>, register_t::REGISTER_RSI);
            temporary_variable * temp_var(f_file.find_temporary_variable(it.first));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary not found in generate_amd64_code()");
            }
            // TODO: transform in a switch once we have other types than string
            if(temp_var->get_type() != node_t::NODE_STRING)
            {
                continue;
            }
            generate_pointer_to_temporary(temp_var, register_t::REGISTER_RDI);
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_INITIALIZE);
        }
    }

    for(auto const & it : fn->get_operations())
    {
std::cerr << "  ++  " << it->to_string() << "\n";
        switch(it->get_operation())
        {
        case node_t::NODE_ABSOLUTE_VALUE:
            generate_absolute_value(it);
            break;

        case node_t::NODE_ADD:
        case node_t::NODE_ASSIGNMENT_ADD:
        case node_t::NODE_ASSIGNMENT_SUBTRACT:
        case node_t::NODE_SUBTRACT:
            generate_additive(it);
            break;

        case node_t::NODE_ALMOST_EQUAL:
        case node_t::NODE_COMPARE:
        case node_t::NODE_EQUAL:
        case node_t::NODE_LESS:
        case node_t::NODE_LESS_EQUAL:
        case node_t::NODE_GREATER:
        case node_t::NODE_GREATER_EQUAL:
        case node_t::NODE_NOT_EQUAL:
        case node_t::NODE_SMART_MATCH:
        case node_t::NODE_STRICTLY_EQUAL:
        case node_t::NODE_STRICTLY_NOT_EQUAL:
            generate_compare(it);
            break;

        case node_t::NODE_ARRAY:
            generate_array(it);
            break;

        case node_t::NODE_ASSIGNMENT:
            generate_assignment(it);
            break;

        case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
        case node_t::NODE_LOGICAL_AND:
        case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
        case node_t::NODE_LOGICAL_OR:
        case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
        case node_t::NODE_LOGICAL_XOR:
            generate_logical(it);
            break;

        case node_t::NODE_ASSIGNMENT_BITWISE_AND:
        case node_t::NODE_ASSIGNMENT_BITWISE_OR:
        case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
        case node_t::NODE_BITWISE_AND:
        case node_t::NODE_BITWISE_OR:
        case node_t::NODE_BITWISE_XOR:
            generate_bitwise(it);
            break;

        case node_t::NODE_ASSIGNMENT_DIVIDE:
        case node_t::NODE_ASSIGNMENT_MODULO:
        case node_t::NODE_DIVIDE:
        case node_t::NODE_MODULO:
            generate_divide(it);
            break;

        case node_t::NODE_ASSIGNMENT_MAXIMUM:
        case node_t::NODE_ASSIGNMENT_MINIMUM:
        case node_t::NODE_MAXIMUM:
        case node_t::NODE_MINIMUM:
            generate_minmax(it);
            break;

        case node_t::NODE_ASSIGNMENT_MULTIPLY:
        case node_t::NODE_MULTIPLY:
            generate_multiply(it);
            break;

        case node_t::NODE_ASSIGNMENT_POWER:
        case node_t::NODE_POWER:
            generate_power(it);
            break;

        case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
        case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
        case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
        case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
        case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
        case node_t::NODE_ROTATE_LEFT:
        case node_t::NODE_ROTATE_RIGHT:
        case node_t::NODE_SHIFT_LEFT:
        case node_t::NODE_SHIFT_RIGHT:
        case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
            generate_shift(it);
            break;

        case node_t::NODE_BITWISE_NOT:
            generate_bitwise_not(it);
            break;

        case node_t::NODE_CALL:
            generate_call(it);
            break;

        case node_t::NODE_DECREMENT:
        case node_t::NODE_INCREMENT:
        case node_t::NODE_POST_DECREMENT:
        case node_t::NODE_POST_INCREMENT:
            generate_increment(it);
            break;

        case node_t::NODE_GOTO:
            generate_goto(it);
            break;

        case node_t::NODE_IF_FALSE:
        case node_t::NODE_IF_TRUE:
            generate_if(it);
            break;

        case node_t::NODE_IDENTITY:
            generate_identity(it);
            break;

        case node_t::NODE_LABEL:
            generate_label(it);
            break;

        case node_t::NODE_LIST:
            generate_list(it);
            break;

        case node_t::NODE_LOGICAL_NOT:
            generate_logical_not(it);
            break;

        case node_t::NODE_NEGATE:
            generate_negate(it);
            break;

        case node_t::NODE_PARAM:
            generate_param(it);
            break;

        default:
            throw not_implemented(
                  std::string("operation ")
                + node::type_to_string(it->get_operation())
                + " is not yet implemented.");

        }
    }

    {
        auto const & it(fn->get_operations().back());
        node::pointer_t n(it->get_node());
        node::pointer_t t(n->get_type_node());
        if(t != nullptr)
        {
            f_file.set_return_type(get_type_of_node(n));
        }
    }

    // the temporary strings need to be freed
    //
int vcount(0);
    for(auto const & it : fn->get_variables())
    {
        if(it.second->is_temporary()
        && !it.second->no_init())
        {
            // at this point, a temporary variable does not have a value,
            // only a type and a name; temporaries automatically have a
            // STORE before a LOAD
            //
            //generate_reg_mem_string(<...>, register_t::REGISTER_RSI);
            temporary_variable * temp_var(f_file.find_temporary_variable(it.first));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary not found in generate_amd64_code()");
            }
            // TODO: transform in a switch once we have other types than string
            if(temp_var->get_type() != node_t::NODE_STRING)
            {
                continue;
            }
            generate_pointer_to_temporary(temp_var, register_t::REGISTER_RDI);
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_FREE);
std::cerr << "--- free var #" << vcount << " named \"" << temp_var->get_name() << "\".\n";
++vcount;
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
            std::uint8_t buf[] = {   // ADD $imm8, %rsp
                0x48,
                0x83,
                0xC4,
                static_cast<std::uint8_t>(temp_size),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        else
        {
            std::uint8_t buf[] = {   // ADD $imm32, %rsp
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
    std::uint8_t restore_frame[] = {  // POP %rbp  &  RET
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




void binary_assembler::generate_reg_mem_integer(data::pointer_t d, register_t const reg, std::uint8_t code, int adjust_offset)
{
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_INTEGER: // immediate integer
        {
            std::int64_t const value(d->get_node()->get_integer().get());
            switch(get_smallest_size(value))
            {
            case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_64BITS:
                {
                    std::uint8_t buf[] = {   // REX.W MOV $imm64, %rn
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        static_cast<std::uint8_t>(0xB8 | (static_cast<int>(reg) & 7)),
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
                    std::uint8_t buf[] = {   // REX.W MOV $imm32, %rn
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0xC7,
                        static_cast<std::uint8_t>(0xC0 | (static_cast<int>(reg) & 7)),
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

    case node_t::NODE_FLOATING_POINT: // immediate floating point to load as an integer
        {
            double const floating_point(d->get_node()->get_floating_point().get());
            double const * fp(&floating_point);
            std::int64_t const value(*reinterpret_cast<std::int64_t const *>(fp));
            std::uint8_t buf[] = {   // REX.W MOV $imm64, %rn
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                static_cast<std::uint8_t>(0xB8 | (static_cast<int>(reg) & 7)),
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

    case node_t::NODE_VARIABLE:
        if(d->is_temporary())
        {
            std::string const name(n->get_string());
            temporary_variable * temp_var(f_file.find_temporary_variable(name));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary not found in generate_reg_mem_integer()");
            }
            switch(temp_var->get_size())
            {
            case 1: // Boolean
                {
                    ssize_t const offset(temp_var->get_offset());
                    integer_size_t const offset_size(get_smallest_size(offset));
                    std::uint8_t rm(0x45);
                    switch(code)
                    {
                    case 0x8B:  // MOVSX 64bit -> MOV 8bit
                        switch(offset_size)
                        {
                        case integer_size_t::INTEGER_SIZE_1BIT:
                        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {          // REX.W MOV byte disp8(%rbp), %rn
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x0F,
                                    0xBE,
                                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset),
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {          // REX.W MOV disp32(%rbp), %rn
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x0F,
                                    0xBE,
                                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset >>  0),
                                    static_cast<std::uint8_t>(offset >>  8),
                                    static_cast<std::uint8_t>(offset >> 16),
                                    static_cast<std::uint8_t>(offset >> 24),
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
                        return;

                    case 0x83:  // CMP 64bit, imm8 -> CMP 8bit, imm8
                        code = 0x80;
                        rm = 0x7D;
                        break;

                    default:
                        throw not_implemented(
                              "8 bit code \""
                            + std::to_string(static_cast<int>(code))
                            + "\" in generate_reg_mem_integer() not yet supported (temporary variable).");

                    }
                    switch(offset_size)
                    {
                    case integer_size_t::INTEGER_SIZE_1BIT:
                    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                        if(reg >= register_t::REGISTER_RSP)
                        {
                            std::uint8_t buf[] = {          // REX.W MOV byte disp8(%rbp), %rn.b
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                code,
                                static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        else
                        {
                            std::uint8_t buf[] = {          // MOV byte disp8(%rbp), %rn.b
                                code,
                                static_cast<std::uint8_t>(rm | (static_cast<int>(reg) << 3)),
                                static_cast<std::uint8_t>(offset),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                        if(reg >= register_t::REGISTER_RSP)
                        {
                            // rm = 0x45 -> 0x85 for 32 bit disp
                            //
                            std::uint8_t buf[] = {          // REX.W MOV disp32(%rbp), %r8
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                code,
                                static_cast<std::uint8_t>((rm ^ 0xC0) | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        else
                        {
                            // rm = 0x45 -> 0x85 for 32 bit disp
                            //
                            std::uint8_t buf[] = {          // MOV disp32(%rbp), %r8
                                code,
                                static_cast<std::uint8_t>((rm ^ 0xC0) | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
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

            case 8: // Integer / Double
                {
                    ssize_t const offset(temp_var->get_offset());
                    integer_size_t const offset_size(get_smallest_size(offset));
                    switch(offset_size)
                    {
                    case integer_size_t::INTEGER_SIZE_1BIT:
                    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {    // REX.W MOV disp8(%rbp), %rn
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                code,
                                static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {   // REX.W MOV disp32(%rbp), %rn
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                code,
                                static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
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
                throw not_implemented("temporary size not yet supported in generate_reg_mem_integer()");

            }
        }
        else if(d->is_extern())
        {
            binary_variable const * var(f_file.get_extern_variable(d->get_string()));

            // I do not think this is correct, we probably need to check
            // the type first? (i.e. strings have variable sizes and encoding
            // that as 1 or 8 bytes is not going to work)
            //
            switch(var->f_data_size)
            {
            case 1:
                {
                    std::uint8_t rm(0x05);
                    switch(code)
                    {
                    case 0x0B:  // OR (m8), %r8
                        code = 0x0A;
                        break;

                    case 0x23:  // AND (m8), %r8
                        code = 0x22;
                        break;

                    case 0x33:  // XOR (m8), %r8
                        code = 0x32;
                        break;

                    case 0x3B:  // CMP (m8), %r8
                        code = 0x3A;
                        break;

                    case 0x8B:  // MOV (m8), %r8
                        code = 0x8A;
                        break;

                    case 0x83:  // CMP $imm8, %r8
                        code = 0x80;
                        rm = 0x3D;
                        break;

                    default:
                        throw not_implemented(
                              "8 bit code \""
                            + std::to_string(static_cast<int>(code))
                            + "\" in generate_reg_mem_integer() not yet supported (external variable).");

                    }
//std::cerr << " -- select reg [" << static_cast<int>(reg) << "]\n";
                    if(reg >= register_t::REGISTER_RSP || code == 0x8A)
                    {
                        std::size_t pos(f_file.get_current_text_offset());
                        if(code == 0x8A)
                        {
                            // the MOV requires one more byte using the
                            // MOVZX to clear all the upper bits...
                            //
                            std::uint8_t buf[] = {
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                0x0F,                       // REX.W MOVSBQ disp32(%rip), %r64
                                0xBE,
                                static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                                0x00,
                                0x00,
                                0x00,
                                0x00,
                            };
                            f_file.add_text(buf, sizeof(buf));
                            pos += 4;
                        }
                        else
                        {
                            std::uint8_t buf[] = {          // CMP disp32(%rip), %r8
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                code,
                                static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                                0x00,                       // 32 bit offset
                                0x00,
                                0x00,
                                0x00,
                                // for CMP disp(%rip), $imm8, the immediate
                                // is added by the caller on return
                            };
                            f_file.add_text(buf, sizeof(buf));
                            pos += 3;
                        }
                        f_file.add_relocation(
                                  d->get_string()
                                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                , pos
                                , f_file.get_current_text_offset() + adjust_offset);
                    }
                    else
                    {
                        std::size_t const pos(f_file.get_current_text_offset());
                        std::uint8_t buf[] = {          // MOV disp32(m8), r8
                            code,
                            static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                            0x00,                       // 32 bit offset
                            0x00,
                            0x00,
                            0x00,
                            // for CMP disp(%rip), $imm8, the immediate
                            // is added by the caller on return
                        };
                        f_file.add_text(buf, sizeof(buf));
                        f_file.add_relocation(
                                  d->get_string()
                                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                , pos + 2
                                , f_file.get_current_text_offset() + adjust_offset);
                    }
                }
                break;

            case 8:
                {
                    std::size_t const pos(f_file.get_current_text_offset());
                    std::uint8_t buf[] = {          // REX.W MOV disp32(%rip), %r64
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        code,
                        static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                    };
                    f_file.add_text(buf, sizeof(buf));
                    f_file.add_relocation(
                              d->get_string()
                            , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                            , pos + 3
                            , f_file.get_current_text_offset() + adjust_offset);
                }
                break;

            default:
                throw not_implemented(
                      "WARNING: generate_reg_mem_integer() hit an extern variable size ("
                    + std::to_string(var->f_data_size)
                    + ") not yet implemented...");

            }
        }
        else
        {
            throw not_implemented("WARNING: generate_reg_mem_integer() hit a variable type not yet implemented...");
        }
        break;

    default:
        throw not_implemented("WARNING: generate_reg_mem_integer() hit a data type not yet implemented...");

    }
}


void binary_assembler::generate_reg_mem_floating_point(data::pointer_t d, register_t const reg, sse_operation_t op, int adjust_offset)
{
    std::uint8_t code(0x8B);

    std::uint8_t sse_code(0x10);    // code for MOVSD
    switch(op)
    {
    case sse_operation_t::SSE_OPERATION_ADD:
        sse_code = 0x58;
        break;

    case sse_operation_t::SSE_OPERATION_CMP:
        sse_code = 0xC2;
        break;

    case sse_operation_t::SSE_OPERATION_DIV:
        sse_code = 0x5E;
        break;

    case sse_operation_t::SSE_OPERATION_LOAD:
    case sse_operation_t::SSE_OPERATION_CVT2I: // convert is used like a LOAD
        // sse_code is already set to MOVSD
        break;

    case sse_operation_t::SSE_OPERATION_MAX:
        sse_code = 0x5F;
        break;

    case sse_operation_t::SSE_OPERATION_MIN:
        sse_code = 0x5D;
        break;

    case sse_operation_t::SSE_OPERATION_MUL:
        sse_code = 0x59;
        break;

    case sse_operation_t::SSE_OPERATION_SUB:
        sse_code = 0x5C;
        break;

    default:
        throw internal_error(
                  "unknown SSE operation "
                + std::to_string(static_cast<int>(op))
                + " in generate_reg_mem_floating_point().");

    }

    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_INTEGER: // immediate integer
        if(op == sse_operation_t::SSE_OPERATION_CVT2I)
        {
            // the caller wants the result as an integer in a regular
            // register so do that with a generate_reg_mem_integer() call
            //
            generate_reg_mem_integer(d, reg, 0x8B, adjust_offset);
            break;
        }

        {
            double const value(static_cast<double>(d->get_node()->get_integer().get()));
            std::string name;
            f_file.add_constant(value, name);
            d->set_data_name(name);
        }
        [[fallthrough]];
    case node_t::NODE_FLOATING_POINT: // immediate double--use the copy in the private data section
        {
            offset_t const offset(f_file.get_constant_offset(d->get_data_name()));
            switch(op)
            {
            case sse_operation_t::SSE_OPERATION_ADD:
            case sse_operation_t::SSE_OPERATION_DIV:
            case sse_operation_t::SSE_OPERATION_LOAD:
            case sse_operation_t::SSE_OPERATION_MAX:
            case sse_operation_t::SSE_OPERATION_MIN:
            case sse_operation_t::SSE_OPERATION_MUL:
            case sse_operation_t::SSE_OPERATION_SUB:
                {
                    std::size_t const pos(f_file.get_current_text_offset());
                    std::uint8_t buf[] = {   // MOVSD disp32(%rip), %xmm
                        0xF2,
                        // TODO: add 0x44 if reg >= xmm8
                        0x0F,
                        sse_code,
                        static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                        static_cast<std::uint8_t>(offset >>  0),
                        static_cast<std::uint8_t>(offset >>  8),
                        static_cast<std::uint8_t>(offset >> 16),
                        static_cast<std::uint8_t>(offset >> 24),
                    };
                    f_file.add_text(buf, sizeof(buf));
                    f_file.add_relocation(
                              d->get_data_name()
                            , relocation_t::RELOCATION_CONSTANT_32BITS
                            , pos + 4
                            , f_file.get_current_text_offset() + adjust_offset);
                }
                break;

            case sse_operation_t::SSE_OPERATION_CVT2I:
                {
                    // if we had an INTEGER, it was caught in the previous
                    // case at the previous level so the following call
                    // cannot fail because of that
                    //
                    double const floating_point(static_cast<double>(d->get_node()->get_floating_point().get()));
                    std::int64_t const value(floating_point);
                    switch(get_smallest_size(value))
                    {
                    case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_64BITS:
                        {
                            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rn
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),       // 64 bits
                                static_cast<std::uint8_t>(0xB8 | (static_cast<int>(reg) & 7)),
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
                            std::uint8_t buf[] = {   // REX.W MOV $imm32, %rn
                                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                0xC7,
                                static_cast<std::uint8_t>(0xC0 | (static_cast<int>(reg) & 7)),
                                static_cast<std::uint8_t>(value >>  0),
                                static_cast<std::uint8_t>(value >>  8),
                                static_cast<std::uint8_t>(value >> 16),
                                static_cast<std::uint8_t>(value >> 24),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    }
                    //std::size_t const pos(f_file.get_current_text_offset());
                    //std::uint8_t buf[] = {
                    //    0xF2,       // REX.W CVTSD2SI disp32(%rip), %xmm
                    //    0x48,
                    //    0x0F,
                    //    0x2D,
                    //    static_cast<std::uint8_t>(0x05 | (reg << 3)),
                    //    static_cast<std::uint8_t>(offset >>  0),
                    //    static_cast<std::uint8_t>(offset >>  8),
                    //    static_cast<std::uint8_t>(offset >> 16),
                    //    static_cast<std::uint8_t>(offset >> 24),
                    //};
                    //f_file.add_text(buf, sizeof(buf));
                    //f_file.add_relocation(
                    //          d->get_data_name()
                    //        , relocation_t::RELOCATION_CONSTANT_32BITS
                    //        , pos + 5
                    //        , f_file.get_current_text_offset() + adjust_offset);
                }
                break;

            default:
                throw not_implemented(
                      "floating point operation ("
                    + std::to_string(static_cast<int>(op))
                    + ") not yet implemented in generate_reg_mem_floating_point()");

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
                throw internal_error("temporary not found in generate_reg_mem_floating_point()");
            }
            switch(temp_var->get_size())
            {
            case 1: // Boolean
                {
                    ssize_t const offset(temp_var->get_offset());
                    integer_size_t const offset_size(get_smallest_size(offset));
                    std::uint8_t rm(0x45);
                    switch(op)
                    {
                    case sse_operation_t::SSE_OPERATION_LOAD:  // MOV 64bit -> MOV 8bit
                        code = 0x8A;
                        break;

                    case sse_operation_t::SSE_OPERATION_CMP:  // CMP 64bit, imm8 -> CMP 8bit, imm8
                        code = 0x80;
                        rm = 0x7D;
                        break;

                    default:
                        throw not_implemented("8 bit code in generate_reg_mem_floating_point() not yet supported.");

                    }
                    switch(offset_size)
                    {
                    case integer_size_t::INTEGER_SIZE_1BIT:
                    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {    // MOV disp8(%rbp), %r8
                                code,
                                static_cast<std::uint8_t>(rm | (static_cast<int>(reg) << 3)),
                                static_cast<std::uint8_t>(offset),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {    // MOV disp32(%rbp), %r8
                                code,
                                static_cast<std::uint8_t>((rm ^ 0xC0) | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
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

            case 8: // Integer / Double
                {
                    ssize_t const offset(temp_var->get_offset());
                    integer_size_t const offset_size(get_smallest_size(offset));
                    switch(offset_size)
                    {
                    case integer_size_t::INTEGER_SIZE_1BIT:
                    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {    // MOVSD disp8(%rbp), %xmm
                                0xF2,
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        if(reg >= register_t::REGISTER_R8)
                        {
                            std::uint8_t buf[] = {
                                0x44,
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        {
                            std::uint8_t buf[] = {
                                0x0F,
                                sse_code,
                                static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        break;

                    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                        {
                            std::uint8_t buf[] = {    // MOVSD disp32(%rbp), %xmm
                                0xF2,
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        if(reg >= register_t::REGISTER_R8)
                        {
                            std::uint8_t buf[] = {
                                0x44,
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                        {
                            std::uint8_t buf[] = {
                                0x0F,
                                0x10,
                                static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                                static_cast<std::uint8_t>(offset >>  0),
                                static_cast<std::uint8_t>(offset >>  8),
                                static_cast<std::uint8_t>(offset >> 16),
                                static_cast<std::uint8_t>(offset >> 24),
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
                throw not_implemented("temporary size not yet supported in generate_reg_mem_floating_point()");

            }
        }
        else if(d->is_extern())
        {
            binary_variable const * var(f_file.get_extern_variable(d->get_string()));
            if(var == nullptr)
            {
                throw internal_error(
                      "extern variable \""
                    + d->get_string()
                    + "\" not found in generate_reg_mem_floating_point()");
            }
            switch(var->f_type)
            {
            case VARIABLE_TYPE_BOOLEAN:
                {
                    std::uint8_t rm(0x05);
                    switch(op)
                    {
                    case sse_operation_t::SSE_OPERATION_LOAD:  // MOV 64bit -> MOV 8bit
                        code = 0x8A;
                        break;

                    case sse_operation_t::SSE_OPERATION_CMP:  // CMP 64bit, imm8 -> CMP 8bit, imm8
                        code = 0x80;
                        rm = 0x3D;
                        break;

                    default:
                        throw not_implemented("8 bit code in generate_reg_mem_floating_point() not yet supported.");

                    }
//std::cerr << " -- select reg [" << static_cast<int>(reg) << "]\n";
                    std::size_t pos(f_file.get_current_text_offset());
                    if(code == 0x8A)
                    {
                        // the MOV requires one more byte using the
                        // MOVZX to clear all the upper bits
                        //
                        std::uint8_t buf[] = {      // REX.W MOVZBQ disp32(%rip), %r64
                            0x48,
                            0x0F,
                            0xB6,
                            static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                            0x00,                       // 32 bit offset
                            0x00,
                            0x00,
                            0x00,
                        };
                        f_file.add_text(buf, sizeof(buf));
                        ++pos;
                    }
                    else
                    {
                        std::uint8_t buf[] = {      // REX.W CMP disp32(%rip), %r8 (or $imm8?)
                            0x48,
                            code,
                            static_cast<std::uint8_t>(rm | ((static_cast<int>(reg) & 7) << 3)),
                            0x00,                       // 32 bit offset
                            0x00,
                            0x00,
                            0x00,
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                    f_file.add_relocation(
                              d->get_string()
                            , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                            , pos + 3
                            , f_file.get_current_text_offset() + adjust_offset);
                }
                break;

            case VARIABLE_TYPE_INTEGER:
                switch(sse_code)
                {
                case 0x10: // MOVSD with integers
                    {
                        std::size_t const pos(f_file.get_current_text_offset());
                        std::uint8_t buf[] = {      // MOV disp32(%rip), %rn
                            0x48,
                            code,
                            static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                            0x00,       // 32 bit offset
                            0x00,
                            0x00,
                            0x00,
                        };
                        f_file.add_text(buf, sizeof(buf));
                        f_file.add_relocation(
                                  d->get_string()
                                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                , pos + 3
                                , f_file.get_current_text_offset() + adjust_offset);
                    }
                    break;

                case 0x5D:  // MINSD
                case 0x5F:  // MAXSD
                    {
                        register_t const other_reg(
                                reg == register_t::REGISTER_XMM0
                                        ? register_t::REGISTER_XMM1
                                        : register_t::REGISTER_XMM0);

                        {
                            std::size_t const pos(f_file.get_current_text_offset());
                            std::uint8_t buf[] = {      // CVTSI2SD disp32(%rip), %xmm1
                                0xF2,
                                0x48,
                                0x0F,
                                0x2A,
                                static_cast<std::uint8_t>(0x05 | ((static_cast<int>(other_reg) & 7) << 3)),
                                0x00,       // 32 bit offset
                                0x00,
                                0x00,
                                0x00,
                            };
                            f_file.add_text(buf, sizeof(buf));
                            f_file.add_relocation(
                                      d->get_string()
                                    , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                    , pos + 5
                                    , f_file.get_current_text_offset() + adjust_offset);
                        }

                        {
                            std::uint8_t buf[] = {      // MAXSD or MINSD %xmm1, %xmm0
                                0xF2,
                                0x0F,
                                sse_code,
                                static_cast<std::uint8_t>(0xC0 | ((static_cast<int>(reg) & 7) << 3) | static_cast<int>(other_reg)),
                            };
                            f_file.add_text(buf, sizeof(buf));
                        }
                    }
                    break;

                default:
                    throw not_implemented("Integer/SSE operation not yet implemented in generate_reg_mem_floating_point() -- expected one of MOVSD/MINSD/MAXSD");

                }
                break;

            case VARIABLE_TYPE_FLOATING_POINT:
                switch(op)
                {
                case sse_operation_t::SSE_OPERATION_ADD:
                case sse_operation_t::SSE_OPERATION_CMP:
                case sse_operation_t::SSE_OPERATION_DIV:
                case sse_operation_t::SSE_OPERATION_LOAD:
                case sse_operation_t::SSE_OPERATION_MAX:
                case sse_operation_t::SSE_OPERATION_MIN:
                case sse_operation_t::SSE_OPERATION_MUL:
                case sse_operation_t::SSE_OPERATION_SUB:
                    {
                        std::size_t const pos(f_file.get_current_text_offset());
                        std::uint8_t buf[] = {       // MOVSD disp32(%rip), %xmm
                            0xF2,
                            0x0F,
                            sse_code,
                            static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                            0x00,       // 32 bit offset
                            0x00,
                            0x00,
                            0x00,
                            // CMP??SD uses an immediate as well, which is
                            // added by the caller
                        };
                        f_file.add_text(buf, sizeof(buf));
                        f_file.add_relocation(
                                  d->get_string()
                                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                , pos + 4
                                , f_file.get_current_text_offset() + adjust_offset);
                    }
                    break;

                case sse_operation_t::SSE_OPERATION_CVT2I:
                    {
                        std::size_t const pos(f_file.get_current_text_offset());
                        std::uint8_t buf[] = {     // REX.W CVTSD2SI disp32(%rip), %rn
                            0xF2,
                            0x48,
                            0x0F,
                            0x2D,
                            static_cast<std::uint8_t>(0x05 | (static_cast<int>(reg) << 3)),
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                        };
                        f_file.add_text(buf, sizeof(buf));
                        f_file.add_relocation(
                                  d->get_string()
                                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                                , pos + 5
                                , f_file.get_current_text_offset() + adjust_offset);
                    }
                    break;

                default:
                    throw not_implemented("SSE operation not yet implemented in generate_reg_mem_floating_point()");

                }
                break;

            default:
                throw not_implemented("WARNING: generate_reg_mem_floating_point() hit an extern variable type not yet implemented...");

            }
        }
        else
        {
            throw not_implemented("WARNING: generate_reg_mem_floating_point() hit a variable type not yet implemented...");
        }
        break;

    default:
        throw not_implemented("WARNING: generate_reg_mem_floating_point() hit a data type not yet implemented...");

    }
}


void binary_assembler::generate_reg_mem_string(data::pointer_t d, register_t const reg, int adjust_offset)
{
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_STRING: // immediate string, load pointer in reg
        {
            offset_t const offset(f_file.get_constant_offset(d->get_data_name()));
            std::size_t const pos(f_file.get_current_text_offset());
            std::uint8_t buf[] = {  // REX.W LEA disp32(%rip), %rn
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                0x8D,
                static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                static_cast<std::uint8_t>(offset >>  0),
                static_cast<std::uint8_t>(offset >>  8),
                static_cast<std::uint8_t>(offset >> 16),
                static_cast<std::uint8_t>(offset >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
            f_file.add_relocation(
                      d->get_data_name()
                    , relocation_t::RELOCATION_CONSTANT_32BITS
                    , pos + 3
                    , f_file.get_current_text_offset() + adjust_offset);
        }
        break;

    case node_t::NODE_VARIABLE:
        if(d->is_temporary())
        {
            std::string const name(n->get_string());
            temporary_variable * temp_var(f_file.find_temporary_variable(name));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary \""
                    + name
                    + "\" not found in generate_reg_mem_string()");
            }
            if(temp_var->get_type() != node_t::NODE_STRING)
            {
                throw internal_error("temporary \""
                    + name
                    + "\" in generate_reg_mem_string() is of type "
                    + node::type_to_string(temp_var->get_type())
                    + " when the compiler expected it to be of type string.");
            }
            ssize_t const offset(temp_var->get_offset());
            integer_size_t const offset_size(get_smallest_size(offset));
            switch(offset_size)
            {
            case integer_size_t::INTEGER_SIZE_1BIT:
            case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                {
                    std::uint8_t buf[] = {  // REX.W LEA disp8(%rbp), %rn
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0x8D,
                        static_cast<std::uint8_t>(0x45 | (static_cast<int>(reg) << 3)),
                        static_cast<std::uint8_t>(offset),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }
                break;

            case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
            case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                {
                    std::uint8_t buf[] = {  // REX.W LEA disp32(%rbp), %rn
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0x8D,
                        static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                        static_cast<std::uint8_t>(offset >>  0),
                        static_cast<std::uint8_t>(offset >>  8),
                        static_cast<std::uint8_t>(offset >> 16),
                        static_cast<std::uint8_t>(offset >> 24),
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
        else if(d->is_extern())
        {
            binary_variable const * var(f_file.get_extern_variable(d->get_string()));
            if(var == nullptr)
            {
                throw internal_error(
                      "extern variable \""
                    + d->get_string()
                    + "\" not found in generate_reg_mem_string()");
            }
            // TODO: other variable types need an auto-cast (but that should
            //       be done by the compiler ahead of time?)
            //
            switch(var->f_type)
            {
            case VARIABLE_TYPE_STRING:
                {
                    std::size_t pos(f_file.get_current_text_offset());
                    std::uint8_t buf[] = {      // REX.W LEA disp32(%rip), %rn
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0x8D,
                        static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                    };
                    f_file.add_text(buf, sizeof(buf));
                    f_file.add_relocation(
                              d->get_string()
                            , relocation_t::RELOCATION_VARIABLE_32BITS
                            , pos + 3
                            , f_file.get_current_text_offset() + adjust_offset);
                }
                break;

            default:
                throw not_implemented(
                      std::string("WARNING: generate_reg_mem_string() hit an extern variable type \"")
                    + variable_type_to_string(var->f_type)
                    + "\" not yet implemented...");

            }
        }
        else
        {
            throw not_implemented("WARNING: generate_reg_mem_string() hit a variable type not yet implemented...");
        }
        break;

    default:
        throw not_implemented(
              "WARNING: generate_reg_mem_string() hit data type "
            + std::to_string(static_cast<int>(d->get_data_type()))
            + "/"
            + node::type_to_string(d->get_data_type())
            + " not yet implemented...");

    }
}


void binary_assembler::generate_load_string_size(data::pointer_t d, register_t const reg)
{
    node::pointer_t n(d->get_node());
    if(d->is_temporary())
    {
        std::string const name(n->get_string());
        temporary_variable * temp_var(f_file.find_temporary_variable(name));
        if(temp_var == nullptr)
        {
            throw internal_error("temporary \""
                + name
                + "\" not found in generate_load_string_size()");
        }
        if(temp_var->get_type() != node_t::NODE_STRING)
        {
            throw internal_error("temporary \""
                + name
                + "\" in generate_load_string_size() is not of type string.");
        }
        ssize_t const offset(temp_var->get_offset() + offsetof(binary_variable, f_data_size));
        integer_size_t const offset_size(get_smallest_size(offset));
        switch(offset_size)
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {  // MOV disp8(%rbp), %eax
                    0x8B,
                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                    static_cast<std::uint8_t>(offset),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {  // MOV disp32(%rbp), %eax
                    0x8B,
                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                    static_cast<std::uint8_t>(offset >>  0),
                    static_cast<std::uint8_t>(offset >>  8),
                    static_cast<std::uint8_t>(offset >> 16),
                    static_cast<std::uint8_t>(offset >> 24),
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
                + name
                + "\" (type: "
                + std::to_string(static_cast<int>(offset_size))
                + " for size: "
                + std::to_string(offset)
                + ").");

        }
    }
    else if(d->is_extern())
    {
        std::size_t const pos(f_file.get_current_text_offset());
        std::uint8_t buf[] = {      // MOV disp32(%rip), %eax
            0x8B,
            static_cast<std::uint8_t>(0x05 + ((static_cast<int>(reg) & 7) << 3)),
            0x00,
            0x00,
            0x00,
            0x00,
        };
        f_file.add_text(buf, sizeof(buf));
        f_file.add_relocation(
                  d->get_string()
                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA_SIZE
                , pos + 2
                , f_file.get_current_text_offset());
    }
    else
    {
        throw not_implemented("load string size not implemented for this data type?!");
    }
}


void binary_assembler::generate_pointer_to_temporary(temporary_variable * temp_var, register_t reg)
{
    ssize_t const offset(temp_var->get_offset());
    integer_size_t const offset_size(get_smallest_size(offset));
    switch(offset_size)
    {
    case integer_size_t::INTEGER_SIZE_1BIT:
    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
        {
            std::uint8_t buf[] = {  // REX.W LEA disp8(%rbp), %rn
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                0x8D,
                static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                static_cast<std::uint8_t>(offset),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
        {
            std::uint8_t buf[] = {  // REX.W LEA disp32(%rbp), %rn
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                0x8D,
                static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                static_cast<std::uint8_t>(offset >>  0),
                static_cast<std::uint8_t>(offset >>  8),
                static_cast<std::uint8_t>(offset >> 16),
                static_cast<std::uint8_t>(offset >> 24),
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


void binary_assembler::generate_pointer_to_variable(data::pointer_t d, register_t const reg, int adjust_offset)
{
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_STRING:
        generate_reg_mem_string(d, reg, adjust_offset);
        break;

    case node_t::NODE_VARIABLE:
        if(d->is_temporary())
        {
            std::string const name(n->get_string());
            temporary_variable * temp_var(f_file.find_temporary_variable(name));
            if(temp_var == nullptr)
            {
                throw internal_error("temporary \""
                    + name
                    + "\" not found in generate_pointer_to_variable()");
            }
std::cerr << "--- generate pointer to temp var \"" << name << "\".\n";
            generate_pointer_to_temporary(temp_var, reg);
        }
        else if(d->is_extern())
        {
            binary_variable const * var(f_file.get_extern_variable(d->get_string()));
            if(var == nullptr)
            {
                throw internal_error(
                      "extern variable \""
                    + d->get_string()
                    + "\" not found in generate_pointer_to_variable()");
            }
            std::size_t pos(f_file.get_current_text_offset());
            std::uint8_t buf[] = {      // REX.W LEA disp32(%rip), %rn
                static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                0x8D,
                static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                0x00,
                0x00,
                0x00,
                0x00,
            };
            f_file.add_text(buf, sizeof(buf));
            f_file.add_relocation(
                      d->get_string()
                    , relocation_t::RELOCATION_VARIABLE_32BITS
                    , pos + 3
                    , f_file.get_current_text_offset() + adjust_offset);
        }
        else
        {
            throw not_implemented("WARNING: generate_pointer_to_variable() hit a variable type not yet implemented...");
        }
        break;

    default:
        throw not_implemented(
              "WARNING: generate_pointer_to_variable() hit data type "
            + std::to_string(static_cast<int>(d->get_data_type()))
            + "/"
            + node::type_to_string(d->get_data_type())
            + " not yet implemented...");

    }
}


void binary_assembler::generate_store_integer(data::pointer_t d, register_t const reg)
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
                    throw internal_error("temporary not found in generate_store_integer()");
                }
                switch(temp_var->get_size())
                {
                case 1: // Boolean
                    {
                        ssize_t const offset(temp_var->get_offset());
                        integer_size_t const offset_size(get_smallest_size(offset));
                        switch(offset_size)
                        {
                        case integer_size_t::INTEGER_SIZE_1BIT:
                        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                            if(reg >= register_t::REGISTER_RSP)
                            {
                                std::uint8_t buf[] = {    // MOV %rn.b, disp8(%rbp)
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x88,
                                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                                                // 'r' and disp(rbp) (r/m)
                                    static_cast<std::uint8_t>(offset),
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            else
                            {
                                std::uint8_t buf[] = {    // MOV %rn.b, disp8(%rbp)
                                    0x88,
                                    static_cast<std::uint8_t>(0x45 | (static_cast<int>(reg) << 3)),
                                    static_cast<std::uint8_t>(offset),
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                            if(reg >= register_t::REGISTER_RSP)
                            {
                                std::uint8_t buf[] = {    // REX.W MOV %rn.b, disp8(%rbp)
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x88,
                                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset >>  0),
                                    static_cast<std::uint8_t>(offset >>  8),
                                    static_cast<std::uint8_t>(offset >> 16),
                                    static_cast<std::uint8_t>(offset >> 24),
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            else
                            {
                                std::uint8_t buf[] = {    // REX.W MOV %rn.b, disp32(%rbp)
                                    0x88,
                                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset >>  0),
                                    static_cast<std::uint8_t>(offset >>  8),
                                    static_cast<std::uint8_t>(offset >> 16),
                                    static_cast<std::uint8_t>(offset >> 24),
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

                case 8: // Integer / Double
                    {
                        ssize_t const offset(temp_var->get_offset());
                        integer_size_t const offset_size(get_smallest_size(offset));
                        switch(offset_size)
                        {
                        case integer_size_t::INTEGER_SIZE_1BIT:
                        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {   // REX.W MOV %rn, disp8(%rbp)
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x89,
                                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset),
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {   // REX.W MOV %rn, disp32(%rbp)
                                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                    0x89,
                                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                                    static_cast<std::uint8_t>(offset >>  0),
                                    static_cast<std::uint8_t>(offset >>  8),
                                    static_cast<std::uint8_t>(offset >> 16),
                                    static_cast<std::uint8_t>(offset >> 24),
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

                case sizeof(binary_variable):
                    if(!n->get_flag(flag_t::NODE_VARIABLE_FLAG_VARIABLE))
                    {
                        throw not_implemented("temporary size not supported yet (sizeof(binary_varible) when the NODE_VARIABLE_FLAG_VARIABLE is not set)");
                    }
                    generate_save_reg_in_binary_variable(temp_var, reg, get_type_of_node(d->get_node()));
                    break;

                default:
                    throw not_implemented("temporary size not yet supported in generate_store_integer()");

                }
            }
            else if(d->is_extern())
            {
                std::size_t const pos(f_file.get_current_text_offset());
                std::uint8_t buf[] = {
                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                                                // 64 bits
                    0x89,                       // MOV r := m
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
                        , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                        , pos + 3
                        , f_file.get_current_text_offset());
            }
            else
            {
                throw not_implemented("generate_store_integer() unhandled variable type.");
            }
        }
        break;

    default:
        throw not_implemented("generate_store_integer() hit a data type other than already implemented.");

    }
}


void binary_assembler::generate_store_floating_point(data::pointer_t d, register_t const reg)
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
                    throw internal_error("temporary not found in generate_store_floating_point()");
                }
                switch(temp_var->get_type())
                {
                case node_t::NODE_DOUBLE:
                    {
                        ssize_t const offset(temp_var->get_offset());
                        integer_size_t const offset_size(get_smallest_size(offset));
                        switch(offset_size)
                        {
                        case integer_size_t::INTEGER_SIZE_1BIT:
                        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {    // MOVSD %xmm, disp8(%rbp)
                                    0xF2,
                                    // TODO: if reg >= 8 insert 0x44
                                    0x0F,
                                    0x11,
                                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                                                                // 'r' and disp(rbp) (r/m)
                                    static_cast<std::uint8_t>(offset),
                                                                // 8 bit offset
                                };
                                f_file.add_text(buf, sizeof(buf));
                            }
                            break;

                        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
                        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                            {
                                std::uint8_t buf[] = {    // MOVSD %xmm, disp32(%rbp)
                                    0xF2,
                                    // TODO: if reg >= 8 insert 0x44
                                    0x0F,
                                    0x11,
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
                    throw not_implemented("temporary size not yet supported in generate_store_floating_point()");

                }
            }
            else if(d->is_extern())
            {
                std::size_t const pos(f_file.get_current_text_offset());
                std::uint8_t buf[] = {    // MOVSD %xmm, disp32(%rip)
                    0xF2,
                    // TODO: if reg >= 8 insert 0x44
                    0x0F,
                    0x11,
                    static_cast<std::uint8_t>(0x05 | ((static_cast<int>(reg) & 7) << 3)),
                    0x00,
                    0x00,
                    0x00,
                    0x00,
                };
                f_file.add_text(buf, sizeof(buf));
                f_file.add_relocation(
                          d->get_string()
                        , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                        , pos + 4
                        , f_file.get_current_text_offset());
            }
            else
            {
                throw not_implemented("generate_store_floating_point() unhandled variable type.");
            }
        }
        break;

    default:
        throw not_implemented("generate_store_floating_point() hit a data type other than already implemented.");

    }
}


void binary_assembler::generate_store_string(data::pointer_t d, register_t const reg)
{
    // reg is pointer to a variable_t to be copied
    //
    node::pointer_t n(d->get_node());
    switch(d->get_data_type())
    {
    case node_t::NODE_STRING:
        throw not_implemented("generate_store_string() hit data type STRING which is a constant and you cannot store in a constant.");

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
                    throw internal_error("temporary not found in generate_store_string().");
                }
                if(temp_var->get_type() != node_t::NODE_STRING)
                {
                    throw internal_error("temporary in generate_store_string() is not of type string.");
                }

                // load source if not already RSI
                //
                if(reg != register_t::REGISTER_RSI)
                {
                    std::uint8_t buf[] = {       // MOV %rn, %rsi
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0x89,
                        static_cast<std::uint8_t>(0xC6 | ((static_cast<int>(reg) & 7) << 3)),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                // load destination
                //
                ssize_t const offset(temp_var->get_offset());
                switch(get_smallest_size(offset))
                {
                case integer_size_t::INTEGER_SIZE_1BIT:
                case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
                    {
                        std::uint8_t buf[] = {  // REX.W LEA disp32(%rbp), %rdi
                            0x48,
                            0x8D,
                            0x7D,
                            static_cast<std::uint8_t>(offset),
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                    break;

                case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
                case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:        // there is no r64 + imm16, use r64 + imm32 instead
                case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
                case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
                    {
                        std::uint8_t buf[] = {  // REX.W LEA disp32(%rbp), %rdi
                            0x48,
                            0x8D,
                            0xBD,
                            static_cast<std::uint8_t>(offset >>  0),
                            static_cast<std::uint8_t>(offset >>  8),
                            static_cast<std::uint8_t>(offset >> 16),
                            static_cast<std::uint8_t>(offset >> 24),
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                    break;

                default:
                    throw not_implemented("generate_store_string() unhandled integer size for displacement.");

                };

                generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_COPY);
            }
            else if(d->is_extern())
            {
                // load source if not already RSI
                //
                if(reg != register_t::REGISTER_RSI)
                {
                    std::uint8_t buf[] = {       // MOV %rn, %rsi
                        static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                        0x89,
                        static_cast<std::uint8_t>(0xC6 | ((static_cast<int>(reg) & 7) << 3)),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                std::size_t const pos(f_file.get_current_text_offset());
                std::uint8_t buf[] = {      // LEA disp32(%rip), %rdi
                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                    0x8D,
                    0x3D,
                    0x00,
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

                generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_COPY);
            }
            else
            {
                throw not_implemented("generate_store_string() unhandled variable type.");
            }
        }
        break;

    default:
        throw not_implemented(
              "generate_store_string() hit data type "
            + std::string(node::type_to_string(d->get_data_type()))
            + " which is not yet implemented.");

    }
}


void binary_assembler::generate_external_function_call(external_function_t func)
{
    // load pointer to table in RAX
    //
    generate_reg_mem_integer(f_extern_functions, register_t::REGISTER_RAX);

    offset_t const disp(func * 8);
    if(disp == 0)
    {
        std::uint8_t buf[] = {
            0xFF,       // CALL *(%rax)
            0x10,
        };
        f_file.add_text(buf, sizeof(buf));
    }
    else if(disp < 128)
    {
        std::uint8_t buf[] = {
            0xFF,       // CALL *disp8(%rax)
            0x50,
            static_cast<std::uint8_t>(disp),
        };
        f_file.add_text(buf, sizeof(buf));
    }
    else
    {
        std::uint8_t buf[] = {
            0xFF,       // CALL *disp32(%rax)
            0x90,
            static_cast<std::uint8_t>(disp >>  0),
            static_cast<std::uint8_t>(disp >>  8),
            static_cast<std::uint8_t>(disp >> 16),
            static_cast<std::uint8_t>(disp >> 24),
        };
        f_file.add_text(buf, sizeof(buf));
    }
}


void binary_assembler::generate_absolute_value(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    //variable_type_t const type(get_type_of_node(lhs->get_node()));
    node_t const type(lhs->get_node()->get_type());
//std::cerr << "--- generate absolute value =\n"
//<< *lhs->get_node()
//<< "\n"
//<< "--- with lhs type = " << static_cast<int>(type)
//<< "\n";
    switch(type)
    {
    case node_t::NODE_INTEGER:
        {
            variable_type_t const var_type(get_type_of_node(lhs->get_node()));
            switch(var_type)
            {
            case VARIABLE_TYPE_INTEGER:
                {
                    // integers to integer (function abs(Integer) : Integer)
                    //
                    // TBD: err if integer is out of range? (i.e. MIN_INT)
                    //
                    std::int64_t const value(llabs(lhs->get_node()->get_integer().get()));
                    std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                        0x48,
                        0xB8,
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

                    generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
                }
                break;

            default:
                throw not_implemented("generate_absolute_value() unhandled result type for integers.");

            }
        }
        break;

    case node_t::NODE_FLOATING_POINT:
        {
            double const floating_point(fabs(lhs->get_node()->get_floating_point().get()));
            std::int64_t const value(*reinterpret_cast<std::int64_t const *>(&floating_point));
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
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

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        }
        break;

    case node_t::NODE_VARIABLE:
        {
            variable_type_t const var_type(get_type_of_node(lhs->get_node()));
            switch(var_type)
            {
            case VARIABLE_TYPE_INTEGER:
                generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
                {
                    // ABS %rax with three instructions
                    std::uint8_t buf[] = {
                        0x48,   // REX.W MOV %rax, %rcx
                        0x8B,
                        0xC8,

                        0x48,   // REX.W SAR $63, %rcx
                        0xC1,
                        0xF9,
                        0x3F,

                        0x48,   // REX.W XOR %rcx, %rax
                        0x31,
                        0xC8,

                        0x48,   // REX.W SUB %rcx, %rax
                        0x29,
                        0xC8,
                    };
                    f_file.add_text(buf, sizeof(buf));
                }
                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
                break;

            case VARIABLE_TYPE_FLOATING_POINT:
                generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);

                // there is no ABSPD instruction, use integers instructions
                // to remove the upper bit
                {
                    std::uint8_t buf[] = {
                        0x66,   // PADDQ %xmm0, %xmm0
                        0x0F,
                        0xD4,
                        0xC0,

                        0x66,   // PSRLQ $1, %xmm0
                        0x0F,
                        0x73,
                        0xD0,
                        0x01,
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
                break;

            default:
                throw not_implemented("generate_absolute_value() unhandled variable type.");

            }
        }
        break;

    default:
        throw not_implemented(
              "absolute value node type "
            + std::to_string(static_cast<int>(type))
            + " not implemented.");

    }
}


void binary_assembler::generate_additive(operation::pointer_t op)
{
    bool is_add(false);
    bool is_assignment(false);
    switch(op->get_operation())
    {
    case node_t::NODE_ADD:
        is_add = true;
        break;

    case node_t::NODE_ASSIGNMENT_ADD:
        is_add = true;
        is_assignment = true;
        break;

    case node_t::NODE_ASSIGNMENT_SUBTRACT:
        is_assignment = true;
        break;

    default:
        break;

    }
    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());

    variable_type_t const op_type(get_type_of_node(op->get_node()));
    switch(op_type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
        // floating points
        //
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);

        switch(rhs->get_integer_size())
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:        // there is no r64 + imm16, use r64 + imm32 instead
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_64BITS:
            //{
            //    generate_reg_mem_floating_point(rhs, register_t::REGISTER_RCX);
            //    std::uint8_t buf[] = {
            //        0xF2,       // ADD or SUB %xmm1, %xmm0
            //        0x0F,
            //        static_cast<std::uint8_t>(is_add ? 0x58 : 0x5C),
            //        0xC8,
            //    };
            //    f_file.add_text(buf, sizeof(buf));
            //}
            //break;

        case integer_size_t::INTEGER_SIZE_FLOATING_POINT:
            generate_reg_mem_floating_point(
                      rhs
                    , register_t::REGISTER_XMM0
                    , is_add
                        ? sse_operation_t::SSE_OPERATION_ADD
                        : sse_operation_t::SSE_OPERATION_SUB);
            break;

        default:
            if(rhs->get_data_type() == node_t::NODE_VARIABLE)
            {
                // a variable needs to be loaded from memory so we need an ADD
                // which reads the variable location
                //
                generate_reg_mem_floating_point(
                          rhs
                        , register_t::REGISTER_XMM0
                        , is_add
                            ? sse_operation_t::SSE_OPERATION_ADD
                            : sse_operation_t::SSE_OPERATION_SUB);
            }
            else
            {
                if(rhs->get_data_type() != node_t::NODE_INTEGER)
                {
                    throw not_implemented(
                          std::string("trying to add/subtract a \"")
                        + node::type_to_string(rhs->get_data_type())
                        + "\" which is not yet implemented.");
                }
                throw not_implemented(
                      "found integer size "
                    + std::to_string(static_cast<int>(rhs->get_integer_size()))
                    + " which is not yet implemented in generate_additive().");
            }
            break;

        }

        if(is_assignment)
        {
            generate_store_floating_point(op->get_left_handside(), register_t::REGISTER_RAX);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_INTEGER:
        // integers
        //
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);

        switch(rhs->get_integer_size())
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {
                    0x48,       // 64 bits
                    0x83,       // ADD or SUB r64 +/-= imm8
                    static_cast<std::uint8_t>(is_add ? 0xC0 : 0xE8),
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
                    static_cast<std::uint8_t>(is_add ? 0x05 : 0x2D),
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
                generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);
                std::uint8_t buf[] = {       // ADD or SUB %rdx, %rax
                    0x48,
                    static_cast<std::uint8_t>(is_add ? 0x01 : 0x29),
                    0xD0,
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        default:
            if(rhs->get_data_type() == node_t::NODE_VARIABLE)
            {
                // a variable needs to be loaded from memory so we need an ADD
                // which reads the variable location
                //
                generate_reg_mem_integer(rhs, register_t::REGISTER_RAX, static_cast<std::uint8_t>(is_add ? 0x03 : 0x2B));
            }
            else
            {
                if(rhs->get_data_type() != node_t::NODE_INTEGER)
                {
                    throw not_implemented(
                          std::string("trying to add/subtract a \"")
                        + node::type_to_string(rhs->get_data_type())
                        + "\" which is not yet implemented.");
                }
                throw not_implemented(
                      "found integer size "
                    + std::to_string(static_cast<int>(rhs->get_integer_size()))
                    + " which is not yet implemented in generate_additive().");
            }
            break;

        }

        if(is_assignment)
        {
            generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
        }
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_reg_mem_string(rhs, register_t::REGISTER_RDX);

        // TODO: this optimization works unless the next step makes use of
        //       the temporary variable (which at least happens for the
        //       last expression--a.k.a. the result)
        //
        //       I think those optimizations would have to happen in the
        //       flattening step (output.cpp)
        //
        //if(is_assignment)
        //{
        //    // save the results in the same location (RSI == RDI)
        //    //
        //    //generate_reg_mem_string(lhs, register_t::REGISTER_RDI);
        //    std::uint8_t buf[] = {       // MOV %rsi, %rdi
        //        0x48,
        //        0x89,
        //        0xF7,
        //    };
        //    f_file.add_text(buf, sizeof(buf));
        //}
        //else
        //{
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
        //}

        if(is_add)
        {
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_CONCAT);
        }
        else
        {
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_UNCONCAT);
        }
        if(is_assignment)
        {
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RSI);
            generate_store_string(lhs, register_t::REGISTER_RSI);
        }
        break;

    default:
        throw not_implemented(
              "additive node type "
            + std::to_string(static_cast<int>(op_type))
            + " not implemented.");

    }
}


void binary_assembler::generate_compare(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());

    // if the left or right hand side are floating points then use the
    // floating point compare mechanism
    //
    if(get_type_of_node(lhs->get_node()) == VARIABLE_TYPE_FLOATING_POINT
    || get_type_of_node(rhs->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        // TODO: if lhs or rhs is a string, we need to transform it to a
        //       floating point first
        //
        std::uint8_t cmp_code(0x00);
        bool swapped(false);
        switch(op->get_operation())
        {
        case node_t::NODE_ALMOST_EQUAL:
            {
                generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
                generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM1);

                {
                    // this is way too long as one single block of code!
                    //
                    // TODO: use an external function instead, especially
                    //       if we want to support a variable epsilon
                    //       (right now, we use 0.00001, hard coded)
                    //
                    std::uint8_t buf[] = {
                        0xF2,       // MOVSD %xmm0, %xmm3
                        0x0F,
                        0x10,
                        0xD8,

                        0xF2,       // CMPORDSD %xmm1, %xmm3
                        0x0F,
                        0xC2,
                        0xD9,
                        0x07,

                        0x66,       // REX.W MOVQ %xmm3, %rax
                        0x48,
                        0x0F,
                        0x7E,
                        0xD8,

                        0x85,       // TEST %eax, %eax
                        0xC0,

                        0x74,       // JZ false
                        0x53,

                        0x66,       // COMISD %xmm0, %xmm1
                        0x0F,
                        0x2F,
                        0xC8,

                        0x74,       // JE true
                        0x76,

                        // here xmm3 is already 0xFFFFFFFF:FFFFFFFF
                        //0xF2,       // CMPEQSD %xmm3, %xmm3
                        //0x0F,
                        //0xC2,
                        //0xDB,
                        //0x00,

                        0x66,       // PSRLD $1, %xmm3
                        0x0F,
                        0x73,
                        0xD3,
                        0x01,

                        0xF2,       // MOVSD %xmm0, %xmm2
                        0x0F,
                        0x10,
                        0xD0,

                        0xF2,       // SUBSD %xmm1, %xmm2
                        0x0F,
                        0x5C,
                        0xD1,

                        0x66,       // REX.W MOVQ %xmm0, %rax
                        0x48,
                        0x0F,
                        0x7E,
                        0xC0,

                        0x66,       // REX.W MOVQ %xmm1, %rdx
                        0x48,
                        0x0F,
                        0x7E,
                        0xCA,

                        0x66,       // ANDPD %xmm3, %xmm2 -- xmm2 = fabs(xmm0 - xmm1)
                        0x0F,
                        0x54,
                        0xD3,

                        0x48,       // REX.W AND %rdx, %rax  (xmm0 == 0 || xmm1 == 0)
                        0x21,
                        0xD0,

                        0x48,       // REX.W ADD %rax, %rax (ignore sign)
                        0x01,
                        0xC0,

                        0x74,       // JZ zero
                        0x15,

                        0x48,       // REX.W MOV $0x100000:00000000, %rax   (i.e. std::numeric_limits<double>::min())
                        0xB8,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0x10,
                        0x00,

                        0x66,       // MOVQ %rax, %xmm4
                        0x48,
                        0x0F,
                        0x6E,
                        0xE0,

                        0x66,       // COMISD %xmm4, %xmm2
                        0x0F,
                        0x2F,
                        0xD4,

                        0x73,       // JNB full_cmp
                        0x19,

                    //zero: %xmm0 == 0 || %xmm1 == 0
                        0x48,       // REX.W MOV $0xA7C5AC472, %rax   (i.e. epsilon * std::numeric_limits<double>::min())
                        0xB8,
                        0x72,
                        0xC4,
                        0x5A,
                        0x7C,
                        0x0A,
                        0x00,
                        0x00,
                        0x00,

                        0x66,       // MOVQ %rax, %xmm4
                        0x48,
                        0x0F,
                        0x6E,
                        0xE0,

                        0x66,       // COMISD %xmm4, %xmm2
                        0x0F,
                        0x2F,
                        0xD4,

                        0x72,       // JB true
                        0x29,

                        //[[fallthrough]];

                    //false:
                        0x33,       // XOR $rax, $rax
                        0xC0,

                        0xEB,       // JMP done
                        0x2A,

                    //full_cmp:
                        0x66,       // ANDPD %xmm3, %xmm0 -- xmm0 = fabs(xmm1)
                        0x0F,
                        0x54,
                        0xC3,

                        0x66,       // ANDPD %xmm3, %xmm1 -- xmm1 = fabs(xmm0)
                        0x0F,
                        0x54,
                        0xCB,

                        0xF2,       // ADDSD %xmm1, %xmm0
                        0x0F,
                        0x58,
                        0xC1,

                        0xF2,       // DIVSD %xmm0, %xmm2 -- %xmm2 = %xmm2 / %xmm0
                        0x0F,
                        0x5E,
                        0xD0,

                        0x48,       // REX.W MOV $0x3EE4F8B588E368F1, %rax   (i.e. epsilon)
                        0xB8,
                        0xF1,
                        0x68,
                        0xE3,
                        0x88,
                        0xB5,
                        0xF8,
                        0xE4,
                        0x3E,

                        0x66,       // MOVQ %rax, %xmm1
                        0x48,
                        0x0F,
                        0x6E,
                        0xC8,

                        0x66,       // COMISD %xmm1, %xmm2
                        0x0F,
                        0x2F,
                        0xD1,

                        0x73,       // JBN false
                        0xD7,

                        //[[fallthrough]];

                    //true:
                        0xB8,       // MOV $1, $eax
                        0x01,
                        0x00,
                        0x00,
                        0x00,
                    //done:
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
            return;

        case node_t::NODE_COMPARE:
            {
                generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
                generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM0, sse_operation_t::SSE_OPERATION_SUB);

                {
                    std::uint8_t buf[] = {
                        //0x33,       // XOR %eax, %eax (rax := 0)
                        //0xC0,

                        0x66,       // REX.W MOVQ %xmm0, %rcx
                        0x48,
                        0x0F,
                        0x7E,
                        0xC1,

                        0x48,       // REX.W CMP $0, %rcx
                        0x83,
                        0xF9,
                        0x00,

                        0x0F,       // SETG %al
                        0x9F,
                        0xC0,

                        0x0F,       // SETL %cl
                        0x9C,
                        0xC1,

                        0x28,       // SUB %cl, %al
                        0xC8,

                        0x48,       // REX.W MOVSX %al, %rax
                        0x0F,
                        0xBE,
                        0xC0,
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
            return;

        case node_t::NODE_EQUAL:
        case node_t::NODE_SMART_MATCH:
        case node_t::NODE_STRICTLY_EQUAL:
            // for double, smart match is the same as equal
            //
            // no coersion here, so strictly equal is the same
            //
            // default cmp_code is EQUAL
            break;

        case node_t::NODE_LESS:
            cmp_code = 0x01;
            break;

        case node_t::NODE_LESS_EQUAL:
            cmp_code = 0x02;
            break;

        case node_t::NODE_GREATER:
            cmp_code = 0x01;
            swapped = true;
            break;

        case node_t::NODE_GREATER_EQUAL:
            cmp_code = 0x02;
            swapped = true;
            break;

        case node_t::NODE_NOT_EQUAL:
        case node_t::NODE_STRICTLY_NOT_EQUAL:
            // no coersion here, so strictly not equal is the same
            //
            cmp_code = 0x04;
            break;

        default:
            throw internal_error("generate_compare() called with the wrong operation.");

        }
        if(swapped)
        {
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM0);
            generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0, sse_operation_t::SSE_OPERATION_CMP, 1);
        }
        else
        {
            generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM0, sse_operation_t::SSE_OPERATION_CMP, 1);
        }

        {
            std::uint8_t buf[] = {
                cmp_code,   // this is part of the CMP??SD generated by generate_reg_mem_floating_point()

                0x66,       // REX.W MOVQ %xmm0, %rax
                0x48,
                0x0F,
                0x7E,
                0xC0,

                0x48,       // REX.W SHR $63, %rax
                0xC1,
                0xE8,
                0x3F,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
    else if(get_type_of_node(lhs->get_node()) == VARIABLE_TYPE_INTEGER
         || get_type_of_node(lhs->get_node()) == VARIABLE_TYPE_BOOLEAN
         || get_type_of_node(rhs->get_node()) == VARIABLE_TYPE_INTEGER
         || get_type_of_node(rhs->get_node()) == VARIABLE_TYPE_BOOLEAN)
    {
        // TODO: if lhs or rhs is a string, we need to transform it to an
        //       integer first (we may need to separate booleans?)
        //
        generate_reg_mem_integer(lhs, register_t::REGISTER_RDX);

        {
            std::uint8_t buf[] = {       // XOR %eax, %eax (rax := 0)
                0x33,
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        generate_reg_mem_integer(rhs, register_t::REGISTER_RDX, 0x3B);
        if(op->get_operation() == node_t::NODE_COMPARE)
        {
            // shortest code "found" here:
            //    https://codegolf.stackexchange.com/questions/259087/write-the-smallest-possible-code-in-x86-64-to-implement-the-operator/259110#259110
            //
            std::uint8_t buf[] = {
                0x0F,       // SETG %al
                0x9F,
                0xC0,

                0x0F,       // SETL %cl
                0x9C,
                0xC1,

                0x28,       // SUB %cl, %al
                0xC8,

                0x48,       // REX.W MOVSX %al, %rax
                0x0F,
                0xBE,
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));
        }
        else
        {
            std::uint8_t buf[] = {
                0x0F,
                0x00,       // SETcc r64
                0xC0,       // r/m
            };

            switch(op->get_operation())
            {
            case node_t::NODE_ALMOST_EQUAL:
            case node_t::NODE_EQUAL:
            case node_t::NODE_SMART_MATCH:
            case node_t::NODE_STRICTLY_EQUAL:
                // for integer, almost equal & smart match are the same as equal
                // also if coersion happens, it wouldn't be here so strictly
                // equal is the same too
                //
                buf[1] = 0x94;
                break;

            case node_t::NODE_LESS:
                buf[1] = 0x9C;
                break;

            case node_t::NODE_LESS_EQUAL:
                buf[1] = 0x9E;
                break;

            case node_t::NODE_GREATER:
                buf[1] = 0x9F;
                break;

            case node_t::NODE_GREATER_EQUAL:
                buf[1] = 0x9D;
                break;

            case node_t::NODE_NOT_EQUAL:
            case node_t::NODE_STRICTLY_NOT_EQUAL:
                // no coersion here, so strictly not equal is the same
                //
                buf[1] = 0x95;
                break;

            default:
                throw internal_error("generate_compare() called with the wrong operation.");

            }
            f_file.add_text(buf, sizeof(buf));
        }

        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
    else if(get_type_of_node(lhs->get_node()) == VARIABLE_TYPE_STRING
         && get_type_of_node(rhs->get_node()) == VARIABLE_TYPE_STRING)
    {
        generate_reg_mem_string(lhs, register_t::REGISTER_RDI);
        generate_reg_mem_string(rhs, register_t::REGISTER_RSI);
        {
            int const value(static_cast<int>(op->get_operation()));
            std::uint8_t buf[] = {   // REX.W MOV $imm32, %rdx  (%rdx = operation)
                0x48,
                0xC7,
                0xC2,
                static_cast<std::uint8_t>(value >>  0),
                static_cast<std::uint8_t>(value >>  8),
                static_cast<std::uint8_t>(value >> 16),
                static_cast<std::uint8_t>(value >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_COMPARE);
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
    else
    {
        throw internal_error("generate_compare() called with unsupported parameter types.");
    }
}


void binary_assembler::generate_array(operation::pointer_t op)
{
    variable_type_t const type(get_type_of_node(op->get_node()));
    data::pointer_t lhs(op->get_left_handside());
    variable_type_t lhs_type(get_type_of_node(lhs->get_node()));
    data::pointer_t rhs(op->get_right_handside());

    std::string type_name;
    if(lhs_type == VARIABLE_TYPE_UNKNOWN)
    {
        node::pointer_t type_node(lhs->get_node()->get_type_node());
        if(type_node == nullptr)
        {
            throw not_implemented("binary_assembler::generate_array(): could not determine object type.");
        }
        type_name = type_node->get_string();
    }

    // if rhs is an identifier, then this is a field (the "b" in "a.b")
    //
    if(rhs->get_data_type() == node_t::NODE_IDENTIFIER
    || rhs->get_data_type() == node_t::NODE_STRING)
    {
        std::string const & name(rhs->get_string());
        if(name == "length"
        && lhs_type == VARIABLE_TYPE_STRING
        && type == VARIABLE_TYPE_INTEGER)
        {
            // directly move the string length to %rax
            //
            generate_load_string_size(lhs, register_t::REGISTER_RAX);
            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "MAX_VALUE"
        && lhs_type == VARIABLE_TYPE_INTEGER
        && type == VARIABLE_TYPE_INTEGER)
        {
            // load maximum integer value in %rax
            //
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0x7F,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "MIN_VALUE"
        && lhs_type == VARIABLE_TYPE_INTEGER
        && type == VARIABLE_TYPE_INTEGER)
        {
            // load minimum integer value in %rax
            //
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x80,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "MAX_VALUE"
        && lhs_type == VARIABLE_TYPE_FLOATING_POINT
        && type == VARIABLE_TYPE_FLOATING_POINT)
        {
            // load maximum floating point value in %rax
            // and then save it as an integer in a floating point variable
            //
            // maximum value in a double represented in hex: 0x7fefffffffffffff
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xEF,
                0x7F,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "MIN_VALUE"
        && lhs_type == VARIABLE_TYPE_FLOATING_POINT
        && type == VARIABLE_TYPE_FLOATING_POINT)
        {
            // load minimum floating point value in %rax
            // and then save it as an integer in a floating point variable
            //
            // minimum value in a double represented in hex: 0x0010000000000000
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x10,
                0x00,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "NEGATIVE_INFINITY"
        && lhs_type == VARIABLE_TYPE_FLOATING_POINT
        && type == VARIABLE_TYPE_FLOATING_POINT)
        {
            // load minimum floating point value in %rax
            // and then save it as an integer in a floating point variable
            //
            // -inf value in a double represented in hex: 0xFFF0000000000000
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0xF0,
                0xFF,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "POSITIVE_INFINITY"
        && lhs_type == VARIABLE_TYPE_FLOATING_POINT
        && type == VARIABLE_TYPE_FLOATING_POINT)
        {
            // load minimum floating point value in %rax
            // and then save it as an integer in a floating point variable
            //
            // +inf value in a double represented in hex: 0x7FF0000000000000
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0xF0,
                0x7F,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }
        if(name == "EPSILON"
        && lhs_type == VARIABLE_TYPE_FLOATING_POINT
        && type == VARIABLE_TYPE_FLOATING_POINT)
        {
            // load minimum floating point value in %rax
            // and then save it as an integer in a floating point variable
            //
            // minimum value in a double represented in hex: 0x3CB0000000000000
            std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                0x48,
                0xB8,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0xB0,
                0x3C,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            return;
        }

        throw not_implemented(
              "unknown field (\""
            + name
            + "\") / unsupported type for array operator.");
    }

    data::pointer_t range_end;

    variable_type_t index_type(get_type_of_node(rhs->get_node()));
    std::size_t has_range(op->get_parameter_size());
    if(has_range != 0)
    {
        range_end = op->get_parameter(0);
        variable_type_t const range_end_type(get_type_of_node(range_end->get_node()));
        if(index_type != range_end_type)
        {
            throw not_implemented("array range start & end conversion not yet implemented; they need to be of the same type for now.");
        }
    }

    switch(type)
    {
    case VARIABLE_TYPE_STRING:
        switch(index_type)
        {
        case VARIABLE_TYPE_INTEGER:
        case VARIABLE_TYPE_FLOATING_POINT:
            // if range is defined, do a subtr(), otherwise just retrieve that
            // one character
            //
            generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_RDX, sse_operation_t::SSE_OPERATION_CVT2I);
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_AT);
            break;

        case VARIABLE_TYPE_RANGE:
            // TODO: the RANGE variable type is not yet supported
            //
            generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_RDX, sse_operation_t::SSE_OPERATION_CVT2I);
            generate_reg_mem_floating_point(range_end, register_t::REGISTER_RCX, sse_operation_t::SSE_OPERATION_CVT2I);
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
            generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_SUBSTR);
            break;

        default:
            // we should probably have a toNumber() check first?
            //
            throw not_implemented("the string array operator only functions with Numbers.");

        }
        break;

    default:
        throw not_implemented("type not yet supported by the array operator");

    }
}


void binary_assembler::generate_assignment(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());
    if(rhs == nullptr)
    {
        rhs = lhs;
    }

    // standard assignments are "inverted", we load the right handside
    // and save it to the left handside
    //
    int const type(get_type_of_node(op->get_node()));
    switch(type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
    case VARIABLE_TYPE_INTEGER:
    case VARIABLE_TYPE_BOOLEAN:
        generate_reg_mem_integer(rhs, register_t::REGISTER_RAX);
        generate_store_integer(lhs, register_t::REGISTER_RAX);
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(rhs, register_t::REGISTER_RSI);
        generate_store_string(lhs, register_t::REGISTER_RSI);

        // the generate_store_string() has a CALL which blows up RSI
        //
        generate_reg_mem_string(rhs, register_t::REGISTER_RSI);
        generate_store_string(op->get_result(), register_t::REGISTER_RSI);
        break;

    default:
        throw not_implemented(
              std::string("trying to generate_assignment() with an unknown node type \"")
            + std::to_string(type)
            + "\" which is not yet implemented.");

    }
}


void binary_assembler::generate_bitwise(operation::pointer_t op)
{
    bool is_assignment(false);
    std::uint8_t code_imm32(0); // RAX specific (i.e. OP RAX, imm32)
    std::uint8_t code_r64(0);
    std::uint8_t rm(0);
    switch(op->get_operation())
    {
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_BITWISE_AND:
        code_imm32 = 0x25;
        code_r64 = 0x23;
        rm = 0xE0; // /4
        break;

    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_BITWISE_OR:
        code_imm32 = 0x0D;
        code_r64 = 0x0B;
        rm = 0xC8; // /1
        break;

    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_BITWISE_XOR:
        code_imm32 = 0x35;
        code_r64 = 0x33;
        rm = 0xF0; // /6
        break;

    default:
        throw internal_error("generate_bitwise() called with an unsupported operation");

    }

    data::pointer_t lhs(op->get_left_handside());
    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        // load double as an integer in RAX
        //
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_RAX, sse_operation_t::SSE_OPERATION_CVT2I);
    }
    else
    {
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
    }

    data::pointer_t rhs(op->get_right_handside());
    switch(rhs->get_integer_size())
    {
    case integer_size_t::INTEGER_SIZE_1BIT:
    case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,           // AND/OR/XOR $imm8, %rax
                0x83,
                rm,
                static_cast<std::uint8_t>(rhs->get_node()->get_integer().get()),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:        // there is no r64 & imm16, use r64 & imm32 instead
    case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
    case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
        {
            std::uint8_t buf[] = {
                0x48,           // AND/OR/XOR $imm32, %rax
                code_imm32,
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
            generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);
            std::uint8_t buf[] = {
                0x48,       // AND/OR/XOR %rdx, %rax
                code_r64,
                0xC2,
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    case integer_size_t::INTEGER_SIZE_FLOATING_POINT:
        {
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_RDX, sse_operation_t::SSE_OPERATION_CVT2I);
            std::uint8_t buf[] = {
                0x48,       // AND/OR/XOR %rdx, %rax
                code_r64,
                0xC2,
            };
            f_file.add_text(buf, sizeof(buf));
        }
        break;

    default:
        if(rhs->get_data_type() == node_t::NODE_VARIABLE)
        {
            if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
            {
                // if the variable is a floating point then it needs conversion
                // before it can be used
                //
                generate_reg_mem_floating_point(rhs, register_t::REGISTER_RDX, sse_operation_t::SSE_OPERATION_CVT2I);
                std::uint8_t buf[] = {
                    0x48,       // AND/OR/XOR %rdx, %rax
                    code_r64,
                    0xC2,
                };
                f_file.add_text(buf, sizeof(buf));
            }
            else
            {
                // a variable needs to be loaded from memory so we need an AND
                // which reads the variable location
                //
                generate_reg_mem_integer(rhs, register_t::REGISTER_RAX, code_r64);
            }
        }
        else
        {
            if(rhs->get_data_type() != node_t::NODE_INTEGER)
            {
                throw not_implemented(
                      std::string("trying to apply a bitwise operator on a \"")
                    + node::type_to_string(rhs->get_data_type())
                    + "\" which is not yet implemented.");
            }
            throw not_implemented(
                  "found integer size "
                + std::to_string(static_cast<int>(rhs->get_integer_size()))
                + " which is not yet implemented in generate_bitwise().");
        }

    }

    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        // a convert is rather expensive, in this case do it just once
        // ahead of time
        //
        std::uint8_t buf[] = {
            0xF2,       // CVTSI2SD %rax, %xmm0
            0x48,
            0x0F,
            0x2A,
            0xC0,
        };
        f_file.add_text(buf, sizeof(buf));

        if(is_assignment)
        {
            generate_store_floating_point(lhs, register_t::REGISTER_RAX);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_RAX);
    }
    else
    {
        if(is_assignment)
        {
            generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
        }
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
}


void binary_assembler::generate_bitwise_not(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    variable_type_t const type(get_type_of_node(op->get_node()));
    switch(type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
    case VARIABLE_TYPE_INTEGER:
        if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
        {
            generate_reg_mem_floating_point(lhs, register_t::REGISTER_RAX, sse_operation_t::SSE_OPERATION_CVT2I);
        }
        else
        {
            generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        }

        {
            std::uint8_t buf[] = {
                0x48,       // REX.W NOT %rax
                0xF7,
                0xD0,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
        {
            // a convert is rather expensive, in this case do it just once
            // ahead of time
            //
            std::uint8_t buf[] = {
                0xF2,       // CVTSI2SD %rax, %xmm0
                0x48,
                0x0F,
                0x2A,
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));

            generate_store_floating_point(op->get_result(), register_t::REGISTER_RAX);
        }
        else
        {
            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        }
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_FLIP_CASE);
        break;

    default:
        throw not_implemented(
              "bitwise not of type "
            + std::to_string(static_cast<int>(type))
            + " is not yet implemented.");

    }
}


void binary_assembler::generate_call(operation::pointer_t op)
{
    // a call creates a list of parameters; here we have to transform
    // that list in a vector and pass the vector to the function; there
    // may be no list or the list may be empty when the function does
    // not expect parameters
    //
    // also the call may reference a native function (an actual CALL
    // in assembly) or a user function call
    //
    data::pointer_t lhs(op->get_left_handside());       // the function being called or a member left handside variable
    data::pointer_t params(op->get_parameter(0));       // ARRAY variable where we create the list of parameters

    // allocate a vector for the list of parameters
    //
    temporary_variable * const params_var(f_file.find_temporary_variable(params->get_string()));
    if(params_var == nullptr)
    {
        throw internal_error("temporary for parameters not found in binary_assembler::generate_call()");
    }
    if(params_var->get_type() != node_t::NODE_ARRAY)
    {
        throw internal_error("temporary for parameters in binary_assembler::generate_call() was expected to be of type ARRAY");
    }
    generate_pointer_to_temporary(params_var, register_t::REGISTER_RDI);
    generate_external_function_call(EXTERNAL_FUNCTION_ARRAY_INITIALIZE);

    // add parameters found in list to the array
    //
    // TODO: verify that having a list just before a call does not disturb
    //       the call (i.e. `b, c, d; callme();` should not see the `b, c, d;`
    //       list as the list of parameters of `callme();`)
    //
    std::size_t const param_count(op->get_parameter_size()); // EXCLUDING PARAM [0] which is the ARRAY variable
    for(std::size_t idx(1); idx < param_count; ++idx)
    {
        data::pointer_t item(op->get_parameter(idx));
std::cerr << "--- pushing item to param array...\n";
        generate_pointer_to_variable(item, register_t::REGISTER_RSI);
std::cerr << "--- pointer ready...\n";
        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDI);
        generate_external_function_call(EXTERNAL_FUNCTION_ARRAY_PUSH);
    }

    // make the call
    //
    node::pointer_t member(op->get_node()->get_child(0));
//std::cerr << "\n--- CALL node to transform in a function call:\n" << *member << "\n";
    if(member->get_type() == node_t::NODE_MEMBER)
    {
        node::pointer_t function(member->get_instance());
        if(function->get_type() != node_t::NODE_FUNCTION
        || !function->get_attribute(attribute_t::NODE_ATTR_NATIVE))
        {
            throw not_implemented("binary_assembler::generate_call(): we only support native function calls at the moment.");
        }
        node::pointer_t field(member->get_child(1));
        if(field->get_type() != node_t::NODE_IDENTIFIER)
        {
            throw not_implemented("binary_assembler::generate_call(): we only support identifiers for the field name.");
        }
        std::string const & field_name(field->get_string());
        if(field_name.empty())
        {
            throw internal_error("binary_assembler::generate_call(): field name is somehow empty.");
        }
        node::pointer_t object(member->get_child(0));
        node::pointer_t type_node(object->get_type_node());
        if(type_node == nullptr)
        {
            throw not_implemented("binary_assembler::generate_call(): we only support typed objects.");
        }
        std::string const & type_name(type_node->get_string());
        if(type_name.empty())
        {
            throw internal_error("binary_assembler::generate_call(): type name is somehow empty.");
        }
        bool found(true);
        switch(type_name[0])
        {
        case 'B':
            if(type_name == "Boolean")
            {
                switch(field_name[0])
                {
                case 't':
                    if(field_name == "toString")
                    {
                        generate_reg_mem_integer(lhs, register_t::REGISTER_RSI);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_BOOLEANS_TO_STRING);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'v':
                    if(field_name == "valueOf")
                    {
                        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
                        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                default:
                    found = false;
                    break;

                }
            }
            else
            {
                found = false;
            }
            break;

        case 'D':
            if(type_name == "Double")
            {
                goto number;
            }
            break;

        case 'I':
            if(type_name == "Integer")
            {
                switch(field_name[0])
                {
                case 't':
                    if(field_name == "toString")
                    {
                        generate_reg_mem_integer(lhs, register_t::REGISTER_RSI);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_external_function_call(EXTERNAL_FUNCTION_INTEGERS_TO_STRING);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'v':
                    if(field_name == "valueOf")
                    {
                        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
                        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                default:
                    found = false;
                    break;

                }
            }
            else
            {
                found = false;
            }
            break;

        case 'M':
            if(type_name == "Math")
            {
                switch(field_name[0])
                {
                case 'E':
                    if(field_name == "E")
                    {
                        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RSI);
                        generate_external_function_call(EXTERNAL_FUNCTION_FLOATING_POINTS_TO_STRING);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                default:
                    found = false;
                    break;

                }
            }
            else
            {
                found = false;
            }
            break;

        case 'N':
            if(type_name == "Number")
            {
number:
                switch(field_name[0])
                {
                case 't':
                    if(field_name == "toString")
                    {
                        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RSI);
                        generate_external_function_call(EXTERNAL_FUNCTION_FLOATING_POINTS_TO_STRING);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'v':
                    if(field_name == "valueOf")
                    {
                        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
                        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                default:
                    found = false;
                    break;

                }
            }
            else
            {
                found = false;
            }
            break;

        case 'S':
            if(type_name == "String")
            {
                switch(field_name[0])
                {
                case 'c':
                    if(field_name == "charAt")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_CHAR_AT);
                    }
                    else if(field_name == "charCodeAt")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_CHAR_CODE_AT);
                    }
                    else if(field_name == "concat")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_CONCAT_PARAMS);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'i':
                    if(field_name == "indexOf")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_INDEX_OF);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'l':
                    if(field_name == "lastIndexOf")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_LAST_INDEX_OF);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'r':
                    if(field_name == "replace")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_REPLACE);
                    }
                    else if(field_name == "replaceAll")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_REPLACE_ALL);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 's':
                    if(field_name == "slice")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_SLICE);
                    }
                    else if(field_name == "substring")
                    {
                        generate_pointer_to_temporary(params_var, register_t::REGISTER_RDX);
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_SUBSTRING);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 't':
                    if(field_name == "toLowerCase")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_TO_LOWERCASE);
                    }
                    else if(field_name == "toUpperCase")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_TO_UPPERCASE);
                    }
                    else if(field_name == "toString")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_COPY);
                    }
                    else if(field_name == "trim")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_TRIM);
                    }
                    else if(field_name == "trimStart" || field_name == "trimLeft")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_TRIM_START);
                    }
                    else if(field_name == "trimEnd" || field_name == "trimRight")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_pointer_to_variable(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_TRIM_END);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                case 'v':
                    if(field_name == "valueOf")
                    {
                        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
                        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_COPY);
                    }
                    else
                    {
                        found = false;
                    }
                    break;

                default:
                    found = false;
                    break;

                }
            }
            else
            {
                found = false;
            }
            break;

        default:
            found = false;
            break;

        }

        if(!found)
        {
            throw not_implemented(
                  "binary_assembler::generate_call(): it looks like function \""
                + std::string(function->get_attribute(attribute_t::NODE_ATTR_NATIVE) ? "native " : "")
                + type_name
                + "::"
                + field_name
                + "()\" is not yet implemented.");
        }
    }
    else
    {
        throw not_implemented(
              "binary_assembler::generate_call(): we only support member calls at the moment.");
    }

    // done with those parameters, free them
    //
    generate_pointer_to_temporary(params_var, register_t::REGISTER_RDI);
    generate_external_function_call(EXTERNAL_FUNCTION_ARRAY_FREE);
}


void binary_assembler::generate_goto(operation::pointer_t op)
{
    std::size_t const pos(f_file.get_current_text_offset());
    std::uint8_t jcc[] = {
        0xE9,       // JMP disp32
        0x00,
        0x00,
        0x00,
        0x00,
    };
    f_file.add_text(jcc, sizeof(jcc));
    f_file.add_relocation(
              op->get_label()
            , relocation_t::RELOCATION_LABEL_32BITS
            , pos + 1
            , f_file.get_current_text_offset());
}


void binary_assembler::generate_identity(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());

    switch(lhs->get_data_type())
    {
    case node_t::NODE_INTEGER:
    case node_t::NODE_FLOATING_POINT:
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case node_t::NODE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_store_string(op->get_result(), register_t::REGISTER_RSI);
        break;

    case node_t::NODE_VARIABLE:
        {
            variable_type_t const type(get_type_of_node(lhs->get_node()));
            switch(type)
            {
            case VARIABLE_TYPE_INTEGER:
            case VARIABLE_TYPE_FLOATING_POINT:
                generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
                break;

            case VARIABLE_TYPE_STRING:
                generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
                generate_store_string(op->get_result(), register_t::REGISTER_RSI);
                break;

            default:
                throw not_implemented(
                      "identity of type "
                    + std::to_string(static_cast<int>(type))
                    + " is not yet implemented.");
                break;

            }
        }
        break;

    default:
        throw not_implemented(
              std::string("identity of type ")
            + node::type_to_string(lhs->get_data_type())
            + " is not yet implemented.");

    }
}


void binary_assembler::generate_if(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    switch(lhs->get_data_type())
    {
    case node_t::NODE_BOOLEAN:
    case node_t::NODE_INTEGER:
    case node_t::NODE_FLOATING_POINT:
    case node_t::NODE_STRING:
        throw internal_error("somehow a conditional was not optimized properly.");

    default:
        break;

    }

    // with Intel we can use CMP 0, mem
    // (use RAX since it is 0 and will have no effect on the encoding)
    //
    generate_reg_mem_integer(lhs, register_t::REGISTER_RAX, 0x83, 1);
    {
        std::uint8_t buf[] = {
            0x00,       // ib (the 0 in the CMP instruction we just generated)
        };
        f_file.add_text(buf, sizeof(buf));
    }

    {
        std::size_t const pos(f_file.get_current_text_offset());
        std::uint8_t const code(op->get_operation() == node_t::NODE_IF_TRUE ? 0x85 : 0x84);
        std::uint8_t buf[] = {
            0x0F,       // JNE or JE disp32
            code,
            0x00,
            0x00,
            0x00,
            0x00,
        };
        f_file.add_text(buf, sizeof(buf));
        f_file.add_relocation(
                  op->get_label()
                , relocation_t::RELOCATION_LABEL_32BITS
                , pos + 2
                , f_file.get_current_text_offset());
    }
}


void binary_assembler::generate_divide(operation::pointer_t op)
{
    // TODO: support exception for DIV by 0
    //
    bool is_divide(false);
    bool is_assignment(false);
    switch(op->get_operation())
    {
    case node_t::NODE_DIVIDE:
        is_divide = true;
        break;

    case node_t::NODE_ASSIGNMENT_DIVIDE:
        is_divide = true;
        is_assignment = true;
        break;

    case node_t::NODE_ASSIGNMENT_MODULO:
        is_assignment = true;
        break;

    default:
        break;

    }

    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());

    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        if(is_divide)
        {
            generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM0, sse_operation_t::SSE_OPERATION_DIV);
        }
        else
        {
            // there is no easy remainder in SSE
            //
            // just call the system function, it's faster and much more
            // reliable
            //
            generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
            generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM1);

            generate_external_function_call(EXTERNAL_FUNCTION_FMOD);
        }

        if(is_assignment)
        {
            generate_store_floating_point(lhs, register_t::REGISTER_XMM0);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
    }
    else
    {
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        generate_reg_mem_integer(rhs, register_t::REGISTER_RCX);

        // TODO: add support for the reg/mem instead of using RCX
        {
            std::uint8_t buf[] = {
                0x48,       // CQO (extend rax sign to rdx)
                0x99,

                0x48,       // REX.W IDIV %rcx
                0xF7,
                static_cast<std::uint8_t>(0xF8 | static_cast<int>(register_t::REGISTER_RCX)),
            };
            f_file.add_text(buf, sizeof(buf));
        }

        if(is_assignment)
        {
            generate_store_integer(
                      op->get_left_handside()
                    , is_divide ? register_t::REGISTER_RAX : register_t::REGISTER_RDX);
        }
        generate_store_integer(
                  op->get_result()
                , is_divide ? register_t::REGISTER_RAX : register_t::REGISTER_RDX);

        if(!is_divide)
        {
            // we need the result in RAX, with the modulo, it is still in
            // the RDX register
            //
            // TODO: once we optimize, this could be done only if we need the
            //       value in RAX which many times may not be the case
            //
            std::uint8_t buf[] = {       // REX.W MOV %rdx, %rax
                0x48,
                0x89,
                0xD0,
            };
            f_file.add_text(buf, sizeof(buf));
        }
    }
}


void binary_assembler::generate_increment(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());

    node_t type(op->get_operation());
    bool const is_post(type == node_t::NODE_POST_DECREMENT
                    || type == node_t::NODE_POST_INCREMENT);

    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        // we have to load the value in this case since there are not
        // INC/DEC for floating points
        //
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);

        // make sure we have a 1.0 number somewhere
        //
        double const value(1.0);
        std::string name;
        f_file.add_constant(value, name);
        lhs->set_data_name(name);

        std::uint8_t const code(type == node_t::NODE_INCREMENT
                             || type == node_t::NODE_POST_INCREMENT
                                    ? 0x58
                                    : 0x5C);

        if(is_post)
        {
            // in this case xmm0 must not be changed so we move it xmm1
            //
            std::uint8_t buf[] = {           // MOVQ %xmm0, %xmm1
                0xF3,
                0x0F,
                0x7E,
                0xC8,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        {
            std::size_t const pos(f_file.get_current_text_offset());
            std::uint8_t buf[] = {          // ADDPS or SUBPS disp32[%rip], %xmm0|1
                0xF2,
                0x0F,
                code,
                static_cast<std::uint8_t>(is_post ? 0x0D : 0x05),
                0x00,
                0x00,
                0x00,
                0x00,
            };
            f_file.add_text(buf, sizeof(buf));
            f_file.add_relocation(
                      name
                    , relocation_t::RELOCATION_CONSTANT_32BITS
                    , pos + 4
                    , f_file.get_current_text_offset());
        }

        // store back inside input & temp
        //
        generate_store_floating_point(lhs, is_post ? register_t::REGISTER_XMM1 : register_t::REGISTER_XMM0);
        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
    }
    else
    {
        if(is_post)
        {
            generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        }

        std::uint8_t const code(type == node_t::NODE_INCREMENT
                             || type == node_t::NODE_POST_INCREMENT
                                    ? 0x05 | (0 << 3)
                                    : 0x05 | (1 << 3));

        std::size_t const pos(f_file.get_current_text_offset());
        std::uint8_t buf[] = {
            0x48,                       // REX.W INC m
            0xFF,
            code,                       // disp(rip) (/m)
            0x00,                       // 32 bit offset
            0x00,
            0x00,
            0x00,
        };
        f_file.add_text(buf, sizeof(buf));
        f_file.add_relocation(
                  lhs->get_string()
                , relocation_t::RELOCATION_VARIABLE_32BITS_DATA
                , pos + 3
                , f_file.get_current_text_offset());

        if(!is_post)
        {
            generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        }

        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
}


void binary_assembler::generate_label(operation::pointer_t op)
{
    f_file.add_label(op->get_label());
}


void binary_assembler::generate_list(operation::pointer_t op)
{
    data::pointer_t result(op->get_result());

    // for a list the result is the last item in the list
    // so here we copy that last item's result in the list's result
    //
    std::size_t const max(op->get_parameter_size());
    if(max > 0)
    {
        data::pointer_t d(op->get_parameter(max - 1));

        switch(get_type_of_node(d->get_node()))
        {
        case VARIABLE_TYPE_INTEGER:
        case VARIABLE_TYPE_BOOLEAN:
            generate_reg_mem_integer(d, register_t::REGISTER_RAX);
            generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            break;

        case VARIABLE_TYPE_FLOATING_POINT:
            generate_reg_mem_floating_point(d, register_t::REGISTER_XMM0);
            generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
            break;

        case VARIABLE_TYPE_STRING:
            generate_reg_mem_string(d, register_t::REGISTER_RSI);
            generate_store_string(op->get_result(), register_t::REGISTER_RSI);
            break;

        default:
            throw not_implemented(
                  "found a list item with a type not yet implemented in generate_list().");

        }
    }

}


void binary_assembler::generate_logical(operation::pointer_t op)
{
    bool is_assignment(false);
    std::uint8_t code(0);
    switch(op->get_operation())
    {
    case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_LOGICAL_AND:
        code = 0x23;
        break;

    case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_LOGICAL_OR:
        code = 0x0B;
        break;

    case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_LOGICAL_XOR:
        code = 0x33;
        break;

    default:
        throw internal_error("generate_logical() called with an unsupported operation");

    }

    data::pointer_t lhs(op->get_left_handside());
    generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);

    data::pointer_t rhs(op->get_right_handside());
    if(rhs->get_data_type() != node_t::NODE_VARIABLE)
    {
        throw not_implemented(
              "found a literal which is not yet implemented in generate_logical().");
    }
    generate_reg_mem_integer(rhs, register_t::REGISTER_RAX, code);

    if(is_assignment)
    {
        generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
    }
    generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
}


void binary_assembler::generate_logical_not(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    switch(get_type_of_node(lhs->get_node()))
    {
    case VARIABLE_TYPE_FLOATING_POINT:
    case VARIABLE_TYPE_INTEGER:
    case VARIABLE_TYPE_BOOLEAN:
        generate_reg_mem_integer(lhs, register_t::REGISTER_RDI);

        {
            std::uint8_t buf[] = {
                0x33,       // XOR %eax, %eax (rax := 0)
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        if(get_type_of_node(lhs->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
        {
            // to check for NaNs (because the test is flipped in that case)
            //
            std::uint64_t const nan_exponent_x2(0x7FF0000000000000ULL * 2 + 1ULL);

            // ignore the sign if floating point (important for -0.0 and -NaN)
            //
            std::uint8_t buf[] = {
                0x48,       // REX.W ADD $rdi, %rdi
                0x03,
                0xFF,

                // next we want to test for the NaN special case
                // the number is a NaN if the mantissa != 0 and the exponent
                // is all 1s so we can use a simple compare for this test
                //
                // TBD: look at using a constant instead of loading 64 bits
                //      each time?
                //
                0x49,       // REX.W MOV $nan_exponent_x2, %r11
                static_cast<std::uint8_t>(0xB8 | (static_cast<int>(register_t::REGISTER_R11) & 7)),
                static_cast<std::uint8_t>(nan_exponent_x2 >>  0),
                static_cast<std::uint8_t>(nan_exponent_x2 >>  8),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 16),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 24),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 32),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 40),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 48),
                static_cast<std::uint8_t>(nan_exponent_x2 >> 56),

                0x4C,       // REX.W CMP %r11, %rdi
                0x39,
                static_cast<std::uint8_t>(0xC0 | ((static_cast<int>(register_t::REGISTER_R11) & 7) << 3) | (static_cast<int>(register_t::REGISTER_RDI) & 7)),

                0x48,       // REX.W CMOVG %rax, %rdi
                0x0F,
                0x4F,
                0xF8,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        {
            std::uint8_t buf[] = {
                0x48,       // REX.W TEST %rdi, %rdi
                0x85,
                0xFF,

                0x0F,       // SETZ %al
                0x94,
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_STRING:
        // directly move the string length in %rdx
        //
        generate_load_string_size(lhs, register_t::REGISTER_RDX);

        {
            std::uint8_t buf[] = {
                0x33,       // XOR %eax, %eax   (%rax := 0)
                0xC0,

                0x85,       // TEST %edx, %edx
                0xD2,

                0x0F,       // SETZ %al
                0x94,
                0xC0,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    default:
        throw not_implemented("node type not yet handled in generate_logical_not().");

    }
}


void binary_assembler::generate_minmax(operation::pointer_t op)
{
    bool is_assignment(false);
    std::uint8_t code(0x4F);
    std::uint8_t fp_code(0x5D);
    switch(op->get_operation())
    {
    case node_t::NODE_ASSIGNMENT_MAXIMUM:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_MAXIMUM:
        code = 0x4C;
        fp_code = 0x5F;
        break;

    case node_t::NODE_ASSIGNMENT_MINIMUM:
        is_assignment = true;
        break;

    default:
        break;

    }

    variable_type_t const type(get_type_of_node(op->get_node()));

    data::pointer_t lhs(op->get_left_handside());
    if(lhs == nullptr)
    {
        // this is a Math.min() or Math.max() instead, so we need to handle
        // the list of parameters instead of a simple binary operator
        //

        std::size_t const max(op->get_parameter_size());
        switch(type)
        {
        case VARIABLE_TYPE_FLOATING_POINT:
            if(max == 0)
            {
                {
                    // load minimum floating point value in %rax
                    // and then save it as an integer in a floating point variable
                    //
                    // +/-inf value in a double represented in hex:
                    //      0x7FF0000000000000 (+inf)
                    //      0xFFF0000000000000 (-inf)
                    std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                        0x48,
                        0xB8,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0x00,
                        0xF0,
                        static_cast<std::uint8_t>(code == 0x4C ? 0xFF : 0x7F),
                    };
                    f_file.add_text(buf, sizeof(buf));
                }

                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
            else
            {
                generate_reg_mem_floating_point(op->get_parameter(0), register_t::REGISTER_XMM0);
                for(std::size_t idx(1); idx < max; ++idx)
                {
                    generate_reg_mem_floating_point(
                          op->get_parameter(idx)
                        , register_t::REGISTER_XMM0
                        , code == 0x4C
                            ? sse_operation_t::SSE_OPERATION_MAX
                            : sse_operation_t::SSE_OPERATION_MIN);
                }

                generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
            }
            break;

        case VARIABLE_TYPE_INTEGER:
            if(max == 0)
            {
                // WARNING: this case does not happen, it selects the
                //          Number version automatically... (since there
                //          are 0 parameters, we cannot really chose one
                //          function over the other, yet that part works
                //          but we choose the first function)
                {
                    // load minimum/maximum integer value in %rax
                    // and then save it as an integer in a floating point variable
                    //
                    // "+/-inf" value for an integer...
                    //      0x8000000000000000 (INT_MIN)
                    //      0x7FFFFFFFFFFFFFFF (INT_MAX)
                    std::int64_t const value(code == 0x4C ? INT_MIN : INT_MAX);
                    std::uint8_t buf[] = {      // REX.W MOV $imm64, %rax
                        0x48,
                        0xB8,
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

                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
            else
            {
                generate_reg_mem_integer(op->get_parameter(0), register_t::REGISTER_RAX);
                for(std::size_t idx(1); idx < max; ++idx)
                {
                    generate_reg_mem_integer(op->get_parameter(idx), register_t::REGISTER_RDX);

                    {
                        std::uint8_t buf[] = {
                            0x48,       // REX.W CMP %rax, %rdx
                            0x39,
                            0xD0,

                            0x48,       // REX.W CMOVL or CMOVG %rdx, %rax
                            0x0F,
                            code,
                            0xC2,
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                }

                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
            break;

        default:
            throw not_implemented(
                  "minimum/maximum of type "
                + std::to_string(static_cast<int>(type))
                + " is not yet implemented.");

        }

        return;
    }

    data::pointer_t rhs(op->get_right_handside());

    switch(type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
        generate_reg_mem_floating_point(
              rhs
            , register_t::REGISTER_XMM0
            , code == 0x4C
                ? sse_operation_t::SSE_OPERATION_MAX
                : sse_operation_t::SSE_OPERATION_MIN);

        if(is_assignment)
        {
            generate_store_floating_point(op->get_left_handside(), register_t::REGISTER_XMM0);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
        break;

    case VARIABLE_TYPE_INTEGER:
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
        generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);

        {
            std::uint8_t buf[] = {
                0x48,       // REX.W CMP %rax, %rdx
                0x39,
                0xD0,

                0x48,       // REX.W CMOVL or CMOVG %rdx, %rax
                0x0F,
                code,
                0xC2,
            };
            f_file.add_text(buf, sizeof(buf));
        }

        if(is_assignment)
        {
            generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
        }
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_reg_mem_string(rhs, register_t::REGISTER_RDX);
        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);

        {
            std::uint8_t buf[] = {   // MOV $imm8, %rcx  (%rcx = 1 or -1)
                0xB1,
                static_cast<std::uint8_t>(code == 0x4C ? 1 : -1),
            };
            f_file.add_text(buf, sizeof(buf));
        }

        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_MINMAX);
        if(is_assignment)
        {
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RSI);
            generate_store_string(lhs, register_t::REGISTER_RSI);
        }
        break;

    default:
        throw not_implemented(
              "minimum/maximum of type "
            + std::to_string(static_cast<int>(type))
            + " is not yet implemented.");

    }
}


void binary_assembler::generate_multiply(operation::pointer_t op)
{
    bool const is_assignment(op->get_operation() == node_t::NODE_ASSIGNMENT_MULTIPLY);

    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());

    variable_type_t const type(get_type_of_node(op->get_node()));
    switch(type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
        generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM0, sse_operation_t::SSE_OPERATION_MUL);

        if(is_assignment)
        {
            generate_store_floating_point(lhs, register_t::REGISTER_XMM0);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
        break;

    case VARIABLE_TYPE_INTEGER:
        generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);

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
                generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);
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
            // rhs is not an integer
            //
            switch(rhs->get_data_type())
            {
            case node_t::NODE_VARIABLE:
                {
                    generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);
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

        if(is_assignment)
        {
            generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
        }
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_reg_mem_integer(rhs, register_t::REGISTER_RDX);
        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_MULTIPLY);
        if(is_assignment)
        {
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RSI);
            generate_store_string(lhs, register_t::REGISTER_RSI);
        }
        break;

    default:
        throw not_implemented(
              "multiply of type "
            + std::to_string(static_cast<int>(type))
            + " is not yet implemented.");

    }
}


void binary_assembler::generate_negate(operation::pointer_t op)
{
    data::pointer_t lhs(op->get_left_handside());
    generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);

    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        // to negate a double just flip the sign bit
        //
        std::uint8_t buf[] = {
            0x48,       // REX.W BTC $63, %rax
            0x0F,
            0xBA,
            0xF8,
            0x3F,
        };
        f_file.add_text(buf, sizeof(buf));
    }
    else
    {
        std::uint8_t buf[] = {
            0x48,       // REX.W NEG %rax
            0xF7,
            0xD8,
        };
        f_file.add_text(buf, sizeof(buf));
    }

    generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
}


void binary_assembler::generate_param(operation::pointer_t op)
{
    // a NODE_PARAM here means we got an integer, floating point, or
    // boolean that needs to be converted in a binary_variable; this
    // is done this way because at the moment CALL is only expecting
    // such variables

    // load data to copy in RAX or XMM0
    //
    data::pointer_t lhs(op->get_left_handside());
    variable_type_t const binary_variable_type(get_type_of_node(lhs->get_node()));
    switch(binary_variable_type)
    {
    case VARIABLE_TYPE_INTEGER:
    case VARIABLE_TYPE_BOOLEAN:
    case VARIABLE_TYPE_FLOATING_POINT:
        break;

    default:
        throw not_implemented(
              "found a param item with a type not yet implemented in generate_param().");

    }
    generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);

    // now save the result in the binary variable defined in "result"
    //
    data::pointer_t result(op->get_result());
    if(result->get_data_type() != node_t::NODE_VARIABLE)
    {
        throw not_implemented(
              "generate_param() only supports results of type NODE_VARIABLE.");
    }
    if(!result->is_temporary())
    {
        throw not_implemented(
              "generate_param() only supports temporary variables for their results.");
    }

    node::pointer_t n(result->get_node());
    std::string const name(n->get_string());
    temporary_variable * temp_var(f_file.find_temporary_variable(name));
    if(temp_var == nullptr)
    {
        throw internal_error("temporary not found in generate_param()");
    }
    if(temp_var->get_size() != sizeof(binary_variable))
    {
        throw internal_error("temporary was expected to be exactly sizeof(binary_variable) in size in generate_param()");
    }

    generate_save_reg_in_binary_variable(temp_var, register_t::REGISTER_RAX, binary_variable_type);
}


void binary_assembler::generate_save_reg_in_binary_variable(temporary_variable * temp_var, register_t reg, variable_type_t const binary_variable_type)
{
    if(reg == register_t::REGISTER_RCX)
    {
        throw not_implemented("generate_save_reg_in_binary_variable() does not support reg parameter as RCX just yet");
    }

    // at this point the variable is consider uninitialized if it is an
    // integer, a boolean, or a floating point variable; this means we
    // not only save the RAX (reg) value in the f_data field, we also want to
    // set the f_type, f_flag, f_data_size properly (also clear the
    // f_name/f_name_size too, for forward compatibility)
    //
    ssize_t const offset(temp_var->get_offset());

    std::size_t size(0);
    switch(binary_variable_type)
    {
    case VARIABLE_TYPE_INTEGER:
        size = sizeof(std::int64_t);
        break;

    case VARIABLE_TYPE_BOOLEAN:
        size = sizeof(std::uint8_t);
        break;

    case VARIABLE_TYPE_FLOATING_POINT:
        size = sizeof(double);
        break;

    default:
        throw not_implemented(
              "found a param item with a type not yet implemented in generate_param().");

    }

    { // zero to clear a few fields
        std::uint8_t buf[] = {    // XOR %rcx, %rcx
            0x33,
            static_cast<std::uint8_t>(0xC9),
        };
        f_file.add_text(buf, sizeof(buf));
    }

    // save type in var.f_type
    {
        ssize_t const o(offset + offsetof(binary_variable, f_type));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW $imm16, disp8(%rbp)
                    0x66,
                    0xC7,
                    static_cast<std::uint8_t>(0x45),
                    static_cast<std::uint8_t>(o),
                    static_cast<std::uint8_t>(binary_variable_type >> 0),
                    static_cast<std::uint8_t>(binary_variable_type >> 8),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW $imm16, disp32(%rbp)
                    0x66,
                    0xC7,
                    static_cast<std::uint8_t>(0x85),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
                    static_cast<std::uint8_t>(binary_variable_type >> 0),
                    static_cast<std::uint8_t>(binary_variable_type >> 8),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }

    // clear flags in var.f_flags
    {
        ssize_t const o(offset + offsetof(binary_variable, f_flags));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW %cx, disp8(%rbp)
                    0x66,
                    0x89,
                    static_cast<std::uint8_t>(0x4D),
                    static_cast<std::uint8_t>(o),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW %cx, disp32(%rbp)
                    0x66,
                    0x89,
                    static_cast<std::uint8_t>(0x8D),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }

    // clear name size (no name specified) in var.f_name_size
    {
        ssize_t const o(offset + offsetof(binary_variable, f_name_size));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW %cx, disp8(%rbp)
                    0x66,
                    0x89,
                    static_cast<std::uint8_t>(0x4D),
                    static_cast<std::uint8_t>(o),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOVW %cx, disp32(%rbp)
                    0x66,
                    0x89,
                    static_cast<std::uint8_t>(0x8D),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }

    // clear name offset (no name specified) in var.f_name
    {
        ssize_t const o(offset + offsetof(binary_variable, f_name));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOV %ecx, disp8(%rbp)
                    0x89,
                    static_cast<std::uint8_t>(0x4D),
                    static_cast<std::uint8_t>(o),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOV %ecx, disp32(%rbp)
                    0x89,
                    static_cast<std::uint8_t>(0x8D),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }

    // save size in var.f_data_size
    {
        ssize_t const o(offset + offsetof(binary_variable, f_data_size));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOV $imm32, disp8(%rbp)
                    0xC7,
                    static_cast<std::uint8_t>(0x45),
                    static_cast<std::uint8_t>(o),
                    static_cast<std::uint8_t>(size >>  0),
                    static_cast<std::uint8_t>(size >>  8),
                    static_cast<std::uint8_t>(size >> 16),
                    static_cast<std::uint8_t>(size >> 24),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // MOV $imm32, disp32(%rbp)
                    0xC7,
                    static_cast<std::uint8_t>(0x85),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
                    static_cast<std::uint8_t>(size >>  0),
                    static_cast<std::uint8_t>(size >>  8),
                    static_cast<std::uint8_t>(size >> 16),
                    static_cast<std::uint8_t>(size >> 24),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }

    // now save the value in var.f_data
    {
        ssize_t const o(offset + offsetof(binary_variable, f_data));
        switch(get_smallest_size(o))
        {
        case integer_size_t::INTEGER_SIZE_1BIT:
        case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // REX.W MOV %rn, disp8(%rbp)
                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                    0x89,
                    static_cast<std::uint8_t>(0x45 | ((static_cast<int>(reg) & 7) << 3)),
                    static_cast<std::uint8_t>(o),
                };
                f_file.add_text(buf, sizeof(buf));
            }
            break;

        case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
        case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
        case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            {
                std::uint8_t buf[] = {    // REX.W MOV %rn, disp32(%rbp)
                    static_cast<std::uint8_t>(reg >= register_t::REGISTER_R8 ? 0x49 : 0x48),
                    0x89,
                    static_cast<std::uint8_t>(0x85 | ((static_cast<int>(reg) & 7) << 3)),
                    static_cast<std::uint8_t>(o >>  0),
                    static_cast<std::uint8_t>(o >>  8),
                    static_cast<std::uint8_t>(o >> 16),
                    static_cast<std::uint8_t>(o >> 24),
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
                + std::to_string(static_cast<int>(get_smallest_size(o)))
                + " for size: "
                + std::to_string(o)
                + ").");

        }
    }
}


void binary_assembler::generate_power(operation::pointer_t op)
{
    bool const is_assignment(op->get_operation() == node_t::NODE_ASSIGNMENT_POWER);

    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());

    if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
    {
        generate_reg_mem_floating_point(lhs, register_t::REGISTER_XMM0);
        generate_reg_mem_floating_point(rhs, register_t::REGISTER_XMM1);

        generate_external_function_call(EXTERNAL_FUNCTION_POW);

        if(is_assignment)
        {
            generate_store_floating_point(lhs, register_t::REGISTER_XMM0);
        }
        generate_store_floating_point(op->get_result(), register_t::REGISTER_XMM0);
    }
    else
    {
        generate_reg_mem_integer(lhs, register_t::REGISTER_RDI);
        generate_reg_mem_integer(rhs, register_t::REGISTER_RSI);

        generate_external_function_call(EXTERNAL_FUNCTION_IPOW);

        //f_file.add_rt_function(f_rt_functions_oar, "ipow");
        //std::size_t const pos(f_file.get_current_text_offset());
        //std::uint8_t buf[] = {
        //    0xE8,       // CALL disp32(rip)
        //    0x00,
        //    0x00,
        //    0x00,
        //    0x00,
        //};
        //f_file.add_text(buf, sizeof(buf));
        //f_file.add_relocation(
        //          "ipow"
        //        , relocation_t::RELOCATION_RT_32BITS
        //        , pos + 1
        //        , f_file.get_current_text_offset());

        if(is_assignment)
        {
            generate_store_integer(lhs, register_t::REGISTER_RAX);
        }
        generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
    }
}


void binary_assembler::generate_shift(operation::pointer_t op)
{
    bool is_assignment(false);
    std::uint8_t rm(0);
    switch(op->get_operation())
    {
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_ROTATE_LEFT:
        rm = 0xC0;
        break;

    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_ROTATE_RIGHT:
        rm = 0xC8;
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_SHIFT_LEFT:
        rm = 0xE0;
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_SHIFT_RIGHT:
        rm = 0xF8;
        break;

    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
        is_assignment = true;
        [[fallthrough]];
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
        rm = 0xE8;
        break;

    default:
        throw internal_error("generate_shift() called with an invalid operator");

    }

    data::pointer_t lhs(op->get_left_handside());
    data::pointer_t rhs(op->get_right_handside());
    variable_type_t type(get_type_of_node(op->get_node()));
    switch(type)
    {
    case VARIABLE_TYPE_FLOATING_POINT:
    case VARIABLE_TYPE_INTEGER:
        {
            // TODO: as an optimization, we can directly shift the memory with x86
            //       however, with our current implementation we must have the value
            //       in RAX at the time we're done
            //
            if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
            {
                // load double as an integer in RAX
                //
                generate_reg_mem_floating_point(
                      lhs
                    , register_t::REGISTER_RAX
                    , sse_operation_t::SSE_OPERATION_CVT2I);
            }
            else
            {
                generate_reg_mem_integer(lhs, register_t::REGISTER_RAX);
            }

            switch(rhs->get_integer_size())
            {
            case integer_size_t::INTEGER_SIZE_1BIT:
            case integer_size_t::INTEGER_SIZE_8BITS_SIGNED:
            case integer_size_t::INTEGER_SIZE_8BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_16BITS_SIGNED:
            case integer_size_t::INTEGER_SIZE_16BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_32BITS_SIGNED:
            case integer_size_t::INTEGER_SIZE_32BITS_UNSIGNED:
            case integer_size_t::INTEGER_SIZE_64BITS:
                // since IA-32 the shift is limited to 32 or 64 bits so any immediate
                // can be converted to (imm8 & 0x3F)
                {
                    std::uint8_t const shift(rhs->get_node()->get_integer().get() & 0x3F);
                    if(shift == 0)
                    {
                        // no shifting, we're done
                        break;
                    }
                    if(shift == 1)
                    {
                        std::uint8_t buf[] = {
                            0x48,       // 64 bits
                            0xD1,       // SAL rax <<= 1
                            rm,         // r/m
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                    else
                    {
                        std::uint8_t buf[] = {
                            0x48,       // 64 bits
                            0xC1,       // SAL r64 <<= imm8
                            rm,         // r/m
                            shift,
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                }
                break;

            case integer_size_t::INTEGER_SIZE_UNKNOWN:
                // rhs is not an integer
                //
                switch(rhs->get_data_type())
                {
                case node_t::NODE_VARIABLE:
                    {
                        if(get_type_of_node(rhs->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
                        {
                            generate_reg_mem_floating_point(
                                      rhs
                                    , register_t::REGISTER_RCX
                                    , sse_operation_t::SSE_OPERATION_CVT2I);
                        }
                        else
                        {
                            generate_reg_mem_integer(rhs, register_t::REGISTER_RCX);
                        }
                        std::uint8_t buf[] = {
                            0x48,       // SAL rax <<= cl
                            0xD3,
                            rm,
                        };
                        f_file.add_text(buf, sizeof(buf));
                    }
                    break;

                default:
                    throw not_implemented("non-integer node not yet handled in generate_shift().");

                }
                break;

            default:
                throw not_implemented("integer size not yet implemented in generate_shift().");

            }

            if(get_type_of_node(op->get_node()) == VARIABLE_TYPE_FLOATING_POINT)
            {
                // a convert is rather expensive, in this case do it just once
                // ahead of time
                //
                std::uint8_t buf[] = {
                    0xF2,       // CVTSI2SD %rax, %xmm0
                    0x48,
                    0x0F,
                    0x2A,
                    0xC0,
                };
                f_file.add_text(buf, sizeof(buf));

// 268:	f2 48 0f 2d 05 af 01 00 00	cvtsd2si 0x1af(%rip),%rax      # 0x420
// 271:	48 8b 0d c0 01 00 00	 	mov    0x1c0(%rip),%rcx        # 0x438
// 278:	48 d3 e0             		shl    %cl,%rax
// 27b:	f2 48 0f 2a c0       		cvtsi2sd %rax,%xmm0
// 280:	66 0f d6 85 40 ff ff ff		movq   %xmm0,-0xc0(%rbp)
// 288:	48 8b 85 40 ff ff ff 		mov    -0xc0(%rbp),%rax
// 28f:	48 89 05 b2 00 00 00 		mov    %rax,0xb2(%rip)        # 0x348
// 296:	48 89 85 50 ff ff ff 		mov    %rax,-0xb0(%rbp)

                if(is_assignment)
                {
                    generate_store_floating_point(lhs, register_t::REGISTER_RAX);
                }
                generate_store_floating_point(op->get_result(), register_t::REGISTER_RAX);
            }
            else
            {
                if(is_assignment)
                {
                    generate_store_integer(op->get_left_handside(), register_t::REGISTER_RAX);
                }
                generate_store_integer(op->get_result(), register_t::REGISTER_RAX);
            }
        }
        break;

    case VARIABLE_TYPE_STRING:
        generate_reg_mem_string(lhs, register_t::REGISTER_RSI);
        generate_reg_mem_floating_point(rhs, register_t::REGISTER_RDX, sse_operation_t::SSE_OPERATION_CVT2I);
        {
            int const value(static_cast<int>(op->get_operation()));
            std::uint8_t buf[] = {   // REX.W MOV $imm32, %rcx  (%rcx = operation)
                0x48,
                0xC7,
                0xC1,
                static_cast<std::uint8_t>(value >>  0),
                static_cast<std::uint8_t>(value >>  8),
                static_cast<std::uint8_t>(value >> 16),
                static_cast<std::uint8_t>(value >> 24),
            };
            f_file.add_text(buf, sizeof(buf));
        }
        generate_reg_mem_string(op->get_result(), register_t::REGISTER_RDI);
        generate_external_function_call(EXTERNAL_FUNCTION_STRINGS_SHIFT);
        if(is_assignment)
        {
            generate_reg_mem_string(op->get_result(), register_t::REGISTER_RSI);
            generate_store_string(lhs, register_t::REGISTER_RSI);
        }
        break;

    default:
        throw not_implemented("type not yet supported by the shift operator");

    }
}



} // namespace as2js
// vim: ts=4 sw=4 et
