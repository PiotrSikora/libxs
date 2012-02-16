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
    fprintf (stderr, "reentrant test running...\n");

    //  Initialise the context and set REENTRANT option.
    void *ctx = xs_init (1);
    assert (ctx);
    int val = 1;
    int rc = xs_setctxopt (ctx, XS_REENTRANT, &val, sizeof (val));
    assert (rc == 0);

    //  Do a set of operations to make sure that REENTRANT option doesn't
    //  break anything.
    void *sb = xs_socket (ctx, XS_REP);
    assert (sb);
    rc = xs_bind (sb, "inproc://a");
    assert (rc == 0);
    void *sc = xs_socket (ctx, XS_REQ);
    assert (sc);
    rc = xs_connect (sc, "inproc://a");
    assert (rc == 0);
    bounce (sb, sc);
    rc = xs_close (sc);
    assert (rc == 0);
    rc = xs_close (sb);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
