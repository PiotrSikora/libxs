/*
    Copyright (c) 2010-2012 250bpm s.r.o.
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

#ifndef __XS_POLLER_HPP_INCLUDED__
#define __XS_POLLER_HPP_INCLUDED__

#include "platform.hpp"

#if defined XS_FORCE_SELECT
#define XS_USE_SELECT
#include "select.hpp"
#elif defined XS_FORCE_POLL
#define XS_USE_POLL
#include "poll.hpp"
#elif defined XS_FORCE_EPOLL
#define XS_USE_EPOLL
#include "epoll.hpp"
#elif defined XS_FORCE_DEVPOLL
#define XS_USE_DEVPOLL
#include "devpoll.hpp"
#elif defined XS_FORCE_KQUEUE
#define XS_USE_KQUEUE
#include "kqueue.hpp"
#elif defined XS_HAVE_LINUX
#define XS_USE_EPOLL
#include "epoll.hpp"
#elif defined XS_HAVE_WINDOWS
#define XS_USE_SELECT
#include "select.hpp"
#elif defined XS_HAVE_FREEBSD
#define XS_USE_KQUEUE
#include "kqueue.hpp"
#elif defined XS_HAVE_OPENBSD
#define XS_USE_KQUEUE
#include "kqueue.hpp"
#elif defined XS_HAVE_NETBSD
#define XS_USE_KQUEUE
#include "kqueue.hpp"
#elif defined XS_HAVE_SOLARIS
#define XS_USE_DEVPOLL
#include "devpoll.hpp"
#elif defined XS_HAVE_OSX
#define XS_USE_KQUEUE
#include "kqueue.hpp"
#elif defined XS_HAVE_QNXNTO
#define XS_USE_POLL
#include "poll.hpp"
#elif defined XS_HAVE_AIX
#define XS_USE_POLL
#include "poll.hpp"
#elif defined XS_HAVE_HPUX
#define XS_USE_DEVPOLL
#include "devpoll.hpp"
#elif defined XS_HAVE_OPENVMS
#define XS_USE_SELECT
#include "select.hpp"
#elif defined XS_HAVE_CYGWIN
#define XS_USE_SELECT
#include "select.hpp"
#else
#error Unsupported platform
#endif

#endif
