#ifndef TEST_AS2JS_COMPILER_H
#define TEST_AS2JS_COMPILER_H
/* tests/test_as2js_compiler.h

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


class As2JsCompilerUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsCompilerUnitTests );
        CPPUNIT_TEST( test_compiler_invalid_nodes );
        CPPUNIT_TEST( test_compiler_class );
        CPPUNIT_TEST( test_compiler_enum );
        CPPUNIT_TEST( test_compiler_expression );
        //CPPUNIT_TEST( test_compiler_compare );
        //CPPUNIT_TEST( test_compiler_conditional );
        //CPPUNIT_TEST( test_compiler_equality );
        //CPPUNIT_TEST( test_compiler_logical );
        //CPPUNIT_TEST( test_compiler_match );
        //CPPUNIT_TEST( test_compiler_multiplicative );
        //CPPUNIT_TEST( test_compiler_relational );
        //CPPUNIT_TEST( test_compiler_statements );
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

protected:
    void test_compiler_invalid_nodes();

    void test_compiler_class();
    void test_compiler_enum();
    void test_compiler_expression();
    //void test_compiler_compare();
    //void test_compiler_conditional();
    //void test_compiler_equality();
    //void test_compiler_logical();
    //void test_compiler_match();
    //void test_compiler_multiplicative();
    //void test_compiler_relational();
    //void test_compiler_statements();
};

#endif
// vim: ts=4 sw=4 et
