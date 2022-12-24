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
    compiler/compiler.cpp
    compiler/compiler_attributes.cpp
    compiler/compiler_class.cpp
    compiler/compiler_compile.cpp
    compiler/compiler_directive.cpp
    compiler/compiler_expression.cpp
    compiler/compiler_function.cpp
    compiler/compiler_package.cpp
    compiler/compiler_program.cpp
    compiler/compiler_statement.cpp
    compiler/compiler_variable.cpp

    node/node.cpp
    node/node_attribute.cpp
    node/node_compare.cpp
    node/node_convert.cpp
    node/node_display.cpp
    node/node_flag.cpp
    node/node_lock.cpp
    node/node_operator.cpp
    node/node_param.cpp
    node/node_tree.cpp
    node/node_type.cpp
    node/node_value.cpp

    optimizer/optimizer.cpp
    optimizer/optimizer_matches.cpp
    optimizer/optimizer_optimize.cpp
    optimizer/optimizer_tables.cpp

    parser/lexer.cpp
    parser/parser.cpp
    parser/parser_class.cpp
    parser/parser_directive.cpp
    parser/parser_expression.cpp
    parser/parser_function.cpp
    parser/parser_numeric_type.cpp
    parser/parser_package.cpp
    parser/parser_pragma.cpp
    parser/parser_program.cpp
    parser/parser_statement.cpp
    parser/parser_variable.cpp

    file/db.cpp
    file/position.cpp
    file/rc.cpp
    file/stream.cpp

    types/string.cpp

    json.cpp
    message.cpp
    options.cpp
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
    FILES
        compare.h
        compiler.h
        exception.h
        floating_point.h
        integer.h
        json.h
        lexer.h
        message.h
        node.h
        optimizer.h
        options.h
        parser.h
        position.h
        stream.h
        string.h

    DESTINATION
        include/as2js
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/as2js
)


# vim: ts=4 sw=4 et
