/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

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

#ifndef __ZMQ_H_INCLUDED__
#define __ZMQ_H_INCLUDED__

//  ZeroMQ compatibility header file.

#include <xs.h>

#define ZMQ_USE_XS
#define ZMQ_VERSION_MAJOR XS_VERSION_MAJOR
#define ZMQ_VERSION_MINOR XS_VERSION_MINOR
#define ZMQVERSION_PATCH XS_VERSION_PATCH
#define ZMQ_MAKE_VERSION(major, minor, patch) \
    XS_MAKE_VERSION(major, ninor, patch)
#define ZMQ_VERSION XS_VERSION
#define zmq_version xs_version

#define zmq_errno xs_errno
#define zmq_strerror xs_strerror

#define zmq_msg_t xs_msg_t
#define zmq_free_fd xs_free_fn
#define zmq_msg_init xs_msg_init
#define zmq_msg_init_size xs_msg_init_size
#define zmq_msg_init_data xs_msg_init_data
#define zmq_msg_close xs_msg_close
#define zmq_msg_move xs_msg_move
#define zmq_msg_copy xs_msg_copy
#define zmq_msg_data xs_msg_data
#define zmq_msg_size xs_msg_size
#define zmq_getmsgopt xs_getmsgopt

#define zmq_init xs_init
#define zmq_term xs_term

#define ZMQ_PAIR XS_PAIR
#define ZMQ_PUB XS_PUB
#define ZMQ_SUB XS_SUB
#define ZMQ_REQ XS_REQ
#define ZMQ_REP XS_REP
#define ZMQ_XREQ XS_XREQ
#define ZMQ_XRER XS_XREP
#define ZMQ_PULL XS_PULL
#define ZMQ_PUSH XS_PUSH
#define ZMQ_XPUB XS_XPUB
#define ZMQ_XSUB XS_XSUB
#define ZMQ_ROUTER XS_ROUTER
#define ZMQ_DEALER XS_DEALER

#define ZMQ_AFFINITY XS_AFFINITY
#define ZMQ_IDENTITY XS_IDENTITY
#define ZMQ_SUBSCRIBE XS_SUBSCRIBE
#define ZMQ_UNSUBSCRIBE XS_UNSUBSCRIBE
#define ZMQ_RATE XS_RATE
#define ZMQ_RECOVERY_IVL XS_RECOVERY_IVL
#define ZMQ_SNDBUF XS_SNDBUF
#define ZMQ_RCVBUF XS_RCVBUF
#define ZMQ_RCVMORE XS_RCVMORE
#define ZMQ_FD XS_FD
#define ZMQ_EVENTS XS_EVENTS
#define ZMQ_TYPE XS_TYPE
#define ZMQ_LINGER XS_LINGER
#define ZMQ_RCONNECT_IVL XS_RECONNECT_IVL
#define ZMQ_BACKLOG XS_BACKLOG
#define ZMQ_RECONNECT_IVL_MAX XS_RECONNECT_IVL_MAX
#define ZMQ_MAXMSGSIZE XS_MAXMSGSIZE
#define ZMQ_SNDHWM XS_SNDHWM
#define ZMQ_RCVHWM XS_RCVHWM
#define ZMQ_MULTICAST_HOPS XS_MULTICAST_HOPS
#define ZMQ_RCVTIMEO XS_RCVTIMEO
#define ZMQ_SNDTIMEO XS_SNDTIMEO
#define ZMQ_IPV4ONLY XS_IPV4ONLY

#define ZMQ_MORE XS_MORE
#define ZMQ_DONTWAIT XS_DONTWAIT
#define ZMQ_SNDMORE XS_SNDMORE

#define zmq_socket xs_socket
#define zmq_close xs_close
#define zmq_setsockopt xs_setsockopt
#define zmq_getsockopt xs_getsockopt
#define zmq_bind xs_bind
#define zmq_connect xs_connect
#define zmq_send xs_send
#define zmq_recv xs_recv
#define zmq_sendmsg xs_sendmsg
#define zmq_recvmsg xs_recvmsg

#define ZMQ_POLLIN XS_POLLIN
#define ZMQ_POLLOUT XS_POLLOUT
#define ZMQ_POLLERR XS_POLLERR

#define zmq_pollitem_t xs_pollitem_t
#define zmq_poll xs_poll

#endif

