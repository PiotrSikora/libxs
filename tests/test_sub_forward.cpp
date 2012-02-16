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

#include <assert.h>
#include <stdio.h>

#include "../include/xs.h"
#include "../include/xs_utils.h"

int main (int argc, char *argv [])
{
    fprintf (stderr, "test_sub_forward running...\n");

    void *ctx = xs_init (1);
    assert (ctx);

    //  First, create an intermediate device.
    void *xpub = xs_socket (ctx, XS_XPUB);
    assert (xpub);
    int rc = xs_bind (xpub, "tcp://127.0.0.1:5560");
    assert (rc == 0);
    void *xsub = xs_socket (ctx, XS_XSUB);
    assert (xsub);
    rc = xs_bind (xsub, "tcp://127.0.0.1:5561");
    assert (rc == 0);

    //  Create a publisher.
    void *pub = xs_socket (ctx, XS_PUB);
    assert (pub);
    rc = xs_connect (pub, "tcp://127.0.0.1:5561");
    assert (rc == 0);

    //  Create a subscriber.
    void *sub = xs_socket (ctx, XS_SUB);
    assert (sub);
    rc = xs_connect (sub, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    //  Subscribe for all messages.
    rc = xs_setsockopt (sub, XS_SUBSCRIBE, "", 0);
    assert (rc == 0);

    //  Pass the subscription upstream through the device.
    char buff [32];
    rc = xs_recv (xpub, buff, sizeof (buff), 0);
    assert (rc >= 0);
    rc = xs_send (xsub, buff, rc, 0);
    assert (rc >= 0);

    //  Wait a bit till the subscription gets to the publisher.
    xs_sleep (1);

    //  Send an empty message.
    rc = xs_send (pub, NULL, 0, 0);
    assert (rc == 0);

    //  Pass the message downstream through the device.
    rc = xs_recv (xsub, buff, sizeof (buff), 0);
    assert (rc >= 0);
    rc = xs_send (xpub, buff, rc, 0);
    assert (rc >= 0);

    //  Receive the message in the subscriber.
    rc = xs_recv (sub, buff, sizeof (buff), 0);
    assert (rc == 0);

    //  Clean up.
    rc = xs_close (xpub);
    assert (rc == 0);
    rc = xs_close (xsub);
    assert (rc == 0);
    rc = xs_close (pub);
    assert (rc == 0);
    rc = xs_close (sub);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
