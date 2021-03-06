#ifndef TEST_AS2JS_LEXER_H
#define TEST_AS2JS_LEXER_H
/* tests/test_as2js_lexer.h

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


class As2JsLexerUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsLexerUnitTests );
        CPPUNIT_TEST( test_invalid_pointers );
        CPPUNIT_TEST( test_tokens );
        CPPUNIT_TEST( test_valid_strings );
        CPPUNIT_TEST( test_invalid_strings );
        CPPUNIT_TEST( test_invalid_numbers );
        CPPUNIT_TEST( test_identifiers );
        CPPUNIT_TEST( test_invalid_input );
        CPPUNIT_TEST( test_mixed_tokens );
    CPPUNIT_TEST_SUITE_END();

public:
    //void setUp();

protected:
    void test_invalid_pointers();
    void test_tokens();
    void test_valid_strings();
    void test_invalid_strings();
    void test_invalid_numbers();
    void test_identifiers();
    void test_invalid_input();
    void test_mixed_tokens();
};

#endif
// vim: ts=4 sw=4 et
