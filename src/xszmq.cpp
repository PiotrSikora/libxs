/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Martin Lucina
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

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

#include "../include/zmq.h"
#include "../include/zmq_utils.h"

#include "../include/xs.h"

#include "platform.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#if !defined XS_HAVE_WINDOWS
#include <unistd.h>
#else
#include "windows.hpp"
#endif

void zmq_version (int *major_, int *minor_, int *patch_)
{
    *major_ = ZMQ_VERSION_MAJOR;
    *minor_ = ZMQ_VERSION_MINOR;
    *patch_ = ZMQ_VERSION_PATCH;
}

int zmq_errno ()
{
    return xs_errno ();
}

const char *zmq_strerror (int errnum)
{
    return xs_strerror (errnum);
}

int zmq_msg_init (zmq_msg_t *msg)
{
    xs_msg_t *content = (xs_msg_t*) malloc (sizeof (xs_msg_t));
    assert (content);
    int rc = xs_msg_init (content);
    if (rc != 0)
        return -1;
    msg->content = (void*) content;
    return 0;
}

int zmq_msg_init_size (zmq_msg_t *msg, size_t size)
{
    xs_msg_t *content = (xs_msg_t*) malloc (sizeof (xs_msg_t));
    assert (content);
    int rc = xs_msg_init_size (content, size);
    if (rc != 0)
        return -1;
    msg->content = (void*) content;
    return 0;
}

int zmq_msg_init_data (zmq_msg_t *msg, void *data,
    size_t size, zmq_free_fn *ffn, void *hint)
{
    xs_msg_t *content = (xs_msg_t*) malloc (sizeof (xs_msg_t));
    assert (content);
    int rc = xs_msg_init_data (content, data, size, ffn, hint);
    if (rc != 0)
        return -1;
    msg->content = (void*) content;
    return 0;
}

int zmq_msg_close (zmq_msg_t *msg)
{
    int rc = xs_msg_close ((xs_msg_t*) msg->content);
    if (rc != 0)
        return -1;
    free (msg->content);
    msg->content = NULL;
    return 0;
}

int zmq_msg_move (zmq_msg_t *dest, zmq_msg_t *src)
{
    return xs_msg_move ((xs_msg_t*) dest->content, (xs_msg_t*) src->content);
}

int zmq_msg_copy (zmq_msg_t *dest, zmq_msg_t *src)
{
    return xs_msg_copy ((xs_msg_t*) dest->content, (xs_msg_t*) src->content);
}

void *zmq_msg_data (zmq_msg_t *msg)
{
    return xs_msg_data ((xs_msg_t*) msg->content);
}

size_t zmq_msg_size (zmq_msg_t *msg)
{
    return xs_msg_size ((xs_msg_t*) msg->content);
}

void *zmq_init (int io_threads)
{
    void *ctx = xs_init ();
    if (!ctx)
        return NULL;

    //  Crossroads don't allow for zero I/O threads.
    if (io_threads < 1)
        io_threads = 1;

    int rc = xs_setctxopt (ctx, XS_IO_THREADS, &io_threads,
        sizeof (io_threads));
    if (rc != 0) {
        xs_term (ctx);
        return NULL;
    }

    return ctx;
}

int zmq_term (void *context)
{
    return xs_term (context);
}

void *zmq_socket (void *context, int type)
{
    return xs_socket (context, type);
}

int zmq_close (void *s)
{
    return xs_close (s);
}

int zmq_setsockopt (void *s, int option, const void *optval,
    size_t optvallen)
{
    switch (option) {

    case ZMQ_AFFINITY:
    case ZMQ_IDENTITY:
    case ZMQ_SUBSCRIBE:
    case ZMQ_UNSUBSCRIBE:
    case ZMQ_LINGER:
    case ZMQ_RECONNECT_IVL:
    case ZMQ_RECONNECT_IVL_MAX:
    case ZMQ_BACKLOG:
        return xs_setsockopt (s, option, optval, optvallen);

    case ZMQ_HWM:
    {
        if (optvallen != sizeof (uint64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val = (int) *(uint64_t*) optval;
        int rc = xs_setsockopt (s, XS_SNDHWM, &val, sizeof (int));
        if (rc < 0)
            return -1;
        return xs_setsockopt (s, XS_RCVHWM, &val, sizeof (int));
    }

    case ZMQ_RATE:
    {
        if (optvallen != sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val = (int) *(int64_t*) optval;
        return xs_setsockopt (s, option, &val, sizeof (int));
    }

    case ZMQ_RECOVERY_IVL:
    {
        if (optvallen != sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val = ((int) *(int64_t*) optval) * 1000;
        return xs_setsockopt (s, option, &val, sizeof (int));
    }

    case ZMQ_RECOVERY_IVL_MSEC:
    {
        if (optvallen != sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val = (int) *(int64_t*) optval;
        return xs_setsockopt (s, option, &val, sizeof (int));
    }

    case ZMQ_SNDBUF:
    case ZMQ_RCVBUF:
    {
        if (optvallen != sizeof (uint64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val = (int) *(uint64_t*) optval;
        return xs_setsockopt (s, option, &val, sizeof (int));
    }

    default:
        errno = EINVAL;
        return -1;
    }
}

int zmq_getsockopt (void *s, int option, void *optval,
    size_t *optvallen)
{
    switch (option)
    {
    case ZMQ_TYPE:
    case ZMQ_AFFINITY:
    case ZMQ_IDENTITY:
    case ZMQ_FD:
    case ZMQ_BACKLOG:
    case ZMQ_LINGER:
    case ZMQ_RECONNECT_IVL:
    case ZMQ_RECONNECT_IVL_MAX:
        return xs_getsockopt (s, option, optval, optvallen);

    case ZMQ_RCVMORE:
    case ZMQ_RATE:
    {
        if (!optvallen || *optvallen < sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, option, &val, &size);
        if (rc < 0)
            return -1;
        assert (size == sizeof (int));
        *(int64_t*) optval = (int64_t) val;
        *optvallen = sizeof (int64_t);
        return 0;
    }

    case ZMQ_HWM:
    {
        if (!optvallen || *optvallen < sizeof (uint64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, XS_SNDHWM, &val, &size);
        if (rc < 0)
            return -1;
        assert (size == sizeof (int));
        *(uint64_t*) optval = (uint64_t) val;
        *optvallen = sizeof (uint64_t);
        return 0;
    }

    case ZMQ_RECOVERY_IVL:
    {
        if (!optvallen || *optvallen < sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, option, &val, &size);
        if (rc < 0)
            return -1;
        val /= 1000;
        assert (size == sizeof (int));
        *(int64_t*) optval = (int64_t) val;
        *optvallen = sizeof (int64_t);
        return 0;
    }

    case ZMQ_RECOVERY_IVL_MSEC:
    {
        if (!optvallen || *optvallen < sizeof (int64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, option, &val, &size);
        if (rc < 0)
            return -1;
        assert (size == sizeof (int));
        *(int64_t*) optval = (int64_t) val;
        *optvallen = sizeof (int64_t);
        return 0;
    }

    case ZMQ_SNDBUF:
    case ZMQ_RCVBUF:
    {
        if (!optvallen || *optvallen < sizeof (uint64_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, option, &val, &size);
        if (rc < 0)
            return -1;
        assert (size == sizeof (int));
        *(int64_t*) optval = (uint64_t) val;
        *optvallen = sizeof (uint64_t);
        return 0;
    }

    case ZMQ_EVENTS:
    {
        if (!optvallen || *optvallen < sizeof (uint32_t)) {
            errno = EINVAL;
            return -1;
        }
        int val;
        size_t size;
        int rc = xs_getsockopt (s, option, &val, &size);
        if (rc < 0)
            return -1;
        assert (size == sizeof (int));
        *(int32_t*) optval = (uint32_t) val;
        *optvallen = sizeof (uint32_t);
        return 0;
    }

    default:
        errno = EINVAL;
        return -1;
    }
}

int zmq_bind (void *s, const char *addr)
{
    return xs_bind (s, addr);
}

int zmq_connect (void *s, const char *addr)
{
    return xs_connect (s, addr);
}

int zmq_send (void *s, zmq_msg_t *msg, int flags)
{
    int rc = xs_sendmsg (s, (xs_msg_t*) msg->content, flags);
    return rc < 0 ? -1 : 0;
}

int zmq_recv (void *s, zmq_msg_t *msg, int flags)
{
    int rc = xs_recvmsg (s, (xs_msg_t*) msg->content, flags);
    return rc < 0 ? -1 : 0;
}

int zmq_poll (zmq_pollitem_t *items, int nitems, long timeout)
{
    return xs_poll ((xs_pollitem_t*) items, nitems, timeout / 1000);
}

int zmq_device (int device, void *frontend, void *backend)
{
    int more;
    size_t size;
    xs_msg_t msg;
    xs_pollitem_t items [2];

    int rc = xs_msg_init (&msg);
    if (rc != 0)
        return -1;

    items [0].socket = frontend;
    items [0].events = XS_POLLIN;
    items [1].socket = backend;
    items [1].events = XS_POLLIN;

    while (1) {

        rc = xs_poll (&items [0], 2, -1);
        if (rc < 0)
            return -1;

        if (items [0].revents & XS_POLLIN) {
            while (1) {

                rc = xs_recvmsg (frontend, &msg, 0);
                if (rc < 0)
                    return -1;

                size = sizeof (more);
                rc = xs_getsockopt (frontend, XS_RCVMORE, &more, &size);
                if (rc < 0)
                    return -1;

                rc = xs_sendmsg (backend, &msg, more ? XS_SNDMORE : 0);
                if (rc < 0)
                    return -1;

                if (!more)
                    break;
            }
        }

        if (items [1].revents & XS_POLLIN) {
            while (1) {

                rc = xs_recvmsg (backend, &msg, 0);
                if (rc < 0)
                    return -1;

                size = sizeof (more);
                rc = xs_getsockopt (backend, XS_RCVMORE, &more, &size);
                if (rc < 0)
                    return -1;

                rc = xs_sendmsg (frontend, &msg, more ? ZMQ_SNDMORE : 0);
                if (rc < 0)
                    return -1;

                if (!more)
                    break;
            }
        }
    }

    return 0;
}

void *zmq_stopwatch_start ()
{
    return xs_stopwatch_start ();
}

unsigned long zmq_stopwatch_stop (void *watch)
{
    return xs_stopwatch_stop (watch);
}

void zmq_sleep (int seconds_)
{
#if defined XS_HAVE_WINDOWS
    Sleep (seconds_ * 1000);
#else
    sleep (seconds_);
#endif
}

