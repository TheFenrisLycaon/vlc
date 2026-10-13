/*****************************************************************************
 * playlist.h:  Playlist import module common functions
 *****************************************************************************
 * Copyright (C) 2004 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
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

#include <heimdall_input.h>

char *ProcessMRL( const char *, const char * );

int PlaylistControl( stream_t *p_access, int i_query, va_list args );

int Import_M3U ( heimdall_object_t * );

int Import_RAM ( heimdall_object_t * );

int Import_PLS ( heimdall_object_t * );

int Import_B4S ( heimdall_object_t * );

int Import_DVB ( heimdall_object_t * );

int Import_podcast ( heimdall_object_t * );

int Import_xspf ( heimdall_object_t * );
void Close_xspf ( heimdall_object_t * );

int Import_Shoutcast ( heimdall_object_t * );

int Import_ASX ( heimdall_object_t * );

int Import_SGIMB ( heimdall_object_t * );
void Close_SGIMB ( heimdall_object_t * );

int Import_QTL ( heimdall_object_t * );

int Import_IFO ( heimdall_object_t * );
void Close_IFO ( heimdall_object_t * );

int Import_BDMV ( heimdall_object_t * );
void Close_BDMV ( heimdall_object_t * );

int Import_iTML ( heimdall_object_t * );

int Import_WMS(heimdall_object_t *);

int Import_WPL ( heimdall_object_t * );
void Close_WPL ( heimdall_object_t * );

#define GetCurrentItem(obj) ((obj)->p_input_item)
#define GetSource(obj) ((obj)->s)
