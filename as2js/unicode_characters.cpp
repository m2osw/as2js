// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Find different types of Unicode characters.
 *
 * This tool determines what's what as per the ECMAScript definitions
 * used by the lexer.
 *
 * For example, <USP> means all Unicode defined spaces. Here we check
 * all the Unicode characters and determine which are spaces (as one
 * of the functions). This ensures that our lexer implementation is
 * correct.
 *
 * Note that ECMA expects Unicode 3.0 as a base so if we do not support
 * newer characters we are fine (i.e. that means we do not have to check
 * the unicode characters in our lexer, but we have to make sure that at
 * least all Unicode 3.0 characters are supported).
 */


// snapdev
//
#include    <snapdev/not_used.h>


// ICU
// See http://icu-project.org/apiref/icu4c/index.html
//
#include    <unicode/uchar.h>
//#include    <unicode/cuchar> // once available in Linux...


// C
//
#include    <iostream>
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>



#if 0
void usp()
{
    // TBD: do we need this one?
    std::cout
        << "constexpr char32_t const g_whitespace_characters[] =\n{\n";
    for(UChar32 c(0); c < 0x110000; ++c)
    {
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            continue;
        }
        // from Linux
        //if(u_isspace(c)) // this one includes many controls
        //if(u_isJavaSpaceChar(c)) // this one includes 0x2028 and 0x2029
        if(u_charType(c) == U_SPACE_SEPARATOR) // this is what ECMAScript defines as legal
        {
            std::cout
                << std::hex
                << std::setfill('0')
                << "    0x"
                << std::setw(6)
                << c
                << ",\n";
        }
    }
    std::cout
        << "};\n"
           "\n"
        << std::endl;
}
#endif


void identifier()
{
    //          Uppercase letter (Lu)
    //          Lowercase letter (Ll)
    //          Titlecase letter (Lt)
    //          Modifier letter (Lm)
    //          Other letter (Lo)
    //          Letter number (Nl)
    //          Non-spacing mark (Mn)
    //          Combining spacing mark (Mc)
    //          Decimal number (Nd)
    //          Connector punctuation (Pc)
    //          ZWNJ
    //          ZWJ
    //          $
    //          _

    std::cout
        <<
           "/** \\brief Define one valid range of characters.\n"
           " *\n"
           " * This structure defines the range of characters that represent\n"
           " * letters viewed as being valid in EMCAScript version 5.\n"
           " *\n"
           " * The range is defined as min/max pairs. The two values are inclusive.\n"
           " */\n"
           "struct identifier_characters_t\n"
           "{\n"
           "    bool operator < (identifier_characters_t const & rhs) const\n"
           "    {\n"
           "        return f_min < rhs.f_min;\n"
           "    }\n"
           "\n"
           "    char32_t    f_min;\n"
           "    char32_t    f_max;\n"
           "};\n"
           "\n"
           "\n"
           "/** \\brief List of characters that are considered to be letters.\n"
           " *\n"
           " * The ECMAScript version 5 document defines the letters supported in\n"
           " * its identifiers in terms of Unicode characters. This includes many\n"
           " * characters that represent either letters or punctuation.\n"
           " *\n"
           " * The following table includes ranges (min/max) that include characters\n"
           " * that are considered letters in JavaScript code.\n"
           " *\n"
           " * The table was generated using the code in:\n"
           " *\n"
           " * tests/unicode_characters.cpp\n"
           " *\n"
           " * The number of items in the table is defined as\n"
           " * g_identifier_characters_size (see below).\n"
           " *\n"
           " * Characters 200c and 200d are two special cases which can be part of\n"
           " * identifiers even though they are punctuation.\n"
           " */\n"
           "constexpr identifier_characters_t const g_identifier_characters[] =\n"
           "{\n";

    int32_t first(-1);
    int32_t count(0);

    // start at 0x80 since we do not need the ASCII for now
    //
    for(UChar32 c(0x80); c < 0x110000; ++c)
    {
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            if(first != -1)
            {
                std::cout << std::hex << "  " << first << ", " << c - 1 << ",\n";
                ++count;
                first = -1;
            }
            continue;
        }
        // from Linux
        int type(u_charType(c));
        if(c == 0x00200C        // ZWJ
        || c == 0x00200D)       // ZWNJ
        {
            type = U_CONNECTOR_PUNCTUATION;
        }
        switch(type)
        {
        case U_UPPERCASE_LETTER:
        case U_LOWERCASE_LETTER:
        case U_TITLECASE_LETTER:
        case U_MODIFIER_LETTER:
        case U_OTHER_LETTER:
        case U_LETTER_NUMBER:
        case U_NON_SPACING_MARK:
        case U_COMBINING_SPACING_MARK:
        case U_DECIMAL_DIGIT_NUMBER:
        case U_CONNECTOR_PUNCTUATION:
            if(first == -1)
            {
                first = c;
            }
            break;

        default:
            if(first != -1)
            {
                std::cout
                    << std::hex
                    << std::setfill('0')
                    << "    { 0x"
                    << std::setw(5)
                    << first
                    << ", 0x"
                    << std::setw(5)
                    << c - 1
                    << " },\n";
                ++count;
                first = -1;
            }
            break;

        }
    }
    if(first != -1)
    {
        std::cout
            << std::hex
            << "  "
            << first
            << ", "
            << 0x10FFFF
            << ",\n";
        ++count;
    }
    std::cout
        << std::dec
        << "};\n"
           "\n"
           "\n"
           "/** \\brief The size of the character table.\n"
           " *\n"
           " * When defining the type of a character, the lexer uses the\n"
           " * character table. This parameter defines the number of\n"
           " * entries defined in the table.\n"
           " */\n"
           "constexpr std::size_t const g_identifier_characters_size = "
        << count
        << ";\n"
           "\n";
}


int main(int argc, char * argv[])
{
    snapdev::NOT_USED(argc);

    std::string progname(argv[0]);
    std::string::size_type const pos(progname.rfind('/'));
    if(pos != std::string::npos)
    {
        progname = progname.substr(pos + 1);
    }
    std::cout
        << "// WARNING: this file was auto-generated by "
        << progname
        << "\n"
           "namespace\n"
           "{\n"
           "\n";

    // at this point we don't use this table
    //usp();

    identifier();
    std::cout
        << "}\n"
           "// no name namespace\n";
    return 0;
}

// vim: ts=4 sw=4 et
