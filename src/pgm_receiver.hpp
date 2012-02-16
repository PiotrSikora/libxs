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

#ifndef __XS_PGM_RECEIVER_HPP_INCLUDED__
#define __XS_PGM_RECEIVER_HPP_INCLUDED__

#include "platform.hpp"

#if defined XS_HAVE_OPENPGM

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#endif

#include <map>
#include <algorithm>

#include "io_object.hpp"
#include "i_engine.hpp"
#include "options.hpp"
#include "decoder.hpp"
#include "pgm_socket.hpp"

namespace xs
{

    class io_thread_t;
    class session_base_t;

    class pgm_receiver_t : public io_object_t, public i_engine
    {
    
    public:

        pgm_receiver_t (xs::io_thread_t *parent_, const options_t &options_);
        ~pgm_receiver_t ();

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
        void timer_event (handle_t handle_);

    private:

        //  PGM is not able to move subscriptions upstream. Thus, drop all
        //  the pending subscriptions.
        void drop_subscriptions ();

        //  If joined is true we are already getting messages from the peer.
        //  It it's false, we are getting data but still we haven't seen
        //  beginning of a message.
        struct peer_info_t
        {
            bool joined;
            decoder_t *decoder;
        };

        struct tsi_comp
        {
            bool operator () (const pgm_tsi_t &ltsi,
                const pgm_tsi_t &rtsi) const
            {
                uint32_t ll[2], rl[2];
                memcpy (ll, &ltsi, sizeof (ll));
                memcpy (rl, &rtsi, sizeof (rl));
                return (ll[0] < rl[0]) || (ll[0] == rl[0] && ll[1] < rl[1]);
            }
        };

        typedef std::map <pgm_tsi_t, peer_info_t, tsi_comp> peers_t;
        peers_t peers;

        //  PGM socket.
        pgm_socket_t pgm_socket;

        //  Socket options.
        options_t options;

        //  Associated session.
        xs::session_base_t *session;

        //  Most recently used decoder.
        decoder_t *mru_decoder;

        //  Number of bytes not consumed by the decoder due to pipe overflow.
        size_t pending_bytes;

        //  Pointer to data still waiting to be processed by the decoder.
        unsigned char *pending_ptr;

        //  Poll handle associated with PGM socket.
        handle_t socket_handle;

        //  Poll handle associated with engine PGM waiting pipe.
        handle_t pipe_handle;

        //  Receive timer, if active, otherwise NULL.
        handle_t rx_timer;

        pgm_receiver_t (const pgm_receiver_t&);
        const pgm_receiver_t &operator = (const pgm_receiver_t&);
    };

}

#endif

#endif
