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

#ifndef __XS_MUTEX_HPP_INCLUDED__
#define __XS_MUTEX_HPP_INCLUDED__

#include "platform.hpp"
#include "err.hpp"

//  Mutex class encapsulates OS mutex in a platform-independent way.

#ifdef XS_HAVE_WINDOWS

#include "windows.hpp"

namespace xs
{

    class mutex_t
    {
    public:
        inline mutex_t (bool fake_ = false) :
            fake (fake_)
        {
            if (!fake)
                InitializeCriticalSection (&cs);
        }

        inline ~mutex_t ()
        {
            if (!fake)
                DeleteCriticalSection (&cs);
        }

        inline void lock ()
        {
            if (!fake)
                EnterCriticalSection (&cs);
        }

        inline void unlock ()
        {
            if (!fake)
                LeaveCriticalSection (&cs);
        }

    private:

        bool fake;
        CRITICAL_SECTION cs;

        //  Disable copy construction and assignment.
        mutex_t (const mutex_t&);
        void operator = (const mutex_t&);
    };

}

#else

#include <pthread.h>

namespace xs
{
 
    class mutex_t
    {
    public:
        inline mutex_t (bool fake_ = false) :
            fake (fake_)
        {
            if (!fake) {
                int rc = pthread_mutex_init (&mutex, NULL);
                if (rc)
                    posix_assert (rc);
            }
        }
 
        inline ~mutex_t ()
        {
            if (!fake) {
                int rc = pthread_mutex_destroy (&mutex);
                if (rc)
                    posix_assert (rc);
            }
        }
 
        inline void lock ()
        {
            if (!fake) {
                int rc = pthread_mutex_lock (&mutex);
                if (rc)
                    posix_assert (rc);
            }
        }
 
        inline void unlock ()
        {
            if (!fake) {
                int rc = pthread_mutex_unlock (&mutex);
                if (rc)
                    posix_assert (rc);
            }
        }
 
    private:
 
        bool fake;
        pthread_mutex_t mutex;
 
        // Disable copy construction and assignment.
        mutex_t (const mutex_t&);
        const mutex_t &operator = (const mutex_t&);
    };
 
}

#endif

#endif
