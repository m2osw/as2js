// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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


// no Qt at the moment unless the "Qt <USP>" is required
//// Qt
////
//#include    <QString>
//#include    <QChar>


// ICU
// See http://icu-project.org/apiref/icu4c/index.html
//
#include    <unicode/uchar.h>
//#include    <unicode/cuchar> // once available in Linux...

// C
//
#include    <iostream>
#include    <iomanip>


void usp()
{
    // TBD: do I need this one?
    //std::cout << "Qt <USP>";
    //for(uint c(0); c < 0x110000; ++c)
    //{
    //    if(c >= 0xD800 && c <= 0xDFFF)
    //    {
    //        continue;
    //    }
    //    // from Qt
    //    QChar::Category cat(QChar::category(c));
    //    if(cat == QChar::Separator_Space)
    //    {
    //        std::cout << std::hex << " 0x" << c;
    //    }
    //}
    std::cout << std::endl << "Lx <USP>";
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
            std::cout << std::hex << " 0x" << c;
        }
    }
    std::cout << std::endl;
}

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
    std::cout << std::endl << "id characters:\n";
    int32_t first(-1);
    int32_t count(0);
    for(UChar32 c(0); c < 0x110000; ++c)
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
        switch(u_charType(c))
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
                std::cout << std::hex << std::setfill('0') << "    { 0x" << std::setw(5) << first << ", 0x" << std::setw(5) << c - 1 << " },\n";
                ++count;
                first = -1;
            }
            break;

        }
    }
    if(first != -1)
    {
        std::cout << std::hex << "  " << first << ", " << 0x10FFFF << std::dec << ",\n";
        ++count;
    }
    std::cout << "got " << count << " groups\n\n";
}

int main()
{
    usp();
    identifier();
    return 0;
}

// vim: ts=4 sw=4 et
