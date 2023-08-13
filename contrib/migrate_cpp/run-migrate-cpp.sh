#!/bin/bash -e

. contrib/migrate_cpp/setup.sh

if test ! -d "${MIGRATE_CPP_PATH}"
then
	echo "error: invalid path to migrate_cpp in \"${MIGRATE_CPP_PATH}\"."
	exit 1
fi
cd "${MIGRATE_CPP_PATH}"

DELETE=0
while test -n "$1"
do
	case "$1" in
	"--delete")
		shift
		DELETE=1
		;;

	"--help"|"-h")
		echo "Usage: `basename $0` [--opts]"
		echo "By default, this command tries to START the migration tests."
		echo "Where --opts is one or more of:"
		echo "  --delete      try to delete all the projects."
		echo "  --help | -h   print out this help screen."
		echo
		echo "Make sure that the migrate_cpp service is running:"
		echo "  cd ${MIGRATE_CPP_PATH}"
		echo "  venv/bin/python3 run.py"
		exit 1
		;;

	*)
		echo "error: unknown command line option \"$1\""
		exit 1
		;;

	esac
done

# TODO: rewrite the following using PHP so I can have lists of items
#       without the need to learn bash array of array of array or something
#       like that (I could use Node.js too?)
#
if test "${DELETE}" = "1"
then
	venv/bin/python3 cli/delete_project.py \
		--project=as2js-string
else
	if test -z "${UID}"
	then
	    echo "error: expected UID to be defined with your user identifier."
	    exit 1
	fi

	if test -f /run/user/${UID}/mk-as2js.lock
	then
	    echo "error: \"/run/user/${UID}/mk-as2js.lock\" lock is in place."
	    exit 1
	fi

	venv/bin/python3 cli/create_project.py \
		--name=as2js-string \
		--workdir=/home/snapwebsites/snapcpp/contrib/as2js \
		--build-command='./mk' \
		--test-command='./mk -t [string]' \
		--test-timeout=60 \
		--clean-command="rm -f /run/user/${UID}/mk-as2js.lock"

	venv/bin/python3 cli/add_files.py \
		--project=as2js-string \
		/home/snapwebsites/snapcpp/contrib/as2js/as2js/types/string.cpp

	venv/bin/python3 cli/generate_patches.py \
		--project=as2js-string

	venv/bin/python3 cli/queue_control.py \
		start
fi
