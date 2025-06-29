# Copyright (c) 2022-2025  Made to Order Software Corp.  All Rights Reserved
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

project(as2js_cmake)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/As2JsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/As2JsConfig.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/As2JsConfigVersion.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/As2JsConfigVersion.cmake
    @ONLY
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/As2JsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/As2JsConfigVersion.cmake

    DESTINATION
        share/cmake/As2Js
)

install(
    FILES
        AsRcConfig.cmake

    DESTINATION
        share/cmake/AsRc
)

# vim: ts=4 sw=4 et nocindent
