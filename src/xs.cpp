/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <new>

#include "socket_base.hpp"
#include "stdint.hpp"
#include "config.hpp"
#include "likely.hpp"
#include "clock.hpp"
#include "ctx.hpp"
#include "err.hpp"
#include "msg.hpp"
#include "fd.hpp"

#if !defined XS_HAVE_WINDOWS
#include <unistd.h>
#endif

#if defined XS_HAVE_OPENPGM
#define __PGM_WININT_H__
#include <pgm/pgm.h>
#endif

//  Compile time check whether msg_t fits into xs_msg_t.
typedef char check_msg_t_size
    [sizeof (xs::msg_t) ==  sizeof (xs_msg_t) ? 1 : -1];

void xs_version (int *major_, int *minor_, int *patch_)
{
    *major_ = XS_VERSION_MAJOR;
    *minor_ = XS_VERSION_MINOR;
    *patch_ = XS_VERSION_PATCH;
}

const char *xs_strerror (int errnum_)
{
    return xs::errno_to_string (errnum_);
}

void *xs_init (int io_threads_)
{
    //  We need at least one I/O thread to run the monitor object in.
    if (io_threads_ < 1) {
        errno = EINVAL;
        return NULL;
    }

#if defined XS_HAVE_OPENPGM

    //  Init PGM transport. Ensure threading and timer are enabled. Find PGM
    //  protocol ID. Note that if you want to use gettimeofday and sleep for
    //  openPGM timing, set environment variables PGM_TIMER to "GTOD" and
    //  PGM_SLEEP to "USLEEP".
    pgm_error_t *pgm_error = NULL;
    const bool ok = pgm_init (&pgm_error);
    if (ok != TRUE) {

        //  Invalid parameters don't set pgm_error_t
        xs_assert (pgm_error != NULL);
        if (pgm_error->domain == PGM_ERROR_DOMAIN_TIME && (
              pgm_error->code == PGM_ERROR_FAILED)) {

            //  Failed to access RTC or HPET device.
            pgm_error_free (pgm_error);
            errno = EINVAL;
            return NULL;
        }

        //  PGM_ERROR_DOMAIN_ENGINE: WSAStartup errors or missing WSARecvMsg.
        xs_assert (false);
    }
#endif

#ifdef XS_HAVE_WINDOWS
    //  Intialise Windows sockets. Note that WSAStartup can be called multiple
    //  times given that WSACleanup will be called for each WSAStartup.
   //  We do this before the ctx constructor since its embedded mailbox_t
   //  object needs Winsock to be up and running.
    WORD version_requested = MAKEWORD (2, 2);
    WSADATA wsa_data;
    int rc = WSAStartup (version_requested, &wsa_data);
    xs_assert (rc == 0);
    xs_assert (LOBYTE (wsa_data.wVersion) == 2 &&
        HIBYTE (wsa_data.wVersion) == 2);
#endif

    //  Create the context.
    xs::ctx_t *ctx = new (std::nothrow) xs::ctx_t ((uint32_t) io_threads_);
    alloc_assert (ctx);
    return (void*) ctx;
}

int xs_term (void *ctx_)
{
    if (!ctx_ || !((xs::ctx_t*) ctx_)->check_tag ()) {
        errno = EFAULT;
        return -1;
    }

    int rc = ((xs::ctx_t*) ctx_)->terminate ();
    int en = errno;

#ifdef XS_HAVE_WINDOWS
    //  On Windows, uninitialise socket layer.
    rc = WSACleanup ();
    wsa_assert (rc != SOCKET_ERROR);
#endif

#if defined XS_HAVE_OPENPGM
    //  Shut down the OpenPGM library.
    if (pgm_shutdown () != TRUE)
        xs_assert (false);
#endif

    errno = en;
    return rc;
}

int xs_setctxopt (void *ctx_, int option_, const void *optval_,
    size_t optvallen_)
{
    if (!ctx_ || !((xs::ctx_t*) ctx_)->check_tag ()) {
        errno = EFAULT;
        return -1;
    }

    return ((xs::ctx_t*) ctx_)->setctxopt (option_, optval_, optvallen_);
}

void *xs_socket (void *ctx_, int type_)
{
    if (!ctx_ || !((xs::ctx_t*) ctx_)->check_tag ()) {
        errno = EFAULT;
        return NULL;
    }
    return (void*) (((xs::ctx_t*) ctx_)->create_socket (type_));
}

int xs_close (void *s_)
{
    if (!s_ || !((xs::socket_base_t*) s_)->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    ((xs::socket_base_t*) s_)->close ();
    return 0;
}

int xs_setsockopt (void *s_, int option_, const void *optval_,
    size_t optvallen_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    s->lock ();
    int rc = s->setsockopt (option_, optval_, optvallen_);
    s->unlock ();
    return rc;
}

int xs_getsockopt (void *s_, int option_, void *optval_, size_t *optvallen_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    s->lock ();
    int rc = s->getsockopt (option_, optval_, optvallen_);
    s->unlock ();
    return rc;
}

int xs_bind (void *s_, const char *addr_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    s->lock ();
    int rc = s->bind (addr_);
    s->unlock ();
    return rc;
}

int xs_connect (void *s_, const char *addr_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    s->lock ();
    int rc = s->connect (addr_);
    s->unlock ();
    return rc;
}

int xs_send (void *s_, const void *buf_, size_t len_, int flags_)
{
    xs_msg_t msg;
    int rc = xs_msg_init_size (&msg, len_);
    if (rc != 0)
        return -1;
    memcpy (xs_msg_data (&msg), buf_, len_);

    rc = xs_sendmsg (s_, &msg, flags_);
    if (unlikely (rc < 0)) {
        int err = errno;
        int rc2 = xs_msg_close (&msg);
        errno_assert (rc2 == 0);
        errno = err;
        return -1;
    }
    
    //  Note the optimisation here. We don't close the msg object as it is
    //  empty anyway. This may change when implementation of xs_msg_t changes.
    return rc;
}

int xs_recv (void *s_, void *buf_, size_t len_, int flags_)
{
    xs_msg_t msg;
    int rc = xs_msg_init (&msg);
    errno_assert (rc == 0);

    int nbytes = xs_recvmsg (s_, &msg, flags_);
    if (unlikely (nbytes < 0)) {
        int err = errno;
        rc = xs_msg_close (&msg);
        errno_assert (rc == 0);
        errno = err;
        return -1;
    }

    //  At the moment an oversized message is silently truncated.
    //  TODO: Build in a notification mechanism to report the overflows.
    size_t to_copy = size_t (nbytes) < len_ ? size_t (nbytes) : len_;
    memcpy (buf_, xs_msg_data (&msg), to_copy);

    rc = xs_msg_close (&msg);
    errno_assert (rc == 0);

    return nbytes;    
}

int xs_sendmsg (void *s_, xs_msg_t *msg_, int flags_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    int sz = (int) xs_msg_size (msg_);
    s->lock ();
    int rc = s->send ((xs::msg_t*) msg_, flags_);
    s->unlock ();
    if (unlikely (rc < 0))
        return -1;
    return sz;
}

int xs_recvmsg (void *s_, xs_msg_t *msg_, int flags_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    s->lock ();
    int rc = s->recv ((xs::msg_t*) msg_, flags_);
    s->unlock ();
    if (unlikely (rc < 0))
        return -1;
    return (int) xs_msg_size (msg_);
}

int xs_msg_init (xs_msg_t *msg_)
{
    return ((xs::msg_t*) msg_)->init ();
}

int xs_msg_init_size (xs_msg_t *msg_, size_t size_)
{
    return ((xs::msg_t*) msg_)->init_size (size_);
}

int xs_msg_init_data (xs_msg_t *msg_, void *data_, size_t size_,
    xs_free_fn *ffn_, void *hint_)
{
    return ((xs::msg_t*) msg_)->init_data (data_, size_, ffn_, hint_);
}

int xs_msg_close (xs_msg_t *msg_)
{
    return ((xs::msg_t*) msg_)->close ();
}

int xs_msg_move (xs_msg_t *dest_, xs_msg_t *src_)
{
    return ((xs::msg_t*) dest_)->move (*(xs::msg_t*) src_);
}

int xs_msg_copy (xs_msg_t *dest_, xs_msg_t *src_)
{
    return ((xs::msg_t*) dest_)->copy (*(xs::msg_t*) src_);
}

void *xs_msg_data (xs_msg_t *msg_)
{
    return ((xs::msg_t*) msg_)->data ();
}

size_t xs_msg_size (xs_msg_t *msg_)
{
    return ((xs::msg_t*) msg_)->size ();
}

int xs_getmsgopt (xs_msg_t *msg_, int option_, void *optval_,
    size_t *optvallen_)
{
    switch (option_) {
    case XS_MORE:
        if (*optvallen_ < sizeof (int)) {
            errno = EINVAL;
            return -1;
        }
        *((int*) optval_) =
            (((xs::msg_t*) msg_)->flags () & xs::msg_t::more) ? 1 : 0;
        *optvallen_ = sizeof (int);
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}

int xs_poll (xs_pollitem_t *items_, int nitems_, long timeout_)
{
#if defined XS_POLL_BASED_ON_POLL
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

int xs_errno ()
{
    return errno;
}

#if defined XS_POLL_BASED_ON_SELECT
#undef XS_POLL_BASED_ON_SELECT
#endif
#if defined XS_POLL_BASED_ON_POLL
#undef XS_POLL_BASED_ON_POLL
#endif

