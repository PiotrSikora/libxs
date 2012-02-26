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

#ifndef __XS_PREFIX_FILTER_HPP_INCLUDED__
#define __XS_PREFIX_FILTER_HPP_INCLUDED__

namespace xs
{

    //  Canonical extension object.
    extern void *prefix_filter;

    class prefix_filter_t
    {
    public:

        prefix_filter_t ();
        ~prefix_filter_t ();

        void create (void *fid_);
        void destroy (void *fid_);
        void subscribe (void *fid_, unsigned char *data_, size_t size_);
        void unsubscribe (void *fid_, unsigned char *data_, size_t size_);
        void enumerate ();
        int match (void *fid_, unsigned char *data_, size_t size_);
        void match_all (unsigned char *data_, size_t size_);

    private:

        prefix_filter_t (const prefix_filter_t&);
        const prefix_filter_t &operator = (const prefix_filter_t&);
    };

}

#endif
