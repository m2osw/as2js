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

// self
//
#include    "as2js/message.h"

#include    "as2js/exception.h"


// libutf8
//
#include    <libutf8/libutf8.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{

namespace
{


constexpr char const * const g_message_names[] =
{
    "off",
    "trace",
    "debug",
    "info",
    "warning",
    "error",
    "fatal",
};


message_callback *  g_message_callback = nullptr;
message_level_t     g_minimum_message_level = message_level_t::MESSAGE_LEVEL_INFO;
int                 g_warning_count = 0;
int                 g_error_count = 0;



}
// no name namespace


/** \brief Create a message object with the specified information.
 *
 * This function generates a message object that can be used to generate
 * a message with the << operator and then gets sent to the client using
 * the message callback function on destruction.
 *
 * The level can be set to any one of the message levels available in
 * the message_level_t enumeration. The special MESSAGE_LEVEL_OFF value
 * can be used to avoid the message altogether (can be handy when you
 * support a varying message level.)
 *
 * \param[in] message_level  The level of the message.
 * \param[in] error_code  An error code to print in the output message.
 * \param[in] pos  The position to which the message applies.
 */
message::message(
          message_level_t message_level
        , err_code_t error_code
        , position const & pos)
    : f_message_level(message_level)
    , f_error_code(error_code)
    , f_position(pos)
{
}


/** \brief Create a message object with the specified information.
 *
 * This function is an overload of the default constructor that does not
 * include the position information. This is used whenever we generate
 * an error from outside of the node tree, parser, etc.
 *
 * \param[in] message_level  The level of the message.
 * \param[in] error_code  An error code to print in the output message.
 */
message::message(message_level_t message_level, err_code_t error_code)
    : f_message_level(message_level)
    , f_error_code(error_code)
{
}


/** \brief Output the message created with the << operators.
 *
 * The destructor of the message object is where things happen. This function
 * prints out the message that was built using the different << operators
 * and the parameters specified in the constructor.
 *
 * The result is then passed to the message callback. If you did not setup
 * that function, the message is lost.
 *
 * If the level of the message was set to MESSAGE_LEVEL_OFF (usualy via
 * a command line option) then the message callback does not get called.
 */
message::~message()
{
    // emit the message if the message is available
    //
    if(message_level_t::MESSAGE_LEVEL_OFF != f_message_level
    && f_message_level >= g_minimum_message_level
    && rdbuf()->in_avail() != 0)
    {
        if(f_position.get_filename().empty())
        {
            f_position.set_filename("unknown-file");
        }
        if(f_position.get_function().empty())
        {
            f_position.set_function("unknown-func");
        }

        switch(f_message_level)
        {
        case message_level_t::MESSAGE_LEVEL_FATAL:
        case message_level_t::MESSAGE_LEVEL_ERROR:
            ++g_error_count;
            break;

        case message_level_t::MESSAGE_LEVEL_WARNING:
            ++g_warning_count;
            break;

        // others are not currently counted
        default:
            break;

        }

        if(g_message_callback == nullptr)
        {
            std::ostream & out(f_message_level >= message_level_t::MESSAGE_LEVEL_WARNING ? std::cerr : std::cout);
            out << message_level_to_string(f_message_level)
                << ':'
                << f_position
                << ':';
            if(f_error_code != err_code_t::AS_ERR_NONE)
            {
                // TODO: have a function to convert error codes to strings
                //
                out << static_cast<int>(f_error_code) << ':';
            }
            std::string const msg(str());
            out << ' ' << msg;
            if(msg.back() != '\n')
            {
                out << '\n';
            }
        }
        else
        {
            g_message_callback->output(f_message_level, f_error_code, f_position, str());
        }
    }
}


/** \brief Append an char string.
 *
 * This function appends an char string to the message.
 *
 * \param[in] s  A character string.
 *
 * \return A reference to the message.
 */
message & message::operator << (char const * s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    static_cast<std::stringstream &>(*this) << s;
    return *this;
}


message & message::operator << (wchar_t const * s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    static_cast<std::stringstream &>(*this) << libutf8::to_u8string(s);
    return *this;
}


/** \brief Append an std::string value.
 *
 * This function appends an std::string value to the message.
 *
 * \param[in] s  An std::string value.
 *
 * \return A reference to the message.
 */
message & message::operator << (std::string const & s)
{
    static_cast<std::stringstream &>(*this) << s;
    return *this;
}


/** \brief Append a char value.
 *
 * This function appends a character to the message.
 *
 * \param[in] v  A char value.
 *
 * \return A reference to the message.
 */
message & message::operator << (char const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


message & message::operator << (char32_t const v)
{
    // convert char to UTF-8 then output
    //
    static_cast<std::stringstream &>(*this) << libutf8::to_u8string(v);
    return *this;
}


/** \brief Append a signed char value.
 *
 * This function appends a signed char value to the message.
 *
 * \param[in] v  A signed char value.
 *
 * \return A reference to the message.
 */
message & message::operator << (signed char const v)
{
    static_cast<std::stringstream &>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a unsigned char value.
 *
 * This function appends a unsigned char value to the message.
 *
 * \param[in] v  A unsigned char value.
 *
 * \return A reference to the message.
 */
message & message::operator << (unsigned char const v)
{
    static_cast<std::stringstream &>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a signed short value.
 *
 * This function appends a signed short value to the message.
 *
 * \param[in] v  A signed short value.
 *
 * \return A reference to the message.
 */
message & message::operator << (signed short const v)
{
    static_cast<std::stringstream &>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a unsigned short value.
 *
 * This function appends a unsigned short value to the message.
 *
 * \param[in] v  A unsigned short value.
 *
 * \return A reference to the message.
 */
message & message::operator << (unsigned short const v)
{
    static_cast<std::stringstream &>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a signed int value.
 *
 * This function appends a signed int value to the message.
 *
 * \param[in] v  A signed int value.
 *
 * \return A reference to the message.
 */
message & message::operator << (signed int const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a unsigned int value.
 *
 * This function appends a unsigned int value to the message.
 *
 * \param[in] v  A unsigned int value.
 *
 * \return A reference to the message.
 */
message & message::operator << (unsigned int const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a signed long value.
 *
 * This function appends a signed long value to the message.
 *
 * \param[in] v  A signed long value.
 *
 * \return A reference to the message.
 */
message & message::operator << (signed long const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a unsigned long value.
 *
 * This function appends a unsigned long value to the message.
 *
 * \param[in] v  A unsigned long value.
 *
 * \return A reference to the message.
 */
message & message::operator << (unsigned long const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a signed long long value.
 *
 * This function appends a signed long long value to the message.
 *
 * \param[in] v  A signed long long value.
 *
 * \return A reference to the message.
 */
message & message::operator << (signed long long const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append an Int64 value.
 *
 * This function appends the value saved in an Int64 value.
 *
 * \param[in] v  An as2js::Int64 value.
 */
message & message::operator << (integer const v)
{
    static_cast<std::stringstream &>(*this) << v.get();
    return *this;
}


/** \brief Append a unsigned long long value.
 *
 * This function appends a unsigned long long value to the message.
 *
 * \param[in] v  A unsigned long long value.
 *
 * \return A reference to the message.
 */
message & message::operator << (unsigned long long const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a float value.
 *
 * This function appends a float value to the message.
 *
 * \param[in] v  A float value.
 *
 * \return A reference to the message.
 */
message & message::operator << (float const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a double value.
 *
 * This function appends a double value to the message.
 *
 * \param[in] v  A double value.
 *
 * \return A reference to the message.
 */
message & message::operator << (double const v)
{
    static_cast<std::stringstream &>(*this) << v;
    return *this;
}


/** \brief Append a Float64 value.
 *
 * This function appends the value saved in an Float64 value.
 *
 * \param[in] v  An as2js::floating_point value.
 *
 * \return A reference to this message.
 */
message & message::operator << (floating_point const v)
{
    static_cast<std::stringstream &>(*this) << v.get();
    return *this;
}


/** \brief Append a Boolean value.
 *
 * This function appends a Boolean value to the message as a 0 or a 1.
 *
 * \param[in] v  A Boolean value.
 *
 * \return A reference to the message.
 */
message & message::operator << (bool const v)
{
    static_cast<std::stringstream &>(*this) << static_cast<int>(v);;
    return *this;
}


/** \brief Convert the message level to a string.
 *
 * This function converts the \p level to a string that can be printed
 * when outputing a message.
 *
 * \exception out_of_range
 * If level is out of range, this exception is raised.
 *
 * \return A string representing the message level.
 */
std::string message_level_to_string(message_level_t level)
{
    if(static_cast<std::size_t>(level) >= std::size(g_message_names))
    {
        throw out_of_range("level is out of range");
    }

    return g_message_names[static_cast<int>(level)];
}


/** \brief Setup the callback so tools can receive error messages.
 *
 * This function is used by external processes to setup a callback. The
 * callback receives the message output as generated by the message
 * class.
 *
 * \sa configure()
 */
void set_message_callback(message_callback * callback)
{
    g_message_callback = callback;
}


/** \brief Define the minimum level for a message to be displayed.
 *
 * This function is used to change the minimum level at which a message
 * is output. In other words, messages with a smaller level are not sent
 * to any output.
 *
 * Note that errors and fatal errors cannot be ignored using this
 * mechanism (i.e. the largest possible value for \p min_level is
 * MESSAGE_LEVEL_ERROR).
 *
 * The default value is MESSAGE_LEVEL_INFO.
 *
 * \note
 * This value is saved in a global variable. It is common to all instances
 * of the lexer, parser, compiler.
 *
 * \param[in] min_level  The minimum level a message must have to be sent to
 * the output.
 */
void set_message_level(message_level_t min_level)
{
    g_minimum_message_level = std::min(min_level, message_level_t::MESSAGE_LEVEL_ERROR);
}


/** \brief The number of warnings that were found so far.
 *
 * This function returns the number of warnings that were
 * processed so far.
 *
 * Note that this number is a global counter and it cannot be reset.
 *
 * \return The number of warnings that were processed so far.
 */
int warning_count()
{
    return g_warning_count;
}


/** \brief The number of errors that were found so far.
 *
 * This function returns the number of errors and fatal errors that were
 * processed so far.
 *
 * Note that this number is a global counter and it cannot be reset.
 *
 * \return The number of errors that were processed so far.
 */
int error_count()
{
    return g_error_count;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
