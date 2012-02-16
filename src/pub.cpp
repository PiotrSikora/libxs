/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

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

#include "pub.hpp"
#include "msg.hpp"

xs::pub_t::pub_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    xpub_t (parent_, tid_, sid_)
{
    options.type = XS_PUB;
}

xs::pub_t::~pub_t ()
{
}

int xs::pub_t::xrecv (class msg_t *msg_, int flags_)
{
    //  Messages cannot be received from PUB socket.
    errno = ENOTSUP;
    return -1;
}

bool xs::pub_t::xhas_in ()
{
    return false;
}

xs::pub_session_t::pub_session_t (io_thread_t *io_thread_, bool connect_,
      socket_base_t *socket_, const options_t &options_,
      const char *protocol_, const char *address_) :
    xpub_session_t (io_thread_, connect_, socket_, options_, protocol_,
        address_)
{
}

xs::pub_session_t::~pub_session_t ()
{
}

