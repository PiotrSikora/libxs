/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
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

#ifndef __XS_THREAD_HPP_INCLUDED__
#define __XS_THREAD_HPP_INCLUDED__

#include "platform.hpp"

#ifdef XS_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <pthread.h>
#endif

namespace xs
{

    typedef void (thread_fn) (void*);

    //  Class encapsulating OS thread. Thread initiation/termination is done
    //  using special functions rather than in constructor/destructor so that
    //  thread isn't created during object construction by accident, causing
    //  newly created thread to access half-initialised object. Same applies
    //  to the destruction process: Thread should be terminated before object
    //  destruction begins, otherwise it can access half-destructed object.

    class thread_t
    {
    public:

        inline thread_t ()
        {
        }

        //  Creates OS thread. 'tfn' is main thread function. It'll be passed
        //  'arg' as an argument.
        void start (thread_fn *tfn_, void *arg_);

        //  Waits for thread termination.
        void stop ();

        //  These are internal members. They should be private, however then
        //  they would not be accessible from the main C routine of the thread.
        thread_fn *tfn;
        void *arg;
        
    private:

#ifdef XS_HAVE_WINDOWS
        HANDLE descriptor;
#else
        pthread_t descriptor;
#endif

        thread_t (const thread_t&);
        const thread_t &operator = (const thread_t&);
    };

}

#endif
