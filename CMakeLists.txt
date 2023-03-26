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

cmake_minimum_required(VERSION 3.10.2)

project(as2js_compiler)

enable_language(CXX)
enable_language(C)

find_package(LibExcept        REQUIRED)
find_package(LibUtf8          REQUIRED)
find_package(SnapCMakeModules REQUIRED)
find_package(SnapDev          REQUIRED)
find_package(ICU              REQUIRED COMPONENTS i18n uc)
find_package(Versiontheca     REQUIRED)

SnapGetVersion(AS2JS ${CMAKE_CURRENT_SOURCE_DIR})

# Compatibility to a given JavaScript version (so far none)
set(JS_COMPATIBILITY_VERSION_MAJOR -1)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(cmake)
add_subdirectory(as2js)
add_subdirectory(scripts)
add_subdirectory(tools)
add_subdirectory(doc)
add_subdirectory(tests)

# vim: ts=4 sw=4 et
