/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

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

#include "monitor.hpp"
#include "poller_base.hpp"
#include "options.hpp"
#include "random.hpp"
#include "err.hpp"

xs::monitor_t::monitor_t (xs::poller_base_t *io_thread_) :
    own_t (io_thread_, options_t ()),
    io_object_t (io_thread_),
    timer (NULL)
{
}

xs::monitor_t::~monitor_t ()
{
}

void xs::monitor_t::start ()
{
    send_plug (this);
}

void xs::monitor_t::stop ()
{
    send_stop ();
}

void xs::monitor_t::log (int sid_, const char *text_)
{
    sync.lock ();
    text = text_;
    sync.unlock ();
}

void xs::monitor_t::process_plug ()
{
    //  Schedule sending of the first snapshot.
    timer = add_timer (500 + (generate_random () % 1000));
}

void xs::monitor_t::process_stop ()
{
    rm_timer (timer);
    timer = NULL;
    send_done ();
    delete this;
}

void xs::monitor_t::timer_event (handle_t handle_)
{
    xs_assert (handle_ == timer);

    //  Send the snapshot here!
    sync.lock ();
    publish_logs (text.c_str ());
    sync.unlock ();

    //  Wait before sending next snapshot.
    timer = add_timer (500 + (generate_random () % 1000));
}
