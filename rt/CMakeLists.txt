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
## Compile Run-Time Files from assembly to binary
##
#
# Here we create binary files without the .ELF format so that way we can
# easily use the code in our output (without the need for a complex linker)
#
# To verify the output, you can use the following command:
#
#    objdump -b binary -m i386:x86-64 -D ${RT_NAME}.bin | less
#

function(CompileRTFile RT_NAME)
    project(${RT_NAME})

message("Adding RT project [${PROJECT_NAME}]")
    set(RT_NAME_OUTPUT "${PROJECT_BINARY_DIR}/${RT_NAME}.bin")

    add_custom_command(
        OUTPUT
            ${RT_NAME_OUTPUT}

        COMMAND
            as -o ${PROJECT_BINARY_DIR}/${RT_NAME}.o ${RT_NAME}.rt

        COMMAND
            objcopy -O binary ${PROJECT_BINARY_DIR}/${RT_NAME}.o ${RT_NAME_OUTPUT}

        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}

        DEPENDS
            ${RT_NAME}.rt
    )

    add_custom_target(${PROJECT_NAME}
        DEPENDS
            ${RT_NAME_OUTPUT}
    )
endfunction()


set(RT_LIST
    rt_fmod
    rt_ipow
    rt_pow
)


##
## Compile Run-Time Files
##
foreach(RT_NAME ${RT_LIST})
    CompileRTFile(${RT_NAME})
endforeach()


##
## Now Create the Archive
##
project(rt-archive)

set(RT_ARCHIVE_OUTPUT "${PROJECT_BINARY_DIR}/rt.oar")

foreach(RT_NAME ${RT_LIST})
    set(RT_BIN_LIST ${RT_BIN_LIST} "${PROJECT_BINARY_DIR}/${RT_NAME}.bin")
endforeach()


add_custom_command(
    OUTPUT
        ${RT_ARCHIVE_OUTPUT}

    COMMAND
        ${CMAKE_BINARY_DIR}/tools/as2js --create-archive -o ${RT_ARCHIVE_OUTPUT} ${RT_BIN_LIST}

    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}

    DEPENDS
        ${RT_BIN_LIST}
        as2js-compiler
)

add_custom_target(run ALL
    DEPENDS
        ${RT_ARCHIVE_OUTPUT}
)

install(
    FILES
        ${RT_ARCHIVE_OUTPUT}

    DESTINATION
        lib/as2js/rt
)


##
## For the C++ implementation (not started)
##
project(rt)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    rt.s
    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
#        ${LIBEXCEPT_INCLUDE_DIRS}
#        ${LIBUTF8_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
#        ${VERSIONTHECA_INCLUDE_DIRS}
)

#target_link_libraries(${PROJECT_NAME}
#    ${LIBEXCEPT_LIBRARIES}
#    ${LIBUTF8_LIBRARIES}
#    ${VERSIONTHECA_LIBRARIES}
#)

# TODO: fix the following, maybe with the enable_language(ASM) because the
#       dependencies are not correctly detected in this way
#
set_property(SOURCE rt.s PROPERTY LANGUAGE C)

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
        rt.h

    DESTINATION
        lib/as2js/rt
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        lib/as2js/rt
)

# vim: ts=4 sw=4 et
