/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
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

#include "../include/xs_utils.h"

#include <stdlib.h>

#include "stdint.hpp"
#include "clock.hpp"
#include "err.hpp"

#if !defined XS_HAVE_WINDOWS
#include <unistd.h>
#include <pthread.h>
#else
#include "windows.hpp"
#endif

void xs_sleep (int seconds_)
{
#if defined XS_HAVE_WINDOWS
    Sleep (seconds_ * 1000);
#else
    sleep (seconds_);
#endif
}

void *xs_stopwatch_start ()
{
    uint64_t *watch = (uint64_t*) malloc (sizeof (uint64_t));
    alloc_assert (watch);
    *watch = xs::clock_t::now_us ();
    return (void*) watch;
}

unsigned long xs_stopwatch_stop (void *watch_)
{
    uint64_t end = xs::clock_t::now_us ();
    uint64_t start = *(uint64_t*) watch_;
    free (watch_);
    return (unsigned long) (end - start);
}

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

void *xs_thread_create (void (*fn_) (void *arg_), void *arg_)
{
    arg_t *arg = (arg_t*) malloc (sizeof (arg_t));
    alloc_assert (arg);
    arg->fn = fn_;
    arg->arg = arg_;
    arg->handle = (HANDLE) _beginthreadex (NULL, 0,
        &::thread_routine, (void*) arg, 0 , NULL);
    win_assert (arg->handle != NULL);
    return (void*) arg;
}

void xs_thread_join (void *thread_)
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

void *xs_thread_create (void (*fn_) (void *arg_), void *arg_)
{
    arg_t *arg = (arg_t*) malloc (sizeof (arg_t));
    alloc_assert (arg);
    arg->fn = fn_;
    arg->arg = arg_;
    int rc = pthread_create (&arg->handle, NULL, thread_routine, (void*) arg);
    posix_assert (rc);
    return (void*) arg;
}

void xs_thread_join (void *thread_)
{
    arg_t *arg = (arg_t*) thread_;
    int rc = pthread_join (arg->handle, NULL);
    posix_assert (rc);
    free (arg);
}

#endif
