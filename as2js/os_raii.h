// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/as2js
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// C++
//
#include    <ios>

namespace as2js
{


class raii_stream_flags
{
public:
                            raii_stream_flags(std::ios_base & s);
                            raii_stream_flags(raii_stream_flags const & rhs) = delete;
                            ~raii_stream_flags();

    raii_stream_flags &     operator = (raii_stream_flags const  & rhs) = delete;

    void                    restore();

private:
    std::ios_base *         f_stream = nullptr;
    std::ios_base::fmtflags f_flags = std::ios_base::fmtflags();
    std::streamsize         f_precision = 0;
    std::streamsize         f_width = 0;
};


} // namespace as2js
// vim: ts=4 sw=4 et
