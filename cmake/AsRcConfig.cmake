# Copyright (c) 2022-2023  Made to Order Software Corp.  All Rights Reserved
#
# http://snapwebsites.org/project/as2js
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
################################################################################
#
# File:         AsRcConfig.cmake
# Object:       Provide function to generate C/C++ files as resources.
#
include(CMakeParseArguments)

# First try to find the one we just compiled (developer environment)
get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_DIR} DIRECTORY)
get_filename_component(CMAKE_BINARY_PARENT_DIR_NAME ${CMAKE_BINARY_PARENT_DIR} NAME)
if(${CMAKE_BINARY_PARENT_DIR_NAME} STREQUAL "coverage")
    # we have a sub-sub-directory when building coverage
    get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_PARENT_DIR} DIRECTORY)
    get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_PARENT_DIR} DIRECTORY)
endif()
find_program(
    AS_RC_PROGRAM
        as-rc

    HINTS
        ${CMAKE_BINARY_PARENT_DIR}/as2js/tools
        ${CMAKE_BINARY_PARENT_DIR}/contrib/as2js/tools

    NO_DEFAULT_PATH
)

# Second, if the first find_program() came up empty handed, try again to
# find an installed version (i.e. generally under /usr/bin)
# This one is marked as REQUIRED.
find_program(
    AS_RC_PROGRAM
        as-rc

    REQUIRED
)

if(${AS_RC_PROGRAM} STREQUAL "AS_RC_PROGRAM-NOTFOUND")
    message(FATAL_ERROR "as-rc tool not found")
endif()

# This function generates two outputs files:
#
#    <name>.cpp and <name>.h
#
# From one or more <name>.<ext> files which are expected to be text files.
# This allows you to create resource-like files from external files and
# compile them in your project.
#
# \example
#
# We use this tool in our iplock project to include some text files in the
# ipload tool (see `tools/ipload/CMakeLists.txt`):
#
# \code
#     # first generate the files
#     AsRc(
#         OUTPUT
#             ${CMAKE_CURRENT_BINARY_DIR}/default_firewall.cpp
#
#         NAME
#             default_firewall
#
#         NAMESPACE
#             tools_ipload
#
#         INPUTS
#             default_firewall.conf
#     )
#
#     # second include the files in your library or executable
#     add_executable(${PROJECT_NAME}
#         ...
#         ${CMAKE_CURRENT_BINARY_DIR}/default_firewall.cpp
#     )
# \endcode
#
# \todo
# Make the name optional (i.e. the input filename is used by default).
# Make the namespace optional for C users.
#
# \param INPUTS  The input filenames.
# \param BINARY  If specified, treat input as binary.
# \param OUTPUT  The output filename.
# \param NAME  The name of the variable and size.
# \param NAMESPACE  The name of a namespace to encapsulate the variable.
#
function(AsRc)
    set(options BINARY)
    set(oneValueArgs OUTPUT NAME NAMESPACE)
    set(multiValueArgs INPUTS)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "You must specify OUTPUT <filename>.")
    endif()

    if(NOT ARG_NAME)
        message(FATAL_ERROR "You must specify NAME <variable-name>.")
    endif()

    if(NOT ARG_NAMESPACE)
        message(FATAL_ERROR "You must specify NAMESPACE <namespace-name>.")
    endif()

    if(NOT ARG_INPUTS)
        message(FATAL_ERROR "You must specify INPUTS <filename1> <filename2> ... <filenameN>.")
    endif()

    get_filename_component(ASRC_BASENAME ${ARG_OUTPUT} NAME_WE)
    get_filename_component(ASRC_DIRECTORY ${ARG_OUTPUT} DIRECTORY)

    file(RELATIVE_PATH RELATIVE_SOURCE_DIR ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "/" "_" INTRODUCER ${RELATIVE_SOURCE_DIR})
    string(REPLACE "/" "_" OUTPUT_NAME ${ASRC_BASENAME})

    project(${INTRODUCER}_${OUTPUT_NAME}_asrc)

    if(ARG_BINARY)
        set(ASRC_BINARY "--binary")
    endif()

    add_custom_command(
        OUTPUT
            ${ASRC_DIRECTORY}/${ASRC_BASENAME}.cpp
            ${ASRC_DIRECTORY}/${ASRC_BASENAME}.h

        COMMAND
            "${AS_RC_PROGRAM}"
                    "--name"
                        "${ARG_NAME}"
                    "--namespace"
                        "${ARG_NAMESPACE}"
                    "--output"
                        "${ARG_OUTPUT}"
                    ${ASRC_BINARY}
                    ${ARG_INPUTS}

        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}

        MAIN_DEPENDENCY
            ${ARG_INPUTS}
    )

    add_custom_target(${PROJECT_NAME}
        DEPENDS
            ${ASRC_DIRECTORY}/${ASRC_BASENAME}.cpp
            ${ASRC_DIRECTORY}/${ASRC_BASENAME}.h
    )
endfunction()

# vim: ts=4 sw=4 et
