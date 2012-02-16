/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

    This file is part of Crossroads project.

    Crossroads is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Crossroads is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __XS_EPOLL_HPP_INCLUDED__
#define __XS_EPOLL_HPP_INCLUDED__

#include "platform.hpp"

#if defined XS_HAVE_EPOLL

#include <vector>
#include <sys/epoll.h>

#include "fd.hpp"
#include "thread.hpp"
#include "io_thread.hpp"

namespace xs
{

    class ctx_t;
    struct i_poll_events;

    //  This class implements socket polling mechanism using the Linux-specific
    //  epoll mechanism.

    class epoll_t : public io_thread_t
    {
    public:

        epoll_t (xs::ctx_t *ctx_, uint32_t tid_);
        ~epoll_t ();

        //  Implementation of virtual functions from io_thread_t.
        handle_t add_fd (fd_t fd_, xs::i_poll_events *events_);
        void rm_fd (handle_t handle_);
        void set_pollin (handle_t handle_);
        void reset_pollin (handle_t handle_);
        void set_pollout (handle_t handle_);
        void reset_pollout (handle_t handle_);
        void xstart ();
        void xstop ();

    private:

        //  Main worker thread routine.
        static void worker_routine (void *arg_);

        //  Main event loop.
        void loop ();

        //  Main epoll file descriptor
        fd_t epoll_fd;

        struct poll_entry_t
        {
            fd_t fd;
            epoll_event ev;
            xs::i_poll_events *events;
        };

        //  List of retired event sources.
        typedef std::vector <poll_entry_t*> retired_t;
        retired_t retired;

        //  If true, thread is in the process of shutting down.
        bool stopping;

        //  Handle of the physical thread doing the I/O work.
        thread_t worker;

        epoll_t (const epoll_t&);
        const epoll_t &operator = (const epoll_t&);
    };

}

#endif

#endif
