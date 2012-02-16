/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "monitor.hpp"
#include "io_thread.hpp"
#include "options.hpp"
#include "random.hpp"
#include "err.hpp"

zmq::monitor_t::monitor_t (zmq::io_thread_t *io_thread_) :
    own_t (io_thread_, options_t ()),
    io_object_t (io_thread_)
{
}

zmq::monitor_t::~monitor_t ()
{
}

void zmq::monitor_t::start ()
{
    send_plug (this);
}

void zmq::monitor_t::stop ()
{
    send_stop ();
}

void zmq::monitor_t::log (int sid_, const char *text_)
{
    sync.lock ();
    text = text_;
    sync.unlock ();
}

void zmq::monitor_t::process_plug ()
{
    //  Schedule sending of the first snapshot.
    add_timer (500 + (generate_random () % 1000), timer_id);
}

void zmq::monitor_t::process_stop ()
{
    cancel_timer (timer_id);
    send_done ();
    delete this;
}

void zmq::monitor_t::timer_event (int id_)
{
    zmq_assert (id_ == timer_id);

    //  Send the snapshot here!
    sync.lock ();
    publish_logs (text.c_str ());
    sync.unlock ();

    //  Wait before sending next snapshot.
    add_timer (500 + (generate_random () % 1000), timer_id);
}
