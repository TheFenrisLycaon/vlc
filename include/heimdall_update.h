/*****************************************************************************
 * heimdall_update.h: Heimdall update download
 *****************************************************************************
 * Copyright © 2005-2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea -at- lycaria -dot- org>
 *          Rafaël Carré <funman@lycariaorg>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either release 2 of the License, or
 * (at your option) any later release.
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

#ifndef HEIMDALL_UPDATE_H
#define HEIMDALL_UPDATE_H

/**
 * \defgroup update Software updates
 * \ingroup interface
 * Over-the-air Heimdall software updates
 * @{
 * \file
 *Heimdall software update interface
 */

/**
 * Describes an update Heimdall release number
 */
struct update_release_t
{
    int i_major;        ///< Version major
    int i_minor;        ///< Version minor
    int i_revision;     ///< Version revision
    int i_extra;        ///< Version extra
    char* psz_url;      ///< Download URL
    char* psz_desc;     ///< Release description
};

typedef struct update_release_t update_release_t;

HEIMDALL_API update_t * update_New( heimdall_object_t * );
#define update_New( a ) update_New( HEIMDALL_OBJECT( a ) )
HEIMDALL_API void update_Delete( update_t * );
HEIMDALL_API void update_Check( update_t *, void (*callback)( void*, bool ), void * );
HEIMDALL_API bool update_NeedUpgrade( update_t * );
HEIMDALL_API void update_Download( update_t *, const char* );
HEIMDALL_API update_release_t* update_GetRelease( update_t * );

/**
 * @}
 */

#endif /* _HEIMDALL_UPDATE_H */
