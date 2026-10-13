/*****************************************************************************
 * libheimdall_internal.h : Definition of opaque structures for libheimdall exported API
 * Also contains some internal utility functions
 *****************************************************************************
 * Copyright (C) 2005-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef _LIBHEIMDALL_INTERNAL_H
#define _LIBHEIMDALL_INTERNAL_H 1

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_dialog.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>

#include <heimdall_atomic.h>
#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_threads.h>

/* Note well: this header is included from LibHeimdall core.
 * Therefore, static inline functions MUST NOT call LibHeimdall functions here
 * (this can cause linkage failure on some platforms). */

/***************************************************************************
 * Internal creation and destruction functions
 ***************************************************************************/
HEIMDALL_API libheimdall_int_t *libheimdall_InternalCreate( void );
HEIMDALL_API int libheimdall_InternalInit( libheimdall_int_t *, int, const char *ppsz_argv[] );
HEIMDALL_API void libheimdall_InternalCleanup( libheimdall_int_t * );
HEIMDALL_API void libheimdall_InternalDestroy( libheimdall_int_t * );

/**
 * Try to start a user interface for the libheimdall instance.
 *
 * \param libheimdallint the internal instance
 * \param name interface name, or NULL for default
 * \return 0 on success, -1 on error.
 */
HEIMDALL_API int libheimdall_InternalAddIntf( libheimdall_int_t *libheimdallint, const char *name );

/**
 * Start playing the main playlist
 *
 * The main playlist can only be populated via an interface created by the
 * libheimdall_InternalAddIntf() function. The control and media flow will only be
 * controlled by the interface previously added.
 *
 * One of these 2 functions (libheimdall_InternalAddIntf() or libheimdall_InternalPlay())
 * will trigger the creation of an internal playlist and player.
 *
 * \param libheimdallint the internal instance
 */
HEIMDALL_API void libheimdall_InternalPlay( libheimdall_int_t *libheimdallint );
HEIMDALL_API void libheimdall_InternalWait( libheimdall_int_t * );

/**
 * Registers a callback for the LibHeimdall exit event. This is mostly useful if
 * the Heimdall playlist and/or at least one interface are started with
 * libheimdall_InternalPlay() or libheimdall_InternalAddIntf () respectively.
 * Typically, this function will wake up your application main loop (from
 * another thread).
 *
 * \note This function should be called before the playlist or interface are
 * started. Otherwise, there is a small race condition: the exit event could
 * be raised before the handler is registered.
 *
 * \param libheimdallint the internal instance
 * \param cb callback to invoke when LibHeimdall wants to exit,
 *           or NULL to disable the exit handler (as by default)
 * \param opaque data pointer for the callback
 */
HEIMDALL_API void libheimdall_SetExitHandler( libheimdall_int_t *libheimdallint, void (*cb) (void *),
                                    void *opaque );

/***************************************************************************
 * Opaque structures for libheimdall API
 ***************************************************************************/

struct libheimdall_instance_t
{
    libheimdall_int_t *p_libheimdall_int;
    heimdall_atomic_rc_t ref_count;
    struct libheimdall_callback_entry_list_t *p_callback_list;

    struct
    {
        void (*cb) (void *, int, const libheimdall_log_t *, const char *, va_list);
        void *data;
    } log;
    struct
    {
        libheimdall_dialog_cbs cbs;
        void *data;
    } dialog;
};

/***************************************************************************
 * Other internal functions
 ***************************************************************************/

/* Not synchronized: set the LibHeimdall owner before publishing the item. */
HEIMDALL_API void *input_item_GetLibheimdallOwner(input_item_t *);
HEIMDALL_API void input_item_SetLibheimdallOwner(input_item_t *, void *,
                                       void (*release)(void *));

/* Thread context */
void libheimdall_threads_init (void);
void libheimdall_threads_deinit (void);

static inline libheimdall_time_t libheimdall_time_from_heimdall_tick(heimdall_tick_t time)
{
    return US_FROM_HEIMDALL_TICK(time);
}

static inline heimdall_tick_t heimdall_tick_from_libheimdall_time(libheimdall_time_t time)
{
    return HEIMDALL_TICK_FROM_US(time);
}

#endif
