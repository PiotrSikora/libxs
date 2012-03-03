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

#include "../include/xs_filter.h"

#include "xpub.hpp"
#include "xsub.hpp"

void xs_filter_subscribed (int filter_id_, unsigned char *data_,
    size_t size_, void *arg_)
{
    xs::xsub_t::send_subscription (filter_id_, data_, size_, arg_);
}

void xs_filter_unsubscribed (int filter_id_, unsigned char *data_,
    size_t size_, void *arg_)
{
    xs::xpub_t::send_unsubscription (filter_id_, data_, size_, arg_);
}

void xs_filter_matching (void *fid_, void *arg_)
{
    xs::xpub_t *self = (xs::xpub_t*) arg_;
    self->dist.match ((xs::pipe_t*) fid_);
}

