/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2011 iMatix Corporation
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
    fprintf (stderr, "test_reqrep_tcp running...\n");

    void *ctx = xs_init (1);
    assert (ctx);

    void *sb = xs_socket (ctx, XS_REP);
    assert (sb);
    int rc = xs_bind (sb, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    void *sc = xs_socket (ctx, XS_REQ);
    assert (sc);
    rc = xs_connect (sc, "tcp://127.0.0.1:5560");
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
