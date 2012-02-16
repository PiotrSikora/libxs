/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2010-2011 Other contributors as noted in the AUTHORS file

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

#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include "../include/xs.h"
#include "../include/xs_utils.h"

extern "C"
{
    void *worker(void *ctx)
    {
        //  Worker thread connects after delay of 1 second. Then it waits
        //  for 1 more second, so that async connect has time to succeed.
        xs_sleep (1);
        void *sc = xs_socket (ctx, XS_PUSH);
        assert (sc);
        int rc = xs_connect (sc, "inproc://timeout_test");
        assert (rc == 0);
        xs_sleep (1);
        rc = xs_close (sc);
        assert (rc == 0);
        return NULL;
    }
}

int main (int argc, char *argv [])
{
    fprintf (stderr, "test_timeo running...\n");

    void *ctx = xs_init (1);
    assert (ctx);

    //  Create a disconnected socket.
    void *sb = xs_socket (ctx, XS_PULL);
    assert (sb);
    int rc = xs_bind (sb, "inproc://timeout_test");
    assert (rc == 0);

    //  Check whether non-blocking recv returns immediately.
    char buf [] = "12345678ABCDEFGH12345678abcdefgh";
    rc = xs_recv (sb, buf, 32, XS_DONTWAIT);
    assert (rc == -1);
    assert (xs_errno() == EAGAIN);

    //  Check whether recv timeout is honoured.
    int timeout = 500;
    size_t timeout_size = sizeof timeout;
    rc = xs_setsockopt(sb, XS_RCVTIMEO, &timeout, timeout_size);
    assert (rc == 0);    
    void *watch = xs_stopwatch_start ();
    rc = xs_recv (sb, buf, 32, 0);
    assert (rc == -1);
    assert (xs_errno () == EAGAIN);
    unsigned long elapsed = xs_stopwatch_stop (watch);
    assert (elapsed > 440000 && elapsed < 550000);

    //  Check whether connection during the wait doesn't distort the timeout.
    timeout = 2000;
    rc = xs_setsockopt(sb, XS_RCVTIMEO, &timeout, timeout_size);
    assert (rc == 0);
    pthread_t thread;
    rc = pthread_create (&thread, NULL, worker, ctx);
    assert (rc == 0);
    watch = xs_stopwatch_start ();
    rc = xs_recv (sb, buf, 32, 0);
    assert (rc == -1);
    assert (xs_errno () == EAGAIN);
    elapsed = xs_stopwatch_stop (watch);
    assert (elapsed > 1900000 && elapsed < 2100000);
    rc = pthread_join (thread, NULL);
    assert (rc == 0);

    //  Check that timeouts don't break normal message transfer.
    void *sc = xs_socket (ctx, XS_PUSH);
    assert (sc);
    rc = xs_setsockopt(sb, XS_RCVTIMEO, &timeout, timeout_size);
    assert (rc == 0);
    rc = xs_setsockopt(sb, XS_SNDTIMEO, &timeout, timeout_size);
    assert (rc == 0);
    rc = xs_connect (sc, "inproc://timeout_test");
    assert (rc == 0);
    rc = xs_send (sc, buf, 32, 0);
    assert (rc == 32);
    rc = xs_recv (sb, buf, 32, 0);
    assert (rc == 32);

    //  Clean-up.
    rc = xs_close (sc);
    assert (rc == 0);
    rc = xs_close (sb);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}

