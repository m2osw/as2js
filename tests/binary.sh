#!/bin/sh -e
#
# Script to compile some .ajs files to binary to verify that it works as
# expected.

OPTIONS="--ignore-unknown-variables"
COMPILER=false
DISASSEMBLE=false
EXECUTE=false
EXECUTE_WITH_GDB=false
PARSER=false
SHOW_VARIABLES=false
GDB=false
GDB_AUTO_RUN=true
SCRIPT=
while test -n "$1"
do
    case "$1" in
    "--compiler"|"-T")
        shift
        # Test the compiler tree only
        COMPILER=true
        ;;

    "--debug"|"-d")
        shift
        OPTIONS="${OPTIONS} --log-level DEBUG"
        ;;

    "--show-all-results")
        shift
        OPTIONS="${OPTIONS} --show-all-results"
        ;;

    "--disassemble"|"-S")
        shift
        DISASSEMBLE=true
        ;;

    "--execute"|"-x")
        shift
        EXECUTE=true
        EXECUTE_WITH_GDB=false
        GDB_AUTO_RUN=false
        ;;

    "--floating-point"|"double")
        shift
        OPTIONS="${OPTIONS} x:double=405 y:double=-172 z:double=331 w:double=0 n:double=-0.0"
        ;;

    "--gdb"|"-g")
        shift
        GDB=true
        ;;

    "--gdb-execute"|"-X")
        shift
        EXECUTE=true
        EXECUTE_WITH_GDB=true
        GDB_AUTO_RUN=false
        ;;

    "--gdb-run"|"-R")
        shift
        EXECUTE=true
        EXECUTE_WITH_GDB=true
        GDB_AUTO_RUN=true
        ;;

    "--help"|"-h")
        echo "Usage: `basename $0` [-opts] [<script>]"
        echo "where -opts is one or more of:"
        echo "  -d | --debug        set log level to debug"
        echo "       --floating-point  pass floating points to as2js"
        echo "  -g | --gdb          run using gdb"
        echo "  -h | --help         print out this help screen"
        echo "       --integer      pass integers to as2js"
        echo "  -R | --gdb-run      execute the resulting code in gdb with auto-run"
        echo "  -S | --disassemble  run the compiler and show the results"
        echo "       --string       pass strings to as2js"
        echo "  -T | --compiler     run the compiler and show the results"
        echo "       --trace        set log level to \"trace\""
        echo "  -t | --parser       run the parser and show the results"
        echo "  -V | --show-variables  list external variables"
        echo "  -x | --execute      execute the resulting code"
        echo "  -X | --gdb-execute  execute the resulting code in gdb"
        echo "       --trace        set log level to trace"
        echo
        echo "The <script> name can be specified. The default is \"simple\"."
        exit 1
        ;;

    "--integer")
        shift
        OPTIONS="${OPTIONS} x=100 y=7"
        ;;

    "--parser"|"-t")
        # Test the parser tree only
        PARSER=true
        shift
        ;;

    "--show-variables"|"-V")
        SHOW_VARIABLES=true
        shift
        ;;

    "--trace")
        shift
        OPTIONS="${OPTIONS} --log-level trace"
        ;;

    "--string")
        shift
        OPTIONS="${OPTIONS} --three-underscores-to-space sx=small sy=and___a___long___string sz=another___string"
        ;;

    "--var")
        shift
        OPTIONS="${OPTIONS} $1=$2"
        shift
        shift
        ;;

    "-"*)
        echo "error: unknown command line option \"$1\"."
        exit 1
        ;;

    *)
        if test -z "${SCRIPT}"
        then
            SCRIPT=$1
            shift
        else
            echo "error: unsupported additional filenames \"$1\" (already defined \"${SCRIPT}\"."
            exit 1
        fi
        ;;

    esac
done

if $COMPILER && $PARSER
then
    echo "error: the --compiler and --parser options are exclusive."
    exit 1
fi

COMMAND="<undefined>"
if ${COMPILER}
then
    COMMAND="-T"
elif ${PARSER}
then
    COMMAND="-t"
else
    COMMAND="-b"
fi

if test -z "${SCRIPT}"
then
    SCRIPT=simple
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

# compile test script
#
echo
echo "--- run as2js compiler ---"
echo

if test ! -f ${SCRIPT}
then
    if test ! -f tests/binary/${SCRIPT}.ajs
    then
        echo "error: script ${SCRIPT} not found."
        exit 1
    fi
    SCRIPT=tests/binary/${SCRIPT}.ajs
fi

if ${GDB}
then
    gdb -ex 'run' \
        --args ${AS2JS} ${OPTIONS} ${COMMAND} ${SCRIPT}
else
    ${AS2JS} ${OPTIONS} ${COMMAND} ${SCRIPT}

    if ${DISASSEMBLE}
    then
        echo
        echo "--- disassemble output ---"
        echo
        TEXT_START=`${AS2JS} --text-section`
        DATA_START=`${AS2JS} --data-section`
        TEXT_SIZE=`expr ${DATA_START} - ${TEXT_START}`

        # remove header
        dd ibs=1 skip=${TEXT_START} count=${TEXT_SIZE} if=a.out of=b.out 2>/dev/null

        # disassemble
        objdump -b binary -m i386:x86-64 -D b.out
    fi

    if ${SHOW_VARIABLES}
    then
        ${AS2JS} --variables
    fi

    if ${EXECUTE}
    then
        echo
        echo "--- execute script from binary ---"
        echo
        if ${EXECUTE_WITH_GDB}
        then
            if ${GDB_AUTO_RUN}
            then
                gdb -ex 'run' \
                    --args ${AS2JS} ${OPTIONS} --execute
            else
                gdb --args ${AS2JS} ${OPTIONS} --execute
            fi
        else
            ${AS2JS} ${OPTIONS} --execute
        fi
    fi
fi


# vim: ts=4 sw=4 et
