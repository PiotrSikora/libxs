/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

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

#include <stdlib.h>

#include "upoll.hpp"
#include "fd.hpp"
#include "err.hpp"
#include "clock.hpp"
#include "likely.hpp"
#include "platform.hpp"

#if defined XS_FORCE_SELECT
#define XS_POLL_BASED_ON_SELECT
#elif defined XS_FORCE_POLL
#define XS_POLL_BASED_ON_POLL
#elif defined XS_HAVE_LINUX || defined XS_HAVE_FREEBSD ||\
    defined XS_HAVE_OPENBSD || defined XS_HAVE_SOLARIS ||\
    defined XS_HAVE_OSX || defined XS_HAVE_QNXNTO ||\
    defined XS_HAVE_HPUX || defined XS_HAVE_AIX ||\
    defined XS_HAVE_NETBSD
#define XS_POLL_BASED_ON_POLL
#elif defined XS_HAVE_WINDOWS || defined XS_HAVE_OPENVMS ||\
	defined XS_HAVE_CYGWIN
#define XS_POLL_BASED_ON_SELECT
#endif

//  On AIX platform, poll.h has to be included first to get consistent
//  definition of pollfd structure (AIX uses 'reqevents' and 'retnevents'
//  instead of 'events' and 'revents' and defines macros to map from POSIX-y
//  names to AIX-specific names).
#if defined XS_POLL_BASED_ON_POLL
#include <poll.h>
#endif

#if defined XS_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <unistd.h>
#endif

#include <limits.h>

int xs::upoll (xs_pollitem_t *items_, int nitems_, long timeout_)
{
#if defined XS_POLL_BASED_ON_POLL
    if (unlikely (nitems_ < 0 || timeout_ > INT_MAX)) {
        errno = EINVAL;
        return -1;
    }
    if (unlikely (nitems_ == 0)) {
        if (timeout_ == 0)
            return 0;
#if defined XS_HAVE_WINDOWS
        Sleep (timeout_ > 0 ? timeout_ : INFINITE);
        return 0;
#elif defined XS_HAVE_ANDROID
        usleep (timeout_ * 1000);
        return 0;
#else
        return usleep (timeout_ * 1000);
#endif
    }

    if (!items_) {
        errno = EFAULT;
        return -1;
    }

    xs::clock_t clock;
    uint64_t now = 0;
    uint64_t end = 0;

    pollfd *pollfds = (pollfd*) malloc (nitems_ * sizeof (pollfd));
    alloc_assert (pollfds);

    //  Build pollset for poll () system call.
    for (int i = 0; i != nitems_; i++) {

        //  If the poll item is a Crossroads socket, we poll on the file
        //  descriptor retrieved by the XS_FD socket option.
        if (items_ [i].socket) {
            size_t xs_fd_size = sizeof (xs::fd_t);
            if (xs_getsockopt (items_ [i].socket, XS_FD, &pollfds [i].fd,
                &xs_fd_size) == -1) {
                free (pollfds);
                return -1;
            }
            pollfds [i].events = items_ [i].events ? POLLIN : 0;
        }
        //  Else, the poll item is a raw file descriptor. Just convert the
        //  events to normal POLLIN/POLLOUT for poll ().
        else {
            pollfds [i].fd = items_ [i].fd;
            pollfds [i].events =
                (items_ [i].events & XS_POLLIN ? POLLIN : 0) |
                (items_ [i].events & XS_POLLOUT ? POLLOUT : 0);
        }
    }

    bool first_pass = true;
    int nevents = 0;

    while (true) {

         //  Compute the timeout for the subsequent poll.
         int timeout;
         if (first_pass)
             timeout = 0;
         else if (timeout_ < 0)
             timeout = -1;
         else
             timeout = end - now;

        //  Wait for events.
        while (true) {
            int rc = poll (pollfds, nitems_, timeout);
            if (rc == -1 && errno == EINTR) {
                free (pollfds);
                return -1;
            }
            errno_assert (rc >= 0);
            break;
        }

        //  Check for the events.
        for (int i = 0; i != nitems_; i++) {

            items_ [i].revents = 0;

            //  The poll item is a Crossroads socket. Retrieve pending events
            //  using the XS_EVENTS socket option.
            if (items_ [i].socket) {
                size_t xs_events_size = sizeof (uint32_t);
                uint32_t xs_events;
                if (xs_getsockopt (items_ [i].socket, XS_EVENTS, &xs_events,
                    &xs_events_size) == -1) {
                    free (pollfds);
                    return -1;
                }
                if ((items_ [i].events & XS_POLLOUT) &&
                      (xs_events & XS_POLLOUT))
                    items_ [i].revents |= XS_POLLOUT;
                if ((items_ [i].events & XS_POLLIN) &&
                      (xs_events & XS_POLLIN))
                    items_ [i].revents |= XS_POLLIN;
            }
            //  Else, the poll item is a raw file descriptor, simply convert
            //  the events to xs_pollitem_t-style format.
            else {
                if (pollfds [i].revents & POLLIN)
                    items_ [i].revents |= XS_POLLIN;
                if (pollfds [i].revents & POLLOUT)
                    items_ [i].revents |= XS_POLLOUT;
                if (pollfds [i].revents & ~(POLLIN | POLLOUT))
                    items_ [i].revents |= XS_POLLERR;
            }

            if (items_ [i].revents)
                nevents++;
        }

        //  If timout is zero, exit immediately whether there are events or not.
        if (timeout_ == 0)
            break;

        //  If there are events to return, we can exit immediately.
        if (nevents)
            break;

        //  At this point we are meant to wait for events but there are none.
        //  If timeout is infinite we can just loop until we get some events.
        if (timeout_ < 0) {
            if (first_pass)
                first_pass = false;
            continue;
        }

        //  The timeout is finite and there are no events. In the first pass
        //  we get a timestamp of when the polling have begun. (We assume that
        //  first pass have taken negligible time). We also compute the time
        //  when the polling should time out.
        if (first_pass) {
            now = clock.now_ms ();
            end = now + timeout_;
            if (now == end)
                break;
            first_pass = false;
            continue;
        }

        //  Find out whether timeout have expired.
        now = clock.now_ms ();
        if (now >= end)
            break;
    }

    free (pollfds);
    return nevents;

#elif defined XS_POLL_BASED_ON_SELECT

    if (unlikely (nitems_ < 0)) {
        errno = EINVAL;
        return -1;
    }
    if (unlikely (nitems_ == 0)) {
        if (timeout_ == 0)
            return 0;
#if defined XS_HAVE_WINDOWS
        Sleep (timeout_ > 0 ? timeout_ : INFINITE);
        return 0;
#else
        return usleep (timeout_ * 1000);
#endif
    }

    if (!items_) {
        errno = EFAULT;
        return -1;
    }

    xs::clock_t clock;
    uint64_t now = 0;
    uint64_t end = 0;

    //  Ensure we do not attempt to select () on more than FD_SETSIZE
    //  file descriptors.
    xs_assert (nitems_ <= FD_SETSIZE);

    fd_set pollset_in;
    FD_ZERO (&pollset_in);
    fd_set pollset_out;
    FD_ZERO (&pollset_out);
    fd_set pollset_err;
    FD_ZERO (&pollset_err);

    xs::fd_t maxfd = 0;

    //  Build the fd_sets for passing to select ().
    for (int i = 0; i != nitems_; i++) {

        //  If the poll item is a Crossroads socket we are interested in input
        //  on the notification file descriptor retrieved by the XS_FD socket
        //  option.
        if (items_ [i].socket) {
            size_t xs_fd_size = sizeof (xs::fd_t);
            xs::fd_t notify_fd;
            if (xs_getsockopt (items_ [i].socket, XS_FD, &notify_fd,
                &xs_fd_size) == -1)
                return -1;
            if (items_ [i].events) {
                FD_SET (notify_fd, &pollset_in);
                if (maxfd < notify_fd)
                    maxfd = notify_fd;
            }
        }
        //  Else, the poll item is a raw file descriptor. Convert the poll item
        //  events to the appropriate fd_sets.
        else {
            if (items_ [i].events & XS_POLLIN)
                FD_SET (items_ [i].fd, &pollset_in);
            if (items_ [i].events & XS_POLLOUT)
                FD_SET (items_ [i].fd, &pollset_out);
            if (items_ [i].events & XS_POLLERR)
                FD_SET (items_ [i].fd, &pollset_err);
            if (maxfd < items_ [i].fd)
                maxfd = items_ [i].fd;
        }
    }

    bool first_pass = true;
    int nevents = 0;
    fd_set inset, outset, errset;

    while (true) {

        //  Compute the timeout for the subsequent poll.
        timeval timeout;
        timeval *ptimeout;
        if (first_pass) {
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            ptimeout = &timeout;
        }
        else if (timeout_ < 0)
            ptimeout = NULL;
        else {
            timeout.tv_sec = (long) ((end - now) / 1000);
            timeout.tv_usec = (long) ((end - now) % 1000 * 1000);
            ptimeout = &timeout;
        }

        //  Wait for events. Ignore interrupts if there's infinite timeout.
        while (true) {
            memcpy (&inset, &pollset_in, sizeof (fd_set));
            memcpy (&outset, &pollset_out, sizeof (fd_set));
            memcpy (&errset, &pollset_err, sizeof (fd_set));
#if defined XS_HAVE_WINDOWS
            int rc = select (0, &inset, &outset, &errset, ptimeout);
            if (unlikely (rc == SOCKET_ERROR)) {
                xs::wsa_error_to_errno ();
                if (errno == ENOTSOCK)
                    return -1;
                wsa_assert (false);
            }
#else
            int rc = select (maxfd + 1, &inset, &outset, &errset, ptimeout);
            if (unlikely (rc == -1)) {
                if (errno == EINTR || errno == EBADF)
                    return -1;
                errno_assert (false);
            }
#endif
            break;
        }

        //  Check for the events.
        for (int i = 0; i != nitems_; i++) {

            items_ [i].revents = 0;

            //  The poll item is a Crossroads socket. Retrieve pending events
            //  using the XS_EVENTS socket option.
            if (items_ [i].socket) {
                size_t xs_events_size = sizeof (uint32_t);
                uint32_t xs_events;
                if (xs_getsockopt (items_ [i].socket, XS_EVENTS, &xs_events,
                      &xs_events_size) == -1)
                    return -1;
                if ((items_ [i].events & XS_POLLOUT) &&
                      (xs_events & XS_POLLOUT))
                    items_ [i].revents |= XS_POLLOUT;
                if ((items_ [i].events & XS_POLLIN) &&
                      (xs_events & XS_POLLIN))
                    items_ [i].revents |= XS_POLLIN;
            }
            //  Else, the poll item is a raw file descriptor, simply convert
            //  the events to xs_pollitem_t-style format.
            else {
                if (FD_ISSET (items_ [i].fd, &inset))
                    items_ [i].revents |= XS_POLLIN;
                if (FD_ISSET (items_ [i].fd, &outset))
                    items_ [i].revents |= XS_POLLOUT;
                if (FD_ISSET (items_ [i].fd, &errset))
                    items_ [i].revents |= XS_POLLERR;
            }

            if (items_ [i].revents)
                nevents++;
        }

        //  If timout is zero, exit immediately whether there are events or not.
        if (timeout_ == 0)
            break;

        //  If there are events to return, we can exit immediately.
        if (nevents)
            break;

        //  At this point we are meant to wait for events but there are none.
        //  If timeout is infinite we can just loop until we get some events.
        if (timeout_ < 0) {
            if (first_pass)
                first_pass = false;
            continue;
        }

        //  The timeout is finite and there are no events. In the first pass
        //  we get a timestamp of when the polling have begun. (We assume that
        //  first pass have taken negligible time). We also compute the time
        //  when the polling should time out.
        if (first_pass) {
            now = clock.now_ms ();
            end = now + timeout_;
            if (now == end)
                break;
            first_pass = false;
            continue;
        }

        //  Find out whether timeout have expired.
        now = clock.now_ms ();
        if (now >= end)
            break;
    }

    return nevents;

#else
    //  Exotic platforms that support neither poll() nor select().
    errno = ENOTSUP;
    return -1;
#endif
}

#if defined XS_POLL_BASED_ON_SELECT
#undef XS_POLL_BASED_ON_SELECT
#endif
#if defined XS_POLL_BASED_ON_POLL
#undef XS_POLL_BASED_ON_POLL
#endif
