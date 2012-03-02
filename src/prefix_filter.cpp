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

#include <new>

#include "../include/xs_filter.h"

#include "prefix_filter.hpp"
#include "err.hpp"

xs::prefix_filter_t::prefix_filter_t ()
{
}

xs::prefix_filter_t::~prefix_filter_t ()
{
}

void xs::prefix_filter_t::create (void *fid_)
{
    //  Do nothing. There's no need to register filters explicitly.
}

void xs::prefix_filter_t::destroy (void *fid_, void *arg_)
{
    trie.rm ((pipe_t*) fid_, unsubscribed, arg_);
}

int xs::prefix_filter_t::subscribe (void *fid_, unsigned char *data_,
    size_t size_)
{
    return trie.add (data_, size_, (pipe_t*) fid_) ? 1 : 0;
}

int xs::prefix_filter_t::unsubscribe (void *fid_, unsigned char *data_,
    size_t size_)
{
    return trie.rm (data_, size_, (pipe_t*) fid_) ? 1 : 0;
}

void xs::prefix_filter_t::enumerate (void *arg_)
{
    trie.apply (subscribed, arg_);
}

int xs::prefix_filter_t::match (void *fid_, unsigned char *data_, size_t size_)
{
    //  TODO: What should we do with fid_?
    return trie.check (data_, size_) ? 1 : 0;
}

void xs::prefix_filter_t::match_all (unsigned char *data_, size_t size_,
    void *arg_)
{
    trie.match (data_, size_, matched, arg_);
}

void xs::prefix_filter_t::subscribed (unsigned char *data_, size_t size_,
    void *arg_)
{
    xs_filter_subscribed (data_, size_, arg_);
}

void xs::prefix_filter_t::unsubscribed (unsigned char *data_, size_t size_,
    void *arg_)
{
    xs_filter_unsubscribed (data_, size_, arg_);
}

void xs::prefix_filter_t::matched (pipe_t *pipe_, void *arg_)
{
    xs_filter_matching (pipe_, arg_);
}

xs::prefix_filter_t::trie_t::trie_t () :
    pipes (0),
    min (0),
    count (0),
    live_nodes (0)
{
}

xs::prefix_filter_t::trie_t::~trie_t ()
{
    if (pipes) {
        delete pipes;
        pipes = 0;
    }

    if (count == 1) {
        xs_assert (next.node);
        delete next.node;
        next.node = 0;
    }
    else if (count > 1) {
        for (unsigned short i = 0; i != count; ++i)
            if (next.table [i])
                delete next.table [i];
        free (next.table);
    }
}

bool xs::prefix_filter_t::trie_t::add (unsigned char *prefix_, size_t size_,
    pipe_t *pipe_)
{
    return add_helper (prefix_, size_, pipe_);
}

bool xs::prefix_filter_t::trie_t::add_helper (unsigned char *prefix_,
    size_t size_, pipe_t *pipe_)
{
    //  We are at the node corresponding to the prefix. We are done.
    if (!size_) {
        bool result = !pipes;
        if (!pipes)
            pipes = new pipes_t;
        pipes_t::iterator it =
            pipes->insert (pipes_t::value_type (pipe_, 0)).first;
        ++it->second;
        return result;
    }

    unsigned char c = *prefix_;
    if (c < min || c >= min + count) {

        //  The character is out of range of currently handled
        //  charcters. We have to extend the table.
        if (!count) {
            min = c;
            count = 1;
            next.node = NULL;
        }
        else if (count == 1) {
            unsigned char oldc = min;
            trie_t *oldp = next.node;
            count = (min < c ? c - min : min - c) + 1;
            next.table = (trie_t**)
                malloc (sizeof (trie_t*) * count);
            xs_assert (next.table);
            for (unsigned short i = 0; i != count; ++i)
                next.table [i] = 0;
            min = std::min (min, c);
            next.table [oldc - min] = oldp;
        }
        else if (min < c) {

            //  The new character is above the current character range.
            unsigned short old_count = count;
            count = c - min + 1;
            next.table = (trie_t**) realloc ((void*) next.table,
                sizeof (trie_t*) * count);
            xs_assert (next.table);
            for (unsigned short i = old_count; i != count; i++)
                next.table [i] = NULL;
        }
        else {

            //  The new character is below the current character range.
            unsigned short old_count = count;
            count = (min + old_count) - c;
            next.table = (trie_t**) realloc ((void*) next.table,
                sizeof (trie_t*) * count);
            xs_assert (next.table);
            memmove (next.table + min - c, next.table,
                old_count * sizeof (trie_t*));
            for (unsigned short i = 0; i != min - c; i++)
                next.table [i] = NULL;
            min = c;
        }
    }

    //  If next node does not exist, create one.
    if (count == 1) {
        if (!next.node) {
            next.node = new (std::nothrow) trie_t;
            ++live_nodes;
            xs_assert (next.node);
        }
        return next.node->add_helper (prefix_ + 1, size_ - 1, pipe_);
    }
    else {
        if (!next.table [c - min]) {
            next.table [c - min] = new (std::nothrow) trie_t;
            ++live_nodes;
            xs_assert (next.table [c - min]);
        }
        return next.table [c - min]->add_helper (prefix_ + 1, size_ - 1, pipe_);
    }
}


void xs::prefix_filter_t::trie_t::rm (pipe_t *pipe_,
    void (*func_) (unsigned char *data_, size_t size_, void *arg_),
    void *arg_)
{
    unsigned char *buff = NULL;
    rm_helper (pipe_, &buff, 0, 0, func_, arg_);
    free (buff);
}

void xs::prefix_filter_t::trie_t::rm_helper (pipe_t *pipe_,
    unsigned char **buff_, size_t buffsize_, size_t maxbuffsize_,
    void (*func_) (unsigned char *data_, size_t size_, void *arg_),
    void *arg_)
{
    //  Remove the subscription from this node.
    if (pipes) {
        pipes_t::iterator it = pipes->find (pipe_);
        if (it != pipes->end ()) {
            xs_assert (it->second);
            --it->second;
            if (!it->second) {
                pipes->erase (it);
                if (pipes->empty ()) {
                    func_ (*buff_, buffsize_, arg_);
                    delete pipes;
                    pipes = 0;
                }
            }
        }
    }

    //  Adjust the buffer.
    if (buffsize_ >= maxbuffsize_) {
        maxbuffsize_ = buffsize_ + 256;
        *buff_ = (unsigned char*) realloc (*buff_, maxbuffsize_);
        alloc_assert (*buff_);
    }

    //  If there are no subnodes in the trie, return.
    if (count == 0)
        return;

    //  If there's one subnode (optimisation).
    if (count == 1) {
        (*buff_) [buffsize_] = min;
        buffsize_++;
        next.node->rm_helper (pipe_, buff_, buffsize_, maxbuffsize_,
            func_, arg_);

        //  Prune the node if it was made redundant by the removal
        if (next.node->is_redundant ()) {
            delete next.node;
            next.node = 0;
            count = 0;
            --live_nodes;
            xs_assert (live_nodes == 0);
        }
        return;
    }

    //  If there are multiple subnodes.
    //
    //  New min non-null character in the node table after the removal.
    unsigned char new_min = min;
    //  New max non-null character in the node table after the removal.
    unsigned char new_max = min + count - 1;
    for (unsigned short c = 0; c != count; c++) {
        (*buff_) [buffsize_] = min + c;
        if (next.table [c]) {
            next.table [c]->rm_helper (pipe_, buff_, buffsize_ + 1,
                maxbuffsize_, func_, arg_);

            //  Prune redudant nodes from the the trie.
            if (next.table [c]->is_redundant ()) {
                delete next.table [c];
                next.table [c] = 0;

                xs_assert (live_nodes > 0);
                --live_nodes;
            }
            else {
                if (c + min > new_min)
                    new_min = c + min;
                if (c + min < new_max)
                    new_max = c + min;
            }
        }
    }

    xs_assert (count > 1);

    //  Compact the node table if possible.
    if (live_nodes == 1) {
        //  If there's only one live node in the table we can
        //  switch to using the more compact single-node
        //  representation.
        xs_assert (new_min == new_max);
        xs_assert (new_min >= min && new_min < min + count);
        trie_t *node = next.table [new_min - min];
        xs_assert (node);
        free (next.table);
        next.node = node;
        count = 1;
        min = new_min;
    }
    else if (live_nodes > 1 && (new_min > min || new_max < min + count - 1)) {
        xs_assert (new_max - new_min + 1 > 1);

        trie_t **old_table = next.table;
        xs_assert (new_min > min || new_max < min + count - 1);
        xs_assert (new_min >= min);
        xs_assert (new_max <= min + count - 1);
        xs_assert (new_max - new_min + 1 < count);

        count = new_max - new_min + 1;
        next.table = (trie_t**) malloc (sizeof (trie_t*) * count);
        xs_assert (next.table);

        memmove (next.table, old_table + (new_min - min),
                 sizeof (trie_t*) * count);
        free (old_table);

        min = new_min;
    }
}

bool xs::prefix_filter_t::trie_t::rm (unsigned char *prefix_, size_t size_,
    pipe_t *pipe_)
{
    return rm_helper (prefix_, size_, pipe_);
}

bool xs::prefix_filter_t::trie_t::rm_helper (unsigned char *prefix_,
    size_t size_, pipe_t *pipe_)
{
    if (!size_) {

        //  Remove the subscription from this node.
        if (pipes) {
            pipes_t::iterator it = pipes->find (pipe_);
            if (it != pipes->end ()) {
                xs_assert (it->second);
                --it->second;
                if (!it->second) {
                    pipes->erase (it);
                    if (pipes->empty ()) {
                        delete pipes;
                        pipes = 0;
                    }
                }
            }
        }
        return !pipes;
    }

    unsigned char c = *prefix_;
    if (!count || c < min || c >= min + count)
        return false;

    trie_t *next_node =
        count == 1 ? next.node : next.table [c - min];

    if (!next_node)
        return false;

    bool ret = next_node->rm_helper (prefix_ + 1, size_ - 1, pipe_);

    if (next_node->is_redundant ()) {
        delete next_node;
        xs_assert (count > 0);

        if (count == 1) {
            next.node = 0;
            count = 0;
            --live_nodes;
            xs_assert (live_nodes == 0);
        }
        else {
            next.table [c - min] = 0;
            xs_assert (live_nodes > 1);
            --live_nodes;

            //  Compact the table if possible.
            if (live_nodes == 1) {
                //  If there's only one live node in the table we can
                //  switch to using the more compact single-node
                //  representation
                trie_t *node = 0;
                for (unsigned short i = 0; i < count; ++i) {
                    if (next.table [i]) {
                        node = next.table [i];
                        min = i + min;
                        break;
                    }
                }

                xs_assert (node);
                free (next.table);
                next.node = node;
                count = 1;
            }
            else if (c == min) {
                //  We can compact the table "from the left".
                unsigned char new_min = min;
                for (unsigned short i = 1; i < count; ++i) {
                    if (next.table [i]) {
                        new_min = i + min;
                        break;
                    }
                }
                xs_assert (new_min != min);

                trie_t **old_table = next.table;
                xs_assert (new_min > min);
                xs_assert (count > new_min - min);

                count = count - (new_min - min);
                next.table = (trie_t**) malloc (sizeof (trie_t*) * count);
                xs_assert (next.table);

                memmove (next.table, old_table + (new_min - min),
                         sizeof (trie_t*) * count);
                free (old_table);

                min = new_min;
            }
            else if (c == min + count - 1) {
                //  We can compact the table "from the right".
                unsigned short new_count = count;
                for (unsigned short i = 1; i < count; ++i) {
                    if (next.table [count - 1 - i]) {
                        new_count = count - i;
                        break;
                    }
                }
                xs_assert (new_count != count);
                count = new_count;

                trie_t **old_table = next.table;
                next.table = (trie_t**) malloc (sizeof (trie_t*) * count);
                xs_assert (next.table);

                memmove (next.table, old_table, sizeof (trie_t*) * count);
                free (old_table);
            }
        }
    }

    return ret;
}

void xs::prefix_filter_t::trie_t::match (unsigned char *data_, size_t size_,
    void (*func_) (pipe_t *pipe_, void *arg_), void *arg_)
{
    trie_t *current = this;
    while (true) {

        //  Signal the pipes attached to this node.
        if (current->pipes) {
            for (pipes_t::iterator it = current->pipes->begin ();
                  it != current->pipes->end (); ++it)
                func_ (it->first, arg_);
        }

        //  If we are at the end of the message, there's nothing more to match.
        if (!size_)
            break;

        //  If there are no subnodes in the trie, return.
        if (current->count == 0)
            break;

        //  If there's one subnode (optimisation).
		if (current->count == 1) {
            if (data_ [0] != current->min)
                break;
            current = current->next.node;
            data_++;
            size_--;
		    continue;
		}

		//  If there are multiple subnodes.
        if (data_ [0] < current->min || data_ [0] >=
              current->min + current->count)
            break;
        if (!current->next.table [data_ [0] - current->min])
            break;
        current = current->next.table [data_ [0] - current->min];
        data_++;
        size_--;
    }
}

void xs::prefix_filter_t::trie_t::apply (void (*func_) (unsigned char *data_,
    size_t size_, void *arg_), void *arg_)
{
    unsigned char *buff = NULL;
    apply_helper (&buff, 0, 0, func_, arg_);
    free (buff);
}

void xs::prefix_filter_t::trie_t::apply_helper (
    unsigned char **buff_, size_t buffsize_, size_t maxbuffsize_,
    void (*func_) (unsigned char *data_, size_t size_, void *arg_), void *arg_)
{
    //  If this node is a subscription, apply the function.
    if (pipes)
        func_ (*buff_, buffsize_, arg_);

    //  Adjust the buffer.
    if (buffsize_ >= maxbuffsize_) {
        maxbuffsize_ = buffsize_ + 256;
        *buff_ = (unsigned char*) realloc (*buff_, maxbuffsize_);
        xs_assert (*buff_);
    }

    //  If there are no subnodes in the trie, return.
    if (count == 0)
        return;

    //  If there's one subnode (optimisation).
    if (count == 1) {
        (*buff_) [buffsize_] = min;
        buffsize_++;
        next.node->apply_helper (buff_, buffsize_, maxbuffsize_, func_, arg_);
        return;
    }

    //  If there are multiple subnodes.
    for (unsigned short c = 0; c != count; c++) {
        (*buff_) [buffsize_] = min + c;
        if (next.table [c])
            next.table [c]->apply_helper (buff_, buffsize_ + 1, maxbuffsize_,
                func_, arg_);
    }
}

bool xs::prefix_filter_t::trie_t::check (unsigned char *data_, size_t size_)
{
    //  This function is on critical path. It deliberately doesn't use
    //  recursion to get a bit better performance.
    trie_t *current = this;
    while (true) {

        //  We've found a corresponding subscription!
        if (current->pipes)
            return true;

        //  We've checked all the data and haven't found matching subscription.
        if (!size_)
            return false;

        //  If there's no corresponding slot for the first character
        //  of the prefix, the message does not match.
        unsigned char c = *data_;
        if (c < current->min || c >= current->min + current->count)
            return false;

        //  Move to the next character.
        if (current->count == 1)
            current = current->next.node;
        else {
            current = current->next.table [c - current->min];
            if (!current)
                return false;
        }
        data_++;
        size_--;
    }
}

bool xs::prefix_filter_t::trie_t::is_redundant () const
{
    return !pipes && live_nodes == 0;
}

//  Implementation of the C interface of the filter.

static void *fset_create ()
{
    void *fset = (void*) new (std::nothrow) xs::prefix_filter_t;
    alloc_assert (fset);
    return fset;
}

static void fset_destroy (void *fset_)
{
    delete (xs::prefix_filter_t*) fset_;
}

static void create (void *fset_, void *fid_)
{
    ((xs::prefix_filter_t*) fset_)->create (fid_);
}

static void destroy (void *fset_, void *fid_, void *arg_)
{
    ((xs::prefix_filter_t*) fset_)->destroy (fid_, arg_);
}

static int subscribe (void *fset_, void *fid_, unsigned char *data_,
    size_t size_)
{
    return ((xs::prefix_filter_t*) fset_)->subscribe (fid_, data_, size_);
}

static int unsubscribe (void *fset_, void *fid_, unsigned char *data_,
    size_t size_)
{
    return ((xs::prefix_filter_t*) fset_)->unsubscribe (fid_, data_, size_);
}

static void enumerate (void *fset_, void *arg_)
{
    ((xs::prefix_filter_t*) fset_)->enumerate (arg_);
}

static int match (void *fset_, void *fid_, unsigned char *data_, size_t size_)
{
    return ((xs::prefix_filter_t*) fset_)->match (fid_, data_, size_);
}

static void match_all (void *fset_, unsigned char *data_, size_t size_,
    void *arg_)
{
    ((xs::prefix_filter_t*) fset_)->match_all (data_, size_, arg_);
}

#define XS_PREFIX_FILTER 1

static xs_filter_t filter = {
    XS_EXTENSION_FILTER,
    XS_PREFIX_FILTER,
    fset_create,
    fset_destroy,
    create,
    destroy,
    subscribe,
    unsubscribe,
    enumerate,
    match,
    match_all
};

void *xs::prefix_filter = (void*) &filter;

