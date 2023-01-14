// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
// 
// https://snapwebsites.org/project/as2js
// 
// Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and
// associated documentation files (the "Software"), to
// deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
// 
// The above copyright notice and this permission notice
// shall be included in all copies or substantial
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

native package Native
{


class BigInt extends Object
{
    use extended_operators(2);

    function BigInt(var in value: BigInt := 0);

    static function asIntN(var in bits: Number, var in bigint: BigInt) : BigInt;
    static function asUintN(var in bits: Number, var in bigint: BigInt) : BigInt;

    function toString(var in radix: Number := 10) : String;
    function toLocaleString(Void) : String;
    function valueOf(Void) : BigInt;

    // unary operators
    //
    function +   (Void) : BigInt;
    function -   (Void) : BigInt;
    function ++  (Void) : BigInt; // pre
    function --  (Void) : BigInt; // pre
    function ++  (var in value: Object) : BigInt; // post
    function --  (var in value: Object) : BigInt; // post
    function ~   (Void) : BigInt;
    function !   (Void) : Boolean;

    // binary operator
    function **  (var in value: Number) : BigInt;

    function ~=  (var in value: RegExp) : Boolean;
    function ~!  (var in value: RegExp) : Boolean;

    function *   (var in value: BigInt) : BigInt;
    function /   (var in value: BigInt) : BigInt;
    function %   (var in value: BigInt) : BigInt;

    function +   (var in value: BigInt) : BigInt;
    function -   (var in value: BigInt) : BigInt;

    function <<  (var in value: Number) : BigInt;
    function >>  (var in value: Number) : BigInt;
    function >>> (var in value: Number) : BigInt;
    function <%  (var in value: Number) : BigInt;
    function >%  (var in value: Number) : BigInt;

    function <   (var in value: BigInt) : Boolean;
    function >   (var in value: BigInt) : Boolean;
    function <=  (var in value: BigInt) : Boolean;
    function >=  (var in value: BigInt) : Boolean;

    function ==  (var in value: BigInt) : Boolean;
    function !=  (var in value: BigInt) : Boolean;
    function === (var in value: BigInt) : Boolean;
    function !== (var in value: BigInt) : Boolean;
    function <=> (var in value: BigInt) : Object.CompareResult;
    function ~~  (var in value: BigInt) : Boolean;

    function &   (var in value: BigInt) : BigInt;

    function ^   (var in value: BigInt) : BigInt;

    function |   (var in value: BigInt) : BigInt;

    function &&  (var in value: BigInt) : Boolean;

    function ^^  (var in value: BigInt) : Boolean;

    function ||  (var in value: BigInt) : Boolean;

    function <?  (var in value: BigInt) : BigInt;
    function >?  (var in value: BigInt) : BigInt;

    // assignment operators
    function :=  (var in value: BigInt) : BigInt;

    function **= (var in value: BigInt) : BigInt;

    function *=  (var in value: BigInt) : BigInt;
    function /=  (var in value: BigInt) : BigInt;
    function %=  (var in value: BigInt) : BigInt;

    function +=  (var in value: BigInt) : BigInt;
    function -=  (var in value: BigInt) : BigInt;

    function <<= (var in value: Number) : BigInt;
    function >>= (var in value: Number) : BigInt;
    function >>>=(var in value: Number) : BigInt;
    function <%= (var in value: Number) : BigInt;
    function >%= (var in value: Number) : BigInt;

    function &=  (var in value: BigInt) : BigInt;

    function ^=  (var in value: BigInt) : BigInt;

    function |=  (var in value: BigInt) : BigInt;

    function &&= (var in value: BigInt) : Boolean;

    function ^^= (var in value: BigInt) : Boolean;

    function ||= (var in value: BigInt) : Boolean;

    function <?= (var in value: BigInt) : BigInt;
    function >?= (var in value: BigInt) : BigInt;

    var length: Number;
};

}

// vim: ts=4 sw=4 et
