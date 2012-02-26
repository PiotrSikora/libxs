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

#include <new>

#include "../include/xs_filter.h"

#include "prefix_filter.hpp"
#include "err.hpp"

xs::prefix_filter_t::prefix_filter_t ()
{
    xs_assert (false);
}

xs::prefix_filter_t::~prefix_filter_t ()
{
    xs_assert (false);
}

void xs::prefix_filter_t::create (void *fid_)
{
    xs_assert (false);
}

void xs::prefix_filter_t::destroy (void *fid_)
{
    xs_assert (false);
}

void xs::prefix_filter_t::subscribe (void *fid_, unsigned char *data_,
    size_t size_)
{
    xs_assert (false);
}

void xs::prefix_filter_t::unsubscribe (void *fid_, unsigned char *data_,
    size_t size_)
{
    xs_assert (false);
}

void xs::prefix_filter_t::enumerate ()
{
    xs_assert (false);
}

int xs::prefix_filter_t::match (void *fid_, unsigned char *data_, size_t size_)
{
    xs_assert (false);
    return 0;
}

void xs::prefix_filter_t::match_all (unsigned char *data_, size_t size_)
{
    xs_assert (false);
}

//  Implementation of the C interface of the filter.

static void *fset_create ()
{
    void *fset = (void*) new (std::nothrow) xs::prefix_filter_t;
    alloc_assert (fset);
    return fset;
}

static void fset_destroy (void *fset_)
{
    delete (xs::prefix_filter_t*) fset_;
}

static void create (void *fset_, void *fid_)
{
    ((xs::prefix_filter_t*) fset_)->create (fid_);
}

static void destroy (void *fset_, void *fid_)
{
    ((xs::prefix_filter_t*) fset_)->destroy (fid_);
}

static void subscribe (void *fset_, void *fid_, unsigned char *data_,
    size_t size_)
{
    ((xs::prefix_filter_t*) fset_)->subscribe (fid_, data_, size_);
}

static void unsubscribe (void *fset_, void *fid_, unsigned char *data_,
    size_t size_)
{
    ((xs::prefix_filter_t*) fset_)->unsubscribe (fid_, data_, size_);
}

static void enumerate (void *fset_)
{
    ((xs::prefix_filter_t*) fset_)->enumerate ();
}

static int match (void *fset_, void *fid_, unsigned char *data_, size_t size_)
{
    return ((xs::prefix_filter_t*) fset_)->match (fid_, data_, size_);
}

static void match_all (void *fset_, unsigned char *data_, size_t size_)
{
    ((xs::prefix_filter_t*) fset_)->match_all (data_, size_);
}

#define XS_PREFIX_FILTER 1

static xs_filter_t filter = {
    XS_EXTENSION_FILTER,
    XS_PREFIX_FILTER,
    fset_create,
    fset_destroy,
    create,
    destroy,
    subscribe,
    unsubscribe,
    enumerate,
    match,
    match_all
};

void *xs::prefix_filter = (void*) &filter;

