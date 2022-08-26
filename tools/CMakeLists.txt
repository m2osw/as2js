# Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/as2js
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

##
## Tool to convert text (or binary) to a C string
##
project(as-rc)

add_executable(${PROJECT_NAME}
    as_rc.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${SNAPDEV_INCLUDE_DIRS}
)

install(
    TARGETS
        ${PROJECT_NAME}

    DESTINATION
        bin
)


##
## Create the license file
##
project(license)

set(LICENSE_CPP ${PROJECT_BINARY_DIR}/license.cpp)
set(LICENSE_H ${PROJECT_BINARY_DIR}/license.h)

add_custom_command(
    OUTPUT
        ${LICENSE_CPP}
        ${LICENSE_H}

    COMMAND
        echo "--- RUNNING CUSTOM COMMAND! ---"

    COMMAND
        "as-rc"
            "--name"
                "license"
            "--namespace"
                "as2js_tools"
            "--output"
                "${LICENSE_CPP}"
            "../LICENSE.txt"

    COMMAND
        echo "--- BUILT license.cpp/.h ---"

    COMMAND
        find ${PROJECT_BINARY_DIR}

    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}

    DEPENDS
        as-rc
        ../LICENSE.txt
)

add_custom_target(${PROJECT_NAME}
    DEPENDS
        ${LICENSE_CPP}
        ${LICENSE_H}
)

define_property(SOURCE
    PROPERTY GENERATED
    BRIEF_DOCS "The license.cpp/h files are a C string generated from the LICENSE.txt file."
    FULL_DOCS "The license.cpp/h files are a C string generated from the LICENSE.txt file."
)


##
## AS to JavaScript tools
##
# The project name should be "as2js", unfortunately the library target
# has to be named that way to work...
project(as2js-compiler)

add_executable(${PROJECT_NAME}
    as2js.cpp
    ${LICENSE_CPP}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        #${as2js_SOURCE_DIR}
        ${ADVGETOPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME
        as2js

    VERSION
        ${AS2JS_VERSION_MAJOR}.${AS2JS_VERSION_MINOR}
)
install(
    TARGETS
        ${PROJECT_NAME}

    DESTINATION
        bin
)


##
## JSON handler
##
project(json)

add_executable(${PROJECT_NAME}
    json.cpp
    #${LICENSE_CPP}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        #${as2js_SOURCE_DIR}
        ${ADVGETOPT_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${BOOST_INCLUDE_DIRS}
)


target_link_libraries(${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    DESTINATION
        bin
)



# vim: ts=4 sw=4 et
