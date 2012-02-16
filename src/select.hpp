/*
    Copyright (c) 2009-2011 250bpm s.r.o.
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

#ifndef __XS_SELECT_HPP_INCLUDED__
#define __XS_SELECT_HPP_INCLUDED__

#include "platform.hpp"

#if defined XS_HAVE_SELECT

#include "platform.hpp"

#include <stddef.h>
#include <vector>

#ifdef XS_HAVE_WINDOWS
#include "winsock2.h"
#elif defined XS_HAVE_OPENVMS
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include "fd.hpp"
#include "thread.hpp"
#include "io_thread.hpp"

namespace xs
{

    struct i_poll_events;

    //  Implements socket polling mechanism using POSIX.1-2001 select()
    //  function.

    class select_t : public io_thread_t
    {
    public:

        select_t (xs::ctx_t *ctx_, uint32_t tid_);
        ~select_t ();

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

        struct fd_entry_t
        {
            fd_t fd;
            xs::i_poll_events *events;
        };

        //  Checks if an fd_entry_t is retired.
        static bool is_retired_fd (const fd_entry_t &entry);

        //  Set of file descriptors that are used to retreive
        //  information for fd_set.
        typedef std::vector <fd_entry_t> fd_set_t;
        fd_set_t fds;

        fd_set source_set_in;
        fd_set source_set_out;
        fd_set source_set_err;

        fd_set readfds;
        fd_set writefds;
        fd_set exceptfds;

        //  Maximum file descriptor.
        fd_t maxfd;

        //  If true, at least one file descriptor has retired.
        bool retired;

        //  If true, thread is shutting down.
        bool stopping;

        //  Handle of the physical thread doing the I/O work.
        thread_t worker;

        select_t (const select_t&);
        const select_t &operator = (const select_t&);
    };

}

#endif

#endif
