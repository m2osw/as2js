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

    "--execute"|"-x")
        EXECUTE=true
        EXECUTE_WITH_GDB=false
        GDB_AUTO_RUN=false
        shift
        ;;

    "--gdb"|"-g")
        GDB=true
        shift
        ;;

    "--gdb-execute"|"-X")
        EXECUTE=true
        EXECUTE_WITH_GDB=true
        GDB_AUTO_RUN=false
        shift
        ;;

    "--gdb-run"|"-R")
        EXECUTE=true
        EXECUTE_WITH_GDB=true
        GDB_AUTO_RUN=true
        shift
        ;;

    "--help"|"-h")
        echo "Usage: `basename $0` [-opts] [<script>]"
        echo "where -opts is one or more of:"
        echo "  -d | --debug        set log level to debug"
        echo "  -g | --gdb          run using gdb"
        echo "  -h | --help         print out this help screen"
        echo "  -R | --gdb-run      execute the resulting code in gdb with auto-run"
        echo "  -S | --disassemble  run the compiler and show the results"
        echo "  -T | --compiler     run the compiler and show the results"
        echo "  -t | --parser       run the parser and show the results"
        echo "  -V | --show-variables  list external variables"
        echo "  -x | --execute      execute the resulting code"
        echo "  -X | --gdb-execute  execute the resulting code in gdb"
        echo "       --trace        set log level to trace"
        echo
        echo "The <script> name can be specified. The default is \"simple\"."
        exit 1
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
        OPTIONS="${OPTIONS} --log-level trace"
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
if ${GDB}
then
    gdb -ex 'run' \
        --args ${AS2JS} ${OPTIONS} ${COMMAND} tests/binary/${SCRIPT}.ajs
else
    ${AS2JS} ${OPTIONS} ${COMMAND} tests/binary/${SCRIPT}.ajs

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
                    --args ${AS2JS} ${OPTIONS} --execute \
                        x=100 y=7 \
                        'sx="small"' 'sy="and a long string"'
            else
                gdb --args ${AS2JS} ${OPTIONS} --execute \
                    x=100 y=7 \
                    'sx="small"' 'sy="and a long string"'
            fi
        else
            ${AS2JS} ${OPTIONS} --execute \
                x=100 y=7 \
                sx="small" sy="and a long string"
        fi
    fi
fi


# vim: ts=4 sw=4 et
