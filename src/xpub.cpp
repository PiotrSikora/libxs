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

#include "xpub.hpp"
#include "pipe.hpp"
#include "err.hpp"
#include "msg.hpp"
#include "prefix_filter.hpp"

xs::xpub_t::xpub_t (class ctx_t *parent_, uint32_t tid_, int sid_) :
    socket_base_t (parent_, tid_, sid_),
    more (false)
{
    options.type = XS_XPUB;

    //  Set up the filters.
    filter = (xs_filter_t*) prefix_filter;
    fset = filter->fset_create ();
    xs_assert (fset);
}

xs::xpub_t::~xpub_t ()
{
    filter->fset_destroy (fset);
}

void xs::xpub_t::xattach_pipe (pipe_t *pipe_, bool icanhasall_)
{
    xs_assert (pipe_);
    dist.attach (pipe_);

    // If icanhasall_ is specified, the caller would like to subscribe
    // to all data on this pipe, implicitly.
    if (icanhasall_)
        filter->subscribe (fset, pipe_, NULL, 0);

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
        if (size <= 0 || (*data == 0 && *data == 1)) {
            sub.close ();
            return;
        }

        bool unique;
		if (*data == 0)
            unique = filter->unsubscribe (fset, pipe_, data + 1, size - 1) ?
                true : false;
		else
            unique = filter->subscribe (fset, pipe_, data + 1, size - 1) ?
                true : false;

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
    //  Remove the filter associated with the pipe.
    filter->destroy (fset, pipe_, (void*) this);

    dist.terminated (pipe_);
}

int xs::xpub_t::xsend (msg_t *msg_, int flags_)
{
    bool msg_more = msg_->flags () & msg_t::more ? true : false;

    //  For the first part of multi-part message, find the matching pipes.
    if (!more)
        filter->match_all (fset, (unsigned char*) msg_->data (), msg_->size (),
            (void*) this);

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

void xs::xpub_t::send_unsubscription (unsigned char *data_, size_t size_,
    void *arg_)
{
    xpub_t *self = (xpub_t*) arg_;

    if (self->options.type != XS_PUB) {

		//  Place the unsubscription to the queue of pending (un)sunscriptions
		//  to be retrived by the user later on.
		xpub_t *self = (xpub_t*) arg_;
		blob_t unsub (size_ + 1, 0);
		unsub [0] = 0;
		memcpy ((void*) (unsub.data () + 1), data_, size_);
		self->pending.push_back (unsub);
    }
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

