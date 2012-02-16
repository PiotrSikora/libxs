/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

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

#include "io_object.hpp"
#include "io_thread.hpp"
#include "err.hpp"

xs::io_object_t::io_object_t (io_thread_t *io_thread_) :
    poller (NULL)
{
    if (io_thread_)
        plug (io_thread_);
}

xs::io_object_t::~io_object_t ()
{
}

void xs::io_object_t::plug (io_thread_t *io_thread_)
{
    xs_assert (io_thread_);
    xs_assert (!poller);

    //  Retrieve the poller from the thread we are running in.
    poller = io_thread_->get_poller ();
}

void xs::io_object_t::unplug ()
{
    xs_assert (poller);

    //  Forget about old poller in preparation to be migrated
    //  to a different I/O thread.
    poller = NULL;
}

xs::handle_t xs::io_object_t::add_fd (fd_t fd_)
{
    return poller->add_fd (fd_, this);
}

void xs::io_object_t::rm_fd (handle_t handle_)
{
    poller->rm_fd (handle_);
}

void xs::io_object_t::set_pollin (handle_t handle_)
{
    poller->set_pollin (handle_);
}

void xs::io_object_t::reset_pollin (handle_t handle_)
{
    poller->reset_pollin (handle_);
}

void xs::io_object_t::set_pollout (handle_t handle_)
{
    poller->set_pollout (handle_);
}

void xs::io_object_t::reset_pollout (handle_t handle_)
{
    poller->reset_pollout (handle_);
}

xs::handle_t xs::io_object_t::add_timer (int timeout_)
{
    return poller->add_timer (timeout_, this);
}

void xs::io_object_t::rm_timer (handle_t handle_)
{
    poller->rm_timer (handle_);
}

void xs::io_object_t::in_event (fd_t fd_)
{
    xs_assert (false);
}

void xs::io_object_t::out_event (fd_t fd_)

{
    xs_assert (false);
}

void xs::io_object_t::timer_event (handle_t handle_)
{
    xs_assert (false);
}
