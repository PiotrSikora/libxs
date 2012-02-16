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

#ifndef __XS_REAPER_HPP_INCLUDED__
#define __XS_REAPER_HPP_INCLUDED__

#include "object.hpp"
#include "mailbox.hpp"
#include "io_thread.hpp"

namespace xs
{

    class ctx_t;
    class socket_base_t;

    class reaper_t : public object_t, public i_poll_events
    {
    public:

        reaper_t (xs::ctx_t *ctx_, uint32_t tid_);
        ~reaper_t ();

        mailbox_t *get_mailbox ();

        void start ();
        void stop ();

        //  i_poll_events implementation.
        void in_event (fd_t fd_);
        void out_event (fd_t fd_);
        void timer_event (handle_t handle_);

    private:

        //  Command handlers.
        void process_stop ();
        void process_reap (xs::socket_base_t *socket_);
        void process_reaped ();

        //  Reaper thread accesses incoming commands via this mailbox.
        mailbox_t mailbox;

        //  Handle associated with mailbox' file descriptor.
        handle_t mailbox_handle;

        //  I/O multiplexing is performed using an io_thread object.
        io_thread_t *io_thread;

        //  Number of sockets being reaped at the moment.
        int sockets;

        //  If true, we were already asked to terminate.
        bool terminating;

        reaper_t (const reaper_t&);
        const reaper_t &operator = (const reaper_t&);
    };

}

#endif
