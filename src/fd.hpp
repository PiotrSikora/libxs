/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
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

#ifndef __XS_FD_HPP_INCLUDED__
#define __XS_FD_HPP_INCLUDED__

#include "platform.hpp"

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#endif

namespace xs
{

#ifdef XS_HAVE_WINDOWS
#if defined _MSC_VER &&_MSC_VER <= 1400
    typedef UINT_PTR fd_t;
    enum {retired_fd = (fd_t)(~0)};
    inline void *fdtoptr (fd_t fd_)
    {
        return (void*) (fd_ + 1);
    }
    inline fd_t ptrtofd (void *ptr_)
    {
        return ((fd_t) ptr_) - 1;
    }
#else
    typedef SOCKET fd_t;
    enum {retired_fd = INVALID_SOCKET};
    inline void *fdtoptr (fd_t fd_)
    {
        return (void*) (fd_ + 1);
    }
    inline fd_t ptrtofd (void *ptr_)
    {
        return ((fd_t) ptr_) - 1;
    }
#endif
#else
    typedef int fd_t;
    enum {retired_fd = -1};
    inline void *fdtoptr (fd_t fd_)
    {
        return (void*) (((char*) 0) + fd_ + 1);
    }
    inline fd_t ptrtofd (void *ptr_)
    {
        return ((int) ((char*) ptr_ - (char*) 0)) - 1;
    }
#endif

}

#endif
