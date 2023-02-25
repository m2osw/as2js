#!/bin/sh -e
#
# Script to compile some .ajs files to binary to verify that it works as
# expected.

OPTIONS=""
COMPILER=false
PARSER=false
while test -n "$1"
do
    case "$1" in
    "--compiler"|"-T")
        # Test the compiler tree only
        COMPILER=true
        shift
        ;;

    "--help"|"-h")
        echo "Usage: `basename $0` [-opts]"
        echo "where -opts is one or more of:"
        echo "  -T | --compiler   run the compiler and show the results"
        echo "  -h | --help       print out this help screen"
        echo "  -t | --parser     run the parser and show the results"
        exit 1
        ;;

    "--parser"|"-t")
        # Test the parser tree only
        PARSER=true
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
COMPILER=../../BUILD/Debug/contrib/as2js/tools/as2js

# Location of as2js.rc file
#
export AS2JS_RC="`pwd`/conf"

# compile simple test
#
echo
echo "--- run as2js compiler ---"
echo
${COMPILER} ${OPTIONS} tests/binary/simple.ajs


# vim: ts=4 sw=4 et
