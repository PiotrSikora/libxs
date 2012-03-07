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

#ifndef __XS_CORE_HPP_INCLUDED__
#define __XS_CORE_HPP_INCLUDED__

#include <stddef.h>

namespace xs
{

    //  This class is not a core of Crossroads. It's rather a callback interface
    //  for extensions, ie. what's extensions see as Crossroads core.

    class core_t
    {
    public:

        core_t ();
        virtual ~core_t ();

        virtual int filter_subscribed (const unsigned char *data_,
            size_t size_);
        virtual int filter_unsubscribed (const unsigned char *data_,
            size_t size_);
        virtual int filter_matching (void *subscriber_);

    private:

        core_t (const core_t&);
        const core_t &operator = (const core_t&);
    };

}

#endif
