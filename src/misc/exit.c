/*****************************************************************************
 * exit.c: LibHeimdall termination event
 *****************************************************************************
 * Copyright (C) 2009-2010 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_interface.h>
#include "../libheimdall.h"
#include "../lib/libheimdall_internal.h"

void heimdall_ExitInit( heimdall_exit_t *exit )
{
    heimdall_mutex_init( &exit->lock );
    exit->handler = NULL;
    exit->opaque = NULL;
}

/**
 * Registers a callback for the LibHeimdall exit event.
 */
void libheimdall_SetExitHandler( libheimdall_int_t *p_libheimdall, void (*handler) (void *),
                            void *opaque )
{
    heimdall_exit_t *exit = &libheimdall_priv( p_libheimdall )->exit;

    heimdall_mutex_lock( &exit->lock );
    exit->handler = handler;
    exit->opaque = opaque;
    heimdall_mutex_unlock( &exit->lock );
}

/**
 * Posts an exit signal to LibHeimdall instance.
 * This function should only be called on behalf of the user.
 */
void libheimdall_Quit( libheimdall_int_t *p_libheimdall )
{
    heimdall_exit_t *exit = &libheimdall_priv( p_libheimdall )->exit;

    msg_Dbg( p_libheimdall, "exiting" );
    heimdall_mutex_lock( &exit->lock );
    if( exit->handler != NULL )
        exit->handler( exit->opaque );
    else
        msg_Dbg( p_libheimdall, "no exit handler" );
    heimdall_mutex_unlock( &exit->lock );
}
