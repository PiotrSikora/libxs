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

#ifndef __XS_TEST_TESTUTIL_HPP_INCLUDED__
#define __XS_TEST_TESTUTIL_HPP_INCLUDED__

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "../include/xs.h"
#include "../include/xs_utils.h"
#include "../src/platform.hpp"

#if !defined XS_HAVE_WINDOWS
#include <unistd.h>
#include <pthread.h>
#else
#include "../src/windows.hpp"
#endif

#if !defined XS_TEST_MAIN
#define XS_TEST_MAIN main
#endif

#if defined XS_HAVE_WINDOWS
#define sleep(s) Sleep ((s) * 1000)
#endif

#if defined XS_HAVE_WINDOWS

struct arg_t
{
    HANDLE handle;
    void (*fn) (void *arg);
    void *arg;
};

extern "C"
{
    static unsigned int __stdcall thread_routine (void *arg_)
    {
        arg_t *arg = (arg_t*) arg_;
        arg->fn (arg->arg);
        return 0;
    }
}

void *thread_create (void (*fn_) (void *arg_), void *arg_)
{
    arg_t *arg = (arg_t*) malloc (sizeof (arg_t));
    assert (arg);
    arg->fn = fn_;
    arg->arg = arg_;
    arg->handle = (HANDLE) _beginthreadex (NULL, 0,
        &::thread_routine, (void*) arg, 0 , NULL);
    win_assert (arg->handle != NULL);
    return (void*) arg;
}

void thread_join (void *thread_)
{
    arg_t *arg = (arg_t*) thread_;
    DWORD rc = WaitForSingleObject (arg->handle, INFINITE);
    win_assert (rc != WAIT_FAILED);
    BOOL rc2 = CloseHandle (arg->handle);
    win_assert (rc2 != 0);
    free (arg);
}

#else

struct arg_t
{
    pthread_t handle;
    void (*fn) (void *arg);
    void *arg;
};

extern "C"
{
    static void *thread_routine (void *arg_)
    {
        arg_t *arg = (arg_t*) arg_;
        arg->fn (arg->arg);
        return NULL;
    }
}

void *thread_create (void (*fn_) (void *arg_), void *arg_)
{
    arg_t *arg = (arg_t*) malloc (sizeof (arg_t));
    assert (arg);
    arg->fn = fn_;
    arg->arg = arg_;
    int rc = pthread_create (&arg->handle, NULL, thread_routine, (void*) arg);
    assert (rc == 0);
    return (void*) arg;
}

void thread_join (void *thread_)
{
    arg_t *arg = (arg_t*) thread_;
    int rc = pthread_join (arg->handle, NULL);
    assert (rc == 0);
    free (arg);
}

#endif

inline void bounce (void *sb, void *sc)
{
    const char *content = "12345678ABCDEFGH12345678abcdefgh";

    //  Send the message.
    int rc = xs_send (sc, content, 32, XS_SNDMORE);
    assert (rc == 32);
    rc = xs_send (sc, content, 32, 0);
    assert (rc == 32);

    //  Bounce the message back.
    char buf1 [32];
    rc = xs_recv (sb, buf1, 32, 0);
    assert (rc == 32);
    int rcvmore;
    size_t sz = sizeof (rcvmore);
    rc = xs_getsockopt (sb, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (rcvmore);
    rc = xs_recv (sb, buf1, 32, 0);
    assert (rc == 32);
    rc = xs_getsockopt (sb, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (!rcvmore);
    rc = xs_send (sb, buf1, 32, XS_SNDMORE);
    assert (rc == 32);
    rc = xs_send (sb, buf1, 32, 0);
    assert (rc == 32);

    //  Receive the bounced message.
    char buf2 [32];
    rc = xs_recv (sc, buf2, 32, 0);
    assert (rc == 32);
    rc = xs_getsockopt (sc, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (rcvmore);
    rc = xs_recv (sc, buf2, 32, 0);
    assert (rc == 32);
    rc = xs_getsockopt (sc, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (!rcvmore);

    //  Check whether the message is still the same.
    assert (memcmp (buf2, content, 32) == 0);
}

#endif
