/*****************************************************************************
 * fingerprinter.c: Fingerprinter creator and destructor
 *****************************************************************************
 * Copyright (C) 2012 VLC authors and VideoLAN
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
#include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_fingerprinter.h>
#include <heimdall_modules.h>
#include "../libheimdall.h"

fingerprinter_thread_t *fingerprinter_Create( heimdall_object_t *p_this )
{
    fingerprinter_thread_t *p_fingerprint;

    p_fingerprint = ( fingerprinter_thread_t * )
            heimdall_custom_create( p_this, sizeof( *p_fingerprint ), "fingerprinter" );
    if( !p_fingerprint )
    {
        msg_Err( p_this, "unable to create fingerprinter" );
        return NULL;
    }

    p_fingerprint->p_module = module_need( p_fingerprint, "fingerprinter",
                                           NULL, false );
    if( !p_fingerprint->p_module )
    {
        heimdall_object_delete(p_fingerprint);
        msg_Err( p_this, "AcoustID fingerprinter not found" );
        return NULL;
    }

    return p_fingerprint;
}

void fingerprinter_Destroy( fingerprinter_thread_t *p_fingerprint )
{
    module_unneed( p_fingerprint, p_fingerprint->p_module );
    heimdall_object_delete(p_fingerprint);
}
