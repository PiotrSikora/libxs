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

#ifndef __XS_XPUB_HPP_INCLUDED__
#define __XS_XPUB_HPP_INCLUDED__

#include "../include/xs_filter.h"

#include <deque>
#include <string>

#include "socket_base.hpp"
#include "session_base.hpp"
#include "array.hpp"
#include "dist.hpp"
#include "blob.hpp"

namespace xs
{

    class ctx_t;
    class msg_t;
    class pipe_t;
    class io_thread_t;

    class xpub_t :
        public socket_base_t
    {
    public:

        xpub_t (xs::ctx_t *parent_, uint32_t tid_, int sid_);
        ~xpub_t ();

        //  Implementations of virtual functions from socket_base_t.
        void xattach_pipe (xs::pipe_t *pipe_, bool icanhasall_);
        int xsend (xs::msg_t *msg_, int flags_);
        bool xhas_out ();
        int xrecv (xs::msg_t *msg_, int flags_);
        bool xhas_in ();
        void xread_activated (xs::pipe_t *pipe_);
        void xwrite_activated (xs::pipe_t *pipe_);
        void xterminated (xs::pipe_t *pipe_);

    private:

    //  TODO: Should this really be public?
    public:
        //  Function to be applied to the trie to send all the subsciptions
        //  upstream.
        static void send_unsubscription (int filter_id_,
            const unsigned char *data_, size_t size_, void *arg_);
    private:

        //  The repository of subscriptions.
        struct filter_t
        {
            xs_filter_t *filter;
            void *fset;
        };
        typedef std::vector <filter_t> filters_t;
        filters_t filters;

    //  TODO: Should this really be public?
    public:
        //  Distributor of messages holding the list of outbound pipes.
        dist_t dist;
    private:

        //  True if we are in the middle of sending a multi-part message.
        bool more;

        //  List of pending (un)subscriptions, ie. those that were already
        //  applied to the trie, but not yet received by the user.
        typedef std::deque <blob_t> pending_t;
        pending_t pending;

        xpub_t (const xpub_t&);
        const xpub_t &operator = (const xpub_t&);
    };

    class xpub_session_t : public session_base_t
    {
    public:

        xpub_session_t (xs::io_thread_t *io_thread_, bool connect_,
            socket_base_t *socket_, const options_t &options_,
            const char *protocol_, const char *address_);
        ~xpub_session_t ();

    private:

        xpub_session_t (const xpub_session_t&);
        const xpub_session_t &operator = (const xpub_session_t&);
    };

}

#endif
