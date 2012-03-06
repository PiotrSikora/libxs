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

#ifndef __XS_FILTER_H_INCLUDED__
#define __XS_FILTER_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

/*  Handle DSO symbol visibility                                             */
#if defined _WIN32
#   if defined DLL_EXPORT
#       define XS_EXPORT __declspec(dllexport)
#   else
#       define XS_EXPORT __declspec(dllimport)
#   endif
#else
#   if defined __SUNPRO_C  || defined __SUNPRO_CC
#       define XS_EXPORT __global
#   elif (defined __GNUC__ && __GNUC__ >= 4) || defined __INTEL_COMPILER
#       define XS_EXPORT __attribute__ ((visibility("default")))
#   else
#       define XS_EXPORT
#   endif
#endif

#include <stddef.h>

#define XS_EXTENSION_FILTER 1

typedef struct
{
    int extension_type;
    int filter_id;
    void *(*create) ();
    void (*destroy) (void *filter);
    int (*subscribe) (void *filter, void *subscriber,
        const unsigned char *data, size_t size);
    int (*unsubscribe) (void *filter, void *subscriber,
        const unsigned char *data, size_t size);
    void (*unsubscribe_all) (void *filter, void *subscriber, void *arg);
    void (*enumerate) (void *filter, void *arg);
    int (*match) (void *filter, void *subscriber,
        const unsigned char *data, size_t size);
    void (*match_all) (void *filter,
        const unsigned char *data, size_t size, void *arg);
} xs_filter_t;

XS_EXPORT void xs_filter_subscribed (int filter_id,
    const unsigned char *data, size_t size, void *arg);

XS_EXPORT void xs_filter_unsubscribed (int filter_id,
    const unsigned char *data, size_t size, void *arg);

XS_EXPORT void xs_filter_matching (void *subscriber, void *arg);

#undef XS_EXPORT

#ifdef __cplusplus
}
#endif

#endif
