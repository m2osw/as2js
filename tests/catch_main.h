// Copyright (c) 2011-2023  Made to Order Software Corp.  All Rights Reserved
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


// catch2
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include    <catch2/snapcatch2.hpp>
#pragma GCC diagnostic pop


// as2js
//
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/options.h>


// C++
//
#include    <string>
#include    <cstring>
#include    <cstdlib>
#include    <cmath>
#include    <limits>


namespace SNAP_CATCH2_NAMESPACE
{

extern  std::string         g_as2js_compiler;
extern  bool                g_run_destructive;
extern  bool                g_save_parser_tests;


extern int                  catch_rc_init();
extern int                  catch_db_init();
extern int                  catch_compiler_init();
extern void                 catch_compiler_cleanup();

extern as2js::err_code_t    str_to_error_code(std::string const & error_name);
extern char const *         error_code_to_str(as2js::err_code_t const error_code);
extern void                 verify_result(
                                      std::string const & result_name
                                    , as2js::json::json_value::pointer_t expected
                                    , as2js::node::pointer_t node
                                    , bool verbose
                                    , bool ignore_children);
extern void                 verify_parser_result(
                                      std::string const & result_name
                                    , as2js::json::json_value::pointer_t expected
                                    , as2js::node::pointer_t node
                                    , bool verbose
                                    , bool ignore_children);


struct named_options
{
    as2js::options::option_t    f_option;
    char const *                f_name;
    char const *                f_neg_name;
    int                         f_value;
};

extern named_options const  g_options[];

extern std::size_t const    g_options_size;


// class used to capture errors and verify they match what's expected
// (since this is a test, everything is public)
//
class test_callback
    : public as2js::message_callback
{
public:
                    test_callback(bool verbose, bool parser = false);
                    ~test_callback();

    static void     fix_counters();
    virtual void    output(
                          as2js::message_level_t message_level
                        , as2js::err_code_t error_code
                        , as2js::position const & pos
                        , std::string const & message);
    void            got_called();

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::position             f_pos = as2js::position();
        std::string                 f_message = std::string(); // UTF-8 string
    };

    std::vector<expected_t>     f_expected = std::vector<expected_t>();
    std::uint16_t               f_position = 0;
    bool                        f_verbose = false;
    bool                        f_parser = false;

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};




// use snapdev::transparent_setenv instead (same functionality)
//class obj_setenv
//{
//public:
//    obj_setenv(const std::string& var)
//        : f_copy(strdup(var.c_str()))
//    {
//        putenv(f_copy);
//        std::string::size_type p(var.find_first_of('='));
//        f_name = var.substr(0, p);
//    }
//
//    obj_setenv(obj_setenv const & rhs) = delete;
//
//    ~obj_setenv()
//    {
//        putenv(strdup((f_name + "=").c_str()));
//        free(f_copy);
//    }
//
//    obj_setenv & operator = (obj_setenv const & rhs) = delete;
//
//private:
//    char *      f_copy = nullptr;
//    std::string f_name = std::string();
//};

}
// namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
