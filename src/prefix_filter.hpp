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

        void create (void *fid_);
        void destroy (void *fid_, void *arg_);
        int subscribe (void *fid_, unsigned char *data_, size_t size_);
        int unsubscribe (void *fid_, unsigned char *data_, size_t size_);
        void enumerate (void *arg_);
        int match (void *fid_, unsigned char *data_, size_t size_);
        void match_all (unsigned char *data_, size_t size_, void *arg_);

    private:

        static void subscribed (unsigned char *data_, size_t size_,
            void *arg_);
        static void unsubscribed (unsigned char *data_, size_t size_,
            void *arg_);
        static void matched (pipe_t *pipe_, void *arg_);

        class trie_t
        {
        public:

            trie_t ();
            ~trie_t ();

            //  Add key to the trie. Returns true if it's a new subscription
            //  rather than a duplicate.
            bool add (unsigned char *prefix_, size_t size_, xs::pipe_t *pipe_);

            //  Remove all subscriptions for a specific peer from the trie.
            //  If there are no subscriptions left on some topics, invoke the
            //  supplied callback function.
            void rm (xs::pipe_t *pipe_,
                void (*func_) (unsigned char *data_, size_t size_, void *arg_),
                void *arg_);

            //  Remove specific subscription from the trie. Return true is it was
            //  actually removed rather than de-duplicated.
            bool rm (unsigned char *prefix_, size_t size_, xs::pipe_t *pipe_);

            //  Signal all the matching pipes.
            void match (unsigned char *data_, size_t size_,
                void (*func_) (xs::pipe_t *pipe_, void *arg_), void *arg_);

            //  Apply the function supplied to each subscription in the trie.
            void apply (void (*func_) (unsigned char *data_, size_t size_,
                void *arg_), void *arg_);

            //  Check whether particular key is in the trie.
            bool check (unsigned char *data_, size_t size_);

        private:

            bool add_helper (unsigned char *prefix_, size_t size_,
                xs::pipe_t *pipe_);
            void rm_helper (xs::pipe_t *pipe_, unsigned char **buff_,
                size_t buffsize_, size_t maxbuffsize_,
                void (*func_) (unsigned char *data_, size_t size_, void *arg_),
                void *arg_);
            bool rm_helper (unsigned char *prefix_, size_t size_,
                xs::pipe_t *pipe_);
            void apply_helper (
                unsigned char **buff_, size_t buffsize_, size_t maxbuffsize_,
                void (*func_) (unsigned char *data_, size_t size_, void *arg_),
                void *arg_);
            bool is_redundant () const;

            //  Pointer to particular pipe associated with the reference count.
            typedef std::map <xs::pipe_t*, int> pipes_t;
            pipes_t *pipes;

            unsigned char min;
            unsigned short count;
            unsigned short live_nodes;
            union {
                class trie_t *node;
                class trie_t **table;
            } next;

            trie_t (const trie_t&);
            const trie_t &operator = (const trie_t&);
        };

        trie_t trie;

        prefix_filter_t (const prefix_filter_t&);
        const prefix_filter_t &operator = (const prefix_filter_t&);
    };

}

#endif
