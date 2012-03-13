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

#include "../include/xs.h"
#include "../include/xs_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/platform.hpp"

#if defined XS_HAVE_WINDOWS
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

static size_t message_size;
static int roundtrip_count;

#if defined XS_HAVE_WINDOWS
static unsigned int __stdcall worker (void *ctx_)
#else
static void *worker (void *ctx_)
#endif
{
    void *s;
    int rc;
    int i;
    xs_msg_t msg;

    s = xs_socket (ctx_, XS_REP);
    if (!s) {
        printf ("error in xs_socket: %s\n", xs_strerror (errno));
        exit (1);
    }

    rc = xs_connect (s, "inproc://lat_test");
    if (rc != 0) {
        printf ("error in xs_connect: %s\n", xs_strerror (errno));
        exit (1);
    }

    rc = xs_msg_init (&msg);
    if (rc != 0) {
        printf ("error in xs_msg_init: %s\n", xs_strerror (errno));
        exit (1);
    }

    for (i = 0; i != roundtrip_count; i++) {
        rc = xs_recvmsg (s, &msg, 0);
        if (rc < 0) {
            printf ("error in xs_recvmsg: %s\n", xs_strerror (errno));
            exit (1);
        }
        rc = xs_sendmsg (s, &msg, 0);
        if (rc < 0) {
            printf ("error in xs_sendmsg: %s\n", xs_strerror (errno));
            exit (1);
        }
    }

    rc = xs_msg_close (&msg);
    if (rc != 0) {
        printf ("error in xs_msg_close: %s\n", xs_strerror (errno));
        exit (1);
    }

    rc = xs_close (s);
    if (rc != 0) {
        printf ("error in xs_close: %s\n", xs_strerror (errno));
        exit (1);
    }

#if defined XS_HAVE_WINDOWS
    return 0;
#else
    return NULL;
#endif
}

int main (int argc, char *argv [])
{
#if defined XS_HAVE_WINDOWS
    HANDLE local_thread;
#else
    pthread_t local_thread;
#endif
    void *ctx;
    void *s;
    int rc;
    int i;
    xs_msg_t msg;
    void *watch;
    unsigned long elapsed;
    double latency;

    if (argc != 3) {
        printf ("usage: inproc_lat <message-size> <roundtrip-count>\n");
        return 1;
    }

    message_size = atoi (argv [1]);
    roundtrip_count = atoi (argv [2]);

    ctx = xs_init ();
    if (!ctx) {
        printf ("error in xs_init: %s\n", xs_strerror (errno));
        return -1;
    }

    s = xs_socket (ctx, XS_REQ);
    if (!s) {
        printf ("error in xs_socket: %s\n", xs_strerror (errno));
        return -1;
    }

    rc = xs_bind (s, "inproc://lat_test");
    if (rc != 0) {
        printf ("error in xs_bind: %s\n", xs_strerror (errno));
        return -1;
    }

#if defined XS_HAVE_WINDOWS
    local_thread = (HANDLE) _beginthreadex (NULL, 0,
        worker, ctx, 0 , NULL);
    if (local_thread == 0) {
        printf ("error in _beginthreadex\n");
        return -1;
    }
#else
    rc = pthread_create (&local_thread, NULL, worker, ctx);
    if (rc != 0) {
        printf ("error in pthread_create: %s\n", xs_strerror (rc));
        return -1;
    }
#endif

    rc = xs_msg_init_size (&msg, message_size);
    if (rc != 0) {
        printf ("error in xs_msg_init_size: %s\n", xs_strerror (errno));
        return -1;
    }
    memset (xs_msg_data (&msg), 0, message_size);

    printf ("message size: %d [B]\n", (int) message_size);
    printf ("roundtrip count: %d\n", (int) roundtrip_count);

    watch = xs_stopwatch_start ();

    for (i = 0; i != roundtrip_count; i++) {
        rc = xs_sendmsg (s, &msg, 0);
        if (rc < 0) {
            printf ("error in xs_sendmsg: %s\n", xs_strerror (errno));
            return -1;
        }
        rc = xs_recvmsg (s, &msg, 0);
        if (rc < 0) {
            printf ("error in xs_recvmsg: %s\n", xs_strerror (errno));
            return -1;
        }
        if (xs_msg_size (&msg) != message_size) {
            printf ("message of incorrect size received\n");
            return -1;
        }
    }

    elapsed = xs_stopwatch_stop (watch);

    rc = xs_msg_close (&msg);
    if (rc != 0) {
        printf ("error in xs_msg_close: %s\n", xs_strerror (errno));
        return -1;
    }

    latency = (double) elapsed / (roundtrip_count * 2);

#if defined XS_HAVE_WINDOWS
    DWORD rc2 = WaitForSingleObject (local_thread, INFINITE);
    if (rc2 == WAIT_FAILED) {
        printf ("error in WaitForSingleObject\n");
        return -1;
    }
    BOOL rc3 = CloseHandle (local_thread);
    if (rc3 == 0) {
        printf ("error in CloseHandle\n");
        return -1;
    }
#else
    rc = pthread_join (local_thread, NULL);
    if (rc != 0) {
        printf ("error in pthread_join: %s\n", xs_strerror (rc));
        return -1;
    }
#endif

    printf ("average latency: %.3f [us]\n", (double) latency);

    rc = xs_close (s);
    if (rc != 0) {
        printf ("error in xs_close: %s\n", xs_strerror (errno));
        return -1;
    }

    rc = xs_term (ctx);
    if (rc != 0) {
        printf ("error in xs_term: %s\n", xs_strerror (errno));
        return -1;
    }

    return 0;
}

