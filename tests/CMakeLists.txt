# Copyright (c) 2005-2023  Made to Order Software Corp.  All Rights Reserved
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

# Note: ICU is currently mandatory, but it will be moved back here as follow
#       once we fully implement that functionality in our libutf8 library
#find_package(ICU COMPONENTS i18n uc)


if(SnapCatch2_FOUND AND ICU_FOUND)
    # All the tests require snapcatch2
    #
    # The "tools" defined here are for the tests so ignore them too if the
    # tests do not get generated

    ##
    ## Create the license file
    ##
    #
    # Quite unfortunate, but the tools generated files cannot be used here
    # because the add_custom_target() is not recognized from another
    # sub-folder (i.e. the ../tools/CMakeLists.txt is invisible from here)
    #
    project(test-license)

    set(LICENSE_CPP ${PROJECT_BINARY_DIR}/license.cpp)
    set(LICENSE_H ${PROJECT_BINARY_DIR}/license.h)

    add_custom_command(
        OUTPUT
            ${LICENSE_CPP}
            ${LICENSE_H}

        COMMAND
            echo "--- GENERATING license.cpp/h ---"

        COMMAND
            "as-rc"
                "--name"
                    "license"
                "--namespace"
                    "as2js_tools"
                "--output"
                    "${LICENSE_CPP}"
                "../LICENSE.txt"

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
    ## JSON to C++ string
    ##
    project(json-to-string)

    add_executable(${PROJECT_NAME}
        json_to_string.cpp

        # Stick a copy of the tools license in the test utilities
        ${PROJECT_BINARY_DIR}/license.cpp
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

    function(ajs_to_cpp TARGET_FOLDER TARGET_NAME)
        project(${TARGET_FOLDER}-${TARGET_NAME})

        add_custom_command(
            OUTPUT
                "${PROJECT_BINARY_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.cpp"

            COMMAND
                mkdir -p "${PROJECT_BINARY_DIR}/${TARGET_FOLDER}"

            COMMAND
                "as-rc"
                    "--name"
                        "${TARGET_NAME}"
                    "--namespace"
                        "as2js_tests"
                    "--output"
                        "${PROJECT_BINARY_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.cpp"
                    "${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ajs"

            DEPENDS
                as-rc
                ${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ajs

            COMMENT
                "Generate ${TARGET_FOLDER}/${TARGET_NAME}.cpp"
        )

        add_custom_target(${PROJECT_NAME}
            DEPENDS
                as-rc
                ${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ajs
        )
    endfunction(ajs_to_cpp)

    function(json_to_cpp TARGET_FOLDER TARGET_NAME)
        project(${TARGET_FOLDER}-${TARGET_NAME})

        add_custom_command(
            OUTPUT
                ${PROJECT_BINARY_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ci

            COMMAND
                mkdir -p ${PROJECT_BINARY_DIR}/${TARGET_FOLDER}

            COMMAND
                json-to-string
                    --newlines
                    -o ${PROJECT_BINARY_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ci
                    ${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ason

            DEPENDS
                json-to-string
                ${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ason

            COMMENT
                "Generate ${TARGET_FOLDER}/${TARGET_NAME}.ci"
        )

        add_custom_target(${PROJECT_NAME}
            DEPENDS
                json-to-string
                ${PROJECT_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_NAME}.ason
        )
    endfunction(json_to_cpp)


    json_to_cpp(parser_data array)
    json_to_cpp(parser_data basics)
    json_to_cpp(parser_data class)
    json_to_cpp(parser_data enum)
    json_to_cpp(parser_data expression)
    json_to_cpp(parser_data for)
    json_to_cpp(parser_data function)
    json_to_cpp(parser_data if)
    json_to_cpp(parser_data parser)  # to be broken up still
    json_to_cpp(parser_data pragma)
    json_to_cpp(parser_data regex)
    json_to_cpp(parser_data switch)
    json_to_cpp(parser_data synchronized)
    json_to_cpp(parser_data trycatch)
    json_to_cpp(parser_data type)
    json_to_cpp(parser_data variable)
    json_to_cpp(parser_data while)
    json_to_cpp(parser_data yield)


    json_to_cpp(optimizer_data additive)
    json_to_cpp(optimizer_data assignments)
    json_to_cpp(optimizer_data bitwise)
    json_to_cpp(optimizer_data compare)
    json_to_cpp(optimizer_data conditional)
    json_to_cpp(optimizer_data equality)
    json_to_cpp(optimizer_data logical)
    json_to_cpp(optimizer_data match)
    json_to_cpp(optimizer_data multiplicative)
    json_to_cpp(optimizer_data relational)
    json_to_cpp(optimizer_data statements)


    ajs_to_cpp(compiler_data attr_native_class)
    ajs_to_cpp(compiler_data class_all_operators_overload)

    json_to_cpp(compiler_data class)
    json_to_cpp(compiler_data enum)
    json_to_cpp(compiler_data expression)



    ##
    ## Test to verify that we capture a lock/unlock mismatch
    ##
    project(locked-node)

    add_executable(${PROJECT_NAME}
        locked_node.cpp
    )
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${SNAPCATCH2_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
        as2js
    )


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

        # global
        catch_version.cpp

        # basic features
        catch_floating_point.cpp
        catch_integer.cpp
        catch_message.cpp
        catch_options.cpp
        catch_string.cpp

        # files
        catch_database.cpp
        catch_stream.cpp
        catch_resources.cpp

        # node/position
        catch_node.cpp
        catch_node_display.cpp
        catch_position.cpp

        # lexer/parsers
        catch_json.cpp
        catch_lexer.cpp
        catch_parser.cpp

        # compiler/optimizer
        catch_compiler_attributes.cpp
        catch_compiler_operators.cpp
        catch_compiler.cpp
        catch_optimizer.cpp

        # binary
        catch_rt.cpp

        # Stick a copy of the tools license in the tests
        ${PROJECT_BINARY_DIR}/license.cpp

        # generated files
        ${PROJECT_BINARY_DIR}/compiler_data/attr_native_class.cpp
        ${PROJECT_BINARY_DIR}/compiler_data/class_all_operators_overload.cpp

        ${PROJECT_BINARY_DIR}/compiler_data/class.ci
        ${PROJECT_BINARY_DIR}/compiler_data/enum.ci
        ${PROJECT_BINARY_DIR}/compiler_data/expression.ci

        ${PROJECT_BINARY_DIR}/optimizer_data/additive.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/assignments.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/bitwise.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/compare.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/conditional.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/equality.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/logical.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/match.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/multiplicative.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/relational.ci
        ${PROJECT_BINARY_DIR}/optimizer_data/statements.ci

        ${PROJECT_BINARY_DIR}/parser_data/array.ci
        ${PROJECT_BINARY_DIR}/parser_data/basics.ci
        ${PROJECT_BINARY_DIR}/parser_data/class.ci
        ${PROJECT_BINARY_DIR}/parser_data/enum.ci
        ${PROJECT_BINARY_DIR}/parser_data/expression.ci
        ${PROJECT_BINARY_DIR}/parser_data/for.ci
        ${PROJECT_BINARY_DIR}/parser_data/function.ci
        ${PROJECT_BINARY_DIR}/parser_data/if.ci
        ${PROJECT_BINARY_DIR}/parser_data/pragma.ci
        ${PROJECT_BINARY_DIR}/parser_data/regex.ci
        ${PROJECT_BINARY_DIR}/parser_data/switch.ci
        ${PROJECT_BINARY_DIR}/parser_data/synchronized.ci
        ${PROJECT_BINARY_DIR}/parser_data/trycatch.ci
        ${PROJECT_BINARY_DIR}/parser_data/type.ci
        ${PROJECT_BINARY_DIR}/parser_data/yield.ci
        ${PROJECT_BINARY_DIR}/parser_data/variable.ci
        ${PROJECT_BINARY_DIR}/parser_data/while.ci
        ${PROJECT_BINARY_DIR}/parser_data/yield.ci
        ${PROJECT_BINARY_DIR}/parser_data/parser.ci
    )
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${ICU_INCLUDE_DIRS}
            ${PROJECT_BINARY_DIR}
            ${ADVGETOPT_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
        as2js
        rt
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
            ${SNAPDEV_INCLUDE_DIRS}
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
