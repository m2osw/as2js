#!/bin/sh -e
#
# Script to compile some .ajs files to binary to verify that it works as
# expected.

OPTIONS=""
COMPILER=false
DISASSEMBLE=false
PARSER=false
GDB=false
while test -n "$1"
do
    case "$1" in
    "--compiler"|"-T")
        # Test the compiler tree only
        COMPILER=true
        shift
        ;;

    "--debug"|"-d")
        OPTIONS="${OPTIONS} --log-level DEBUG"
        shift
        ;;

    "--disassemble"|"-S")
        DISASSEMBLE=true
        shift
        ;;

    "--gdb"|"-g")
        GDB=true
        shift
        ;;

    "--help"|"-h")
        echo "Usage: `basename $0` [-opts]"
        echo "where -opts is one or more of:"
        echo "  -d | --debug        set log level to debug"
        echo "  -g | --gdb          run using gdb"
        echo "  -h | --help         print out this help screen"
        echo "  -T | --compiler     run the compiler and show the results"
        echo "  -t | --parser       run the parser and show the results"
        echo "       --trace        set log level to trace"
        exit 1
        ;;

    "--parser"|"-t")
        # Test the parser tree only
        PARSER=true
        shift
        ;;

    "--trace")
        OPTIONS="${OPTIONS} --log-level trace"
        shift
        ;;

    *)
        echo "error: unknown command line option \"$1\"."
        exit 1
        ;;

    esac
done

if $COMPILER && $PARSER
then
    echo "error: the --compiler and --parser options are exclusive."
    exit 1
fi

if $COMPILER
then
    OPTIONS="${OPTIONS} -T"
elif $PARSER
then
    OPTIONS="${OPTIONS} -t"
else
    OPTIONS="${OPTIONS} -b"
fi

# Make sure code is up to date
#
./mk

# Expected path to compiler from source directory
#
AS2JS=../../BUILD/Debug/contrib/as2js/tools/as2js

# Location of as2js.rc file
#
export AS2JS_RC="`pwd`/conf"

OPTIONS="-L ../../BUILD/Debug/contrib/as2js/rt ${OPTIONS}"

# compile simple test
#
echo
echo "--- run as2js compiler ---"
echo
if ${GDB}
then
    gdb -ex 'run' \
        --args ${AS2JS} ${OPTIONS} tests/binary/simple.ajs
else
    ${AS2JS} ${OPTIONS} tests/binary/simple.ajs

    if ${DISASSEMBLE}
    then
        TEXT_START=`${AS2JS} --text-section`
        DATA_START=`${AS2JS} --data-section`
        TEXT_SIZE=`expr ${DATA_START} - ${TEXT_START}`

        # remove header
        dd ibs=1 skip=${TEXT_START} count=${TEXT_SIZE} if=a.out of=b.out >/dev/null

        # disassemble
        objdump -b binary -m i386:x86-64 -D b.out
    fi
fi


# vim: ts=4 sw=4 et
