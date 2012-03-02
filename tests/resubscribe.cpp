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

int XS_TEST_MAIN ()
{
    fprintf (stderr, "resubscribe test running...\n");

    //  Create the basic infrastructure.
    void *ctx = xs_init (1);
    assert (ctx);
    void *xpub = xs_socket (ctx, XS_XPUB);
    assert (xpub);
    void *sub = xs_socket (ctx, XS_SUB);
    assert (sub);

    //  Send two subscriptions upstream.
    int rc = xs_bind (xpub, "tcp://127.0.0.1:5560");
    assert (rc == 0);
    rc = xs_setsockopt (sub, XS_SUBSCRIBE, "a", 1);
    assert (rc == 0);
    rc = xs_setsockopt (sub, XS_SUBSCRIBE, "b", 1);
    assert (rc == 0);
    rc = xs_connect (sub, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    //  Check whether subscriptions are correctly received.
    char buf [2];
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 2);
    assert (buf [0] == 1 && buf [1] == 'a');
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 2);
    assert (buf [0] == 1 && buf [1] == 'b');

    //  Clean up.
    rc = xs_close (sub);
    assert (rc == 0);
    rc = xs_close (xpub);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
