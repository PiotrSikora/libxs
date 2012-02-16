/*
    Copyright (c) 2010-2012 250bpm s.r.o.
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

#ifndef __XS_IO_THREAD_HPP_INCLUDED__
#define __XS_IO_THREAD_HPP_INCLUDED__

#include <map>

#include "fd.hpp"
#include "clock.hpp"
#include "object.hpp"
#include "mailbox.hpp"
#include "atomic_counter.hpp"

namespace xs
{

    class ctx_t;

    //  Handle of a file descriptor within a pollset.
    typedef void* handle_t;

    // Virtual interface to be exposed by object that want to be notified
    // about events on file descriptors.

    struct i_poll_events
    {
        virtual ~i_poll_events () {}
 
        // Called by I/O thread when file descriptor is ready for reading.
        virtual void in_event (fd_t fd_) = 0;
 
        // Called by I/O thread when file descriptor is ready for writing.
        virtual void out_event (fd_t fd_) = 0;
 
        // Called when timer expires.
        virtual void timer_event (handle_t handle_) = 0;
    };

    class io_thread_t : public object_t, public i_poll_events
    {
    public:

        //  Create optimal polling mechanism for this environment.
        static io_thread_t *create (xs::ctx_t *ctx_, uint32_t tid_);

        virtual ~io_thread_t ();

        //  Returns load of the I/O thread. Note that this function can be
        //  invoked from a different thread!
        int get_load ();

        void start ();
        void stop ();

        //  Returns mailbox associated with this I/O thread.
        mailbox_t *get_mailbox ();

        virtual handle_t add_fd (fd_t fd_, xs::i_poll_events *events_) = 0;
        virtual void rm_fd (handle_t handle_) = 0;
        virtual void set_pollin (handle_t handle_) = 0;
        virtual void reset_pollin (handle_t handle_) = 0;
        virtual void set_pollout (handle_t handle_) = 0;
        virtual void reset_pollout (handle_t handle_) = 0;
        virtual void xstart () = 0;
        virtual void xstop () = 0;

        //  Add a timeout to expire in timeout_ milliseconds. After the
        //  expiration timer_event on sink_ object will be called.
        handle_t add_timer (int timeout_, xs::i_poll_events *sink_);

        //  Cancel the timer identified by the handle.
        void rm_timer (handle_t handle_);

        //  i_poll_events implementation.
        void in_event (fd_t fd_);
        void out_event (fd_t fd_);
        void timer_event (handle_t handle_);

    protected:

        io_thread_t (xs::ctx_t *ctx_, uint32_t tid_);

        //  Called by individual io_thread implementations to manage the load.
        void adjust_load (int amount_);

        //  Executes any timers that are due. Returns number of milliseconds
        //  to wait to match the next timer or 0 meaning "no timers".
        uint64_t execute_timers ();

    private:

        void process_stop ();

        //  Clock instance private to this I/O thread.
        clock_t clock;

        //  List of active timers.
        struct timer_info_t
        {
            xs::i_poll_events *sink;
            std::multimap <uint64_t, timer_info_t>::iterator self;
        };
        typedef std::multimap <uint64_t, timer_info_t> timers_t;
        timers_t timers;

        //  Load of the I/O thread. Currently the number of file descriptors
        //  registered.
        atomic_counter_t load;

        //  I/O thread accesses incoming commands via this mailbox.
        mailbox_t mailbox;

        //  Handle associated with mailbox' file descriptor.
        handle_t mailbox_handle;

        io_thread_t (const io_thread_t&);
        const io_thread_t &operator = (const io_thread_t&);
    };

}

#endif
