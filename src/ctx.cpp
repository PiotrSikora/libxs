/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
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

#include "platform.hpp"
#if defined XS_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <unistd.h>
#endif

#include <new>
#include <string.h>

#include "ctx.hpp"
#include "socket_base.hpp"
#include "io_thread.hpp"
#include "monitor.hpp"
#include "reaper.hpp"
#include "pipe.hpp"
#include "err.hpp"
#include "msg.hpp"

xs::ctx_t::ctx_t (uint32_t io_threads_) :
    tag (0xbadcafe0),
    terminating (false)
{
    //  Initialise the array of mailboxes. Additional three slots are for
    //  xs_term thread and reaper thread.
    slot_count = max_sockets + io_threads_ + 3;
    slots = (mailbox_t**) malloc (sizeof (mailbox_t*) * slot_count);
    alloc_assert (slots);

    //  Initialise the infrastructure for xs_term thread.
    slots [term_tid] = &term_mailbox;

    //  Create the reaper thread.
    reaper = new (std::nothrow) reaper_t (this, reaper_tid);
    alloc_assert (reaper);
    slots [reaper_tid] = reaper->get_mailbox ();
    reaper->start ();

    //  Create I/O thread objects and launch them.
    for (uint32_t i = 2; i != io_threads_ + 2; i++) {
        io_thread_t *io_thread = new (std::nothrow) io_thread_t (this, i);
        alloc_assert (io_thread);
        io_threads.push_back (io_thread);
        slots [i] = io_thread->get_mailbox ();
        io_thread->start ();
    }

    //  In the unused part of the slot array, create a list of empty slots.
    for (int32_t i = (int32_t) slot_count - 1;
          i >= (int32_t) io_threads_ + 2; i--) {
        empty_slots.push_back (i);
        slots [i] = NULL;
    }

    //  Create the socket to send logs to.
    log_socket = create_socket (XS_PUB);
    xs_assert (log_socket);
    int linger = 0;
    int rc = log_socket->setsockopt (XS_LINGER, &linger, sizeof (linger));
    errno_assert (rc == 0);
    int hwm = 1;
    rc = log_socket->setsockopt (XS_SNDHWM, &hwm, sizeof (hwm));
    errno_assert (rc == 0);
    rc = log_socket->connect ("ipc:///tmp/xslogs.ipc");
    errno_assert (rc == 0);

    //  Create the monitor object.
    io_thread_t *io_thread = choose_io_thread (0);
    xs_assert (io_thread);
    monitor = new (std::nothrow) monitor_t (io_thread);
    alloc_assert (monitor);
    monitor->start ();
}

bool xs::ctx_t::check_tag ()
{
    return tag == 0xbadcafe0;
}

xs::ctx_t::~ctx_t ()
{
    //  Check that there are no remaining sockets.
    xs_assert (sockets.empty ());

    //  Ask I/O threads to terminate. If stop signal wasn't sent to I/O
    //  thread subsequent invocation of destructor would hang-up.
    for (io_threads_t::size_type i = 0; i != io_threads.size (); i++)
        io_threads [i]->stop ();

    //  Wait till I/O threads actually terminate.
    for (io_threads_t::size_type i = 0; i != io_threads.size (); i++)
        delete io_threads [i];

    //  Deallocate the reaper thread object.
    delete reaper;

    //  Deallocate the array of mailboxes. No special work is
    //  needed as mailboxes themselves were deallocated with their
    //  corresponding io_thread/socket objects.
    free (slots);

    //  Remove the tag, so that the object is considered dead.
    tag = 0xdeadbeef;
}

int xs::ctx_t::terminate ()
{
    //  Check whether termination was already underway, but interrupted and now
    //  restarted.
    slot_sync.lock ();
    bool restarted = terminating;
    terminating = true;
    slot_sync.unlock ();

    //  First attempt to terminate the context.
    if (!restarted) {

        //  Close the monitor object. Wait for done command from the monitor.
        monitor->stop ();
        command_t cmd;
        int rc = term_mailbox.recv (&cmd, -1);
        xs_assert (rc == 0);
        xs_assert (cmd.type == command_t::done);

        //  Close the logging socket.
        log_sync.lock ();
        rc = log_socket->close ();
        xs_assert (rc == 0);
        log_socket = NULL;
        log_sync.unlock ();

        //  First send stop command to sockets so that any blocking calls
        //  can be interrupted. If there are no sockets we can ask reaper
        //  thread to stop.
        slot_sync.lock ();
        for (sockets_t::size_type i = 0; i != sockets.size (); i++)
            sockets [i]->stop ();
        if (sockets.empty ())
            reaper->stop ();
        slot_sync.unlock ();
    }

    //  Wait till reaper thread closes all the sockets.
    command_t cmd;
    int rc = term_mailbox.recv (&cmd, -1);
    if (rc == -1 && errno == EINTR)
        return -1;
    xs_assert (rc == 0);
    xs_assert (cmd.type == command_t::done);
    slot_sync.lock ();
    xs_assert (sockets.empty ());
    slot_sync.unlock ();

    //  Deallocate the resources.
    delete this;

    return 0;
}

xs::socket_base_t *xs::ctx_t::create_socket (int type_)
{
    slot_sync.lock ();

    //  Once xs_term() was called, we can't create new sockets.
    if (terminating) {
        slot_sync.unlock ();
        errno = ETERM;
        return NULL;
    }

    //  If max_sockets limit was reached, return error.
    if (empty_slots.empty ()) {
        slot_sync.unlock ();
        errno = EMFILE;
        return NULL;
    }

    //  Choose a slot for the socket.
    uint32_t slot = empty_slots.back ();
    empty_slots.pop_back ();

    //  Generate new unique socket ID.
    int sid = ((int) max_socket_id.add (1)) + 1;

    //  Create the socket and register its mailbox.
    socket_base_t *s = socket_base_t::create (type_, this, slot, sid);
    if (!s) {
        empty_slots.push_back (slot);
        slot_sync.unlock ();
        return NULL;
    }
    sockets.push_back (s);
    slots [slot] = s->get_mailbox ();

    slot_sync.unlock ();

    return s;
}

void xs::ctx_t::destroy_socket (class socket_base_t *socket_)
{
    slot_sync.lock ();

    //  Free the associared thread slot.
    uint32_t tid = socket_->get_tid ();
    empty_slots.push_back (tid);
    slots [tid] = NULL;    

    //  Remove the socket from the list of sockets.
    sockets.erase (socket_);

    //  If xs_term() was already called and there are no more socket
    //  we can ask reaper thread to terminate.
    if (terminating && sockets.empty ())
        reaper->stop ();

    slot_sync.unlock ();
}

xs::object_t *xs::ctx_t::get_reaper ()
{
    return reaper;
}

void xs::ctx_t::send_command (uint32_t tid_, const command_t &command_)
{
    slots [tid_]->send (command_);
}

xs::io_thread_t *xs::ctx_t::choose_io_thread (uint64_t affinity_)
{
    if (io_threads.empty ())
        return NULL;

    //  Find the I/O thread with minimum load.
    int min_load = -1;
    io_threads_t::size_type result = 0;
    for (io_threads_t::size_type i = 0; i != io_threads.size (); i++) {
        if (!affinity_ || (affinity_ & (uint64_t (1) << i))) {
            int load = io_threads [i]->get_load ();
            if (min_load == -1 || load < min_load) {
                min_load = load;
                result = i;
            }
        }
    }
    xs_assert (min_load != -1);
    return io_threads [result];
}

int xs::ctx_t::register_endpoint (const char *addr_, endpoint_t &endpoint_)
{
    endpoints_sync.lock ();

    bool inserted = endpoints.insert (endpoints_t::value_type (
        std::string (addr_), endpoint_)).second;
    if (!inserted) {
        errno = EADDRINUSE;
        endpoints_sync.unlock ();
        return -1;
    }

    endpoints_sync.unlock ();
    return 0;
}

void xs::ctx_t::unregister_endpoints (socket_base_t *socket_)
{
    endpoints_sync.lock ();

    endpoints_t::iterator it = endpoints.begin ();
    while (it != endpoints.end ()) {
        if (it->second.socket == socket_) {
            endpoints_t::iterator to_erase = it;
            ++it;
            endpoints.erase (to_erase);
            continue;
        }
        ++it;
    }
        
    endpoints_sync.unlock ();
}

xs::endpoint_t xs::ctx_t::find_endpoint (const char *addr_)
{
     endpoints_sync.lock ();

     endpoints_t::iterator it = endpoints.find (addr_);
     if (it == endpoints.end ()) {
         endpoints_sync.unlock ();
         errno = ECONNREFUSED;
         endpoint_t empty = {NULL, options_t()};
         return empty;
     }
     endpoint_t *endpoint = &it->second;

     //  Increment the command sequence number of the peer so that it won't
     //  get deallocated until "bind" command is issued by the caller.
     //  The subsequent 'bind' has to be called with inc_seqnum parameter
     //  set to false, so that the seqnum isn't incremented twice.
     endpoint->socket->inc_seqnum ();

     endpoints_sync.unlock ();
     return *endpoint;
}

void xs::ctx_t::log (int sid_, const char *text_)
{
    monitor->log (sid_, text_);
}

void xs::ctx_t::publish_logs (const char *text_)
{
    log_sync.lock ();
    msg_t msg;
    msg.init_size (strlen (text_) + 1);
    memcpy (msg.data (), text_, strlen (text_) + 1);
    int rc = log_socket->send (&msg, XS_DONTWAIT);
    errno_assert (rc == 0);
    msg.close ();
    log_sync.unlock ();
}

//  The last used socket ID, or 0 if no socket was used so far. Note that this
//  is a global variable. Thus, even sockets created in different contexts have
//  unique IDs.
xs::atomic_counter_t xs::ctx_t::max_socket_id;

