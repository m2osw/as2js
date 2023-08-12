#!/bin/bash -e

# The lock stays behind on a timeout
# TODO: I think that the clean command was not working, trying again
#rm -f /run/user/${UID}/mk-as2js.lock

# Build
./mk

