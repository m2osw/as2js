#!/bin/sh -e
#
# This script runs our tests in a logical order for when you make changes
# to the lower level and need to test things "from scratch" up to the much
# more complicated tests.
#
# Run from the main folder with:
#
#     tests/tests-in-order.sh 2>&1 | less
#
# Using `less` is optional, but all the content output is not likely to
# fit in your console (over 10,000 lines in verbose mode).

# Basic Types / Functionality
./mk -t '[version]'
./mk -t '[integer]'
./mk -t '[floating_point]'
./mk -t '[string]'
./mk -t '[options]'
./mk -t '[message]'

# File Handling
./mk -t '[stream]'
./mk -t '[rc]'
./mk -t '[db]'

# Node
./mk -t '[position]'
./mk -t '[node]'

# Compiler / JSON
./mk -t '[lexer]'
./mk -t '[json]'
./mk -t '[parser]'
./mk -t '[optimizer]'
./mk -t '[compiler]'

