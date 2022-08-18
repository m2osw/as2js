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

find_package(SnapCatch2)
find_package(ICU COMPONENTS i18n uc)

if(SnapCatch2_FOUND AND ICU_FOUND)
    # All the tests require snapcatch2
    #
    # The "tools" defined here are for the tests so ignore them too if the
    # tests do not get generated

    ##
    ## Unicode character determination (helper executable)
    ##
    project(unicode_characters)

    add_executable(${PROJECT_NAME}
        unicode_characters.cpp
    )
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${ICU_INCLUDE_DIRS}
            ${SNAPCATCH2_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES}
    )


    ##
    ## JSON to C++ string
    ##
    project(json-to-string)

    add_executable(${PROJECT_NAME}
        json_to_string.cpp

        # Stick a copy of the tools license in the test utilities
        ../tools/license.cpp
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${as2js_compiler_SOURCE_DIR}/tools
            ${ADVGETOPT_INCLUDE_DIRS}
            ${LIBEXCEPT_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
        as2js
        ${ADVGETOPT_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
    )

    function(json_to_cpp TARGET_NAME)
        add_custom_command(
            OUTPUT
                ${as2js_compiler_BINARY_DIR}/tests/catch_${TARGET_NAME}.ci

            COMMAND
                json-to-string -o ${as2js_compiler_BINARY_DIR}/tests/catch_${TARGET_NAME}.ci ${as2js_compiler_SOURCE_DIR}/tests/catch_${TARGET_NAME}.json

            DEPENDS
                json-to-string
                ${as2js_compiler_SOURCE_DIR}/tests/catch_${TARGET_NAME}.json

            COMMENT
                "Generating catch_as2js_${TARGET_NAME}.ci"
        )
    endfunction(json_to_cpp)


    json_to_cpp(parser_array)
    json_to_cpp(parser_basics)
    json_to_cpp(parser_class)
    json_to_cpp(parser_enum)
    json_to_cpp(parser_for)
    json_to_cpp(parser_function)
    json_to_cpp(parser_if)
    json_to_cpp(parser_pragma)
    json_to_cpp(parser_switch)
    json_to_cpp(parser_synchronized)
    json_to_cpp(parser_trycatch)
    json_to_cpp(parser_type)
    json_to_cpp(parser_variable)
    json_to_cpp(parser_while)
    json_to_cpp(parser_yield)
    json_to_cpp(parser)


    json_to_cpp(optimizer_additive)
    json_to_cpp(optimizer_assignments)
    json_to_cpp(optimizer_bitwise)
    json_to_cpp(optimizer_compare)
    json_to_cpp(optimizer_conditional)
    json_to_cpp(optimizer_equality)
    json_to_cpp(optimizer_logical)
    json_to_cpp(optimizer_match)
    json_to_cpp(optimizer_multiplicative)
    json_to_cpp(optimizer_relational)
    json_to_cpp(optimizer_statements)


    json_to_cpp(compiler_class)
    json_to_cpp(compiler_enum)
    json_to_cpp(compiler_expression)



    ##
    ## as2js tests
    ##



    # Test library objects
    # Using cppunit does not require a function
    #function( as2jsLibraryTest )
    #    set( options        NO_OPTIONS )
    #    set( oneValueArgs   PROJECT_NAME )
    #    set( multiValueArgs SOURCES )
    #    cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    #
    #    if( NOT ARG_PROJECT_NAME )
    #        message( FATAL_ERROR "You must specify PROJECT_NAME to as2jsLibraryTest!" )
    #    endif()
    #    if( NOT ARG_SOURCES )
    #        message( FATAL_ERROR "You must specify SOURCES to as2jsLibraryTest!" )
    #    endif()
    #
    #    message( "Setup test \"" ${ARG_PROJECT_NAME} "\"" )
    #    project( ${ARG_PROJECT_NAME} )
    #    add_executable( ${ARG_PROJECT_NAME}
    #        ${ARG_SOURCES}
    #    )
    #    target_link_libraries( ${ARG_PROJECT_NAME}
    #        as2js
    #    )
    #endfunction()


    # Library: class String
    #as2jsLibraryTest(PROJECT_NAME catch_string SOURCES catch_string)

    project(unittest)

    add_executable(${PROJECT_NAME}
        catch_main.cpp

        #catch_compiler.cpp
        #catch_db.cpp
        #catch_float64.cpp
        #catch_int64.cpp
        catch_json.cpp
        #catch_lexer.cpp
        #catch_message.cpp
        #catch_node.cpp
        #catch_node_display.cpp
        #catch_optimizer.cpp
        #catch_options.cpp
        #catch_parser.cpp
        #catch_position.cpp
        #catch_rc.cpp
        catch_stream.cpp
        #catch_string.cpp
        catch_version.cpp

        # Stick a copy of the tools license in the tests
        ../tools/license.cpp

        #${as2js_compiler_BINARY_DIR}/tests/catch_compiler_class.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_compiler_enum.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_compiler_expression.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_additive.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_assignments.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_bitwise.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_compare.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_conditional.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_equality.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_logical.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_match.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_multiplicative.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_relational.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_optimizer_statements.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_array.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_basics.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_class.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_enum.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_for.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_function.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_if.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_pragma.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_switch.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_synchronized.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_trycatch.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_type.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_yield.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_variable.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_while.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser_yield.ci
        #${as2js_compiler_BINARY_DIR}/tests/catch_parser.ci
    )
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${ICU_INCLUDE_DIRS}
            #${as2js_compiler_BINARY_DIR}/include
            #${as2js_compiler_BINARY_DIR}/tests
            ${as2js_compiler_SOURCE_DIR}/tools
            #${as2js_compiler_SOURCE_DIR}/lib
            #${as2js_compiler_SOURCE_DIR}/src
            ${ADVGETOPT_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
        as2js
        ${ADVGETOPT_LIBRARIES}
        ${ICU_LIBRARIES}
        ${ICU_I18N_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${SNAPCATCH2_LIBRARIES}
    )


    ##
    ## Run all the tests
    ##
    project(run_as2js_tests)
    add_custom_target(run_as2js_tests
        COMMAND echo "*** run_as2js_tests ***"
        COMMAND unittest
        DEPENDS unittest
    )



    # Source:
    # https://stackoverflow.com/questions/23977313/with-cmake-how-could-i-write-a-test-to-verify-that-a-class-is-abstract
    #
    # Here I test all the abstract classes that are abstract now and are
    # expected to remain abstract forever

    # The CheckCXXAbstractClass is our own file, see the cmake project
    #
    message("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")
    include(CheckCXXAbstractClass)

    include(CheckCXXSourceCompiles)


    #
    # class InputRetriever
    #
    unset(BUILT_ABSTRACT_INPUT_RETRIEVER CACHE)
    set(
        CMAKE_REQUIRED_INCLUDES 
            ${as2js_compiler_BINARY_DIR}
            ${as2js_compiler_SOURCE_DIR}
            ${ADVGETOPT_INCLUDE_DIRS}
    )
    check_cxx_abstract_class(
    "#include <as2js/stream.h>
    int main() { new as2js::input_retriever; }"
        BUILT_ABSTRACT_INPUT_RETRIEVER
    )
    if(BUILT_ABSTRACT_INPUT_RETRIEVER)
        message(FATAL_ERROR "class as2js::input_retriever can be instantiated, but should be abstract.")
    endif()


else(SnapCatch2_FOUND AND ICU_FOUND)

    message("SnapCatch2 and/or ICU not found... no tests will be built.")

endif(SnapCatch2_FOUND AND ICU_FOUND)

# vim: ts=4 sw=4 et
