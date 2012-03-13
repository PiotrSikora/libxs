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
    fprintf (stderr, "max_sockets test running...\n");

    //  Create context and set MAX_SOCKETS to 1.
    void *ctx = xs_init ();
    assert (ctx);
    int max_sockets = 1;
    int rc = xs_setctxopt (ctx, XS_MAX_SOCKETS, &max_sockets,
        sizeof (max_sockets));
    assert (rc == 0);

    //  First socket should be created OK.
    void *s1 = xs_socket (ctx, XS_PUSH);
    assert (s1);

    //  Creation of second socket should fail.
    void *s2 = xs_socket (ctx, XS_PUSH);
    assert (!s2 && errno == EMFILE);

    //  Clean up.
    rc = xs_close (s1);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0;
}

