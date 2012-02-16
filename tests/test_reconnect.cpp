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
    fprintf (stderr, "test_reconnect running...\n");

    //  Create the basic infrastructure.
    void *ctx = xs_init (1);
    assert (ctx);
    void *push = xs_socket (ctx, XS_PUSH);
    assert (push);
    void *pull = xs_socket (ctx, XS_PULL);
    assert (push);

    //  Connect before bind was done at the peer and send one message.
    int rc = xs_connect (push, "tcp://127.0.0.1:5560");
    assert (rc == 0);
    rc = xs_send (push, "ABC", 3, 0);
    assert (rc == 3);

    //  Wait a while for few attempts to reconnect to happen.
    xs_sleep (1);

    //  Bind the peer and get the message.
    rc = xs_bind (pull, "tcp://127.0.0.1:5560");
    assert (rc == 0);
    unsigned char buf [3];
    rc = xs_recv (pull, buf, sizeof (buf), 0);
    assert (rc == 3);

    //  Clean up.
    rc = xs_close (push);
    assert (rc == 0);
    rc = xs_close (pull);
    assert (rc == 0);

    //  Now, let's test the same scenario with IPC.
    push = xs_socket (ctx, XS_PUSH);
    assert (push);
    pull = xs_socket (ctx, XS_PULL);
    assert (push);

    //  Connect before bind was done at the peer and send one message.
    rc = xs_connect (push, "ipc:///tmp/tester");
    assert (rc == 0);
    rc = xs_send (push, "ABC", 3, 0);
    assert (rc == 3);

    //  Wait a while for few attempts to reconnect to happen.
    xs_sleep (1);

    //  Bind the peer and get the message.
    rc = xs_bind (pull, "ipc:///tmp/tester");
    assert (rc == 0);
    rc = xs_recv (pull, buf, sizeof (buf), 0);
    assert (rc == 3);

    //  Clean up.
    rc = xs_close (push);
    assert (rc == 0);
    rc = xs_close (pull);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
