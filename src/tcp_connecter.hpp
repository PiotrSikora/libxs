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

#ifndef __TCP_CONNECTER_HPP_INCLUDED__
#define __TCP_CONNECTER_HPP_INCLUDED__

#include "fd.hpp"
#include "own.hpp"
#include "stdint.hpp"
#include "io_object.hpp"
#include "tcp_address.hpp"

namespace xs
{

    class poller_base_t;
    class session_base_t;

    class tcp_connecter_t : public own_t, public io_object_t
    {
    public:

        //  If 'delay' is true connecter first waits for a while, then starts
        //  connection process.
        tcp_connecter_t (xs::poller_base_t *io_thread_,
            xs::session_base_t *session_, const options_t &options_,
            const char *address_, bool delay_);
        ~tcp_connecter_t ();

    private:

        //  Handlers for incoming commands.
        void process_plug ();

        //  Handlers for I/O events.
        void in_event (fd_t fd_);
        void out_event (fd_t fd_);
        void timer_event (handle_t handle_);

        //  Internal function to start the actual connection establishment.
        void start_connecting ();

        //  Internal function to add a reconnect timer
        void add_reconnect_timer();

        //  Internal function to return a reconnect backoff delay.
        //  Will modify the current_reconnect_ivl used for next call
        //  Returns the currently used interval
        int get_new_reconnect_ivl ();

        //  Set address to connect to.
        int set_address (const char *addr_);

        //  Open TCP connecting socket. Returns -1 in case of error,
        //  0 if connect was successfull immediately. Returns -1 with
        //  EAGAIN errno if async connect was launched.
        int open ();

        //  Close the connecting socket.
        void close ();

        //  Get the file descriptor of newly created connection. Returns
        //  retired_fd if the connection was unsuccessfull.
        fd_t connect ();

        //  Address to connect to.
        tcp_address_t address;

        //  Source address.
        tcp_address_t source_address;

        //  Underlying socket.
        fd_t s;

        //  Handle corresponding to the listening socket or NULL if the socket
        //  is not registered with the poller.
        handle_t handle;

        //  If true, connecter is waiting a while before trying to connect.
        bool wait;

        //  Reference to the session we belong to.
        xs::session_base_t *session;

        //  Current reconnect ivl, updated for backoff strategy
        int current_reconnect_ivl;

        //  Handle to reconnect timer, if active, NULL otherwise.
        handle_t reconnect_timer;

        tcp_connecter_t (const tcp_connecter_t&);
        const tcp_connecter_t &operator = (const tcp_connecter_t&);
    };

}

#endif
