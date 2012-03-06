/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2011-2012 Spotify AB
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

#include <stddef.h>
#include <map>

#include "stdint.hpp"

namespace xs
{

    //  Canonical extension object.
    extern void *prefix_filter;

    class pipe_t;

    class prefix_filter_t
    {
    public:

        prefix_filter_t ();
        ~prefix_filter_t ();

        int subscribe (void *subscriber_,
            const unsigned char *data_, size_t size_);
        int unsubscribe (void *subscriber_,
            const unsigned char *data_, size_t size_);
        void unsubscribe_all (void *subscriber_, void *arg_);
        void enumerate (void *arg_);
        int match (const unsigned char *data_, size_t size_);
        void match_all (const unsigned char *data_, size_t size_, void *arg_);

    private:

        struct node_t
        {
            //  Pointer to particular pipe associated with the reference count.
            typedef std::map <xs::pipe_t*, int> pipes_t;
            pipes_t *pipes;

            unsigned char min;
            unsigned short count;
            unsigned short live_nodes;
            union {
                class node_t *node;
                class node_t **table;
            } next;

        };

        static void init (node_t *node_);
        static void close (node_t *node_);

        //  Add key to the trie. Returns true if it's a new subscription
        //  rather than a duplicate.
        static bool add (node_t *node_, const unsigned char *prefix_,
            size_t size_, xs::pipe_t *pipe_);

        //  Remove specific subscription from the trie. Return true is it
        //  was actually removed rather than de-duplicated.
        static bool rm (node_t *node_, const unsigned char *prefix_,
            size_t size_, xs::pipe_t *pipe_);

        //  Remove all subscriptions for a specific peer from the trie.
        //  If there are no subscriptions left on some topics, invoke the
        //  supplied callback function.
        static void rm (node_t *node_, xs::pipe_t *pipe_, void *arg_);

        static void rm_helper (node_t *node_, xs::pipe_t *pipe_,
            unsigned char **buff_, size_t buffsize_, size_t maxbuffsize_,
            void *arg_);

        //  Lists all the subscriptions in the trie.
        static void list (node_t *node_, unsigned char **buff_,
            size_t buffsize_, size_t maxbuffsize_, void *arg_);

        //  Checks whether node can be safely removed.
        static bool is_redundant (node_t *node_);

        node_t root;

        prefix_filter_t (const prefix_filter_t&);
        const prefix_filter_t &operator = (const prefix_filter_t&);
    };

}

#endif
