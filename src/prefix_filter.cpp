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

#include "../include/xs.h"
#include "../include/xs_filter.h"

#include "prefix_filter.hpp"
#include "err.hpp"

xs::prefix_filter_t::prefix_filter_t ()
{
    init (&root);
}

xs::prefix_filter_t::~prefix_filter_t ()
{
    close (&root);
}

int xs::prefix_filter_t::subscribe (void *core_, void *subscriber_,
    const unsigned char *data_, size_t size_)
{
    return add (&root, data_, size_, (pipe_t*) subscriber_) ? 1 : 0;
}

int xs::prefix_filter_t::unsubscribe (void *core_, void *subscriber_,
    const unsigned char *data_, size_t size_)
{
    return rm (&root, data_, size_, (pipe_t*) subscriber_) ? 1 : 0;
}

void xs::prefix_filter_t::unsubscribe_all (void *core_, void *subscriber_)
{
    rm (&root, (pipe_t*) subscriber_, core_);
}

void xs::prefix_filter_t::enumerate (void *core_)
{
    unsigned char *buff = NULL;
    list (&root, &buff, 0, 0, core_);
    free (buff);
}

int xs::prefix_filter_t::match (void *core_,
    const unsigned char *data_, size_t size_)
{
    //  This function is on critical path. It deliberately doesn't use
    //  recursion to get a bit better performance.
    node_t *current = &root;
    while (true) {

        //  We've found a corresponding subscription!
        if (current->pipes)
            return 1;

        //  We've checked all the data and haven't found matching subscription.
        if (!size_)
            return 0;

        //  If there's no corresponding slot for the first character
        //  of the prefix, the message does not match.
        unsigned char c = *data_;
        if (c < current->min || c >= current->min + current->count)
            return 0;

        //  Move to the next character.
        if (current->count == 1)
            current = current->next.node;
        else {
            current = current->next.table [c - current->min];
            if (!current)
                return 0;
        }
        data_++;
        size_--;
    }

}

void xs::prefix_filter_t::match_all (void *core_,
    const unsigned char *data_, size_t size_)
{
    node_t *current = &root;
    while (true) {

        //  Signal the pipes attached to this node.
        if (current->pipes) {
            for (node_t::pipes_t::iterator it = current->pipes->begin ();
                  it != current->pipes->end (); ++it)
                xs_filter_matching (core_, it->first);
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

void xs::prefix_filter_t::init (node_t *node_)
{
    node_->pipes = NULL;
    node_->min = 0;
    node_->count = 0;
    node_->live_nodes = 0;
}

void xs::prefix_filter_t::close (node_t *node_)
{
    if (node_->pipes) {
        delete node_->pipes;
        node_->pipes = NULL;
    }

    if (node_->count == 1) {
        xs_assert (node_->next.node);
        close (node_->next.node);
        delete node_->next.node;
        node_->next.node = NULL;
    }
    else if (node_->count > 1) {
        for (unsigned short i = 0; i != node_->count; ++i)
            if (node_->next.table [i]) {
                close (node_->next.table [i]);
                delete node_->next.table [i];
            }
        free (node_->next.table);
    }
}

bool xs::prefix_filter_t::add (node_t *node_, const unsigned char *prefix_,
    size_t size_, pipe_t *pipe_)
{
    //  We are at the node corresponding to the prefix. We are done.
    if (!size_) {
        bool result = !node_->pipes;
        if (!node_->pipes)
            node_->pipes = new (std::nothrow) node_t::pipes_t;
        node_t::pipes_t::iterator it =
            node_->pipes->insert (node_t::pipes_t::value_type (pipe_, 0)).first;
        ++it->second;
        return result;
    }

    unsigned char c = *prefix_;
    if (c < node_->min || c >= node_->min + node_->count) {

        //  The character is out of range of currently handled
        //  charcters. We have to extend the table.
        if (!node_->count) {
            node_->min = c;
            node_->count = 1;
            node_->next.node = NULL;
        }
        else if (node_->count == 1) {
            unsigned char oldc = node_->min;
            node_t *oldp = node_->next.node;
            node_->count =
                (node_->min < c ? c - node_->min : node_->min - c) + 1;
            node_->next.table = (node_t**)
                malloc (sizeof (node_t*) * node_->count);
            xs_assert (node_->next.table);
            for (unsigned short i = 0; i != node_->count; ++i)
                node_->next.table [i] = 0;
            node_->min = std::min (node_->min, c);
            node_->next.table [oldc - node_->min] = oldp;
        }
        else if (node_->min < c) {

            //  The new character is above the current character range.
            unsigned short old_count = node_->count;
            node_->count = c - node_->min + 1;
            node_->next.table = (node_t**) realloc ((void*) node_->next.table,
                sizeof (node_t*) * node_->count);
            xs_assert (node_->next.table);
            for (unsigned short i = old_count; i != node_->count; i++)
                node_->next.table [i] = NULL;
        }
        else {

            //  The new character is below the current character range.
            unsigned short old_count = node_->count;
            node_->count = (node_->min + old_count) - c;
            node_->next.table = (node_t**) realloc ((void*) node_->next.table,
                sizeof (node_t*) * node_->count);
            xs_assert (node_->next.table);
            memmove (node_->next.table + node_->min - c, node_->next.table,
                old_count * sizeof (node_t*));
            for (unsigned short i = 0; i != node_->min - c; i++)
                node_->next.table [i] = NULL;
            node_->min = c;
        }
    }

    //  If next node does not exist, create one.
    if (node_->count == 1) {
        if (!node_->next.node) {
            node_->next.node = new (std::nothrow) node_t;
            alloc_assert (node_->next.node);
            init (node_->next.node);
            ++node_->live_nodes;
            xs_assert (node_->next.node);
        }
        return add (node_->next.node, prefix_ + 1, size_ - 1, pipe_);
    }
    else {
        if (!node_->next.table [c - node_->min]) {
            node_->next.table [c - node_->min] = new (std::nothrow) node_t;
            alloc_assert (node_->next.table [c - node_->min]);
            init (node_->next.table [c - node_->min]);
            ++node_->live_nodes;
            xs_assert (node_->next.table [c - node_->min]);
        }
        return add (node_->next.table [c - node_->min], prefix_ + 1, size_ - 1,
            pipe_);
    }
}


void xs::prefix_filter_t::rm (node_t *node_, pipe_t *pipe_, void *arg_)
{
    unsigned char *buff = NULL;
    rm_helper (node_, pipe_, &buff, 0, 0, arg_);
    free (buff);
}

void xs::prefix_filter_t::rm_helper (node_t *node_, pipe_t *pipe_,
    unsigned char **buff_, size_t buffsize_, size_t maxbuffsize_, void *arg_)
{
    //  Remove the subscription from this node.
    if (node_->pipes) {
        node_t::pipes_t::iterator it = node_->pipes->find (pipe_);
        if (it != node_->pipes->end ()) {
            xs_assert (it->second);
            --it->second;
            if (!it->second) {
                node_->pipes->erase (it);
                if (node_->pipes->empty ()) {
                    xs_filter_unsubscribed (arg_, *buff_, buffsize_);
                    delete node_->pipes;
                    node_->pipes = 0;
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
    if (node_->count == 0)
        return;

    //  If there's one subnode (optimisation).
    if (node_->count == 1) {
        (*buff_) [buffsize_] = node_->min;
        buffsize_++;
        rm_helper (node_->next.node, pipe_, buff_, buffsize_, maxbuffsize_,
            arg_);

        //  Prune the node if it was made redundant by the removal
        if (is_redundant (node_->next.node)) {
            close (node_->next.node);
            delete node_->next.node;
            node_->next.node = 0;
            node_->count = 0;
            --node_->live_nodes;
            xs_assert (node_->live_nodes == 0);
        }
        return;
    }

    //  If there are multiple subnodes.
    //
    //  New min non-null character in the node table after the removal.
    unsigned char new_min = node_->min;
    //  New max non-null character in the node table after the removal.
    unsigned char new_max = node_->min + node_->count - 1;
    for (unsigned short c = 0; c != node_->count; c++) {
        (*buff_) [buffsize_] = node_->min + c;
        if (node_->next.table [c]) {
            rm_helper (node_->next.table [c], pipe_, buff_, buffsize_ + 1,
                maxbuffsize_, arg_);

            //  Prune redudant nodes from the the trie.
            if (is_redundant (node_->next.table [c])) {
                close (node_->next.table [c]);
                delete node_->next.table [c];
                node_->next.table [c] = 0;

                xs_assert (node_->live_nodes > 0);
                --node_->live_nodes;
            }
            else {
                if (c + node_->min > new_min)
                    new_min = c + node_->min;
                if (c + node_->min < new_max)
                    new_max = c + node_->min;
            }
        }
    }

    xs_assert (node_->count > 1);

    //  Compact the node table if possible.
    if (node_->live_nodes == 1) {

        //  If there's only one live node in the table we can
        //  switch to using the more compact single-node
        //  representation.
        xs_assert (new_min == new_max);
        xs_assert (new_min >= node_->min &&
            new_min < node_->min + node_->count);
        node_t *node = node_->next.table [new_min - node_->min];
        xs_assert (node);
        free (node_->next.table);
        node_->next.node = node;
        node_->count = 1;
        node_->min = new_min;
    }
    else if (node_->live_nodes > 1 &&
          (new_min > node_->min || new_max < node_->min + node_->count - 1)) {
        xs_assert (new_max - new_min + 1 > 1);

        node_t **old_table = node_->next.table;
        xs_assert (new_min > node_->min ||
            new_max < node_->min + node_->count - 1);
        xs_assert (new_min >= node_->min);
        xs_assert (new_max <= node_->min + node_->count - 1);
        xs_assert (new_max - new_min + 1 < node_->count);

        node_->count = new_max - new_min + 1;
        node_->next.table = (node_t**) malloc (sizeof (node_t*) * node_->count);
        xs_assert (node_->next.table);

        memmove (node_->next.table, old_table + (new_min - node_->min),
                 sizeof (node_t*) * node_->count);
        free (old_table);

        node_->min = new_min;
    }
}

bool xs::prefix_filter_t::rm (node_t *node_, const unsigned char *prefix_,
    size_t size_, pipe_t *pipe_)
{
    if (!size_) {

        //  Remove the subscription from this node.
        if (node_->pipes) {
            node_t::pipes_t::iterator it = node_->pipes->find (pipe_);
            if (it != node_->pipes->end ()) {
                xs_assert (it->second);
                --it->second;
                if (!it->second) {
                    node_->pipes->erase (it);
                    if (node_->pipes->empty ()) {
                        delete node_->pipes;
                        node_->pipes = 0;
                    }
                }
            }
        }
        return !node_->pipes;
    }

    unsigned char c = *prefix_;
    if (!node_->count || c < node_->min || c >= node_->min + node_->count)
        return false;

    node_t *next_node = node_->count == 1 ? node_->next.node :
        node_->next.table [c - node_->min];

    if (!next_node)
        return false;

    bool ret = rm (next_node, prefix_ + 1, size_ - 1, pipe_);

    if (is_redundant (next_node)) {
        close (next_node);
        delete next_node;
        xs_assert (node_->count > 0);

        if (node_->count == 1) {
            node_->next.node = 0;
            node_->count = 0;
            --node_->live_nodes;
            xs_assert (node_->live_nodes == 0);
        }
        else {
            node_->next.table [c - node_->min] = 0;
            xs_assert (node_->live_nodes > 1);
            --node_->live_nodes;

            //  Compact the table if possible.
            if (node_->live_nodes == 1) {
                //  If there's only one live node in the table we can
                //  switch to using the more compact single-node
                //  representation
                node_t *node = 0;
                for (unsigned short i = 0; i < node_->count; ++i) {
                    if (node_->next.table [i]) {
                        node = node_->next.table [i];
                        node_->min += i;
                        break;
                    }
                }

                xs_assert (node);
                free (node_->next.table);
                node_->next.node = node;
                node_->count = 1;
            }
            else if (c == node_->min) {

                //  We can compact the table "from the left".
                unsigned char new_min = node_->min;
                for (unsigned short i = 1; i < node_->count; ++i) {
                    if (node_->next.table [i]) {
                        new_min = i + node_->min;
                        break;
                    }
                }
                xs_assert (new_min != node_->min);

                node_t **old_table = node_->next.table;
                xs_assert (new_min > node_->min);
                xs_assert (node_->count > new_min - node_->min);

                node_->count = node_->count - (new_min - node_->min);
                node_->next.table =
                    (node_t**) malloc (sizeof (node_t*) * node_->count);
                xs_assert (node_->next.table);

                memmove (node_->next.table, old_table + (new_min - node_->min),
                         sizeof (node_t*) * node_->count);
                free (old_table);

                node_->min = new_min;
            }
            else if (c == node_->min + node_->count - 1) {

                //  We can compact the table "from the right".
                unsigned short new_count = node_->count;
                for (unsigned short i = 1; i < node_->count; ++i) {
                    if (node_->next.table [node_->count - 1 - i]) {
                        new_count = node_->count - i;
                        break;
                    }
                }
                xs_assert (new_count != node_->count);
                node_->count = new_count;

                node_t **old_table = node_->next.table;
                node_->next.table =
                    (node_t**) malloc (sizeof (node_t*) * node_->count);
                xs_assert (node_->next.table);

                memmove (node_->next.table, old_table,
                    sizeof (node_t*) * node_->count);
                free (old_table);
            }
        }
    }

    return ret;
}

void xs::prefix_filter_t::list (node_t *node_, unsigned char **buff_,
    size_t buffsize_, size_t maxbuffsize_, void *arg_)
{
    //  If this node is a subscription, apply the function.
    if (node_->pipes)
        xs_filter_subscribed (arg_, *buff_, buffsize_);

    //  Adjust the buffer.
    if (buffsize_ >= maxbuffsize_) {
        maxbuffsize_ = buffsize_ + 256;
        *buff_ = (unsigned char*) realloc (*buff_, maxbuffsize_);
        xs_assert (*buff_);
    }

    //  If there are no subnodes in the trie, return.
    if (node_->count == 0)
        return;

    //  If there's one subnode (optimisation).
    if (node_->count == 1) {
        (*buff_) [buffsize_] = node_->min;
        buffsize_++;
        list (node_->next.node, buff_, buffsize_, maxbuffsize_, arg_);
        return;
    }

    //  If there are multiple subnodes.
    for (unsigned short c = 0; c != node_->count; c++) {
        (*buff_) [buffsize_] = node_->min + c;
        if (node_->next.table [c])
            list (node_->next.table [c], buff_, buffsize_ + 1, maxbuffsize_,
                arg_);
    }
}

bool xs::prefix_filter_t::is_redundant (node_t *node_)
{
    return !node_->pipes && node_->live_nodes == 0;
}

//  Implementation of the C interface of the filter.
//  Following functions convert raw C calls into calls to C++ object methods.

static void *create (void *core_)
{
    void *fset = (void*) new (std::nothrow) xs::prefix_filter_t;
    alloc_assert (fset);
    return fset;
}

static void destroy (void *core_, void *filter_)
{
    delete (xs::prefix_filter_t*) filter_;
}

static int subscribe (void *core_, void *filter_, void *subscriber_,
    const unsigned char *data_, size_t size_)
{
    return ((xs::prefix_filter_t*) filter_)->subscribe (core_, subscriber_,
        data_, size_);
}

static int unsubscribe (void *core_, void *filter_, void *subscriber_,
    const unsigned char *data_, size_t size_)
{
    return ((xs::prefix_filter_t*) filter_)->unsubscribe (core_, subscriber_,
        data_, size_);
}

static void unsubscribe_all (void *core_, void *filter_, void *subscriber_)
{
    ((xs::prefix_filter_t*) filter_)->unsubscribe_all (core_, subscriber_);
}


static void enumerate (void *core_, void *filter_)
{
    ((xs::prefix_filter_t*) filter_)->enumerate (core_);
}

static int match (void *core_, void *filter_,
    const unsigned char *data_, size_t size_)
{
    return ((xs::prefix_filter_t*) filter_)->match (core_, data_, size_);
}

static void match_all (void *core_, void *filter_,
    const unsigned char *data_, size_t size_)
{
    ((xs::prefix_filter_t*) filter_)->match_all (core_, data_, size_);
}

static xs_filter_t filter = {
    XS_EXTENSION_FILTER,
    XS_FILTER_PREFIX,
    create,
    destroy,
    subscribe,
    unsubscribe,
    unsubscribe_all,
    enumerate,
    match,
    match_all
};

void *xs::prefix_filter = (void*) &filter;

