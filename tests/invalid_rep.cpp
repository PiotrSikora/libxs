/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2011 VMware, Inc.
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

#include "testutil.hpp"

int XS_TEST_MAIN ()
{
    fprintf (stderr, "invalid_rep test running...\n");

    //  Create REQ/XREP wiring.
    void *ctx = xs_init (1);
    assert (ctx);
    void *xrep_socket = xs_socket (ctx, XS_XREP);
    assert (xrep_socket);
    void *req_socket = xs_socket (ctx, XS_REQ);
    assert (req_socket);
    int linger = 0;
    int rc = xs_setsockopt (xrep_socket, XS_LINGER, &linger, sizeof (int));
    assert (rc == 0);
    rc = xs_setsockopt (req_socket, XS_LINGER, &linger, sizeof (int));
    assert (rc == 0);
    rc = xs_bind (xrep_socket, "inproc://hi");
    assert (rc == 0);
    rc = xs_connect (req_socket, "inproc://hi");
    assert (rc == 0);

    //  Initial request.
    rc = xs_send (req_socket, "r", 1, 0);
    assert (rc == 1);

    //  Receive the request.
    char addr [32];
    int addr_size;
    char bottom [1];
    char body [1];
    addr_size = xs_recv (xrep_socket, addr, sizeof (addr), 0);
    assert (addr_size >= 0);
    rc = xs_recv (xrep_socket, bottom, sizeof (bottom), 0);
    assert (rc == 0);
    rc = xs_recv (xrep_socket, body, sizeof (body), 0);
    assert (rc == 1);

    //  Send invalid reply.
    rc = xs_send (xrep_socket, addr, addr_size, 0);
    assert (rc == addr_size);

    //  Send valid reply.
    rc = xs_send (xrep_socket, addr, addr_size, XS_SNDMORE);
    assert (rc == addr_size);
    rc = xs_send (xrep_socket, bottom, 0, XS_SNDMORE);
    assert (rc == 0);
    rc = xs_send (xrep_socket, "b", 1, 0);
    assert (rc == 1);

    //  Check whether we've got the valid reply.
    rc = xs_recv (req_socket, body, sizeof (body), 0);
    assert (rc == 1);
	assert (body [0] == 'b');

    //  Tear down the wiring.
    rc = xs_close (xrep_socket);
    assert (rc == 0);
    rc = xs_close (req_socket);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0;
}

