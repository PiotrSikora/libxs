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
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv [])
{
    const char *bind_to;
    int message_count;
    size_t message_size;
    void *ctx;
    void *s;
    int rc;
    int i;
    xs_msg_t msg;
    void *watch;
    unsigned long elapsed;
    unsigned long throughput;
    double megabits;

    if (argc != 4) {
        printf ("usage: local_thr <bind-to> <message-size> <message-count>\n");
        return 1;
    }
    bind_to = argv [1];
    message_size = atoi (argv [2]);
    message_count = atoi (argv [3]);

    ctx = xs_init ();
    if (!ctx) {
        printf ("error in xs_init: %s\n", xs_strerror (errno));
        return -1;
    }

    s = xs_socket (ctx, XS_PULL);
    if (!s) {
        printf ("error in xs_socket: %s\n", xs_strerror (errno));
        return -1;
    }

    //  Add your socket options here.
    //  For example XS_RATE, XS_RECOVERY_IVL and XS_MCAST_LOOP for PGM.

    rc = xs_bind (s, bind_to);
    if (rc != 0) {
        printf ("error in xs_bind: %s\n", xs_strerror (errno));
        return -1;
    }

    rc = xs_msg_init (&msg);
    if (rc != 0) {
        printf ("error in xs_msg_init: %s\n", xs_strerror (errno));
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

    watch = xs_stopwatch_start ();

    for (i = 0; i != message_count - 1; i++) {
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
    if (elapsed == 0)
        elapsed = 1;

    rc = xs_msg_close (&msg);
    if (rc != 0) {
        printf ("error in xs_msg_close: %s\n", xs_strerror (errno));
        return -1;
    }

    throughput = (unsigned long)
        ((double) message_count / (double) elapsed * 1000000);
    megabits = (double) (throughput * message_size * 8) / 1000000;

    printf ("message size: %d [B]\n", (int) message_size);
    printf ("message count: %d\n", (int) message_count);
    printf ("mean throughput: %d [msg/s]\n", (int) throughput);
    printf ("mean throughput: %.3f [Mb/s]\n", (double) megabits);

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
