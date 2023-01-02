/* scripts/native/math.js

Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved

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

native package Native
{

class Math extends Object
{
    // the Math object cannot be instantiated
    private function Math(var in m: Math := undefined) : Math;

    static function abs(var in x: Number) : Number;
    static function acos(var in x: Number) : Number;
    static function acosh(var in x: Number) : Number;
    static function asin(var in x: Number) : Number;
    static function asinh(var in x: Number) : Number;
    static function atan(var in x: Number) : Number;
    static function atanh(var in x: Number) : Number;
    static function atan2(var in y: Number, var in x: Number) : Number;
    static function cbrt(var in x: Number) : Number;
    static function ceil(var in x: Number) : Number;
    static function clz32(var in x: Number) : Number;
    static function cos(var in x: Number) : Number;
    static function cosh(var in x: Number) : Number;
    static function exp(var in x: Number) : Number;
    static function expm1(var in x: Number) : Number;
    static function floor(var in x: Number) : Number;
    static function fround(var in x: Number) : Number;
    static function hypot(var in ... x: Number) : Number;
    static function imul(var in x: Number, var in y: Number) : Number;
    static function log(var in x: Number) : Number;
    static function log1p(var in x: Number) : Number;
    static function log10(var in x: Number) : Number;
    static function log2(var in x: Number) : Number;
    static function max(var in ... x: Number) : Number;
    static function min(var in ... x: Number) : Number;
    static function pow(var in base: Number, var in exponent: Number) : Number;
    static function random() : Number;
    static function round(var in x: Number) : Number;
    static function sin(var in x: Number) : Number;
    static function sinh(var in x: Number) : Number;
    static function sqrt(var in x: Number) : Number;
    static function tan(var in x: Number) : Number;
    static function tanh(var in x: Number) : Number;
    static function trunc(var in x: Number) : Number;

    const var E := 2.718281828459045235360287471352662498;
    const var LN10 := 2.302585092994045684017991454684364208;
    const var LN2 := 0.693147180559945309417232121458176568;
    const var LOG2E := 1.442695040888963407359924681001892137;
    const var LOG10E := 0.434294481903251827651128918916605082;
    const var PI := 3.141592653589793238462643383279502884;
    const var SQRT1_2 := 0.707106781186547524400844362104849039;
    const var SQRT2 := 1.414213562373095048801688724209698079;
};


}

// vim: ts=4 sw=4 et
