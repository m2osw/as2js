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



##
## Compile Run-Time Files
##
CompileRTFile(rt_power)



# vim: ts=4 sw=4 et
