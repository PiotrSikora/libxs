/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

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

#include "sub.hpp"
#include "msg.hpp"
#include "wire.hpp"

xs::sub_t::sub_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    xsub_t (parent_, tid_, sid_)
{
    options.type = XS_SUB;

    //  Switch filtering messages on (as opposed to XSUB which where the
    //  filtering is off).
    options.filter = true;
}

xs::sub_t::~sub_t ()
{
}

int xs::sub_t::xsetsockopt (int option_, const void *optval_,
    size_t optvallen_)
{
    if (option_ != XS_SUBSCRIBE && option_ != XS_UNSUBSCRIBE) {
        errno = EINVAL;
        return -1;
    }

    if (optvallen_ > 0 && !optval_) {
        errno = EFAULT;
        return -1;
    }

    //  Create the subscription message.
    msg_t msg;
    int rc = msg.init_size (optvallen_ + 4);
    errno_assert (rc == 0);
    unsigned char *data = (unsigned char*) msg.data ();
    if (option_ == XS_SUBSCRIBE) 
        put_uint16 (data, XS_CMD_SUBSCRIBE);
    else if (option_ == XS_UNSUBSCRIBE)
        put_uint16 (data, XS_CMD_UNSUBSCRIBE);
    put_uint16 (data + 2, options.filter_id);
    memcpy (data + 4, optval_, optvallen_);

    //  Pass it further on in the stack.
    int err = 0;
    rc = xsub_t::xsend (&msg, 0);
    if (rc != 0)
        err = errno;
    int rc2 = msg.close ();
    errno_assert (rc2 == 0);
    if (rc != 0)
        errno = err;
    return rc;
}

int xs::sub_t::xsend (msg_t *msg_, int flags_)
{
    //  Overload the XSUB's send.
    errno = ENOTSUP;
    return -1;
}

bool xs::sub_t::xhas_out ()
{
    //  Overload the XSUB's send.
    return false;
}

xs::sub_session_t::sub_session_t (io_thread_t *io_thread_, bool connect_,
      socket_base_t *socket_, const options_t &options_,
      const char *protocol_, const char *address_) :
    xsub_session_t (io_thread_, connect_, socket_, options_, protocol_,
        address_)
{
}

xs::sub_session_t::~sub_session_t ()
{
}

