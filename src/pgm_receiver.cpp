/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2012 Lucina & Associates
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

#include "platform.hpp"

#if defined XS_HAVE_OPENPGM

#include <new>

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#endif

#include "pgm_receiver.hpp"
#include "session_base.hpp"
#include "stdint.hpp"
#include "wire.hpp"
#include "err.hpp"

xs::pgm_receiver_t::pgm_receiver_t (class io_thread_t *parent_, 
      const options_t &options_) :
    io_object_t (parent_),
    pgm_socket (true, options_),
    options (options_),
    session (NULL),
    mru_decoder (NULL),
    pending_bytes (0),
    rx_timer (NULL)
{
}

xs::pgm_receiver_t::~pgm_receiver_t ()
{
    //  Destructor should not be called before unplug.
    xs_assert (peers.empty ());
}

int xs::pgm_receiver_t::init (bool udp_encapsulation_, const char *network_)
{
    return pgm_socket.init (udp_encapsulation_, network_);
}

void xs::pgm_receiver_t::plug (io_thread_t *io_thread_,
    session_base_t *session_)
{
    //  Retrieve PGM fds and start polling.
    fd_t socket_fd = retired_fd;
    fd_t waiting_pipe_fd = retired_fd;
    pgm_socket.get_receiver_fds (&socket_fd, &waiting_pipe_fd);
    socket_handle = add_fd (socket_fd);
    pipe_handle = add_fd (waiting_pipe_fd);
    set_pollin (pipe_handle);
    set_pollin (socket_handle);

    session = session_;

    //  If there are any subscriptions already queued in the session, drop them.
    drop_subscriptions ();
}

void xs::pgm_receiver_t::unplug ()
{
    //  Delete decoders.
    for (peers_t::iterator it = peers.begin (); it != peers.end (); ++it) {
        if (it->second.decoder != NULL)
            delete it->second.decoder;
    }
    peers.clear ();

    mru_decoder = NULL;
    pending_bytes = 0;

    if (rx_timer) {
        rm_timer (rx_timer);
        rx_timer = NULL;
    }

    rm_fd (socket_handle);
    rm_fd (pipe_handle);

    session = NULL;
}

void xs::pgm_receiver_t::terminate ()
{
    unplug ();
    delete this;
}

void xs::pgm_receiver_t::activate_out ()
{
    drop_subscriptions ();
}

void xs::pgm_receiver_t::activate_in ()
{
    //  It is possible that the most recently used decoder
    //  processed the whole buffer but failed to write
    //  the last message into the pipe.
    if (pending_bytes == 0) {
        if (mru_decoder != NULL) {
            mru_decoder->process_buffer (NULL, 0);
            session->flush ();
        }

        //  Resume polling.
        set_pollin (pipe_handle);
        set_pollin (socket_handle);

        return;
    }

    xs_assert (mru_decoder != NULL);
    xs_assert (pending_ptr != NULL);

    //  Ask the decoder to process remaining data.
    size_t n = mru_decoder->process_buffer (pending_ptr, pending_bytes);
    pending_bytes -= n;
    session->flush ();

    if (pending_bytes > 0)
        return;

    //  Resume polling.
    set_pollin (pipe_handle);
    set_pollin (socket_handle);

    in_event (retired_fd);
}

void xs::pgm_receiver_t::in_event (fd_t fd_)
{
    // Read data from the underlying pgm_socket.
    unsigned char *data = NULL;
    const pgm_tsi_t *tsi = NULL;

    if (pending_bytes > 0)
        return;

    if (rx_timer) {
        rm_timer (rx_timer);
        rx_timer = NULL;
    }

    //  TODO: This loop can effectively block other engines in the same I/O
    //  thread in the case of high load.
    while (true) {

        //  Get new batch of data.
        //  Note the workaround made not to break strict-aliasing rules.
        void *tmp = NULL;
        ssize_t received = pgm_socket.receive (&tmp, &tsi);
        data = (unsigned char*) tmp;

        //  No data to process. This may happen if the packet received is
        //  neither ODATA nor ODATA.
        if (received == 0) {
            if (errno == ENOMEM || errno == EBUSY) {
                const long timeout = pgm_socket.get_rx_timeout ();
                xs_assert (!rx_timer);
                rx_timer = add_timer (timeout);
            }
            break;
        }

        //  Find the peer based on its TSI.
        peers_t::iterator it = peers.find (*tsi);

        //  Data loss. Delete decoder and mark the peer as disjoint.
        if (received == -1) {
            if (it != peers.end ()) {
                it->second.joined = false;
                if (it->second.decoder == mru_decoder)
                    mru_decoder = NULL;
                if (it->second.decoder != NULL) {
                    delete it->second.decoder;
                    it->second.decoder = NULL;
                }
            }
            break;
        }

        //  New peer. Add it to the list of know but unjoint peers.
        if (it == peers.end ()) {
            peer_info_t peer_info = {false, NULL};
            it = peers.insert (peers_t::value_type (*tsi, peer_info)).first;
        }

        //  Read the offset of the fist message in the current packet.
        xs_assert ((size_t) received >= sizeof (uint16_t));
        uint16_t offset = get_uint16 (data);
        data += sizeof (uint16_t);
        received -= sizeof (uint16_t);

        //  Join the stream if needed.
        if (!it->second.joined) {

            //  There is no beginning of the message in current packet.
            //  Ignore the data.
            if (offset == 0xffff)
                continue;

            xs_assert (offset <= received);
            xs_assert (it->second.decoder == NULL);

            //  We have to move data to the begining of the first message.
            data += offset;
            received -= offset;

            //  Mark the stream as joined.
            it->second.joined = true;

            //  Create and connect decoder for the peer.
            it->second.decoder = new (std::nothrow) decoder_t (0,
                options.maxmsgsize);
            alloc_assert (it->second.decoder);
            it->second.decoder->set_session (session);
        }

        mru_decoder = it->second.decoder;

        //  Push all the data to the decoder.
        ssize_t processed = it->second.decoder->process_buffer (data, received);
        if (processed < received) {
            //  Save some state so we can resume the decoding process later.
            pending_bytes = received - processed;
            pending_ptr = data + processed;
            //  Stop polling.
            reset_pollin (pipe_handle);
            reset_pollin (socket_handle);

            //  Reset outstanding timer.
            if (rx_timer) {
                rm_timer (rx_timer);
                rx_timer = NULL;
            }

            break;
        }
    }

    //  Flush any messages decoder may have produced.
    session->flush ();
}

void xs::pgm_receiver_t::timer_event (handle_t handle_)
{
    xs_assert (handle_ == rx_timer);
    rx_timer = NULL;
    in_event (retired_fd);
}

void xs::pgm_receiver_t::drop_subscriptions ()
{
    msg_t msg;
    while (session->read (&msg))
        msg.close ();
}

#endif

