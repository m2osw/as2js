/* script/extensions/complex.js

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


package Extension
{
	class Complex
	{
		function Complex(Void) : Void
		{
			a = 0.0;
			b = 0.0;
		}
		function Complex(real : Double) : Void
		{
			a = real;
			b = 0;
		}
		function Complex(real : Double, img : Double) : Void
		{
			a = real;
			b = img;
		}

// operator overloading
		function "+" (Void) : Complex
		{
			return this;
		}
		function "-" (Void) : Complex
		{
			a = -a;
			b = -b;
			return this;
		}
		function "+=" (c : Complex) : Complex
		{
			a += c.a;
			b += c.b;
			return this;
		}
		function "+" (c : Complex) : Complex
		{
			return Complex(a + c.a, b + c.b);
		}
		function "+=" (c : Double) : Complex
		{
			a += c;
			return this;
		}
		function "+" (c : Double) : Complex
		{
			return Complex(a + c, b);
		}
		function "-=" (c : Complex) : Complex
		{
			a -= c.a;
			b -= c.b;
			return this;
		}
		function "-" (c : Complex) : Complex
		{
			return Complex(a - c.a, b - c.b);
		}
		function "-=" (c : Double) : Complex
		{
			a -= c;
			b -= c;
			return this;
		}
		function "-" (c : Double) : Complex
		{
			return Complex(a - c, b - c);
		}
		function "*=" (c : Complex) : Complex
		{
			var ta : Double, tb : Double;
			ta = a * c.a - b * c.b;
			tb = a * c.b + b * c.a;
			a = ta;
			b = tb;
			return this;
		}
		function "*" (c : Complex) : Complex
		{
			return Complex(a * c.a - b * c.b, a * c.b + b * c.a);
		}
		function "*=" (c : Double) : Complex
		{
			a *= c;
			b *= c;
			return this;
		}
		function "*" (c : Double) : Complex
		{
			return Complex(a * c, b * c);
		}
		function "/=" (c : Complex) : Complex
		{
			var sqr : Double;
			sqr = c.a ** 2 + c.b ** 2;
			a /= sqr;
			b /= sqr;
			return this;
		}
		function "/" (c : Complex) : Complex
		{
			var sqr : Double;
			sqr = c.a ** 2 + c.b ** 2;
			return Complex(a / sqr, b / sqr);
		}
		function "/=" (c : Double) : Complex
		{
			a /= c;
			b /= c;
			return this;
		}
		function "/" (c : Double) : Complex
		{
			return Complex(a / c, b / c);
		}

// comparison
		function "==" (c : Complex) : Complex
		{
			return a == c.a && b == c.b;
		}
		function "==" (c : Double) : Complex
		{
			return a == c && b == 0.0;
		}
		function "===" (c : Complex) : Complex
		{
			return a === c.a && b === c.b;
		}
		function "===" (c : Double) : Complex
		{
			return a === c && b === 0.0;
		}
		function "!=" (c : Complex) : Complex
		{
			return a != c.a || b != c.b;
		}
		function "!=" (c : Double) : Complex
		{
			return a != c || b != 0.0;
		}
		function "!==" (c : Complex) : Complex
		{
			return a !== c.a || b !== c.b;
		}
		function "!==" (c : Double) : Complex
		{
			return a !== c || b !== 0.0;
		}

// transcendentals
		function abs(Void) : Double
		{
			var m : Double = max(Math.abs(a), Math.abs(b));
			if(m == 0) {
				return 0
			}
			var ra : Double = a / m;
			var rb : Double = b / m;
			return m * Math.sqrt(ra ** 2 + rb ** 2);
		}
		function arg(Void) : Double
		{
			return Math.atan2(b, a);
		}
		function conj(Void) : Complex
		{
			return Complex(a, -b);
		}
		function cos(Void) : Complex
		{
			return Complex(Math.cos(a) * Math.cosh(b), -Math.sin(a) * Math.sinh(b));
		}
		function cosh(Void) : Complex
		{
			return Complex(Math.cosh(a) * Math.cos(b), Math.sinh(a) * Math.sin(b));
		}
		function exp(Void) : Complex
		{
			return polar(Math.exp(a), b);
		}
		function imag(Void) : Double
		{
			return b;
		}
		function log(Void) : Complex
		{
			return Complex(Math.log(abs()), arg());
		}
		function log10(Void) : Complex
		{
			return Complex(log() / log(Complex(10)));
		}
		function norm(Void) : Double
		{
			//return a ** 2 + b ** 2;
			return abs() ** 2;
		}
		static function polar(rho : Double, theta : Double)
		{
			return Complex(rho * Math.cos(theta), rho * Math.sin(theta));
		}
		function pow(n : Double) : Complex
		{
			return exp(n * log());
		}
		function pow(c : Complex) : Complex
		{
			return exp(c * log());
		}
		function real(Void) : Double
		{
			return a;
		}
		function sin(Void) : Complex
		{
			return Complex(Math.sin(a) * Math.cosh(b), Math.cos(a) * Math.sinh(b));
		}
		function sinh(Void) : Complex
		{
			return Complex(Math.sinh(a) * Math.cos(b), Math.cosh(a) * Math.sin(b));
		}
		function sqrt(Void) : Complex
		{
			if(a == 0) {
				var t = sqrt(Math.abs(b) / 2);
				return Complex(t, b < 0 ? -t : t);
			}
			else {
				var t = Math.sqrt(2 * (abs() + Math.abs(a)));
				var u = t / 2;
				return x > 0
					? Complex(u, b / t)
					: Complex(Math.abs(b) / t, y < 0 ? -u : u);
			}
		}
		function tan(Void) : Complex
		{
			var s : Complex = sin();
			var c : Complex = cos();
			return s / c;
		}
		function tanh(Void) : Complex
		{
			var s : Complex = sinh();
			var c : Complex = cosh();
			return s / c;
		}

// misc.
		function toString(Void) : String
		{
			return a.toString() + (b == 0 ? "" : " + " + b.toString() + "i");
		}

		var a : Double;
		var b : Double;
	}
}

