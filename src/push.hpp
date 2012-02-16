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

#ifndef __XS_PUSH_HPP_INCLUDED__
#define __XS_PUSH_HPP_INCLUDED__

#include "socket_base.hpp"
#include "session_base.hpp"
#include "lb.hpp"

namespace xs
{

    class ctx_t;
    class pipe_t;
    class msg_t;
    class io_thread_t;

    class push_t :
        public socket_base_t
    {
    public:

        push_t (xs::ctx_t *parent_, uint32_t tid_, int sid_);
        ~push_t ();

    protected:

        //  Overloads of functions from socket_base_t.
        void xattach_pipe (xs::pipe_t *pipe_, bool icanhasall_);
        int xsend (xs::msg_t *msg_, int flags_);
        bool xhas_out ();
        void xwrite_activated (xs::pipe_t *pipe_);
        void xterminated (xs::pipe_t *pipe_);

    private:

        //  Load balancer managing the outbound pipes.
        lb_t lb;

        push_t (const push_t&);
        const push_t &operator = (const push_t&);
    };

    class push_session_t : public session_base_t
    {
    public:

        push_session_t (xs::io_thread_t *io_thread_, bool connect_,
            socket_base_t *socket_, const options_t &options_,
            const char *protocol_, const char *address_);
        ~push_session_t ();

    private:

        push_session_t (const push_session_t&);
        const push_session_t &operator = (const push_session_t&);
    };

}

#endif
