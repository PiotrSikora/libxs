/*
    Copyright (c) 2012 250bpm s.r.o.
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

#include "testutil.hpp"

extern "C"
{
    void polltimeo_worker(void *ctx_)
    {
        //  Worker thread connects after delay of 1 second. Then it waits
        //  for 1 more second, so that async connect has time to succeed.
        xs_sleep (1);
        void *sc = xs_socket (ctx_, XS_PUSH);
        assert (sc);
        int rc = xs_connect (sc, "inproc://timeout_test");
        assert (rc == 0);
        xs_sleep (1);
        rc = xs_close (sc);
        assert (rc == 0);
    }
}

int XS_TEST_MAIN ()
{
    fprintf (stderr, "polltimeo test running...\n");

    void *ctx = xs_init ();
    assert (ctx);

    //  Create a disconnected socket.
    void *sb = xs_socket (ctx, XS_PULL);
    assert (sb);
    int rc = xs_bind (sb, "inproc://timeout_test");
    assert (rc == 0);

    //  Check whether timeout is honoured.
    xs_pollitem_t pi;
    pi.socket = sb;
    pi.events = XS_POLLIN;
    void *watch = xs_stopwatch_start ();
    rc = xs_poll (&pi, 1, 500);
    assert (rc == 0);
    unsigned long elapsed = xs_stopwatch_stop (watch);
    assert (elapsed > 440000 && elapsed < 550000);

    //  Check whether connection during the wait doesn't distort the timeout.
    void *thread = xs_thread_create (polltimeo_worker, ctx);
    assert (thread);
    watch = xs_stopwatch_start ();
    rc = xs_poll (&pi, 1, 2000);
    assert (rc == 0);
    elapsed = xs_stopwatch_stop (watch);
    assert (elapsed > 1900000 && elapsed < 2100000);
    xs_thread_join (thread);

    //  Clean-up.
    rc = xs_close (sb);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
