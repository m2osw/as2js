#ifndef TEST_AS2JS_MAIN_H
#define TEST_AS2JS_MAIN_H
/* tests/test_as2js_main.h

Copyright (c) 2005-2019  Made to Order Software Corp.  All Rights Reserved

https://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include    <string>
#include    <cstring>
#include    <cstdlib>
#include    <cmath>
#include    <limits>


namespace as2js_test
{

extern  std::string     g_tmp_dir;
extern  std::string     g_as2js_compiler;
extern  bool            g_gui;
extern  bool            g_run_stdout_destructive;
extern  bool            g_save_parser_tests;


class obj_setenv
{
public:
    obj_setenv(const std::string& var)
        : f_copy(strdup(var.c_str()))
    {
        putenv(f_copy);
        std::string::size_type p(var.find_first_of('='));
        f_name = var.substr(0, p);
    }

    obj_setenv(obj_setenv const & rhs) = delete;

    ~obj_setenv()
    {
        putenv(strdup((f_name + "=").c_str()));
        free(f_copy);
    }

    obj_setenv & operator = (obj_setenv const & rhs) = delete;

private:
    char *      f_copy = nullptr;
    std::string f_name = std::string();
};

}
// namespace as2js_test
#endif
// #ifdef TEST_AS2JS_MAIN_H

// vim: ts=4 sw=4 et
