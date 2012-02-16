/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2010 iMatix Corporation
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

#include "push.hpp"
#include "pipe.hpp"
#include "err.hpp"
#include "msg.hpp"

xs::push_t::push_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    socket_base_t (parent_, tid_, sid_)
{
    options.type = XS_PUSH;
}

xs::push_t::~push_t ()
{
}

void xs::push_t::xattach_pipe (pipe_t *pipe_, bool icanhasall_)
{
    xs_assert (pipe_);
    lb.attach (pipe_);
}

void xs::push_t::xwrite_activated (pipe_t *pipe_)
{
    lb.activated (pipe_);
}

void xs::push_t::xterminated (pipe_t *pipe_)
{
    lb.terminated (pipe_);
}

int xs::push_t::xsend (msg_t *msg_, int flags_)
{
    return lb.send (msg_, flags_);
}

bool xs::push_t::xhas_out ()
{
    return lb.has_out ();
}

xs::push_session_t::push_session_t (io_thread_t *io_thread_, bool connect_,
      socket_base_t *socket_, const options_t &options_,
      const char *protocol_, const char *address_) :
    session_base_t (io_thread_, connect_, socket_, options_, protocol_,
        address_)
{
}

xs::push_session_t::~push_session_t ()
{
}

