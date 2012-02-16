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

#ifndef __XS_MONITOR_HPP_INCLUDED__
#define __XS_MONITOR_HPP_INCLUDED__

#include <string>

#include "own.hpp"
#include "mutex.hpp"
#include "io_object.hpp"

namespace xs
{

    class io_thread_t;
    class socket_base_t;

    class monitor_t : public own_t, public io_object_t
    {
    public:

        monitor_t (xs::io_thread_t *io_thread_);
        ~monitor_t ();

        void start ();
        void stop ();

        void log (int sid_, const char *text_);

    private:

        enum {timer_id = 0x44};

        //  Handlers for incoming commands.
        void process_plug ();
        void process_stop ();

        //  Events from the poller.
        void timer_event (int id_);

        //  Actual monitoring data to send and the related critical section.
        std::string text;
        mutex_t sync;

        monitor_t (const monitor_t&);
        const monitor_t &operator = (const monitor_t&);
    };

}

#endif
