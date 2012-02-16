/*
    Copyright (c) 2010-2012 250bpm s.r.o.
    Copyright (c) 2010-2011 Other contributors as noted in the AUTHORS file

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

#ifndef __XS_SIGNALER_HPP_INCLUDED__
#define __XS_SIGNALER_HPP_INCLUDED__

#include "fd.hpp"

namespace xs
{

    //  This is a cross-platform equivalent to signal_fd. However, as opposed
    //  to signal_fd there can be at most one signal in the signaler at any
    //  given moment. Attempt to send a signal before receiving the previous
    //  one will result in undefined behaviour.

    class signaler_t
    {
    public:

        signaler_t ();
        ~signaler_t ();

        fd_t get_fd ();
        void send ();
        int wait (int timeout_);
        void recv ();
        
    private:

        //  Creates a pair of filedescriptors that will be used
        //  to pass the signals.
        static int make_fdpair (fd_t *r_, fd_t *w_);

        //  Underlying write & read file descriptor.
        fd_t w;
        fd_t r;

        //  Disable copying of signaler_t object.
        signaler_t (const signaler_t&);
        const signaler_t &operator = (const signaler_t&);
    };

}

#endif
