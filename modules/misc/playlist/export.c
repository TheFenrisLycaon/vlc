/*****************************************************************************
 * export.c :  Playlist export module
 *****************************************************************************
 * Copyright (C) 2004-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_plugin.h>

/***************************************************************************
 * Prototypes
 ***************************************************************************/
int Export_M3U    ( heimdall_object_t *p_intf );
int Export_M3U8   ( heimdall_object_t *p_intf );
int Export_HTML   ( heimdall_object_t *p_intf );
int xspf_export_playlist( heimdall_object_t *p_intf );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
heimdall_module_begin ()

    set_subcategory( SUBCAT_PLAYLIST_EXPORT )
    add_submodule ()
        set_description( N_("M3U playlist export") )
        add_shortcut( "export-m3u" )
        set_capability( "playlist export" , 0 )
        set_callback( Export_M3U  )

    add_submodule ()
        set_description( N_("M3U8 playlist export") )
        add_shortcut( "export-m3u8" )
        set_capability( "playlist export" , 0 )
        set_callback( Export_M3U8 )

    add_submodule ()
        set_description( N_("XSPF playlist export") )
        add_shortcut( "export-xspf" )
        set_capability( "playlist export" , 0 )
        set_callback( xspf_export_playlist  )

    add_submodule ()
        set_description( N_("HTML playlist export") )
        add_shortcut( "export-html" )
        set_capability( "playlist export", 0 )
        set_callback( Export_HTML )

heimdall_module_end ()
