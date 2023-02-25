# Copyright (c) 2005-2023  Made to Order Software Corp.  All Rights Reserved
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
## Unicode character type generator (helper executable)
## (TODO: replace 100% with libutf8)
##
project(unicode-characters)

add_executable(${PROJECT_NAME}
    unicode_characters.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ICU_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
)


project(unicode-character-types)

set(UNICODE_CHARACTER_TYPES_CI ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.ci)

add_custom_command(
    OUTPUT
        ${UNICODE_CHARACTER_TYPES_CI}

    COMMAND
        unicode-characters >${UNICODE_CHARACTER_TYPES_CI}

    WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}

    DEPENDS
        unicode-characters
)

add_custom_target(${PROJECT_NAME}
    DEPENDS
        ${UNICODE_CHARACTER_TYPES_CI}
)


##
## Verify Compiler
##
project(verify_compiler)

set(VERIFY_COMPILER_H ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.h)

FILE(GLOB ${PROJECT_NAME}_DEPENDENCIES
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler/*.cpp"
)

# Make sure we do not call get_attributes() directly:
#
# This is wrong inside the compiler:
#
#     n->get_attribute(node_attr_t::NODE_ATTR_NATIVE);
#
# This is the correct way of getting the attribute:
#
#     get_attribute(n, node_attr_t::NODE_ATTR_NATIVE);
#
# because the compiler will transform attribute nodes in NODE_ATTR_... flags
# and also make sure attributes are propagated from parents to children as
# defined in our language.
#
add_custom_command(
    OUTPUT
        ${VERIFY_COMPILER_H}

    COMMAND
        ! grep
            "--exclude=compiler_attributes.cpp"
            "-r"
                "'get_attribute([^,]\\+)'"
            ${${PROJECT_NAME}_DEPENDENCIES}

    COMMAND
        touch "${VERIFY_COMPILER_H}"

    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}

    DEPENDS
        ${${PROJECT_NAME}_DEPENDENCIES}
)

add_custom_target(${PROJECT_NAME}
    DEPENDS
        ${VERIFY_COMPILER_H}
)


##
## AS to JavaScript library
##
project(as2js)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    ${VERIFY_COMPILER_H}
    compiler/compiler.cpp
    compiler/compiler_attributes.cpp
    compiler/compiler_class.cpp
    compiler/compiler_compile.cpp
    compiler/compiler_directive.cpp
    compiler/compiler_expression.cpp
    compiler/compiler_function.cpp
    compiler/compiler_package.cpp
    compiler/compiler_program.cpp
    compiler/compiler_resolver.cpp
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
    ${UNICODE_CHARACTER_TYPES_CI}

    output/binary.cpp
    output/output.cpp

    file/database.cpp
    file/position.cpp
    file/resources.cpp
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
