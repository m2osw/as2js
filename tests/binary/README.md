
# Mini-scripts

These minimal scripts are used to run various expressions to make sure that
they work as expected. The Metadata file is used to specify the input and
verify the output.

# Metadata

The .ajs is accompagned by a .meta file which defines the input and
output for the mini-script. The syntax is as follow:

    start: lines

    lines: line
         | lines line

    line: var '\n'
        | type '(' output ')' '\n'
        | '#' anything '\n'       // comment

    var: type name '=' value '\n'

    type: <empty>
        | 'integer'
        | 'boolean'
        | 'double'
        | 'string'
        | 'out'
        | type type

    name: identifier

    // note: anything except ')'
    output: anything

    value: <empty>
         | anything

    // note: identifiers should not start with a digit
    identifier: [a-z]
              | [A-Z]
              | [0-9]
              | '_'
              | identifier identifier

    anything: (any character except '\n')

-- vim: ts=4 sw=4 et
