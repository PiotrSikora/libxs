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

#include "../include/xs.h"
#include "../include/xs_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv [])
{
    const char *connect_to;
    int roundtrip_count;
    size_t message_size;
    void *ctx;
    void *s;
    int rc;
    int i;
    xs_msg_t msg;
    void *watch;
    unsigned long elapsed;
    double latency;

    if (argc != 4) {
        printf ("usage: remote_lat <connect-to> <message-size> "
            "<roundtrip-count>\n");
        return 1;
    }
    connect_to = argv [1];
    message_size = atoi (argv [2]);
    roundtrip_count = atoi (argv [3]);

    ctx = xs_init (1);
    if (!ctx) {
        printf ("error in xs_init: %s\n", xs_strerror (errno));
        return -1;
    }

    s = xs_socket (ctx, XS_REQ);
    if (!s) {
        printf ("error in xs_socket: %s\n", xs_strerror (errno));
        return -1;
    }

    rc = xs_connect (s, connect_to);
    if (rc != 0) {
        printf ("error in xs_connect: %s\n", xs_strerror (errno));
        return -1;
    }

    rc = xs_msg_init_size (&msg, message_size);
    if (rc != 0) {
        printf ("error in xs_msg_init_size: %s\n", xs_strerror (errno));
        return -1;
    }
    memset (xs_msg_data (&msg), 0, message_size);

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

    printf ("message size: %d [B]\n", (int) message_size);
    printf ("roundtrip count: %d\n", (int) roundtrip_count);
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
