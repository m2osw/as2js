# Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/as2js
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

##
## AS to JavaScript library
##
project(as2js)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    compiler.cpp
    compiler_attributes.cpp
    compiler_class.cpp
    compiler_compile.cpp
    compiler_directive.cpp
    compiler_expression.cpp
    compiler_function.cpp
    compiler_package.cpp
    compiler_program.cpp
    compiler_statement.cpp
    compiler_variable.cpp
    db.cpp
    json.cpp
    lexer.cpp
    message.cpp
    node.cpp
    node_attribute.cpp
    node_compare.cpp
    node_convert.cpp
    node_display.cpp
    node_flag.cpp
    node_lock.cpp
    node_operator.cpp
    node_param.cpp
    node_tree.cpp
    node_type.cpp
    node_value.cpp
    optimizer.cpp
    optimizer_matches.cpp
    optimizer_optimize.cpp
    optimizer_tables.cpp
    options.cpp
    os_raii.cpp
    parser.cpp
    parser_class.cpp
    parser_directive.cpp
    parser_expression.cpp
    parser_function.cpp
    parser_numeric_type.cpp
    parser_package.cpp
    parser_pragma.cpp
    parser_program.cpp
    parser_statement.cpp
    parser_variable.cpp
    position.cpp
    rc.cpp
    stream.cpp
    string.cpp
    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBUTF8_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${AS2JS_VERSION_MAJOR}.${AS2JS_VERSION_MINOR}

    SOVERSION
        ${AS2JS_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib

    COMPONENT
        runtime
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}

    DESTINATION
        include

    FILES_MATCHING PATTERN
        "*.h"
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/as2js
)


# vim: ts=4 sw=4 et
