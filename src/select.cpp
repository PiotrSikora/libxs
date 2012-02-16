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

#include "select.hpp"

#if defined XS_HAVE_SELECT

#include "platform.hpp"
#if defined XS_HAVE_WINDOWS
#include "windows.hpp"
#elif defined XS_HAVE_HPUX
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#elif defined XS_HAVE_OPENVMS
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include <string.h>
#include <algorithm>

#include "err.hpp"
#include "config.hpp"

xs::select_t::select_t () :
    maxfd (retired_fd),
    retired (false),
    stopping (false)
{
    //  Clear file descriptor sets.
    FD_ZERO (&source_set_in);
    FD_ZERO (&source_set_out);
    FD_ZERO (&source_set_err);
}

xs::select_t::~select_t ()
{
    worker.stop ();
}

xs::handle_t xs::select_t::add_fd (fd_t fd_, i_poll_events *events_)
{
    //  Store the file descriptor.
    fd_entry_t entry = {fd_, events_};
    fds.push_back (entry);

    //  Ensure we do not attempt to select () on more than FD_SETSIZE
    //  file descriptors.
    xs_assert (fds.size () <= FD_SETSIZE);

    //  Start polling on errors.
    FD_SET (fd_, &source_set_err);

    //  Adjust maxfd if necessary.
    if (fd_ > maxfd)
        maxfd = fd_;

    //  Increase the load metric of the thread.
    adjust_load (1);

    return fdtoptr (fd_);
}

void xs::select_t::rm_fd (handle_t handle_)
{
    int fd = ptrtofd (handle_);

    //  Mark the descriptor as retired.
    fd_set_t::iterator it;
    for (it = fds.begin (); it != fds.end (); ++it)
        if (it->fd == fd)
            break;
    xs_assert (it != fds.end ());
    it->fd = retired_fd;
    retired = true;

    //  Stop polling on the descriptor.
    FD_CLR (fd, &source_set_in);
    FD_CLR (fd, &source_set_out);
    FD_CLR (fd, &source_set_err);

    //  Discard all events generated on this file descriptor.
    FD_CLR (fd, &readfds);
    FD_CLR (fd, &writefds);
    FD_CLR (fd, &exceptfds);

    //  Adjust the maxfd attribute if we have removed the
    //  highest-numbered file descriptor.
    if (fd == maxfd) {
        maxfd = retired_fd;
        for (fd_set_t::iterator it = fds.begin (); it != fds.end (); ++it)
            if (it->fd > maxfd)
                maxfd = it->fd;
    }

    //  Decrease the load metric of the thread.
    adjust_load (-1);
}

void xs::select_t::set_pollin (handle_t handle_)
{
    FD_SET (ptrtofd (handle_), &source_set_in);
}

void xs::select_t::reset_pollin (handle_t handle_)
{
    FD_CLR (ptrtofd (handle_), &source_set_in);
}

void xs::select_t::set_pollout (handle_t handle_)
{
    FD_SET (ptrtofd (handle_), &source_set_out);
}

void xs::select_t::reset_pollout (handle_t handle_)
{
    FD_CLR (ptrtofd (handle_), &source_set_out);
}

void xs::select_t::xstart ()
{
    worker.start (worker_routine, this);
}

void xs::select_t::xstop ()
{
    stopping = true;
}

void xs::select_t::loop ()
{
    while (!stopping) {

        //  Execute any due timers.
        int timeout = (int) execute_timers ();

        //  Intialise the pollsets.
        memcpy (&readfds, &source_set_in, sizeof source_set_in);
        memcpy (&writefds, &source_set_out, sizeof source_set_out);
        memcpy (&exceptfds, &source_set_err, sizeof source_set_err);

        //  Wait for events.
        struct timeval tv = {(long) (timeout / 1000),
            (long) (timeout % 1000 * 1000)};
#ifdef XS_HAVE_WINDOWS
        int rc = select (0, &readfds, &writefds, &exceptfds,
            timeout ? &tv : NULL);
        wsa_assert (rc != SOCKET_ERROR);
#else
        int rc = select (maxfd + 1, &readfds, &writefds, &exceptfds,
            timeout ? &tv : NULL);
        if (rc == -1 && errno == EINTR)
            continue;
        errno_assert (rc != -1);
#endif

        //  If there are no events (i.e. it's a timeout) there's no point
        //  in checking the pollset.
        if (rc == 0)
            continue;

        for (fd_set_t::size_type i = 0; i < fds.size (); i ++) {
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &exceptfds))
                fds [i].events->in_event (fds [i].fd);
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &writefds))
                fds [i].events->out_event (fds [i].fd);
            if (fds [i].fd == retired_fd)
                continue;
            if (FD_ISSET (fds [i].fd, &readfds))
                fds [i].events->in_event (fds [i].fd);
        }

        //  Destroy retired event sources.
        if (retired) {
            fds.erase (std::remove_if (fds.begin (), fds.end (),
                xs::select_t::is_retired_fd), fds.end ());
            retired = false;
        }
    }
}

void xs::select_t::worker_routine (void *arg_)
{
    ((select_t*) arg_)->loop ();
}

bool xs::select_t::is_retired_fd (const fd_entry_t &entry)
{
    return (entry.fd == retired_fd);
}

#endif
