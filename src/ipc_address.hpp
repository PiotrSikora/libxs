/*
    Copyright (c) 2011-2012 250bpm s.r.o.
    Copyright (c) 2011 Other contributors as noted in the AUTHORS file

    This file is part of Crossroads I/O project.

    Crossroads I/O is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Crossroads is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __XS_IPC_ADDRESS_HPP_INCLUDED__
#define __XS_IPC_ADDRESS_HPP_INCLUDED__

#include "platform.hpp"

#if !defined XS_HAVE_WINDOWS && !defined XS_HAVE_OPENVMS

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace xs
{

    class ipc_address_t
    {
    public:

        ipc_address_t ();
        ~ipc_address_t ();

        //  This function sets up the address for UNIX domain transport.
        int resolve (const char* path_);

        sockaddr *addr ();
        socklen_t addrlen ();

    private:

        struct sockaddr_un address;

        ipc_address_t (const ipc_address_t&);
        const ipc_address_t &operator = (const ipc_address_t&);
    };
    
}

#endif

#endif


