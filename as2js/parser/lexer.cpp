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
#include    "as2js/lexer.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"


// libutf8
//
#include    <libutf8/libutf8.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C++
//
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>




#include    "as2js/unicode-character-types.ci"


namespace as2js
{


/** \brief The lexer private functions to handle character types.
 *
 * This unnamed namespace is used by the lexer to define a set of
 * private functions and tables used to handle the characters
 * and tokens.
 */
namespace
{




}
// no name namespace


/** \var MAX_REGEXP_LENGTH
 * \brief Maximum length of a regular expression literal.
 *
 * In order to detect a regular expression literal, we do a lookahead of
 * the input data. If your entire code is written on a single line, this
 * means each time we find a '/' we read everything up to the end of the
 * file. Instead, we assume that regular expressions will have a decent
 * length, much less than MAX_REGEXP_LENGTH which is the maximum number
 * of characters we read before giving up on finding the closing '/'
 * character.
 *
 * Since we now have a flag to determine whether a regular expression can
 * appear as the next token, it is much less of an issue (i.e. in most cases
 * the first '/' will be taken as an operator and thus we do not do the
 * readahead when that happens).
 *
 * If deamed necessary, we may look into offering a way to define the
 * maximum length as a command line argument or a pragma.
 *
 * \note
 * The comments, which start with `//` or `/\*`, are not affected since the
 * first would represent an empty regular expression, which is no allowed
 * by the syntax (and really not useful anyway) and the second would
 * represent an invalid regular expression (the `*` must be preceeded by
 * something to be a valid repeat in a regular expression).
 */


/**********************************************************************/
/**********************************************************************/
/***  LEXER CREATOR  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Initialize the lexer object.
 *
 * The constructor of the lexer expect a valid pointer of an Input
 * stream.
 *
 * It optionally accepts an options pointer. If the pointer is null,
 * then all the options are assumed to be set to zero (0). So all
 * extensions are turned off.
 *
 * \param[in] input  The input stream.
 * \param[in] options  A set of options, may be null.
 */
lexer::lexer(base_stream::pointer_t input, options::pointer_t options)
    : f_input(input)
    , f_options(options)
{
    if(f_input == nullptr)
    {
        throw invalid_data("the 'input' stream is already in error in the lexer() constructor.");
    }
    if(f_options == nullptr)
    {
        throw invalid_data("the 'options' pointer cannot be null in the lexer() constructor.");
    }
}



/** \brief Retrieve the input stream pointer.
 *
 * This function returns the input stream pointer of the lexer object.
 *
 * \return The input pointer as specified when creating the lexer object.
 */
base_stream::pointer_t lexer::get_input()
{
    return f_input;
}


position lexer::get_position() const
{
    return f_input->get_position();
}


/** \brief Retrieve the next character of input.
 *
 * This function reads one character of input and returns it.
 *
 * If the character is a newline, linefeed, etc. it affects the current
 * line number, page number, etc. as required. The following characters
 * have such an effect:
 *
 * \li '\\n' -- the newline character adds a new line
 * \li '\\r' -- the carriage return character adds a new line; if followed
 *              by a '\n', remove it too; always return '\\n' and not '\\r'
 * \li '\\f' -- the formfeed adds a new page
 * \li LINE SEPARATOR (0x2028) -- add a new line
 * \li PARAGRAPH SEPARATOR (0x2029) -- add a new paragraph
 *
 * If the ungetc() function was called before a call to getc(), then
 * that last character is returned instead of a new character from the
 * input stream. In that case, the character has no effect on the line
 * number, page number, etc.
 *
 * \internal
 *
 * \return The next Unicode character.
 */
char32_t lexer::getc()
{
    char32_t c(U'\0');

    // if some characters were ungotten earlier, re-read those first
    // and avoid any side effects on the position... (which means
    // we could be a bit off, but the worst case is for regular expressions
    // and assuming the regular expression is valid, it will not be a
    // problem...)
    //
    if(!f_unget.empty())
    {
        c = f_unget.back();
        f_unget.pop_back();
        f_char_type = char_type(c);
    }
    else
    {
        c = f_input->read_char();
        f_input->get_position().new_column();

        f_char_type = char_type(c);
        if((f_char_type & (CHAR_LINE_TERMINATOR | CHAR_WHITE_SPACE)) != 0)
        {
            // Unix (Linux, Mac OS/X, HP-UX, SunOS, etc.) uses '\n'
            // Microsoft (MS-DOS, MS-Windows) uses '\r\n'
            // Macintosh (OS 1 to OS 9, and Apple 1,2,3) uses '\r'
            switch(c)
            {
            case '\n':   // LINE FEED (LF)
                // '\n' represents a newline
                f_input->get_position().new_line();
                break;

            case '\r':   // CARRIAGE RETURN (CR)
                // skip '\r\n' as one newline
                // also in case we are on Mac, skip each '\r' as one newline
                f_input->get_position().new_line();
                c = f_input->read_char();
                if(c != '\n') // if '\n' follows, skip it silently
                {
                    ungetc(c);
                }
                c = '\n';
                break;

            case '\f':   // FORM FEED (FF)
                // view the form feed as a new page for now...
                f_input->get_position().new_page();
                break;

            //case 0x0085: // NEXT LINE (NEL) -- not in ECMAScript 5
            //    // 
            //    f_input->get_position().new_line();
            //    break;

            case 0x2028: // LINE SEPARATOR (LSEP)
                f_input->get_position().new_line();
                break;

            case 0x2029: // PARAGRAPH SEPARATOR (PSEP)
                f_input->get_position().new_paragraph();
                break;

            }
        }
    }

    return c;
}


/** \brief Unget a character.
 *
 * Whenever reading a token, it is most often that the end of the token
 * is discovered by reading one too many character. This function is
 * used to push that character back in the input stream.
 *
 * Also the stream implementation also includes an unget, we do not use
 * that unget. The reason is that the getc() function needs to know
 * whether the character is a brand new character from that input stream
 * or the last ungotten character. The difference is important to know
 * whether the character has to have an effect on the line number,
 * page number, etc.
 *
 * The getc() function first returns the last character sent via
 * ungetc() (i.e. LIFO).
 *
 * \internal
 *
 * \param[in] c  The input character to "push back in the stream".
 */
void lexer::ungetc(char32_t c)
{
    // unget only if not an invalid characters (especially not CHAR32_EOF)
    //
    if(c < 0x110000)
    {
        f_unget.push_back(c);
    }
}


/** \brief Determine the type of a character.
 *
 * This function determines the type of a character.
 *
 * The function first uses a switch for most of the characters used in
 * JavaScript are ASCII characters and thus are well defined and can
 * have their type defined in a snap.
 *
 * Unicode characters make use of a table to convert the character in
 * a type. Unicode character are either viewed as letters (CHAR_LETTER)
 * or as punctuation (CHAR_PUNCTUATION).
 *
 * The exceptions are the characters viewed as either line terminators
 * or white space characters. Those are captured by the switch.
 *
 * \attention
 * Each character type is is a flag that can be used to check whether
 * the character is of a certain category, or a set of categories all
 * at once (i.e. (CHAR_LETTER | CHAR_DIGIT) means any character which
 * represents a letter or a digit.)
 *
 * \internal
 *
 * \param[in] c  The character of which the type is to be determined.
 *
 * \return The character type (one of the CHAR_...)
 */
lexer::char_type_t lexer::char_type(char32_t c)
{
    switch(c)
    {
    case '\0':   // NULL (NUL)
    case STRING_CONTINUATION: // ( '\' + line terminator )
        return CHAR_INVALID;

    case '\n':   // LINE FEED (LF)
    case '\r':   // CARRIAGE RETURN (CR)
    //case 0x0085: // NEXT LINE (NEL) -- not in ECMAScript 5
    case 0x2028: // LINE SEPARATOR (LSEP)
    case 0x2029: // PARAGRAPH SEPARATOR (PSEP)
        return CHAR_LINE_TERMINATOR;

    case '\t':   // CHARACTER TABULATION (HT)
    case '\v':   // LINE TABULATION (VT)
    case '\f':   // FORM FEED (FF)
    case ' ':    // SPACE (SP)
    case 0x00A0: // NO-BREAK SPACE
    case 0x1680: // OGHAM SPACE MARK
    case 0x180E: // MOGOLIAN VOWEL SEPARATOR (MVS)
    case 0x2000: // EN QUAD (NQSP)
    case 0x2001: // EM QUAD (MQSP)
    case 0x2002: // EN SPACE (EMSP)
    case 0x2003: // EM SPACE (ENSP)
    case 0x2004: // THREE-PER-EM SPACE (3/MSP)
    case 0x2005: // FOUR-PER-EM SPACE (4/MSP)
    case 0x2006: // SIX-PER-EM SPACE (6/MSP)
    case 0x2007: // FIGURE SPACE (FSP)
    case 0x2008: // PUNCTUATION SPACE (PSP)
    case 0x2009: // THIN SPACE (THSP)
    case 0x200A: // HAIR SPACE HSP)
    //case 0x200B: // ZERO WIDTH SPACE (ZWSP) -- this was accepted before, but it is not marked as a Zs category
    case 0x202F: // NARROW NO-BREAK SPACE (NNBSP)
    case 0x205F: // MEDIUM MATHEMATICAL SPACE (MMSP)
    case 0x3000: // IDEOGRAPHIC SPACE (IDSP)
    case 0xFEFF: // BYTE ORDER MARK (BOM)
        return CHAR_WHITE_SPACE;

    case '0': // '0' ... '9'
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return CHAR_DIGIT | CHAR_HEXDIGIT;

    case 'a': // 'a' ... 'f'
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A': // 'A' ... 'F'
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return CHAR_LETTER | CHAR_HEXDIGIT;

    case '_':
    case '$':
        return CHAR_LETTER;

    default:
        if((c >= 'g' && c <= 'z')
        || (c >= 'G' && c <= 'Z'))
        {
            return CHAR_LETTER;
        }
        if((c & 0x0FFFF) >= 0xFFFE
        || (c >= 0xD800 && c <= 0xDFFF))
        {
            // 0xFFFE and 0xFFFF are invalid in all planes and
            // surrogate numbers are not valid standalone characters
            //
            return CHAR_INVALID;
        }
        if(c < 0x7F)
        {
            return CHAR_PUNCTUATION;
        }
        // TODO: this will be true in most cases, but not always!
        //       documentation says:
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
        //
        // TODO: test with std::lower_bound() instead...
        //
        //identifier_characters_t searched{c, 0};
        //auto const & it(std::lower_bound(g_identifier_characters, g_identifier_characters + g_identifier_characters_size, searched);
        //if(it != g_identifier_characters + g_identifier_characters_size
        //&& c <= it->f_max) // make sure upper bound also matches
        //{
        //    return CHAR_LETTER;
        //}

        {
            size_t i, j, p;
            int    r;

            i = 0;
            j = g_identifier_characters_size;
            while(i < j)
            {
                p = (j - i) / 2 + i;
                if(g_identifier_characters[p].f_min <= c && c <= g_identifier_characters[p].f_max)
                {
                    return CHAR_LETTER;
                }
                r = g_identifier_characters[p].f_min - c;
                if(r < 0)
                {
                    i = p + 1;
                }
                else
                {
                    j = p;
                }
            }
        }
        return CHAR_PUNCTUATION;

    }
    snapdev::NOT_REACHED();
}




/** \brief Read a hexadecimal number.
 *
 * This function reads 0's and 1's up until another character is found
 * or \p max digits were read. That other character is ungotten so the
 * next call to getc() will return that non-binary character.
 *
 * Since the function is called without an introducing digit, the
 * number could end up being empty. If that happens, an error is
 * generated and the function returns -1 (although -1 is a valid
 * number assuming you accept all 64 bits.)
 *
 * \internal
 *
 * \param[in] max  The maximum number of digits to read.
 * \param[in] allow_separator  Whether to allow the '_' separator.
 *
 * \return The number just read as an integer (64 bit).
 */
std::int64_t lexer::read_hex(std::uint32_t const max, bool allow_separator)
{
    std::int64_t result(0);
    char32_t c(getc());
    std::uint32_t p(0);
    for(; ((f_char_type & CHAR_HEXDIGIT) != 0) && p < max; ++p)
    {
        if(c <= '9')
        {
            result = result * 16 + c - '0';
        }
        else if(c <= 'F')
        {
            result = result * 16 + c - ('A' - 10);
        }
        else if(c <= 'f')
        {
            result = result * 16 + c - ('a' - 10);
        }
        c = getc();
        if(allow_separator && c == '_')
        {
            c = getc();
            if(c == '_')
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "a hexadecimal number cannot have two \"_\" in a row separating digits.";
                return -1;
            }
            if((f_char_type & CHAR_HEXDIGIT) == 0)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "a hexadecimal number cannot end with \"_\".";
                return -1;
            }
        }
    }
    ungetc(c);

    if(p == 0)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
        msg << "invalid hexadecimal number, at least one digit is required.";
        return -1;
    }

    // TODO: In strict mode, should we check whether we got p == max?
    // WARNING: this is also used by the ReadNumber() function

    return result;
}


/** \brief Read a binary number.
 *
 * This function reads 0's and 1's up until another character is found
 * or \p max digits were read. That other character is ungotten so the
 * next call to getc() will return that non-binary character.
 *
 * Since the function is called without an introducing digit, the
 * number could end up being empty. If that happens, an error is
 * generated and the function returns -1 (although -1 is a valid
 * number assuming you accept all 64 bits.)
 *
 * \internal
 *
 * \param[in] max  The maximum number of digits to read.
 *
 * \return The number just read as an integer (64 bit).
 */
std::int64_t lexer::read_binary(std::uint32_t const max)
{
    std::int64_t result(0);
    char32_t c(getc());
    std::uint32_t p(0);
    for(; (c == '0' || c == '1') && p < max; ++p)
    {
        result = result * 2 + c - '0';
        c = getc();
        if(c == '_')
        {
            c = getc();
            if(c == '_')
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "a binary number cannot have two \"_\" in a row separating digits.";
                return -1;
            }
            if(c != '0' && c != '1')
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "a binary number cannot end with \"_\".";
                return -1;
            }
        }
    }
    ungetc(c);

    if(p == 0)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
        msg << "invalid binary number, at least one digit is required.";
        return -1;
    }

    return result;
}


/** \brief Read an octal number.
 *
 * This function reads octal digits up until a character other than a
 * valid octal digit or \p max digits were read. That \em invalid character
 * is ungotten so the next call to getc() returns that non-octal character.
 *
 * \note
 * The \p legacy flag is set to true to allow for decimal numbers.
 * If the function is called because the number starts with a zero,
 * and yet detect an 8 or a 9, then the function switches to reading
 * a decimal number instead.
 *
 * \todo
 * Check for overflows (since we can have 1 or 2 bits of overflow...)
 *
 * \internal
 *
 * \param[in] c  The character that triggered a call to read_octal().
 * \param[in] max  The maximum number of digits to read.
 * \param[in] legacy  Whether this is reading a legacy octal number.
 * \param[in] allow_separator  Whether to allow the '_' separator.
 *
 * \return The number just read as an integer (64 bit).
 */
std::int64_t lexer::read_octal(char32_t c, std::uint32_t const max, bool legacy, bool allow_separator)
{
    std::string number;
    number += c;
    std::int64_t result(c - '0');
    bool decimal(false);
    char32_t const max_digit(legacy ? '9' : '7');
    std::uint32_t p(1);
    for(; p < max; ++p)
    {
        c = getc();
        if(allow_separator && c == '_')
        {
            c = getc();
            if(c == '_')
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "an octal number cannot have two \"_\" in a row separating digits.";
                return -1;
            }
            if(c < '0' || c > max_digit)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
                msg << "an octal number cannot end with \"_\".";
                return -1;
            }
        }
        if(c < '0' || c > max_digit)
        {
            ungetc(c);
            break;
        }
        number += c;
        if(c == '8' || c == '9')
        {
            decimal = true;
        }
        result = result * 8 + c - '0';
    }

    // if we detected the digits '8' or '9', switch to decimal
    // (if allowed by the `legacy` flag)
    //
    if(decimal)
    {
        result = std::stoull(number, nullptr, 10);
    }

    return result;
}


/** \brief Read characters representing an escape sequence.
 *
 * This function reads the next few characters transforming them in one
 * escape sequence character.
 *
 * Some characters are extensions and require the extended escape
 * sequences to be turned on in order to be accepted. These are marked
 * as an extension in the list below.
 *
 * The function supports:
 *
 * \li \\u#### -- the 4 digit Unicode character
 * \li \\U######## -- the 8 digit Unicode character, this is an extension
 * \li \\x## or \\X## -- the 2 digit ISO-8859-1 character
 * \li \\' -- escape the single quote (') character
 * \li \\" -- escape the double quote (") character
 * \li \\\\ -- escape the backslash (\) character
 * \li \\b -- the backspace character
 * \li \\e -- the escape character, this is an extension
 * \li \\f -- the formfeed character
 * \li \\n -- the newline character
 * \li \\r -- the carriage return character
 * \li \\t -- the tab character
 * \li \\v -- the vertical tab character
 * \li \\\<newline> or \\\<#x2028> or \\\<#x2029> -- continuation characters
 * \li \\### -- 1 to 3 octal digit ISO-8859-1 character, this is an extension
 * \li \\0 -- the NUL character
 *
 * Any other character generates an error message if appearing after a
 * backslash (\).
 *
 * \internal
 *
 * \param[in] accept_continuation  Whether the backslash + newline combination
 *                                 is acceptable in this token.
 *
 * \return The escape character if valid, '?' otherwise.
 */
char32_t lexer::escape_sequence(bool accept_continuation)
{
    char32_t c(getc());
    switch(c)
    {
    case 'u':
        // 4 hex digits
        return read_hex(4, false);

    case 'U':
        // We support full Unicode without the need for the programmer to
        // encode his characters in UTF-16 by hand! The compiler spits out
        // the characters using two '\uXXXX' characters (surrogates).
        //
        if(has_option_set(option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            // 6 hex digits
            return read_hex(6, false);
        }
        break;

    case 'x':
    case 'X':
        // 2 hex digits
        return read_hex(2, false);

    case '\'':
    case '\"':
    case '`':       // for templates (but accepted anywhere)
    case '\\':
        return c;

    case 'b':
        return '\b';

    case 'e':
        if(has_option_set(option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            return '\033';
        }
        break;

    case 'f':
        return '\f';

    case 'n':
        return '\n';

    case 'r':
        return '\r';

    case 't':
        return '\t';

    case 'v':
        return '\v';

    case '\n':
    case 0x2028:
    case 0x2029:
        if(accept_continuation)
        {
            return STRING_CONTINUATION;
        }
        // make sure line terminators do not get skipped
        ungetc(c);
        break;

    default:
        if(has_option_set(option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            if(c >= '0' && c <= '7')
            {
                return read_octal(c, 3, false, false);
            }
        }
        else
        {
            if(c == '0')
            {
                return '\0';
            }
        }
        break;

    }

    if(c > ' ' && c < 0x7F)
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
        msg << "unknown escape letter \"" << static_cast<char>(c) << "\"";
    }
    else
    {
        message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
        msg << "unknown escape letter \"\\U" << std::hex << std::setfill('0') << std::setw(6) << static_cast<int32_t>(c) << "\"";
    }

    return '?';
}


/** \brief Read a set of characters as defined by \p flags.
 *
 * This function reads all the characters as long as their type match
 * the specified flags. The result is saved in the \p str parameter.
 *
 * At the time the function is called, \p c is expected to be the first
 * character to be added to \p str.
 *
 * The first character that does not satisfy the flags is pushed back
 * in the input stream so one can call getc() again to retrieve it.
 *
 * \param[in] c  The character that prompted this call and which ends up
 *               first in \p str.
 * \param[in] flags  The flags that must match each character, including
 *                   \p c character type.
 * \param[in,out] str  The resulting string. It is expected to be empty on
 *                     call but does not need to (it does not get cleared.)
 *
 * \internal
 *
 * \return The next character, although it was also ungotten.
 */
char32_t lexer::read(char32_t c, char_type_t flags, std::string & str)
{
    do
    {
        if((f_char_type & CHAR_INVALID) == 0)
        {
            str += c;
        }
        c = getc();
    }
    while((f_char_type & flags) != 0 && c != CHAR32_EOF);

    ungetc(c);

    return c;
}



/** \brief Read an identifier.
 *
 * This function reads an identifier and checks whether that identifier
 * is a keyword.
 *
 * The list of reserved keywords has defined in ECMAScript is defined
 * below. Note that includes all versions (1 through 5) and we mark
 * all of these identifiers as keywords. Note that a few keywords can
 * also be used a field names (i.e. this.delete = 123). We support that
 * to some extend.
 *
 * \li abstract
 * \li boolean
 * \li break
 * \li byte
 * \li case
 * \li catch
 * \li char
 * \li class
 * \li const
 * \li continue
 * \li debugger
 * \li default
 * \li delete
 * \li do
 * \li double
 * \li else
 * \li enum
 * \li export
 * \li extends
 * \li false
 * \li final
 * \li finally
 * \li float
 * \li for
 * \li function
 * \li goto
 * \li if
 * \li implements
 * \li import
 * \li in
 * \li int
 * \li instanceof
 * \li interface
 * \li let
 * \li long
 * \li native
 * \li new
 * \li null
 * \li package
 * \li private
 * \li protected
 * \li public
 * \li return
 * \li short
 * \li static
 * \li super
 * \li switch
 * \li synchronized
 * \li this
 * \li throw
 * \li throws
 * \li transient
 * \li true
 * \li try
 * \li typeof
 * \li var
 * \li void
 * \li volatile
 * \li while
 * \li with
 * \li yield
 *
 * The function sets the f_result_type and f_result_string as required.
 *
 * We also understand additional keywords as defined here:
 *
 * \li as -- from ActionScript, to do a cast
 * \li is -- from ActionScript, to check a value type
 * \li namespace -- to encompass many declarations in a namespace
 * \li use -- to avoid having to declare certain namespaces, declare number
 *            types, change pragma (options) value
 *
 * We also support the special names:
 *
 * \li Infinity, which is supposed to be a global variable
 * \li NaN, which is supposed to be a global variable
 * \li undefined, which is supposed to never be defined
 * \li __FILE__, which gets transformed to the filename of the input stream
 * \li __LINE__, which gets transformed to the current line number
 *
 * \internal
 *
 * \param[in] c  The current character representing the first identifier character.
 */
void lexer::read_identifier(char32_t c)
{
    // identifiers support character escaping like strings
    // so we have a special identifier read instead of
    // calling the read() function
    std::string str;
    for(;;)
    {
        // here escaping is not used to insert invalid characters
        // in a literal, but instead to add characters that
        // could otherwise be difficult to type (or possibly
        // difficult to share between users).
        //
        // so we immediately manage the backslash and use the
        // character type of the escape character!
        if(c == '\\')
        {
            c = escape_sequence(false);
            f_char_type = char_type(c);
            if((f_char_type & (CHAR_LETTER | CHAR_DIGIT)) == 0 || c == CHAR32_EOF)
            {
                // do not unget() this character...
                break;
            }
        }
        else if((f_char_type & (CHAR_LETTER | CHAR_DIGIT)) == 0 || c == CHAR32_EOF)
        {
            // unget this character
            ungetc(c);
            break;
        }
        if((f_char_type & CHAR_INVALID) == 0)
        {
            str += libutf8::to_u8string(c);
        }
        c = getc();
    }

    // An identifier can be a keyword, we check that right here!
    std::size_t l(str.length());
    if(l > 1)
    {
        // keywords do not require UTF-8 so just check with ASCII
        //
        char const *s(str.c_str());
        switch(s[0])
        {
        case 'a':
            if(l == 8 && str == "abstract")
            {
                f_result_type = node_t::NODE_ABSTRACT;
                return;
            }
            if(l == 5 && str == "async")
            {
                f_result_type = node_t::NODE_ASYNC;
                return;
            }
            if(l == 5 && str == "await")
            {
                f_result_type = node_t::NODE_AWAIT;
                return;
            }
            if(l == 2 && s[1] == 's')
            {
                f_result_type = node_t::NODE_AS;
                return;
            }
            break;

        case 'b':
            if(l == 7 && str == "boolean")
            {
                f_result_type = node_t::NODE_BOOLEAN;
                return;
            }
            if(l == 5 && str == "break")
            {
                f_result_type = node_t::NODE_BREAK;
                return;
            }
            if(l == 4 && str == "byte")
            {
                f_result_type = node_t::NODE_BYTE;
                return;
            }
            break;

        case 'c':
            if(l == 4 && str == "case")
            {
                f_result_type = node_t::NODE_CASE;
                return;
            }
            if(l == 5 && str == "catch")
            {
                f_result_type = node_t::NODE_CATCH;
                return;
            }
            if(l == 4 && str == "char")
            {
                f_result_type = node_t::NODE_CHAR;
                return;
            }
            if(l == 5 && str == "class")
            {
                f_result_type = node_t::NODE_CLASS;
                return;
            }
            if(l == 5 && str == "const")
            {
                f_result_type = node_t::NODE_CONST;
                return;
            }
            if(l == 8 && str == "continue")
            {
                f_result_type = node_t::NODE_CONTINUE;
                return;
            }
            break;

        case 'd':
            if(l == 8 && str == "debugger")
            {
                f_result_type = node_t::NODE_DEBUGGER;
                return;
            }
            if(l == 7 && str == "default")
            {
                f_result_type = node_t::NODE_DEFAULT;
                return;
            }
            if(l == 6 && str == "delete")
            {
                f_result_type = node_t::NODE_DELETE;
                return;
            }
            if(l == 2 && s[1] == 'o')
            {
                f_result_type = node_t::NODE_DO;
                return;
            }
            if(l == 6 && str == "double")
            {
                f_result_type = node_t::NODE_DOUBLE;
                return;
            }
            break;

        case 'e':
            if(l == 4 && str == "else")
            {
                f_result_type = node_t::NODE_ELSE;
                return;
            }
            if(l == 4 && str == "enum")
            {
                f_result_type = node_t::NODE_ENUM;
                return;
            }
            if(l == 6 && str == "ensure")
            {
                f_result_type = node_t::NODE_ENSURE;
                return;
            }
            if(l == 6 && str == "export")
            {
                f_result_type = node_t::NODE_EXPORT;
                return;
            }
            if(l == 7 && str == "extends")
            {
                f_result_type = node_t::NODE_EXTENDS;
                return;
            }
            if(l == 6 && str == "extern")
            {
                f_result_type = node_t::NODE_EXTERN;
                return;
            }
            break;

        case 'f':
            if(l == 5 && str == "false")
            {
                f_result_type = node_t::NODE_FALSE;
                return;
            }
            if(l == 5 && str == "final")
            {
                f_result_type = node_t::NODE_FINAL;
                return;
            }
            if(l == 7 && str == "finally")
            {
                f_result_type = node_t::NODE_FINALLY;
                return;
            }
            if(l == 5 && str == "float")
            {
                f_result_type = node_t::NODE_FLOAT;
                return;
            }
            if(l == 3 && s[1] == 'o' && s[2] == 'r')
            {
                f_result_type = node_t::NODE_FOR;
                return;
            }
            if(l == 8 && str == "function")
            {
                f_result_type = node_t::NODE_FUNCTION;
                return;
            }
            break;

        case 'g':
            if(l == 4 && str == "goto")
            {
                f_result_type = node_t::NODE_GOTO;
                return;
            }
            break;

        case 'i':
            if(l == 2 && s[1] == 'f')
            {
                f_result_type = node_t::NODE_IF;
                return;
            }
            if(l == 10 && str == "implements")
            {
                f_result_type = node_t::NODE_IMPLEMENTS;
                return;
            }
            if(l == 6 && str == "import")
            {
                f_result_type = node_t::NODE_IMPORT;
                return;
            }
            if(l == 2 && s[1] == 'n')
            {
                f_result_type = node_t::NODE_IN;
                return;
            }
            if(l == 6 && str == "inline")
            {
                f_result_type = node_t::NODE_INLINE;
                return;
            }
            if(l == 10 && str == "instanceof")
            {
                f_result_type = node_t::NODE_INSTANCEOF;
                return;
            }
            if(l == 9 && str == "interface")
            {
                f_result_type = node_t::NODE_INTERFACE;
                return;
            }
            if(l == 9 && str == "invariant")
            {
                f_result_type = node_t::NODE_INVARIANT;
                return;
            }
            if(l == 2 && s[1] == 's')
            {
                f_result_type = node_t::NODE_IS;
                return;
            }
            break;

        case 'I':
            if(l == 8 && str == "Infinity")
            {
                // Note:
                //
                // JavaScript does NOT automaticlly see this identifier as
                // a number, so you can write statements such as:
                //
                //     var Infinity = 123;
                //
                // On our end, by immediately transforming that identifier
                // into a number, we at least prevent such strange syntax
                // and we do not have to "specially" handle "Infinity" when
                // encountering an identifier.
                //
                // However, JavaScript considers Infinity as a read-only
                // object defined in the global scope. It can also be
                // retrieved from Number as in:
                //
                //     Number.POSITIVE_INFINITY
                //     Number.NEGATIVE_INFINITY
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Infinity
                //
                f_result_type = node_t::NODE_FLOATING_POINT;
                f_result_floating_point.set_infinity();
                return;
            }
            break;

        case 'l':
            if(l == 4 && str == "long")
            {
                f_result_type = node_t::NODE_LONG;
                return;
            }
            break;

        case 'n':
            if(l == 9 && str == "namespace")
            {
                f_result_type = node_t::NODE_NAMESPACE;
                return;
            }
            if(l == 6 && str == "native")
            {
                f_result_type = node_t::NODE_NATIVE;
                return;
            }
            if(l == 3 && s[1] == 'e' && s[2] == 'w')
            {
                f_result_type = node_t::NODE_NEW;
                return;
            }
            if(l == 4 && str == "null")
            {
                f_result_type = node_t::NODE_NULL;
                return;
            }
            break;

        case 'N':
            if(l == 3 && s[1] == 'a' && s[2] == 'N')
            {
                // Note:
                //
                // JavaScript does NOT automatically see this identifier as
                // a number, so you can write statements such as:
                //
                //     var NaN = 123;
                //
                // On our end, by immediately transforming that identifier
                // into a number, we at least prevent such strange syntax
                // and we do not have to "specially" handle "NaN" when
                // encountering an identifier.
                //
                // However, JavaScript considers NaN as a read-only
                // object defined in the global scope. It can also be
                // retrieved from Number as in:
                //
                //     Number.NaN
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/NaN
                //
                f_result_type = node_t::NODE_FLOATING_POINT;
                f_result_floating_point.set_nan();
                return;
            }
            break;

        case 'p':
            if(l == 7 && str == "package")
            {
                f_result_type = node_t::NODE_PACKAGE;
                return;
            }
            if(l == 7 && str == "private")
            {
                f_result_type = node_t::NODE_PRIVATE;
                return;
            }
            if(l == 9 && str == "protected")
            {
                f_result_type = node_t::NODE_PROTECTED;
                return;
            }
            if(l == 6 && str == "public")
            {
                f_result_type = node_t::NODE_PUBLIC;
                return;
            }
            break;

        case 'r':
            if(l == 7 && str == "require")
            {
                f_result_type = node_t::NODE_REQUIRE;
                return;
            }
            if(l == 6 && str == "return")
            {
                f_result_type = node_t::NODE_RETURN;
                return;
            }
            break;

        case 's':
            if(l == 5 && str == "short")
            {
                f_result_type = node_t::NODE_SHORT;
                return;
            }
            if(l == 6 && str == "static")
            {
                f_result_type = node_t::NODE_STATIC;
                return;
            }
            if(l == 5 && str == "super")
            {
                f_result_type = node_t::NODE_SUPER;
                return;
            }
            if(l == 6 && str == "switch")
            {
                f_result_type = node_t::NODE_SWITCH;
                return;
            }
            if(l == 12 && str == "synchronized")
            {
                f_result_type = node_t::NODE_SYNCHRONIZED;
                return;
            }
            break;

        case 't':
            if(l == 4 && str == "then")
            {
                f_result_type = node_t::NODE_THEN;
                return;
            }
            if(l == 4 && str == "this")
            {
                f_result_type = node_t::NODE_THIS;
                return;
            }
            if(l == 5 && str == "throw")
            {
                f_result_type = node_t::NODE_THROW;
                return;
            }
            if(l == 6 && str == "throws")
            {
                f_result_type = node_t::NODE_THROWS;
                return;
            }
            if(l == 9 && str == "transient")
            {
                f_result_type = node_t::NODE_TRANSIENT;
                return;
            }
            if(l == 4 && str == "true")
            {
                f_result_type = node_t::NODE_TRUE;
                return;
            }
            if(l == 3 && s[1] == 'r' && s[2] == 'y')
            {
                f_result_type = node_t::NODE_TRY;
                return;
            }
            if(l == 6 && str == "typeof")
            {
                f_result_type = node_t::NODE_TYPEOF;
                return;
            }
            break;

        case 'u':
            if(l == 9 && str == "undefined")
            {
                // Note: undefined is actually not a reserved keyword, but
                //       by reserving it, we avoid stupid mistakes like:
                //
                //       var undefined = 5;
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/undefined
                //
                f_result_type = node_t::NODE_UNDEFINED;
                return;
            }
            if(l == 3 && s[1] == 's' && s[2] == 'e')
            {
                f_result_type = node_t::NODE_USE;
                return;
            }
            break;

        case 'v':
            if(l == 3 && s[1] == 'a' && s[2] == 'r')
            {
                f_result_type = node_t::NODE_VAR;
                return;
            }
            if(l == 4 && str == "void")
            {
                f_result_type = node_t::NODE_VOID;
                return;
            }
            if(l == 8 && str == "volatile")
            {
                f_result_type = node_t::NODE_VOLATILE;
                return;
            }
            break;

        case 'w':
            if(l == 5 && str == "while")
            {
                f_result_type = node_t::NODE_WHILE;
                return;
            }
            if(l == 4 && str == "with")
            {
                f_result_type = node_t::NODE_WITH;
                return;
            }
            break;

        case 'y':
            if(l == 5 && str == "yield")
            {
                f_result_type = node_t::NODE_YIELD;
                return;
            }
            break;

        case '_':
            if(l == 8 && str == "__FILE__")
            {
                f_result_type = node_t::NODE_STRING;
                f_result_string = f_input->get_position().get_filename();
                return;
            }
            if(l == 8 && str == "__LINE__")
            {
                f_result_type = node_t::NODE_INTEGER;
                f_result_integer = f_input->get_position().get_line();
                return;
            }
            break;

        }
    }

    if(l == 0)
    {
        f_result_type = node_t::NODE_UNKNOWN;
    }
    else
    {
        f_result_type = node_t::NODE_IDENTIFIER;
        f_result_string = str;
    }
}


/** \brief Read one number from the input stream.
 *
 * This function is called whenever a digit is found in the input
 * stream. It may also be called if a period was read (the rules
 * are a little more complicated for the period.)
 *
 * The function checks the following character, if it is:
 *
 * \li 'x' or 'X' -- it reads a hexadecimal number, see read_hex()
 * \li 'b' or 'B' -- it reads a binary number, see read_binary()
 * \li 'o' or 'O' -- it reads a octal number, see read_octal()
 * \li '0' -- if the number starts with a zero, it gets read as an octal,
 *            in legacy mode, meaning that if the number includes 8 or 9
 *            then it switches back to seeing the number as a decimal number
 * \li '.' -- it reads a floating point number
 * \li otherwise it reads an integer, although if the integer is
 *     followed by '.', 'e', or 'E', it ends up reading the number
 *     as a floating point
 *
 * The result is saved in the necessary `f_result_...` variables.
 *
 * \internal
 *
 * \param[in] c  The digit or period that triggered this call.
 */
void lexer::read_number(char32_t c)
{
    std::string     number;

    // TODO: accept '_' within the number (between digits)
    //
    if(c == '.')
    {
        // in case the std::stod() does not support a missing 0
        // at the start of a floating point
        //
        number = "0";
    }
    else if(c == '0')
    {
        c = getc();
        if(c == 'x' || c == 'X')
        {
            // hexadecimal number
            //
            f_result_integer = read_hex(16, true);
        }
        else if(c == 'o' || c == 'O')
        {
            // octal number (always available)
            //
            f_result_integer = read_octal(getc(), 22, false, true);
        }
        else if(c == 'b' || c == 'B')
        {
            // binary number (always available in newer ECMAScript)
            //
            f_result_integer = read_binary(64);
        }
        else if(has_option_set(option_t::OPTION_OCTAL)
             && c >= '0' && c <= '7')
        {
            // octal is not permitted in ECMAScript version 3+
            // (especially in strict  mode) unless started with
            // 0o or 0O as above -- this should probably never
            // be used
            //
            f_result_integer = read_octal(c, 22, true, true);
        }
        else
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
            number = "0";
#pragma GCC diagnostic pop
            ungetc(c);
        }
        if(number.empty())
        {
            // we get here if we found a hexadecimal, octal, or binary number
            //
            f_result_type = node_t::NODE_INTEGER;

            // an integer can be followed by the 'n' character to tranform it
            // in a BigInt
            //
            c = getc();
            if(c == 'n')
            {
                // we got a BigInt instead
                // make sure to skip the 'n' and read the next character
                // to properly define the f_char_type
                //
                c = getc();

                // TODO: mark integer as a BIGINT
            }
            ungetc(c);
            return;
        }
    }
    else
    {
        c = read(c, CHAR_DIGIT, number);
    }

    f_result_type = node_t::NODE_FLOATING_POINT;
    if(c == '.')
    {
        getc(); // re-read the '.' character

        char32_t const f(getc()); // check the following character
        if(f != '.' && (f_char_type & CHAR_DIGIT) != 0)
        {
            ungetc(f);

            char32_t const q(read(c, CHAR_DIGIT, number));
            if(q == 'e' || q == 'E')
            {
                getc();        // skip the 'e'
                c = getc();    // get the character after!
                if(c == '-' || c == '+' || (c >= '0' && c <= '9'))
                {
                    number += 'e';
                    c = read(c, CHAR_DIGIT, number);
                }
                else
                {
                    ungetc(c);
                    ungetc(q);
                    f_char_type = char_type(q); // restore this character type, we'll most certainly get an error
                }
            }
            f_result_floating_point = to_floating_point(number);
            return;
        }
        if(f == 'e' || f == 'E')
        {
            char32_t const s(getc());
            if(s == '+' || s == '-')
            {
                char32_t const e(getc());
                if((f_char_type & CHAR_DIGIT) != 0)
                {
                    // considered floating point
                    //
                    number += 'e';
                    number += s;
                    c = read(e, CHAR_DIGIT, number);
                    f_result_floating_point = to_floating_point(number);
                    return;
                }
                ungetc(e);
            }
            // TODO:
            // Here we could check to know whether this really
            // represents a decimal number or whether the decimal
            // point is a member operator. This can be very tricky.
            //
            // This is partially done now, we still fail in cases
            // were someone was to use a member name such as e4z
            // because we would detect 'e' as exponent and multiply
            // the value by 10000... then fail on the 'z'
            //
            if((f_char_type & CHAR_DIGIT) != 0)
            {
                // considered floating point
                number += 'e';
                c = read(s, CHAR_DIGIT, number);
                f_result_floating_point = to_floating_point(number);
                return;
            }
            ungetc(s);
        }
        // restore the '.' and following character (another '.' or a letter)
        // this means we allow for 33.length and 3..5
        //
        ungetc(f);
        ungetc('.');
        f_char_type = char_type('.');
    }
    else if(c == 'e' || c == 'E')
    {
        getc(); // re-read the 'e'

        char32_t const s(getc());
        if(s == '+' || s == '-')
        {
            char32_t const e(getc());
            if((f_char_type & CHAR_DIGIT) != 0)
            {
                // considered floating point
                //
                number += 'e';
                number += s;
                c = read(e, CHAR_DIGIT, number);
                f_result_floating_point = to_floating_point(number);
                return;
            }
            ungetc(e);
        }
        // TODO:
        // Here we could check to know whether this really
        // represents a decimal number or whether the decimal
        // point is a member operator. This can be very tricky.
        //
        // This is partially done now, we still fail in cases
        // were someone was to use a member name such as e4z
        // because we would detect 'e' as exponent and multiply
        // the value by 10000... then fail on the 'z'
        //
        if((f_char_type & CHAR_DIGIT) != 0)
        {
            // considered floating point
            number += 'e';
            c = read(s, CHAR_DIGIT, number);
            f_result_floating_point = to_floating_point(number);
            return;
        }
        ungetc(s);
    }


    // TODO: Support 8, 16, 32 bits, unsigned thereof?
    //       (we have NODE_BYTE and NODE_SHORT, but not really a 32bit
    //       definition yet; NODE_LONG should be 64 bits I think,
    //       although really all of those are types, not literals.)
    //
    f_result_type = node_t::NODE_INTEGER;

    if(c == 'n')
    {
        // skip the 'n' character
        //
        snapdev::NOT_USED(getc());

        // retrieve the next character and restore it, we need its type to
        // properly define the error "integer cannot be followed by a letter"
        //
        c = getc();
        ungetc(c);

        // this is a BigInt instead
        //
        // TODO: should we have a node_t::NODE_BIGINT? Will bigint type
        //       be used in any different way than the NODE_INTEGER?
        //       If not, I would suggest we have a flag of some sort
        //       or make our NODE_INTEGER a BigInt anyway and depending
        //       on the size it can be converted to Number or not

        // TODO: actually look at a bigint library to handle this case
        //
        f_result_integer = std::stoull(number, nullptr, 10);
    }
    else
    {
        // TODO: detect whether an error was detected in the conversion
        //       (this would mainly be overflows)
        //
        f_result_integer = std::stoull(number, nullptr, 10);
    }
}


/** \brief Read one string.
 *
 * This function reads one string from the input stream.
 *
 * The function expects \p quote as an input parameter representing the
 * opening quote. It will read the input stream up to the next line
 * terminator (unless escaped) or the closing quote.
 *
 * Note that we support backslash quoted "strings" which actually
 * represent regular expressions. These cannot be continuated on
 * the following line.
 *
 * This function sets the result type to NODE_STRING. It is changed
 * by the caller when a regular expression was found instead.
 *
 * \internal
 *
 * \param[in] quote  The opening quote, which will match the closing quote.
 */
void lexer::read_string(char32_t quote)
{
    f_result_type = node_t::NODE_STRING;
    f_result_string.clear();

    for(char32_t c(getc()); c != quote; c = getc())
    {
        if(c == CHAR32_EOF)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "the last string was not closed before the end of the input was reached.";
            return;
        }
        if((f_char_type & CHAR_LINE_TERMINATOR) != 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "a string cannot include a line terminator.";
            return;
        }
        if(c == '\\')
        {
            c = escape_sequence(true);

            // here c can be equal to quote (c == quote)
        }
        if(c != STRING_CONTINUATION)
        {
            f_result_string += c;
        }
    }
}


/** \brief Read a full template without expressions or a template head.
 *
 * This function gets called when a backtick (`) is found in the input
 * stream. It reads data until a closing backtick (`) is found or when
 * the "${" sequence is found.
 *
 * The parser is responsible for reading the remaining broken up parts
 * of the template. It will not call the get_token() since that would
 * not work. We offer a special get_next_template_token() instead. That
 * other function may return a template middle or a template tail.
 *
 * \sa get_next_template_token()
 */
void lexer::read_template_start()
{
    f_result_type = node_t::NODE_TEMPLATE;
    f_result_string.clear();

    for(char32_t c(getc()); c != '`'; c = getc())
    {
        while(c == '$')
        {
            c = getc();
            if(c == '{')
            {
                f_result_type = node_t::NODE_TEMPLATE_HEAD;
                return;
            }
            f_result_string += '$';
        }
        if(c == CHAR32_EOF)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "the last template was not closed before the end of the input was reached.";
            return;
        }
        if((f_char_type & CHAR_LINE_TERMINATOR) != 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "a template cannot include a line terminator.";
            return;
        }
        if(c == '\\')
        {
            c = escape_sequence(false);

            // here c can be equal to '`'
        }
        f_result_string += c;
    }
}


/** \brief Read more of a template string.
 *
 * When the parser finds a template head, it next reads an expression. Once
 * the expression is read, we expect a closing curvly brace ('}') which closes
 * that expression. It then calls this function to read the additional template
 * data.
 *
 * The function may return a template middle, when it finds another '${'
 * sequence.
 *
 * Otherwise, the function returns a template tail which means the template
 * ends at that point.
 *
 * \return The token read by the function: NODE_TEMPLATE_MIDDLE or
 * NODE_TEMPLATE_TAIL.
 */
node::pointer_t lexer::get_next_template_token()
{
    f_result_type = node_t::NODE_TEMPLATE_TAIL;
    f_result_string.clear();

    for(char32_t c(getc()); c != '`'; c = getc())
    {
        while(c == '$')
        {
            c = getc();
            if(c == '{')
            {
                f_result_type = node_t::NODE_TEMPLATE_MIDDLE;
                goto exit_loop;
            }
            f_result_string += '$';
        }
        if(c == CHAR32_EOF)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "the last template was not closed before the end of the input was reached.";
            break;
        }
        if((f_char_type & CHAR_LINE_TERMINATOR) != 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "a template cannot include a line terminator.";
            break;
        }
        if(c == '\\')
        {
            c = escape_sequence(false);

            // here c can be equal to '`'
        }
        f_result_string += c;
    }
exit_loop:

    node::pointer_t p(get_new_node(f_result_type));
    p->set_string(f_result_string);

    return p;
}


/** \brief Create a new node of the specified type.
 *
 * This helper function creates a new node at the current position. This
 * is useful internally and in the parser when creating nodes to build
 * the input tree and in order for the new node to get the correct
 * position according to the current lexer position.
 *
 * \param[in] type  The type of the new node.
 *
 * \return A pointer to the new node.
 */
node::pointer_t lexer::get_new_node(node_t type)
{
    node::pointer_t node(std::make_shared<node>(type));
    node->set_position(f_input->get_position());
    // no data by default in this case
    return node;
}


/** \brief Get the next token from the input stream.
 *
 * This function reads one token from the input stream and transform
 * it in a node. The node is automatically assigned the position after
 * the token was read.
 *
 * \param[in] regexp_allowed  Whether a regular expression is allowed at this
 * location.
 *
 * \return The node representing the next token, or a NODE_EOF if the
 *         end of the stream was found.
 */
node::pointer_t lexer::get_next_token(bool regexp_allowed)
{
    f_regexp_allowed = regexp_allowed;

    // get the info
    //
    get_token();

    // create a node for the result
    //
    node::pointer_t node(get_new_node(f_result_type));
    switch(f_result_type)
    {
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_TEMPLATE:
    case node_t::NODE_TEMPLATE_HEAD:
    case node_t::NODE_STRING:
        node->set_string(f_result_string);
        break;

    case node_t::NODE_INTEGER:
        if((f_char_type & CHAR_LETTER) != 0)
        {
            // numbers cannot be followed by a letter
            //
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
            if(f_unget.empty())
            {
                // this should never happen, we can't find a letter which
                // then "disappear" from the unget buffer
                //
                msg << "unexpected letter after an integer.";
            }
            else
            {
                msg << "unexpected letter (\""
                    << f_unget.back()
                    << "\") after an integer.";
            }
            f_result_integer = -1;
        }
        node->set_integer(f_result_integer);
        break;

    case node_t::NODE_FLOATING_POINT:
        if((f_char_type & CHAR_LETTER) != 0)
        {
            // numbers cannot be followed by a letter
            //
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
            if(f_unget.empty())
            {
                // this should never happen, we can't find a letter which
                // then "disappear" from the unget buffer
                //
                msg << "unexpected letter after a floating point number.";
            }
            else
            {
                msg << "unexpected letter (\""
                    << f_unget.back()
                    << "\") after a floating point number.";
            }
            f_result_floating_point = -1.0;
        }
        node->set_floating_point(f_result_floating_point);
        break;

    default:
        // no data attached
        break;

    }
    return node;
}


/** \brief Read one token in the f_result_... variables.
 *
 * This function reads one token from the input stream. It reads one
 * character and determine the type of token (identifier, string,
 * number, etc.) and then reads the whole token.
 *
 * The main purpose of the function is to read characters from the
 * stream and determine what token it represents. It uses many
 * sub-functions to read more complex tokens such as identifiers
 * and numbers.
 *
 * If the end of the input stream is reached, the function returns
 * with a NODE_EOF. The function can be called any number of times
 * after the end of the input is reached.
 *
 * Only useful tokens are returned. Comments and white spaces (space,
 * tab, new line, line feed, etc.) are all skipped silently.
 *
 * The function detects invalid characters which are ignored although
 * the function will first emit an error.
 *
 * This is the function that handles the case of a regular expression
 * written between slashes (/.../). One can also use the backward
 * quotes (`...`) for regular expression to avoid potential confusions
 * with the divide character.
 *
 * \note
 * Most extended operators, such as the power operator (**) are
 * silently returned by this function. If the extended operators are
 * not allowed, the parser will emit an error as required. However,
 * a few operators (<> and :=) are returned jus like the standard
 * operator (NODE_NOT_EQUAL and NODE_ASSIGNMENT) and thus the error
 * has to be emitted here, and it is.
 *
 * \internal
 */
void lexer::get_token()
{
    for(char32_t c(getc());; c = getc())
    {
        if(c == CHAR32_EOF)
        {
            // we're done
            //
            f_result_type = node_t::NODE_EOF;
            return;
        }

        if((f_char_type & (CHAR_WHITE_SPACE | CHAR_LINE_TERMINATOR)) != 0)
        {
            continue;
        }

        if((f_char_type & CHAR_INVALID) != 0)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
            msg << "invalid character \"\\U"
                << std::hex
                << std::setfill('0')
                << std::setw(6)
                << static_cast<std::uint32_t>(c)
                << "\" found as is in the input stream.";
            continue;
        }

        if((f_char_type & CHAR_LETTER) != 0)
        {
            read_identifier(c);
            if(f_result_type == node_t::NODE_UNKNOWN)
            {
                // skip empty identifiers, in most cases
                // this was invalid data in the input
                // and we will have had a message output
                // already so we do not have more to do
                // here
                //
                continue; // LCOV_EXCL_LINE
            }
            return;
        }

        if((f_char_type & CHAR_DIGIT) != 0)
        {
            read_number(c);
            return;
        }

        switch(c)
        {
        case '\\':
            // identifiers can start with a character being escaped
            // (it still needs to be a valid character for an identifier though)
            read_identifier(c);
            if(f_result_type != node_t::NODE_UNKNOWN)
            {
                // this is a valid token, return it
                return;
            }
            // not a valid identifier, ignore here
            // (the read_identifier() emits errors as required)
            break;

        case '"':
        case '\'':
            read_string(c);
            return;

        case '`':
            read_template_start();
            return;

        case '<':
            c = getc();
            if(c == '<')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_SHIFT_LEFT;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_SHIFT_LEFT;
                return;
            }
            if(c == '=')
            {
                c = getc();
                if(c == '>')
                {
                    f_result_type = node_t::NODE_COMPARE;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_LESS_EQUAL;
                return;
            }
            if(c == '%')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_ROTATE_LEFT;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_ROTATE_LEFT;
                return;
            }
            if(c == '>')
            {
                // unfortunately we cannot know whether '<>' or '!=' was used
                // once this function returns so in this very specific case
                // the extended operator has to be checked here
                if(!has_option_set(option_t::OPTION_EXTENDED_OPERATORS))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                    msg << "the \"<>\" operator is only available when extended operators are authorized (use extended_operators;).";
                }
                f_result_type = node_t::NODE_NOT_EQUAL;
                return;
            }
            if(c == '?')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_MINIMUM;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_MINIMUM;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_LESS;
            return;

        case '>':
            c = getc();
            if(c == '>')
            {
                c = getc();
                if(c == '>')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED;
                        return;
                    }
                    ungetc(c);
                    f_result_type = node_t::NODE_SHIFT_RIGHT_UNSIGNED;
                    return;
                }
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_SHIFT_RIGHT;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_SHIFT_RIGHT;
                return;
            }
            if(c == '=')
            {
                f_result_type = node_t::NODE_GREATER_EQUAL;
                return;
            }
            if(c == '%')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_ROTATE_RIGHT;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_ROTATE_RIGHT;
                return;
            }
            if(c == '?')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_MAXIMUM;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_MAXIMUM;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_GREATER;
            return;

        case '!':
            c = getc();
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_STRICTLY_NOT_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_NOT_EQUAL;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_LOGICAL_NOT;
            return;

        case '=':
            c = getc();
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_STRICTLY_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_EQUAL;
                return;
            }
            if(c == '>')
            {
                f_result_type = node_t::NODE_ARROW;
                return;
            }
            if((f_options->get_option(option_t::OPTION_EXTENDED_OPERATORS) & 2) != 0)
            {
                // This one most people will not understand it...
                // The '=' operator by itself is often missused and thus a
                // big source of bugs. By forbiding it, we only allow :=
                // and == (and ===) which makes it safer to use the language.
                //
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                msg << "the \"=\" operator is not available when extended operators value bit 1 is set (use extended_operators(2);); use \":=\" instead.";
            }
            ungetc(c);
            f_result_type = node_t::NODE_ASSIGNMENT;
            return;

        case ':':
            c = getc();
            if(c == '=')
            {
                // unfortunately we cannot know whether ':=' or '=' was used
                // once this function returns so in this very specific case
                // the extended operator has to be checked here
                //
                if(!has_option_set(option_t::OPTION_EXTENDED_OPERATORS))
                {
                    message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                    msg << "the \":=\" operator is only available when extended operators are authorized (use extended_operators;).";
                }
                f_result_type = node_t::NODE_ASSIGNMENT;
                return;
            }
            if(c == ':')
            {
                f_result_type = node_t::NODE_SCOPE;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_COLON;
            return;

        case '~':
            c = getc();
            if(c == '=')
            {
                // http://perldoc.perl.org/perlop.html#Binding-Operators
                //
                // Note that we inverse it (perl uses =~) because otherwise
                // we may interfer with a valid expression:
                //
                //    a = ~b;  <=>  a=~b;
                //
                f_result_type = node_t::NODE_MATCH;
                return;
            }
            if(c == '!')
            {
                // http://perldoc.perl.org/perlop.html#Binding-Operators
                //
                // Note that we inverse it (perl uses =!) because we inversed
                // the ~= and it makes sense to inverse this one too
                //
                // if found to be an unary operator, then it gets converted
                // back to separate operators:
                //
                //    a = ~!b;  <=>  a = (~(!b));
                //
                f_result_type = node_t::NODE_NOT_MATCH;
                return;
            }
            if(c == '~')
            {
                // this operator compares objects using a deep compare
                //
                // http://perldoc.perl.org/perlop.html#Smartmatch-Operator
                //
                // WARNING: if ~~ is used as a unary, then it gets
                //          converted back to two BITWISE NOT by the
                //          parser (so 'a = ~~b;' works as expected).
                //
                f_result_type = node_t::NODE_SMART_MATCH;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_BITWISE_NOT;
            return;

        case '+':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_ADD;
                return;
            }
            if(c == '+')
            {
                f_result_type = node_t::NODE_INCREMENT;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_ADD;
            return;

        case '-':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_SUBTRACT;
                return;
            }
            if(c == '-')
            {
                f_result_type = node_t::NODE_DECREMENT;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_SUBTRACT;
            return;

        case '*':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_MULTIPLY;
                return;
            }
            if(c == '*')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_POWER;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_POWER;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_MULTIPLY;
            return;

        case '/':
            c = getc();
            if(c == '/')
            {
                // skip comments (to end of line)
                do
                {
                    c = getc();
                }
                while(c != CHAR32_EOF && (f_char_type & CHAR_LINE_TERMINATOR) == 0);
                break;
            }
            if(c == '*')
            {
                // skip comments (multiline)
                do
                {
                    c = getc();
                    while(c == '*')
                    {
                        c = getc();
                        if(c == '/')
                        {
                            c = CHAR32_EOF;
                            break;
                        }
                    }
                }
                while(c != CHAR32_EOF);
                break;
            }

            // before we can determine whether we have a
            //
            //    * literal RegExp
            //    * /=
            //    * /
            //
            // we have to read more data to match a RegExp (so at least
            // up to another / with valid RegExp characters in between
            // or no such thing and we have to back off)
            //
            if(f_regexp_allowed)
            {
                std::string regexp;
                char32_t r(c);
                for(;;)
                {
                    if(r == '/'
                    || r == CHAR32_EOF
                    || (f_char_type & CHAR_LINE_TERMINATOR) != 0
                    || regexp.length() > MAX_REGEXP_LENGTH)
                    {
                        break;
                    }
                    if((f_char_type & CHAR_INVALID) == 0)
                    {
                        regexp += r;
                    }
                    r = getc();
                }
                if(r == '/')
                {
                    // TBD -- shall we further verify that this looks like a
                    //        regular expression before accepting it as such?
                    //
                    // this is a valid regular expression written between /.../
                    // read the flags that follow if any
                    //
                    read(r, CHAR_LETTER | CHAR_DIGIT, regexp);
                    f_result_type = node_t::NODE_REGULAR_EXPRESSION;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
                    f_result_string = "/";
#pragma GCC diagnostic pop
                    f_result_string += regexp;  // the read() appended the closing '/' already
                    return;
                }

                // not a regular expression, so unget all of that stuff
                //
                ssize_t p(static_cast<ssize_t>(regexp.length()));
                for(--p; p > 0; --p)
                {
                    ungetc(regexp[p]);
                }

                // 'c' is still the character gotten at the start of this case
            }

            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_DIVIDE;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_DIVIDE;
            return;

        case '%':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_MODULO;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_MODULO;
            return;

        case '?':
            c = getc();
            if(c == '?')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_COALESCE;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_COALESCE;
                return;
            }
            if(c == '.')
            {
                c = getc();
                ungetc(c);
                if(c < '0' || c > '9')
                {
                    f_result_type = node_t::NODE_OPTIONAL_MEMBER;
                    return;
                }
                // if '.' is followed by a digit, it will be viewed as
                // a number on the next get_token()
                //
                c = '.';
            }
            ungetc(c);
            f_result_type = node_t::NODE_CONDITIONAL;
            return;

        case '&':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_BITWISE_AND;
                return;
            }
            if(c == '&')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_LOGICAL_AND;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_LOGICAL_AND;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_BITWISE_AND;
            return;

        case '^':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_BITWISE_XOR;
                return;
            }
            if(c == '^')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_LOGICAL_XOR;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_LOGICAL_XOR;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_BITWISE_XOR;
            return;

        case '|':
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_BITWISE_OR;
                return;
            }
            if(c == '|')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = node_t::NODE_ASSIGNMENT_LOGICAL_OR;
                    return;
                }
                ungetc(c);
                f_result_type = node_t::NODE_LOGICAL_OR;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_BITWISE_OR;
            return;

        case '.':
            c = getc();
            if(c >= '0' && c <= '9')
            {
                // this is probably a valid float
                //
                ungetc(c);
                ungetc('.');
                read_number('.');
                return;
            }
            if(c == '.')
            {
                c = getc();
                if(c == '.')
                {
                    // Elipsis!
                    //
                    f_result_type = node_t::NODE_REST; // rest or spread
                    return;
                }
                ungetc(c);

                // Range (not too sure if this is really used yet
                // and whether it will be called RANGE)
                //
                f_result_type = node_t::NODE_RANGE;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_MEMBER;
            return;

        case '[':
            f_result_type = node_t::NODE_OPEN_SQUARE_BRACKET;
            return;

        case ']':
            f_result_type = node_t::NODE_CLOSE_SQUARE_BRACKET;
            return;

        case '{':
            f_result_type = node_t::NODE_OPEN_CURVLY_BRACKET;
            return;

        case '}':
            f_result_type = node_t::NODE_CLOSE_CURVLY_BRACKET;
            return;

        case '(':
            f_result_type = node_t::NODE_OPEN_PARENTHESIS;
            return;

        case ')':
            f_result_type = node_t::NODE_CLOSE_PARENTHESIS;
            return;

        case ';':
            f_result_type = node_t::NODE_SEMICOLON;
            return;

        case ',':
            f_result_type = node_t::NODE_COMMA;
            return;

        case 0x00D7: // 'x'
            f_result_type = node_t::NODE_MULTIPLY;
            return;

        case 0x00F7: // '/' as '-' + ':'
            f_result_type = node_t::NODE_DIVIDE;
            return;

        case 0x21D2: // =>
            f_result_type = node_t::NODE_ARROW;
            return;

        case 0x2208: // IN
        case 0x220A: // IN
            f_result_type = node_t::NODE_IN;
            return;

        case 0x2227: // && (^)
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_LOGICAL_AND;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_LOGICAL_AND;
            return;

        case 0x2228: // || (v)
            c = getc();
            if(c == '=')
            {
                f_result_type = node_t::NODE_ASSIGNMENT_LOGICAL_OR;
                return;
            }
            ungetc(c);
            f_result_type = node_t::NODE_LOGICAL_OR;
            return;

        case 0x2248: // ~~ (= sign written with two tildes over each other)
            f_result_type = node_t::NODE_ALMOST_EQUAL;
            return;

        case 0x2254: // :=
            f_result_type = node_t::NODE_ASSIGNMENT;
            return;

        case 0x2260: // !=
            f_result_type = node_t::NODE_NOT_EQUAL;
            return;

        case 0x2264: // <=
            f_result_type = node_t::NODE_LESS_EQUAL;
            return;

        case 0x2265: // >=
            f_result_type = node_t::NODE_GREATER_EQUAL;
            return;

        case 0x221E: // INFINITY
            // unicode infinity character which is viewed as a punctuation
            // otherwise so we can reinterpret it safely (it could not be
            // part of an identifier)
            //
            f_result_type = node_t::NODE_FLOATING_POINT;
            f_result_floating_point.set_infinity();
            return;

        case 0xFFFD: // REPACEMENT CHARACTER
            // Java has defined character FFFD as representing NaN so if
            // found in the input we take it as such...
            //
            // see Unicode pri74:
            // http://www.unicode.org/review/resolved-pri.html
            //
            f_result_type = node_t::NODE_FLOATING_POINT;
            f_result_floating_point.set_nan();
            return;

        default:
            if(c > ' ' && c < 0x7F)
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation \""
                    << static_cast<char>(c)
                    << "\"";
            }
            else
            {
                message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation \"\\U"
                    << std::hex
                    << std::setfill('0')
                    << std::setw(6)
                    << static_cast<std::uint32_t>(c)
                    << "\"";
            }
            break;

        }
    }
    snapdev::NOT_REACHED();
}


/** \brief Check whether a given option is set.
 *
 * Because the lexer checks options in many places, it makes use of this
 * helper function to simplify the many tests in the rest of the code.
 *
 * This function checks whether the specified option is set. If so,
 * then it returns true, otherwise it returns false.
 *
 * \note
 * Some options may be set to values other than 0 and 1. In that case
 * this function cannot be used. Right now, this function returns true
 * if the option is \em set, meaning that the option value is not zero.
 * For example, the OPTION_EXTENDED_OPERATORS option may be set to
 * 0, 1, 2, or 3.
 *
 * \param[in] option  The option to check.
 *
 * \return true if the option was set, false otherwise.
 */
bool lexer::has_option_set(option_t option) const
{
    return f_options->get_option(option) != 0;
}



} // namespace as2js
// vim: ts=4 sw=4 et
