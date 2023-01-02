/* scripts/native/object.js

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

// global function are attached to the Global

function eval(var in script: String) : Object;
function isFinite(var in value: Number) : Boolean;
function isNaN(var in value: Number) : Boolean;
function parseFloat(var in s: String) : Number;
function parseInt(var in s: String, var in radix: Number) : Number;
function Encode(var in uri: String, var in unescapedSet: String) : String;
function Decode(var in uri: String, var in reservedSet: String) : String;
function decodeURI(var in encodedURI: String) : String;
function decodeURIComponent(var in encodedURIComponent: String) : String;
function encodeURI(var in uri: String) : String;
function encodeURIComponent(var in uriComponent: String) : String;

// snap specific
static function getAs2JsVersion(Void) : String;

}

// vim: ts=4 sw=4 et
