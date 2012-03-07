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

#include "core.hpp"

void xs_filter_subscribed (int filter_id_, const unsigned char *data_,
    size_t size_, void *arg_)
{
    ((xs::core_t*) arg_)->filter_subscribed (filter_id_, data_, size_);
}

void xs_filter_unsubscribed (int filter_id_, const unsigned char *data_,
    size_t size_, void *arg_)
{
    ((xs::core_t*) arg_)->filter_unsubscribed (filter_id_, data_, size_);
}

void xs_filter_matching (void *subscriber_, void *arg_)
{
    ((xs::core_t*) arg_)->filter_matching (subscriber_);
}

