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

#include "kqueue.hpp"

#if defined XS_HAVE_KQUEUE

#include <sys/time.h>
#include <sys/types.h>
#include <sys/event.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <new>

#include "kqueue.hpp"
#include "err.hpp"
#include "config.hpp"
#include "i_poll_events.hpp"
#include "likely.hpp"

//  NetBSD defines (struct kevent).udata as intptr_t, everyone else
//  as void *.
#if defined XS_HAVE_NETBSD
#define kevent_udata_t intptr_t
#else
#define kevent_udata_t void *
#endif

xs::kqueue_t::kqueue_t () :
    stopping (false)
{
    //  Create event queue
    kqueue_fd = kqueue ();
    errno_assert (kqueue_fd != -1);
}

xs::kqueue_t::~kqueue_t ()
{
    worker.stop ();
    close (kqueue_fd);
}

void xs::kqueue_t::kevent_add (fd_t fd_, short filter_, void *udata_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_ADD, 0, 0, (kevent_udata_t)udata_);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

void xs::kqueue_t::kevent_delete (fd_t fd_, short filter_)
{
    struct kevent ev;

    EV_SET (&ev, fd_, filter_, EV_DELETE, 0, 0, 0);
    int rc = kevent (kqueue_fd, &ev, 1, NULL, 0, NULL);
    errno_assert (rc != -1);
}

xs::handle_t xs::kqueue_t::add_fd (fd_t fd_,
    i_poll_events *reactor_)
{
    poll_entry_t *pe = new (std::nothrow) poll_entry_t;
    alloc_assert (pe);

    pe->fd = fd_;
    pe->flag_pollin = 0;
    pe->flag_pollout = 0;
    pe->reactor = reactor_;

    adjust_load (1);

    return pe;
}

void xs::kqueue_t::rm_fd (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_;
    if (pe->flag_pollin)
        kevent_delete (pe->fd, EVFILT_READ);
    if (pe->flag_pollout)
        kevent_delete (pe->fd, EVFILT_WRITE);
    pe->fd = retired_fd;
    retired.push_back (pe);

    adjust_load (-1);
}

void xs::kqueue_t::set_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_;
    if (likely (!pe->flag_pollin)) {
        pe->flag_pollin = true;
        kevent_add (pe->fd, EVFILT_READ, pe);
    }
}

void xs::kqueue_t::reset_pollin (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_;
    if (likely (pe->flag_pollin)) {
        pe->flag_pollin = false;
        kevent_delete (pe->fd, EVFILT_READ);
    }
}

void xs::kqueue_t::set_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_;
    if (likely (!pe->flag_pollout)) {
        pe->flag_pollout = true;
        kevent_add (pe->fd, EVFILT_WRITE, pe);
    }
}

void xs::kqueue_t::reset_pollout (handle_t handle_)
{
    poll_entry_t *pe = (poll_entry_t*) handle_;
    if (likely (pe->flag_pollout)) {
        pe->flag_pollout = false;
        kevent_delete (pe->fd, EVFILT_WRITE);
   }
}

void xs::kqueue_t::start ()
{
    worker.start (worker_routine, this);
}

void xs::kqueue_t::stop ()
{
    stopping = true;
}

void xs::kqueue_t::loop ()
{
    while (!stopping) {

        //  Execute any due timers.
        int timeout = (int) execute_timers ();

        //  Wait for events.
        struct kevent ev_buf [max_io_events];
        timespec ts = {timeout / 1000, (timeout % 1000) * 1000000};
        int n = kevent (kqueue_fd, NULL, 0, &ev_buf [0], max_io_events,
            timeout ? &ts: NULL);
        if (n == -1 && errno == EINTR)
            continue;
        errno_assert (n != -1);

        for (int i = 0; i < n; i ++) {
            poll_entry_t *pe = (poll_entry_t*) ev_buf [i].udata;

            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].flags & EV_EOF)
                pe->reactor->in_event (pe->fd);
            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].filter == EVFILT_WRITE)
                pe->reactor->out_event (pe->fd);
            if (pe->fd == retired_fd)
                continue;
            if (ev_buf [i].filter == EVFILT_READ)
                pe->reactor->in_event (pe->fd);
        }

        //  Destroy retired event sources.
        for (retired_t::iterator it = retired.begin (); it != retired.end ();
              ++it)
            delete *it;
        retired.clear ();
    }
}

void xs::kqueue_t::worker_routine (void *arg_)
{
    ((kqueue_t*) arg_)->loop ();
}

#endif
