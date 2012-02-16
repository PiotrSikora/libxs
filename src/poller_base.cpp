/*
    Copyright (c) 2010-2012 250bpm s.r.o.
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

#include "poller_base.hpp"
#include "err.hpp"

#include "select.hpp"
#include "poll.hpp"
#include "epoll.hpp"
#include "devpoll.hpp"
#include "kqueue.hpp"

xs::poller_base_t *xs::poller_base_t::create ()
{
    poller_base_t *result;
#if defined XS_HAVE_SELECT
    result = new (std::nothrow) select_t;
#elif defined XS_HAVE_POLL
    result = new (std::nothrow) poll_t;
#elif defined XS_HAVE_EPOLL
    result = new (std::nothrow) epoll_t;
#elif defined XS_HAVE_DEVPOLL
    result = new (std::nothrow) devpoll_t;
#elif defined XS_HAVE_KQUEUE
    result = new (std::nothrow) kqueue_t;
#endif
    alloc_assert (result);
    return result;
}

xs::poller_base_t::poller_base_t ()
{
}

xs::poller_base_t::~poller_base_t ()
{
    //  Make sure there is no more load on the shutdown.
    xs_assert (get_load () == 0);
}

int xs::poller_base_t::get_load ()
{
    return load.get ();
}

void xs::poller_base_t::adjust_load (int amount_)
{
    if (amount_ > 0)
        load.add (amount_);
    else if (amount_ < 0)
        load.sub (-amount_);
}

void xs::poller_base_t::add_timer (int timeout_, i_poll_events *sink_, int id_)
{
    uint64_t expiration = clock.now_ms () + timeout_;
    timer_info_t info = {sink_, id_};
    timers.insert (timers_t::value_type (expiration, info));
}

void xs::poller_base_t::cancel_timer (i_poll_events *sink_, int id_)
{
    //  Complexity of this operation is O(n). We assume it is rarely used.
    for (timers_t::iterator it = timers.begin (); it != timers.end (); ++it)
        if (it->second.sink == sink_ && it->second.id == id_) {
            timers.erase (it);
            return;
        }

    //  Timer not found.
    xs_assert (false);
}

uint64_t xs::poller_base_t::execute_timers ()
{
    //  Fast track.
    if (timers.empty ())
        return 0;

    //  Get the current time.
    uint64_t current = clock.now_ms ();

    //   Execute the timers that are already due.
    timers_t::iterator it = timers.begin ();
    while (it != timers.end ()) {

        //  If we have to wait to execute the item, same will be true about
        //  all the following items (multimap is sorted). Thus we can stop
        //  checking the subsequent timers and return the time to wait for
        //  the next timer (at least 1ms).
        if (it->first > current)
            return it->first - current;

        //  Trigger the timer.
        it->second.sink->timer_event (it->second.id);

        //  Remove it from the list of active timers.
        timers_t::iterator o = it;
        ++it;
        timers.erase (o);
    }

    //  There are no more timers.
    return 0;
}
