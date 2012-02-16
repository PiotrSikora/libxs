/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

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

#include "testutil.hpp"

int XS_TEST_MAIN ()
{
    fprintf (stderr, "test_linger running...\n");

    //  Create REQ/XREP wiring.
    void *ctx = xs_init (1);
    assert (ctx);
    void *s = xs_socket (ctx, XS_PUSH);
    assert (s);

    //  Set linger to 0.1 second.
    int linger = 100;
    int rc = xs_setsockopt (s, XS_LINGER, &linger, sizeof (int));

    //  Connect to non-existent endpoing.
    assert (rc == 0);
    rc = xs_connect (s, "ipc:///tmp/this-file-does-not-exist");
    assert (rc == 0);

    //  Send a message.
    rc = xs_send (s, "r", 1, 0);
    assert (rc == 1);

    //  Close the socket.
    rc = xs_close (s);
    assert (rc == 0);

    //  Terminate the context. This should take 0.1 second.
    void *watch = xs_stopwatch_start ();
    rc = xs_term (ctx);
    assert (rc == 0);
    int ms = (int) xs_stopwatch_stop (watch) / 1000;
    assert (ms > 50 && ms < 150);

    return 0;
}

