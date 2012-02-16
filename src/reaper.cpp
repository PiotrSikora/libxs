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

#include "reaper.hpp"
#include "socket_base.hpp"
#include "err.hpp"

xs::reaper_t::reaper_t (class ctx_t *ctx_, uint32_t tid_) :
    object_t (ctx_, tid_),
    sockets (0),
    terminating (false)
{
    io_thread = io_thread_t::create (ctx_, tid_);
    xs_assert (io_thread);

    mailbox_handle = io_thread->add_fd (mailbox.get_fd (), this);
    io_thread->set_pollin (mailbox_handle);
}

xs::reaper_t::~reaper_t ()
{
    delete io_thread;
}

xs::mailbox_t *xs::reaper_t::get_mailbox ()
{
    return &mailbox;
}

void xs::reaper_t::start ()
{
    //  Start the thread.
    io_thread->start ();
}

void xs::reaper_t::stop ()
{
    send_stop ();
}

void xs::reaper_t::in_event (fd_t fd_)
{
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

void xs::reaper_t::out_event (fd_t fd_)
{
    xs_assert (false);
}

void xs::reaper_t::timer_event (handle_t handle_)
{
    xs_assert (false);
}

void xs::reaper_t::process_stop ()
{
    terminating = true;

    //  If there are no sockets being reaped finish immediately.
    if (!sockets) {
        send_done ();
        io_thread->rm_fd (mailbox_handle);
        io_thread->stop ();
    }
}

void xs::reaper_t::process_reap (socket_base_t *socket_)
{
    //  Add the socket to the I/O thread.
    socket_->start_reaping (io_thread);

    ++sockets;
}

void xs::reaper_t::process_reaped ()
{
    --sockets;

    //  If reaped was already asked to terminate and there are no more sockets,
    //  finish immediately.
    if (!sockets && terminating) {
        send_done ();
        io_thread->rm_fd (mailbox_handle);
        io_thread->stop ();
    }
}
