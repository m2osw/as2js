// Copyright (c) 2011-2023  Made to Order Software Corp.  All Rights Reserved
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

// as2js
//
#include    <as2js/node.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <cstring>


// C
//
#include    <signal.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




void sig_abort(int sig)
{
    snapdev::NOT_USED(sig);

    // printing inside a signal is often asking for trouble...
    // but since this process is really simple I do it anyway
    std::cerr << "as2js: node lock/unlock aborted\n";
    exit(1);
}


int main(int argc, char * argv[])
{
    bool do_unlock(true);
    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            std::size_t const max(strlen(argv[i]));
            for(std::size_t j(1); j < max; ++j)
            {
                if(argv[i][j] == 'h')
                {
                    std::cout << "Usage: locked-node [-h | -u]\n"
                                 "where:\n"
                                 "  -h     prints out this help screen.\n"
                                 "  -u     create a node, lock it and then delete it which must fail; without -u, make sure to unlock first.\n";
                    exit(1);
                }
                else if(argv[i][j] == 'u')
                {
                    do_unlock = false;
                }
                else
                {
                    std::cerr << "error: unsupported command line parameter \"-"
                        << argv[i][j]
                        << "\", try -h.\n";
                    exit(1);
                }
            }
        }
        else
        {
            std::cerr << "error: unsupported command line parameter \""
                << argv[i]
                << "\", try -h.\n";
            exit(1);
        }
    }

    signal(SIGABRT, sig_abort);

    // TODO: give user a way to set the node type
    //
    as2js::node::pointer_t node(std::make_shared<as2js::node>(as2js::node_t::NODE_INTEGER));
    node->lock();
    if(do_unlock)
    {
        node->unlock();
    }

    // without the unlock this calls std::terminate()
    //
    node.reset();

    std::cout << "as2js: node lock/unlock success" << std::endl;
    exit(0);
}


// vim: ts=4 sw=4 et
