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

#include <string.h>

#include "../include/xs.h"

#include "xpub.hpp"
#include "pipe.hpp"
#include "wire.hpp"
#include "err.hpp"
#include "msg.hpp"

xs::xpub_t::xpub_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    socket_base_t (parent_, tid_, sid_),
    more (false)
{
    options.type = XS_XPUB;
}

xs::xpub_t::~xpub_t ()
{
    //  Deallocate all the filters.
    for (filters_t::iterator it = filters.begin (); it != filters.end (); ++it)
        it->type->destroy (it->instance);
}

void xs::xpub_t::xattach_pipe (pipe_t *pipe_, bool icanhasall_)
{
    xs_assert (pipe_);
    dist.attach (pipe_);

    // If icanhasall_ is specified, the caller would like to subscribe
    // to all data on this pipe, implicitly.
    if (icanhasall_) {

        //  Find the prefix filter.
        //  TODO: Change this to ALL filter.
        filters_t::iterator it;
        for (it = filters.begin (); it != filters.end (); ++it)
            if (it->type->filter_id == XS_FILTER_PREFIX)
                break;
        if (it == filters.end ()) {
            filter_t f;
            f.type = get_filter (XS_FILTER_PREFIX);
            xs_assert (f.type);
            f.instance = f.type->create ();
            xs_assert (f.instance);
            filters.push_back (f);
            it = filters.end () - 1;
        }

        it->type->subscribe (it->instance, pipe_, NULL, 0);
    }

    //  The pipe is active when attached. Let's read the subscriptions from
    //  it, if any.
    xread_activated (pipe_);
}

void xs::xpub_t::xread_activated (pipe_t *pipe_)
{
    //  There are some subscriptions waiting. Let's process them.
    msg_t sub;
    sub.init ();
    while (true) {

        //  Grab next subscription.
        if (!pipe_->read (&sub)) {
            sub.close ();
            return;
        }

        //  Apply the subscription to the trie.
        unsigned char *data = (unsigned char*) sub.data ();
        size_t size = sub.size ();

        //  TODO: In the case of malformed subscription we will simply ignore
        //  it for now. However, we should close the connection instead.
        if (size <= 4) {
            sub.close ();
            return;
        }
        int cmd = get_uint16 (data);
        int filter_id = get_uint16 (data + 2);
        if (cmd != XS_CMD_SUBSCRIBE && cmd != XS_CMD_UNSUBSCRIBE) {
            sub.close ();
            return;
        }

        //  Find the relevant filter.
        filters_t::iterator it;
        for (it = filters.begin (); it != filters.end (); ++it)
            if (it->type->filter_id == filter_id)
                break;

        bool unique;
		if (cmd == XS_CMD_UNSUBSCRIBE) {
            xs_assert (it != filters.end ());
            unique = it->type->unsubscribe (it->instance, pipe_, data + 4,
                size - 4) ? true : false;
        }
		else {

            //  If the filter of the specified type does not exist yet,
            //  create it.
            if (it == filters.end ()) {
                filter_t f;
                f.type = get_filter (filter_id);
                xs_assert (f.type);
                f.instance = f.type->create ();
                xs_assert (f.instance);
                filters.push_back (f);
                it = filters.end () - 1;
            }

            unique = it->type->subscribe (it->instance, pipe_, data + 4,
                size - 4) ? true : false;
        }

        //  If the subscription is not a duplicate store it so that it can be
        //  passed to used on next recv call.
        if (unique && options.type != XS_PUB)
            pending.push_back (blob_t ((unsigned char*) sub.data (),
                sub.size ()));
    }
    sub.close ();
}

void xs::xpub_t::xwrite_activated (pipe_t *pipe_)
{
    dist.activated (pipe_);
}

void xs::xpub_t::xterminated (pipe_t *pipe_)
{
    //  Remove the pipe from all the filters.
    for (filters_t::iterator it = filters.begin (); it != filters.end (); ++it)
        it->type->unsubscribe_all (it->instance, (void*) pipe_,
            (void*) (core_t*) this);

    dist.terminated (pipe_);
}

int xs::xpub_t::xsend (msg_t *msg_, int flags_)
{
    bool msg_more = msg_->flags () & msg_t::more ? true : false;

    //  For the first part of multi-part message, find the matching pipes.
    if (!more) {
        for (filters_t::iterator it = filters.begin (); it != filters.end ();
              ++it)
            it->type->match_all (it->instance, (unsigned char*) msg_->data (),
                msg_->size (), (void*) (core_t*) this);
    }

    //  Send the message to all the pipes that were marked as matching
    //  in the previous step.
    int rc = dist.send_to_matching (msg_, flags_);
    if (rc != 0)
        return rc;

    //  If we are at the end of multi-part message we can mark all the pipes
    //  as non-matching.
    if (!msg_more)
        dist.unmatch ();

    more = msg_more;

    return 0;
}

bool xs::xpub_t::xhas_out ()
{
    return dist.has_out ();
}

int xs::xpub_t::xrecv (msg_t *msg_, int flags_)
{
    //  If there is at least one 
    if (pending.empty ()) {
        errno = EAGAIN;
        return -1;
    }

    int rc = msg_->close ();
    errno_assert (rc == 0);
    rc = msg_->init_size (pending.front ().size ());
    errno_assert (rc == 0);
    memcpy (msg_->data (), pending.front ().data (),
        pending.front ().size ());
    pending.pop_front ();
    return 0;
}

bool xs::xpub_t::xhas_in ()
{
    return !pending.empty ();
}

void xs::xpub_t::filter_unsubscribed (int filter_id_,
    const unsigned char *data_, size_t size_)
{
    //  In XS_PUB socket, the subscriptions are not passed upstream.
    if (options.type != XS_PUB) {

		//  Place the unsubscription to the queue of pending (un)sunscriptions
		//  to be retrived by the user later on.
		blob_t unsub (size_ + 4, 0);
        put_uint16 ((unsigned char*) unsub.data (), XS_CMD_UNSUBSCRIBE);
        put_uint16 ((unsigned char*) unsub.data () + 2, filter_id_);
		memcpy ((void*) (unsub.data () + 4), data_, size_);
		pending.push_back (unsub);
    }
}

void xs::xpub_t::filter_matching (void *subscriber_)
{
    dist.match ((xs::pipe_t*) subscriber_);
}

xs::xpub_session_t::xpub_session_t (io_thread_t *io_thread_, bool connect_,
      socket_base_t *socket_, const options_t &options_,
      const char *protocol_, const char *address_) :
    session_base_t (io_thread_, connect_, socket_, options_, protocol_,
        address_)
{
}

xs::xpub_session_t::~xpub_session_t ()
{
}

