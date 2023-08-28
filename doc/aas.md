
# Alex Assembler

The `aas` tool is an idea of an x86 assembler which is (one day) used by
the as2js compiler. The idea is to have a library instead of a command
line tool.

The assembler is very basic, but still supports expressions, comments,
offset computations (for relocations or relative), etc.

# Supported Syntax

    start:
    	program

    program:
        line EOL
      | program line EOL

    line:
        comment
      | label
      | instruction

    comment:
        '#' WHATEVER              // add support for filename/line number (preprocessor output)
      | '/*' (WHATEVER - '*/' | EOL)* '*/'
      | '//' WHATEVER

    label:
        IDENTIFIER ':'
      | IDENTIFIER ':' instruction

    instruction:
        WHITESPACE instruction_name
      | WHITESPACE instruction_name addressing

    instruction_name:
        IDENTIFIER
      | instruction_name IDENTIFIER

    adressing:
        register
      | register ',' segment_address
      | segment_address ',' register
      | segment_address
      | segment_address ',' segment_address
      | '*' segment_address // for JMP/CALL
      | '$' expression
      | IDENTIFIER          // name of label

    register:
        '%' IDENTIFIER      // al, ah, ax, eax, rax, etc., r0 to r15, sized (i.e. r1b/r1h, r1w, r1d, r1) except for rip/rsp, mm, xmm, ymm, zmm, eflags, st0 to st7, cr, gdtr, ldtr, idtr, dr, msr

    segment:
        '%' IDENTIFIER      // cs, ss, ds, es, fs, gs

    segment_address:
        address
      | segment ':' address

    address:
        '(' rm ')'
      | expression
      | expression '(' rm ')'

    rm:
        register
      | register ',' register
      | register ',' register ',' expression
      | ',' register ',' expression
      | ',' expression          // disp(,1) to only have a displacement

    expression:
        conditional

    conditional:
        logical_or
      | logical_or '?' expression ':' expression

    logical_or:
        logical_and
      | logical_and '||' logical_or

    logical_and:
        bitwise_or
      | bitwise_or '&&' logical_and

    bitwise_or:
        bitwise_xor
      | bitwise_xor '|' bitwise_or

    bitwise_xor:
        bitwise_and
      | bitwise_and '^' bitwise_xor

    bitwise_and:
        equality
      | equality '&' bitwise_and

    equality:
        relational
      | relational '==' equality
      | relational '!=' equality

    relational:
        shift
      | shift '<' relational
      | shift '<=' relational
      | shift '>' relational
      | shift '>=' relational

    shift:
        shift
      | shift '<<' additive
      | shift '>>' additive
      | shift '>>>' additive

    additive:
        multiplicative
      | multiplicative '+' additive
      | multiplicative '-' additive

    multiplicative:
        primitive
      | primitive '*' multiplicative
      | primitive '/' multiplicative
      | primitive '%' multiplicative

    primitive:
        '-' expression
      | '+' expression
      | '!' expression
      | '~' expression
      | '(' expression ')'
      | 'sizeof' expression
      | INTEGER
      | FLOATING_POINT
      | STRING
      | IDENTIFIER
      -- | IDENTIFIER '(' arguments ')'   // function call i.e. sin(PI) -- not possible if addressing supports 'disp32(rax)'
      | postfix

    postfix:
        IDENTIFIER
      | postfix '.' IDENTIFIER

    arguments:
        expression
      | arguments ',' expression

vim: ts=4 sw=4 et
