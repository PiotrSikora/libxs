/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2011 VMware, Inc.
    Copyright (c) 2010-2011 Other contributors as noted in the AUTHORS file

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
    fprintf (stderr, "reqrep_device test running...\n");

    void *ctx = xs_init ();
    assert (ctx);

    //  Create a req/rep device.
    void *xreq = xs_socket (ctx, XS_XREQ);
    assert (xreq);
    int rc = xs_bind (xreq, "tcp://127.0.0.1:5560");
    assert (rc == 0);
    void *xrep = xs_socket (ctx, XS_XREP);
    assert (xrep);
    rc = xs_bind (xrep, "tcp://127.0.0.1:5561");
    assert (rc == 0);

    //  Create a worker.
    void *rep = xs_socket (ctx, XS_REP);
    assert (rep);
    rc = xs_connect (rep, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    //  Create a client.
    void *req = xs_socket (ctx, XS_REQ);
    assert (req);
    rc = xs_connect (req, "tcp://127.0.0.1:5561");
    assert (rc == 0);

    //  Send a request.
    rc = xs_send (req, "ABC", 3, XS_SNDMORE);
    assert (rc == 3);
    rc = xs_send (req, "DEF", 3, 0);
    assert (rc == 3);

    //  Pass the request through the device.
    for (int i = 0; i != 4; i++) {
        xs_msg_t msg;
        rc = xs_msg_init (&msg);
        assert (rc == 0);
        rc = xs_recvmsg (xrep, &msg, 0);
        assert (rc >= 0);
        int rcvmore;
        size_t sz = sizeof (rcvmore);
        rc = xs_getsockopt (xrep, XS_RCVMORE, &rcvmore, &sz);
        assert (rc == 0);
        rc = xs_sendmsg (xreq, &msg, rcvmore ? XS_SNDMORE : 0);
        assert (rc >= 0);
    }

    //  Receive the request.
    char buff [3];
    rc = xs_recv (rep, buff, 3, 0);
    assert (rc == 3);
    assert (memcmp (buff, "ABC", 3) == 0);
    int rcvmore;
    size_t sz = sizeof (rcvmore);
    rc = xs_getsockopt (rep, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (rcvmore);
    rc = xs_recv (rep, buff, 3, 0);
    assert (rc == 3);
    assert (memcmp (buff, "DEF", 3) == 0);
    rc = xs_getsockopt (rep, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (!rcvmore);

    //  Send the reply.
    rc = xs_send (rep, "GHI", 3, XS_SNDMORE);
    assert (rc == 3);
    rc = xs_send (rep, "JKL", 3, 0);
    assert (rc == 3);

    //  Pass the reply through the device.
    for (int i = 0; i != 4; i++) {
        xs_msg_t msg;
        rc = xs_msg_init (&msg);
        assert (rc == 0);
        rc = xs_recvmsg (xreq, &msg, 0);
        assert (rc >= 0);
        int rcvmore;
        rc = xs_getsockopt (xreq, XS_RCVMORE, &rcvmore, &sz);
        assert (rc == 0);
        rc = xs_sendmsg (xrep, &msg, rcvmore ? XS_SNDMORE : 0);
        assert (rc >= 0);
    }

    //  Receive the reply.
    rc = xs_recv (req, buff, 3, 0);
    assert (rc == 3);
    assert (memcmp (buff, "GHI", 3) == 0);
    rc = xs_getsockopt (req, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (rcvmore);
    rc = xs_recv (req, buff, 3, 0);
    assert (rc == 3);
    assert (memcmp (buff, "JKL", 3) == 0);
    rc = xs_getsockopt (req, XS_RCVMORE, &rcvmore, &sz);
    assert (rc == 0);
    assert (!rcvmore);

    //  Clean up.
    rc = xs_close (req);
    assert (rc == 0);
    rc = xs_close (rep);
    assert (rc == 0);
    rc = xs_close (xrep);
    assert (rc == 0);
    rc = xs_close (xreq);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
