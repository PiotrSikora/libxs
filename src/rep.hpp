/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
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

#ifndef __XS_REP_HPP_INCLUDED__
#define __XS_REP_HPP_INCLUDED__

#include "xrep.hpp"

namespace xs
{

    class ctx_t;
    class msg_t;
    class io_thread_t;
    class socket_base_t;

    class rep_t : public xrep_t
    {
    public:

        rep_t (xs::ctx_t *parent_, uint32_t tid_, int sid_);
        ~rep_t ();

        //  Overloads of functions from socket_base_t.
        int xsend (xs::msg_t *msg_, int flags_);
        int xrecv (xs::msg_t *msg_, int flags_);
        bool xhas_in ();
        bool xhas_out ();

    private:

        //  If true, we are in process of sending the reply. If false we are
        //  in process of receiving a request.
        bool sending_reply;

        //  If true, we are starting to receive a request. The beginning
        //  of the request is the backtrace stack.
        bool request_begins;

        rep_t (const rep_t&);
        const rep_t &operator = (const rep_t&);

    };

    class rep_session_t : public xrep_session_t
    {
    public:

        rep_session_t (xs::io_thread_t *io_thread_, bool connect_,
            xs::socket_base_t *socket_, const options_t &options_,
            const char *protocol_, const char *address_);
        ~rep_session_t ();

    private:

        rep_session_t (const rep_session_t&);
        const rep_session_t &operator = (const rep_session_t&);
    };

}

#endif
