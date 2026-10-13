/*****************************************************************************
 * video.c: Generic lua interface functions
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
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
#ifndef  _GNU_SOURCE
#   define  _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_vout.h>

#include "../heimdall.h"
#include "../libs.h"
#include "input.h"
#include "variables.h"

/*****************************************************************************
 * Vout control
 *****************************************************************************/
static int heimdalllua_fullscreen( lua_State *L )
{
    int i_ret;

    vout_thread_t *p_vout = heimdalllua_get_vout_internal(L);
    if( !p_vout )
        return heimdalllua_error( L );

    i_ret = heimdalllua_var_toggle_or_set( L, p_vout, "fullscreen" );

    vout_Release(p_vout);
    return i_ret;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_video_reg[] = {
    { "fullscreen", heimdalllua_fullscreen },
    { NULL, NULL }
};

void luaopen_video( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_video_reg );
    lua_setfield( L, -2, "video" );
}
