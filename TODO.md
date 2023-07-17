
# TODO

. think about making the library thread safe (i.e. we have globals that would
  benefit from a mutex).
. JSON only accepts " and not ' for strings
. JSON only accepts \n and \r as line terminators
. The parser takes the 'use' definition in a declaration such as
  `class ... { use extended_operators(2); ... };` as a global
  definition instead of only applying it to the current scope.
. The use of `super.<name>()` should be using the `<name>` of the
  current function 99.9% of the time; we should have a warning
  if the user did not do that.
. Implement a `retry` which restarts a loop from the start.
. Implement a `redo` to repeat the loop with the current iterator.
. For the docs, use rr to generate grammar railroads
  https://github.com/GuntherRademacher/rr
. Implement a trace mode (a mode which prints the next statement before
  executing it); more or less, for a browser, it means adding this line:
     console.output("<statement>;");
  before each statement. That way we can see what we are doing (a bit like
  debugger would do showing PC in the source code)
. enum { A, B, C }; -- A and B will automatically be marked as "IN USE"
  because we do B = A + 1 and C = B + 1 -- these are internal computation
  and thus these should not mark the values as "IN USE" at that point

