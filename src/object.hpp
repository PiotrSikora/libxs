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

#ifndef __XS_OBJECT_HPP_INCLUDED__
#define __XS_OBJECT_HPP_INCLUDED__

#include "stdint.hpp"

namespace xs
{

    struct i_engine;
    struct endpoint_t;
    struct command_t;
    class ctx_t;
    class pipe_t;
    class socket_base_t;
    class session_base_t;
    class io_thread_t;
    class own_t;

    //  Base class for all objects that participate in inter-thread
    //  communication.

    class object_t
    {
    public:

        object_t (xs::ctx_t *ctx_, uint32_t tid_);
        object_t (object_t *parent_);
        virtual ~object_t ();

        uint32_t get_tid ();
        ctx_t *get_ctx ();
        void process_command (xs::command_t &cmd_);

    protected:

        //  Using following function, socket is able to access global
        //  repository of inproc endpoints.
        int register_endpoint (const char *addr_, xs::endpoint_t &endpoint_);
        void unregister_endpoints (xs::socket_base_t *socket_);
        xs::endpoint_t find_endpoint (const char *addr_);
        void destroy_socket (xs::socket_base_t *socket_);

        //  Chooses least loaded I/O thread.
        xs::io_thread_t *choose_io_thread (uint64_t affinity_);

        //  Logging related functions.
        void log (int sid_, const char *text_);
        void publish_logs (const char *textr_);

        //  Derived object can use these functions to send commands
        //  to other objects.
        void send_stop ();
        void send_plug (xs::own_t *destination_,
            bool inc_seqnum_ = true);
        void send_own (xs::own_t *destination_,
            xs::own_t *object_);
        void send_attach (xs::session_base_t *destination_,
             xs::i_engine *engine_, bool inc_seqnum_ = true);
        void send_bind (xs::own_t *destination_, xs::pipe_t *pipe_,
             bool inc_seqnum_ = true);
        void send_activate_read (xs::pipe_t *destination_);
        void send_activate_write (xs::pipe_t *destination_,
             uint64_t msgs_read_);
        void send_hiccup (xs::pipe_t *destination_, void *pipe_);
        void send_pipe_term (xs::pipe_t *destination_);
        void send_pipe_term_ack (xs::pipe_t *destination_);
        void send_term_req (xs::own_t *destination_,
            xs::own_t *object_);
        void send_term (xs::own_t *destination_, int linger_);
        void send_term_ack (xs::own_t *destination_);
        void send_reap (xs::socket_base_t *socket_);
        void send_reaped ();
        void send_done ();

        //  These handlers can be overloaded by the derived objects. They are
        //  called when command arrives from another thread.
        virtual void process_stop ();
        virtual void process_plug ();
        virtual void process_own (xs::own_t *object_);
        virtual void process_attach (xs::i_engine *engine_);
        virtual void process_bind (xs::pipe_t *pipe_);
        virtual void process_activate_read ();
        virtual void process_activate_write (uint64_t msgs_read_);
        virtual void process_hiccup (void *pipe_);
        virtual void process_pipe_term ();
        virtual void process_pipe_term_ack ();
        virtual void process_term_req (xs::own_t *object_);
        virtual void process_term (int linger_);
        virtual void process_term_ack ();
        virtual void process_reap (xs::socket_base_t *socket_);
        virtual void process_reaped ();

        //  Special handler called after a command that requires a seqnum
        //  was processed. The implementation should catch up with its counter
        //  of processed commands here.
        virtual void process_seqnum ();

    private:

        //  Context provides access to the global state.
        xs::ctx_t *ctx;

        //  Thread ID of the thread the object belongs to.
        uint32_t tid;

        void send_command (command_t &cmd_);

        object_t (const object_t&);
        const object_t &operator = (const object_t&);
    };

}

#endif
