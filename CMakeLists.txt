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

cmake_minimum_required(VERSION 3.10.2)

project(as2js_compiler)

## Global C++ command line flags
#if( DEFINED ENV{LIBTLD_COVERAGE} )
#    message("*** COVERAGE TURNED ON ***")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option -fprofile-arcs -ftest-coverage")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Werror -Wall -Wextra -fprofile-arcs -ftest-coverage")
#    set(CMAKE_SHARED_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
#    set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
#    set(COVERAGE_LIBS gcov)
#else()
#    # If you have problems compiling with the -W... flags, use the -O3 only
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option")
#    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Werror -Wall -Wextra")
#    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
#endif()

enable_language(CXX)
enable_language(C)


find_package(AdvGetOpt        REQUIRED)
find_package(Boost            REQUIRED)
find_package(LibExcept        REQUIRED)
find_package(LibUtf8          REQUIRED)
find_package(SnapCMakeModules REQUIRED)
find_package(SnapDev          REQUIRED)

SnapGetVersion(AS2JS ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(cmake)
add_subdirectory(as2js)
add_subdirectory(tools)
add_subdirectory(doc)
add_subdirectory(tests)

# vim: ts=4 sw=4 et
