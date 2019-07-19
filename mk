#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROCESSORS=4

case $1 in
"-l")
	make -C ../../../BUILD/contrib/as2js 2>&1 | less -SR
	;;

"-d")
	rm -rf ../../../BUILD/contrib/as2js/doc/libutf8-doc-1.0.tar.gz
	make -C ../../../BUILD/contrib/as2js
	;;

"-i")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/as2js install
	;;

"-t")
	(
		if make -j${PROCESSORS} -C ../../../BUILD/contrib/as2js
		then
			../../../BUILD/contrib/as2js/tests/test_as2js
		fi
	) 2>&1 | less -SR
	;;

"")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/as2js
	;;

*)
	echo "error: unknown command line option \"$1\""
	;;

esac
