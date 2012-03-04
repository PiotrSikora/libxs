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

/*  This structure defines extension point for custom message filtering       */
/*  algorithms. Pointer to this structure (cast to void*) can be supplied     */
/*  to xs_plug function.                                                      */
typedef struct {

    /*  Always to be set to XS_EXTENSION_FILTER.                              */
    int extension_type;

    /*  ID of the filter type. This number is passed on the wire and should   */
    /*  be officially assigned. Right now use crossroads-dev mailing list to  */
    /*  ask for ID assignment. In case you want to use the filter in-house    */
    /*  you may use filter ID 65536 or greater which are defined to be        */
    /*  unspecificed user-defined filters.                                    */
    int filter_id;

    /*  Create a filter set. Filter set is a collection of filters associated */
    /*  with one socket. It is guaranteed that each filter set will be used   */
    /*  from at most one thread in parallel. Returns filter set handle.       */
    void *(*fset_create) ();

    /*  Destroy the filterset identified by the handle. All the associated    */
    /*  resources should be deallocated. This function should automatically   */
    /*  destroy all the filters within the filter set.                        */
    void (*fset_destroy) (void *fset);

    /*  Destroy the specified filter.                                         */
    void (*destroy) (void *fset, void *fid, void *arg);

    /*  Add subscription to the filter. Filter should be able to handle       */
    /*  multiple identical subscriptions (e.g. by reference counting the      */
    /*  subscriptions.) The format of the subscription is not interpreted     */
    /*  by Crossroads core, just by the filter extension. If the filter with  */
    /*  specified fid doesn't exist yet it will be created. The function      */
    /*  returns 0 if the subscription was a duplicate of an existing          */
    /*  subscription or 1 in the case of fresh subscription.                  */
    int (*subscribe) (void *fset, void *fid, const unsigned char *data,
        size_t size);

    /*  Remove existing subscription. If there are several identical          */
    /*  subscriptions this function should remove one of them, leaving the    */
    /*  others in place. In such case the function returns 0. If the removed  */
    /*  subscription was an unique one, it returns 1.                         */
    int (*unsubscribe) (void *fset, void *fid, const unsigned char *data,
        size_t size);

    /*  List all the subscriptions in the set. The function should announce   */
    /*  the subscriptions by calling xs_filter_subscription function.         */
    /*  If there are multiple identical subscriptions, xs_filter_subscription */
    /*  should be called once only.                                           */
    void (*enumerate) (void *fset, void *arg);

    /*  Checks whether particular message matches at least one subscription   */
    /*  in the filter. Returns 0 if it does not and 1 if it does.             */
    int (*match) (void *fset, void *fid, const unsigned char *data,
        size_t size);

    /*  Finds all the filters in the filter set the message is matched by.    */
    /*  The function should announce that the message matches the filter by   */
    /*  calling xs_filter_matching function. It is all right to invoke        */
    /*  xs_filter_matching several times for the same filter.                 */
    void (*match_all) (void *fset, const unsigned char *data, size_t size,
        void *arg);

} xs_filter_t;

/*  To be used from xs_filter_t::enumerate function.                          */
XS_EXPORT void xs_filter_subscribed (int filter_id, const unsigned char *data,
    size_t size, void *arg);

/*  To do used from xs_filter_t::destroy function.                            */
XS_EXPORT void xs_filter_unsubscribed (int filter_id, const unsigned char *data,
    size_t size, void *arg);

/*  To be used from xs_fitler_t::match_all function.                          */
XS_EXPORT void xs_filter_matching (void *fid, void *arg);


#undef XS_EXPORT

#ifdef __cplusplus
}
#endif

#endif
