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

#if defined XS_HAVE_WINDOWS
int XS_TEST_MAIN ()
{
    return 0;
}
#else

#include <pthread.h>
#include <stddef.h>

#define THREAD_COUNT 100

extern "C"
{
    static void *worker (void *s)
    {
        int rc;

        rc = xs_connect (s, "tcp://127.0.0.1:5560");
        assert (rc == 0);

        //  Start closing the socket while the connecting process is underway.
        rc = xs_close (s);
        assert (rc == 0);

        return NULL;
    }
}

int XS_TEST_MAIN ()
{
    void *ctx;
    void *s1;
    void *s2;
    int i;
    int j;
    int rc;
    pthread_t threads [THREAD_COUNT];

    fprintf (stderr, "shutdown_stress test running...\n");

    for (j = 0; j != 10; j++) {

        //  Check the shutdown with many parallel I/O threads.
        ctx = xs_init (7);
        assert (ctx);

        s1 = xs_socket (ctx, XS_PUB);
        assert (s1);

        rc = xs_bind (s1, "tcp://127.0.0.1:5560");
        assert (rc == 0);

        for (i = 0; i != THREAD_COUNT; i++) {
            s2 = xs_socket (ctx, XS_SUB);
            assert (s2);
            rc = pthread_create (&threads [i], NULL, worker, s2);
            assert (rc == 0);
        }

        for (i = 0; i != THREAD_COUNT; i++) {
            rc = pthread_join (threads [i], NULL);
            assert (rc == 0);
        }

        rc = xs_close (s1);
        assert (rc == 0);

        rc = xs_term (ctx);
        assert (rc == 0);
    }

    return 0;
}

#endif