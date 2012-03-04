/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2010-2011 Other contributors as noted in the AUTHORS file

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

#ifndef __XS_XSUB_HPP_INCLUDED__
#define __XS_XSUB_HPP_INCLUDED__

#include <vector>

#include "../include/xs_filter.h"

#include "socket_base.hpp"
#include "session_base.hpp"
#include "dist.hpp"
#include "fq.hpp"

namespace xs
{

    class ctx_t;
    class pipe_t;
    class io_thread_t;

    class xsub_t :
        public socket_base_t
    {
    public:

        xsub_t (xs::ctx_t *parent_, uint32_t tid_, int sid_);
        ~xsub_t ();

    protected:

        //  Overloads of functions from socket_base_t.
        void xattach_pipe (xs::pipe_t *pipe_, bool icanhasall_);
        int xsend (xs::msg_t *msg_, int flags_);
        bool xhas_out ();
        int xrecv (xs::msg_t *msg_, int flags_);
        bool xhas_in ();
        void xread_activated (xs::pipe_t *pipe_);
        void xwrite_activated (xs::pipe_t *pipe_);
        void xhiccuped (pipe_t *pipe_);
        void xterminated (xs::pipe_t *pipe_);

    private:

        //  Check whether the message matches at least one subscription.
        bool match (xs::msg_t *msg_);

    //  TODO: Should this be really public?
    public:
        //  Function to be applied to the trie to send all the subsciptions
        //  upstream.
        static void send_subscription (int filter_id_,
            const unsigned char *data_, size_t size_, void *arg_);
    private:

        //  Fair queueing object for inbound pipes.
        fq_t fq;

        //  Object for distributing the subscriptions upstream.
        dist_t dist;

        //  The repository of subscriptions.
        struct filter_t
        {
            xs_filter_t *filter;
            void *fset;
        };
        typedef std::vector <filter_t> filters_t;
        filters_t filters;

        //  If true, 'message' contains a matching message to return on the
        //  next recv call.
        bool has_message;
        msg_t message;

        //  If true, part of a multipart message was already received, but
        //  there are following parts still waiting.
        bool more;

        xsub_t (const xsub_t&);
        const xsub_t &operator = (const xsub_t&);
    };

    class xsub_session_t : public session_base_t
    {
    public:

        xsub_session_t (class io_thread_t *io_thread_, bool connect_,
            socket_base_t *socket_, const options_t &options_,
            const char *protocol_, const char *address_);
        ~xsub_session_t ();

    private:

        xsub_session_t (const xsub_session_t&);
        const xsub_session_t &operator = (const xsub_session_t&);
    };

}

#endif

