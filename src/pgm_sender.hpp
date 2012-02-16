/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2010-2011 Miru Limited
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

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

#ifndef __XS_PGM_SENDER_HPP_INCLUDED__
#define __XS_PGM_SENDER_HPP_INCLUDED__

#include "platform.hpp"

#if defined XS_HAVE_OPENPGM

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#endif

#include "stdint.hpp"
#include "io_object.hpp"
#include "i_engine.hpp"
#include "options.hpp"
#include "pgm_socket.hpp"
#include "encoder.hpp"

namespace xs
{

    class io_thread_t;
    class session_base_t;

    class pgm_sender_t : public io_object_t, public i_engine
    {

    public:

        pgm_sender_t (xs::io_thread_t *parent_, const options_t &options_);
        ~pgm_sender_t ();

        int init (bool udp_encapsulation_, const char *network_);

        //  i_engine interface implementation.
        void plug (xs::io_thread_t *io_thread_,
            xs::session_base_t *session_);
        void unplug ();
        void terminate ();
        void activate_in ();
        void activate_out ();

        //  i_poll_events interface implementation.
        void in_event (fd_t fd_);
        void out_event (fd_t fd_);
        void timer_event (handle_t handle_);

    private:

        //  Message encoder.
        encoder_t encoder;

        //  PGM socket.
        pgm_socket_t pgm_socket;

        //  Socket options.
        options_t options;

        //  Poll handle associated with PGM socket.
        handle_t handle;
        handle_t uplink_handle;
        handle_t rdata_notify_handle;
        handle_t pending_notify_handle;

        //  Output buffer from pgm_socket.
        unsigned char *out_buffer;
        
        //  Output buffer size.
        size_t out_buffer_size;

        //  Number of bytes in the buffer to be written to the socket.
        //  If zero, there are no data to be sent.
        size_t write_size;

        //  Receive and send timers or NULL if not active.
        handle_t rx_timer;
        handle_t tx_timer;

        pgm_sender_t (const pgm_sender_t&);
        const pgm_sender_t &operator = (const pgm_sender_t&);
    };

}
#endif

#endif
