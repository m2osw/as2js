
# as2js: AlexScript Compiler

The as2js project is a compile as well as many libraries to read and manage
AlexScript code as well as JSON data.

The first idea was to offer many extensions, such as classes, that would
allow you to program in an easy object oriented language that would then
be converted to the JavScript prototyped language. This has changed for
two reasons:

1. We want to be able to compile scripts to binary for use on our backend;
2. The JavaScript language has very much evolved and already supports many
   of the features the AlexScript supports, so it has become somewhat less
   useful in that sense; however, it is still very much capable of supporting
   many advanced features that browsers won't ever support (typing, multiple
   catch statement for one try, advance switch statements, protected/private
   members in classes, user defined operators, etc.)

I have restarted some of the work and I now have some of the AlexScript to
binary feature implemented and functional.

## AlexScript (AS) to binary

The project allows you to create binary files based on Intel (AMD64)
processors. This allows you to run native code instead of running an
interpreter. The current version has some level of optimization, but
it will need quite a bit of help to become the best compiler around.

This supports native classes and functions only at the moment. Once
time allows, I'll also implement user defined functions.

The `as2js` tool or at least `libas2js` library in your own binary
are required to run this code (it does not use the .ELF format at
this point). This can make it a lot faster since there is no need to
`fork()` to execute a new binary. Instead, it becomes part of your
binary.

## AlexScript (AS) to JavaScript (JS) transpiler

The as2js project also offers transpiler functionality. The primary one
is to compile AlexScript code to JavaScript. The AlexScript code can make
use of much advanced features such as operators in classes (like in C++)
and those are still not supported by JavaScript. This transpiler is
expected to be capable of transforming your AS code to function on any
browser.

## AlexScript (AS) to C++ transpiler

I've also been thinking that it could be useful to have a transpiler
to C++ which would allow you to compile with full optimizations in a
binary you can directly execute on your system. In this case, the
AlexScript gets translated to C++ and then compiled with g++.


# Documentation

The [ECMAScript](https://www.ecma-international.org/) documentation is the
most current source for JavaScript. The AlexScript still needs its own
documentation since it has many features that are not in JavaScript (and
at this point, vice versa).

An example of two completely different things in AlexScript:

* All lines must end with a semicolon (;), because it's otherwise nearly
  impossible for the compiler to tell you whether there is a potential
  bug or not;

* The variables can be given a type such as Integer, Double, Number,
  Boolean, String, etc. and you cannot do `a = b;` if `a` and `b` are
  not of the same or a compatible type. In other words, our variables
  are not variants.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
