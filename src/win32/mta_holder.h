/*****************************************************************************
 * mta_holder.c: Hold a MTA from another thread
 *****************************************************************************
 * Copyright (C) 2002-2017 the VideoLAN and AUTHORS
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef MTA_HOLDER_H
# define MTA_HOLDER_H

#include <heimdall_common.h>
#include <heimdall_threads.h>

#include <assert.h>
#include <windows.h>
#include <objbase.h>

typedef struct heimdall_mta_holder
{
    heimdall_thread_t thread;
    int          i_refcount;
    heimdall_sem_t    ready_sem;
    heimdall_sem_t    release_sem;
} heimdall_mta_holder;

static inline void* MtaMainLoop( void* opaque )
{
    heimdall_thread_set_name("heimdall-mta");

    heimdall_mta_holder* p_mta = (heimdall_mta_holder*)opaque;
    CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );

    heimdall_sem_post( &p_mta->ready_sem );

    heimdall_sem_wait( &p_mta->release_sem );

    CoUninitialize();
    return NULL;
}

/**
 * Ensure an MTA context will be available until heimdall_mta_release gets called.
 *
 * In the background, this will create a thread that does nothing but to keep the MTA
 * refcount greater than 0.
 *
 * This is useful in order not to commit a thread to a specific concurrency model.
 * This function is win32 specific.
 */
static inline bool heimdall_mta_acquire( heimdall_object_t *p_parent )
{
    heimdall_object_t *heimdall = HEIMDALL_OBJECT(heimdall_object_instance(p_parent));

    heimdall_global_lock( HEIMDALL_MTA_MUTEX );
    heimdall_mta_holder* p_mta = (heimdall_mta_holder*)var_CreateGetAddress( heimdall, "mta-holder" );
    if ( p_mta == NULL )
    {
        p_mta = (heimdall_mta_holder*)malloc( sizeof( *p_mta ) );
        if ( unlikely( p_mta == NULL ) )
        {
            heimdall_global_unlock( HEIMDALL_MTA_MUTEX );
            return false;
        }
        heimdall_sem_init( &p_mta->ready_sem, 0 );
        heimdall_sem_init( &p_mta->release_sem, 0 );
        p_mta->i_refcount = 1;
        if ( heimdall_clone( &p_mta->thread, MtaMainLoop, p_mta ) )
        {
            free( p_mta );
            p_mta = NULL;
            heimdall_global_unlock( HEIMDALL_MTA_MUTEX );
            return false;
        }
        var_SetAddress( heimdall, "mta-holder", p_mta );
        heimdall_sem_wait( &p_mta->ready_sem );
    }
    else
        ++p_mta->i_refcount;
    heimdall_global_unlock( HEIMDALL_MTA_MUTEX );
    return true;
}

/**
 * Releases a reference to the MTA holder.
 *
 * When its refcount reaches 0, the thread created by
 */
static inline void heimdall_mta_release( heimdall_object_t* p_parent )
{
    heimdall_object_t *heimdall = HEIMDALL_OBJECT(heimdall_object_instance(p_parent));

    heimdall_global_lock( HEIMDALL_MTA_MUTEX );
    heimdall_mta_holder *p_mta = (heimdall_mta_holder*)var_InheritAddress( heimdall, "mta-holder" );
    assert( p_mta != NULL );
    int i_refcount = --p_mta->i_refcount;
    if ( i_refcount == 0 )
        var_SetAddress( heimdall, "mta-holder", NULL );
    heimdall_global_unlock( HEIMDALL_MTA_MUTEX );
    if ( i_refcount == 0 )
    {
        heimdall_sem_post( &p_mta->release_sem );

        heimdall_join( p_mta->thread, NULL );

        free( p_mta );
    }
}

#endif
