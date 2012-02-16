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

#include <new>

#include "io_thread.hpp"
#include "platform.hpp"
#include "err.hpp"
#include "ctx.hpp"

xs::io_thread_t::io_thread_t (ctx_t *ctx_, uint32_t tid_) :
    object_t (ctx_, tid_)
{
    poller = poller_base_t::create ();
    xs_assert (poller);

    mailbox_handle = poller->add_fd (mailbox.get_fd (), this);
    poller->set_pollin (mailbox_handle);
}

xs::io_thread_t::~io_thread_t ()
{
    delete poller;
}

void xs::io_thread_t::start ()
{
    //  Start the underlying I/O thread.
    poller->start ();
}

void xs::io_thread_t::stop ()
{
    send_stop ();
}

xs::mailbox_t *xs::io_thread_t::get_mailbox ()
{
    return &mailbox;
}

int xs::io_thread_t::get_load ()
{
    return poller->get_load ();
}

void xs::io_thread_t::in_event (fd_t fd_)
{
    //  TODO: Do we want to limit number of commands I/O thread can
    //  process in a single go?

    while (true) {

        //  Get the next command. If there is none, exit.
        command_t cmd;
        int rc = mailbox.recv (&cmd, 0);
        if (rc != 0 && errno == EINTR)
            continue;
        if (rc != 0 && errno == EAGAIN)
            break;
        errno_assert (rc == 0);

        //  Process the command.
        cmd.destination->process_command (cmd);
    }
}

void xs::io_thread_t::out_event (fd_t fd_)
{
    //  We are never polling for POLLOUT here. This function is never called.
    xs_assert (false);
}

void xs::io_thread_t::timer_event (int id_)
{
    //  No timers here. This function is never called.
    xs_assert (false);
}

xs::poller_base_t *xs::io_thread_t::get_poller ()
{
    xs_assert (poller);
    return poller;
}

void xs::io_thread_t::process_stop ()
{
    poller->rm_fd (mailbox_handle);
    poller->stop ();
}
