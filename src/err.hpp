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

#ifndef __XS_ERR_HPP_INCLUDED__
#define __XS_ERR_HPP_INCLUDED__

//  Crossroads-specific error codes are defined in xs.h
#include "../include/xs.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "platform.hpp"
#include "likely.hpp"

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <netdb.h>
#endif

namespace xs
{
    const char *errno_to_string (int errno_);
    void xs_abort (const char *errmsg_);
}

#ifdef XS_HAVE_WINDOWS

namespace xs
{
    const char *wsa_error ();
    const char *wsa_error_no (int no_);
    void win_error (char *buffer_, size_t buffer_size_);
    void wsa_error_to_errno ();
}

//  Provides convenient way to check WSA-style errors on Windows.
#define wsa_assert(x) \
    do {\
        if (unlikely (!(x))) {\
            const char *errstr = xs::wsa_error ();\
            if (errstr != NULL) {\
                fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
                    __FILE__, __LINE__);\
                xs::xs_abort (errstr);\
            }\
        }\
    } while (false)

//  Provides convenient way to assert on WSA-style errors on Windows.
#define wsa_assert_no(no) \
    do {\
        const char *errstr = xs::wsa_error_no (no);\
        if (errstr != NULL) {\
            fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
                __FILE__, __LINE__);\
            xs::xs_abort (errstr);\
        }\
    } while (false)

// Provides convenient way to check GetLastError-style errors on Windows.
#define win_assert(x) \
    do {\
        if (unlikely (!(x))) {\
            char errstr [256];\
            xs::win_error (errstr, 256);\
            fprintf (stderr, "Assertion failed: %s (%s:%d)\n", errstr, \
                __FILE__, __LINE__);\
            xs::xs_abort (errstr);\
        }\
    } while (false)

#endif

//  This macro works in exactly the same way as the normal assert. It is used
//  in its stead because standard assert on Win32 in broken - it prints nothing
//  when used within the scope of JNI library.
#define xs_assert(x) \
    do {\
        if (unlikely (!(x))) {\
            fprintf (stderr, "Assertion failed: %s (%s:%d)\n", #x, \
                __FILE__, __LINE__);\
            xs::xs_abort (#x);\
        }\
    } while (false) 

//  Provides convenient way to check for errno-style errors.
#define errno_assert(x) \
    do {\
        if (unlikely (!(x))) {\
            const char *errstr = strerror (errno);\
            fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
            xs::xs_abort (errstr);\
        }\
    } while (false)

//  Provides convenient way to check for POSIX errors.
#define posix_assert(x) \
    do {\
        if (unlikely (x)) {\
            const char *errstr = strerror (x);\
            fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
            xs::xs_abort (errstr);\
        }\
    } while (false)

//  Provides convenient way to check for errors from getaddrinfo.
#define gai_assert(x) \
    do {\
        if (unlikely (x)) {\
            const char *errstr = gai_strerror (x);\
            fprintf (stderr, "%s (%s:%d)\n", errstr, __FILE__, __LINE__);\
            xs::xs_abort (errstr);\
        }\
    } while (false)

//  Provides convenient way to check whether memory allocation have succeeded.
#define alloc_assert(x) \
    do {\
        if (unlikely (!x)) {\
            fprintf (stderr, "FATAL ERROR: OUT OF MEMORY (%s:%d)\n",\
                __FILE__, __LINE__);\
            xs::xs_abort ("FATAL ERROR: OUT OF MEMORY");\
        }\
    } while (false)

#endif


