/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
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

#include "platform.hpp"

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
#include "upoll.hpp"
#include "ctx.hpp"
#include "err.hpp"
#include "msg.hpp"
#include "fd.hpp"

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

int xs_plug (void *ctx_, void *ext_)
{
    if (!ctx_ || !((xs::ctx_t*) ctx_)->check_tag ()) {
        errno = EFAULT;
        return -1;
    }

    return ((xs::ctx_t*) ctx_)->plug (ext_);
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
    int rc = s->setsockopt (option_, optval_, optvallen_);
    return rc;
}

int xs_getsockopt (void *s_, int option_, void *optval_, size_t *optvallen_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    int rc = s->getsockopt (option_, optval_, optvallen_);
    return rc;
}

int xs_bind (void *s_, const char *addr_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    int rc = s->bind (addr_);
    return rc;
}

int xs_connect (void *s_, const char *addr_)
{
    xs::socket_base_t *s = (xs::socket_base_t*) s_;
    if (!s || !s->check_tag ()) {
        errno = ENOTSOCK;
        return -1;
    }
    int rc = s->connect (addr_);
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
    int rc = s->send ((xs::msg_t*) msg_, flags_);
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
    int rc = s->recv ((xs::msg_t*) msg_, flags_);
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
    return xs::upoll (items_, nitems_, timeout_);
}

int xs_errno ()
{
    return errno;
}


