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

//  This file is used only in MSVC build.
//  It gathers all the tests into a single executable.

#include "testutil.hpp"

#include <assert.h>

#undef XS_TEST_MAIN

#define XS_TEST_MAIN hwm
#include "hwm.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN invalid_rep
#include "invalid_rep.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN linger
#include "linger.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN msg_flags
#include "msg_flags.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN pair_inproc
#include "pair_inproc.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN pair_ipc
#include "pair_ipc.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN pair_tcp
#include "pair_tcp.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN reconnect
#include "reconnect.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN reqrep_device
#include "reqrep_device.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN reqrep_inproc
#include "reqrep_inproc.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN reqrep_ipc
#include "reqrep_ipc.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN reqrep_tcp
#include "reqrep_tcp.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN shutdown_stress
#include "shutdown_stress.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN sub_forward
#include "sub_forward.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN timeo
#include "timeo.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN max_sockets
#include "max_sockets.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN emptyctx
#include "emptyctx.cpp"
#undef XS_TEST_MAIN

#define XS_TEST_MAIN polltimeo
#include "polltimeo.cpp"
#undef XS_TEST_MAIN

int main ()
{
    int rc;

    rc = hwm ();
    assert (rc == 0);
    rc = invalid_rep ();
    assert (rc == 0);
    rc = linger ();
    assert (rc == 0);
    rc = msg_flags ();
    assert (rc == 0);
    rc = pair_inproc ();
    assert (rc == 0);
    rc = pair_ipc ();
    assert (rc == 0);
    rc = reconnect ();
    assert (rc == 0);
    rc = reqrep_device ();
    assert (rc == 0);
    rc = reqrep_inproc ();
    assert (rc == 0);
    rc = reqrep_ipc ();
    assert (rc == 0);
    rc = reqrep_tcp ();
    assert (rc == 0);
    rc = shutdown_stress ();
    assert (rc == 0);
    rc = sub_forward ();
    assert (rc == 0);
    rc = timeo ();
    assert (rc == 0);
    rc = max_sockets ();
    assert (rc == 0);
    rc = emptyctx ();
    assert (rc == 0);
    rc = polltimeo ();
    assert (rc == 0);

    fprintf (stderr, "SUCCESS\n");
    xs_sleep (1);

    return 0;
}
