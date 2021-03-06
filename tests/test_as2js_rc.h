#ifndef TEST_AS2JS_RC_H
#define TEST_AS2JS_RC_H
/* tests/test_as2js_rc.h

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


#include <cppunit/extensions/HelperMacros.h>


class As2JsRCUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsRCUnitTests );
        CPPUNIT_TEST( test_basics );
        CPPUNIT_TEST( test_load_from_var );
        CPPUNIT_TEST( test_load_from_local );
        CPPUNIT_TEST( test_load_from_user_config );
        CPPUNIT_TEST( test_load_from_system_config );
        CPPUNIT_TEST( test_empty_home );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void test_basics();
    void test_load_from_var();
    void test_load_from_local();
    void test_load_from_user_config();
    void test_load_from_system_config();
    void test_empty_home();
};

#endif
// vim: ts=4 sw=4 et
