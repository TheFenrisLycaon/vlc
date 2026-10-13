/*****************************************************************************
 * misc.c
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
 *          Pierre d'Herbemont <pdherbemont # videolan.org>
 *          Rémi Duraffort <ivoire # lycaria tod org>
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

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_meta.h>
#include <heimdall_interface.h>
#include <heimdall_actions.h>
#include <heimdall_interrupt.h>
#include <heimdall_rand.h>

#include "../heimdall.h"
#include "../libs.h"
#include "misc.h"

/*****************************************************************************
 * Internal lua<->heimdall utils
 *****************************************************************************/
void heimdalllua_set_object( lua_State *L, void *id, void *value )
{
    lua_pushlightuserdata( L, id );
    lua_pushlightuserdata( L, value );
    lua_rawset( L, LUA_REGISTRYINDEX );
}

void *heimdalllua_get_object( lua_State *L, void *id )
{
    lua_pushlightuserdata( L, id );
    lua_rawget( L, LUA_REGISTRYINDEX );
    const void *p = lua_topointer( L, -1 );
    lua_pop( L, 1 );
    return (void *)p;
}

#undef heimdalllua_set_this
void heimdalllua_set_this( lua_State *L, heimdall_object_t *p_this )
{
    heimdalllua_set_object( L, heimdalllua_set_this, p_this );
}

heimdall_object_t * heimdalllua_get_this( lua_State *L )
{
    return heimdalllua_get_object( L, heimdalllua_set_this );
}

heimdall_player_t *heimdalllua_get_player_internal( lua_State *L )
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    return heimdall_playlist_GetPlayer(playlist);
}

/*****************************************************************************
 * Heimdall error code translation
 *****************************************************************************/
int heimdalllua_push_ret( lua_State *L, int i_error )
{
    const char *str;

    lua_pushnumber( L, i_error );

    if( i_error == HEIMDALL_EGENERIC )
        str = "Generic error";
    else
        str = heimdall_strerror_c( -i_error );

    lua_pushstring( L, str );
    return 2;
}

/*****************************************************************************
 * Get the Heimdall version string
 *****************************************************************************/
static int heimdalllua_version( lua_State *L )
{
    lua_pushstring( L, VERSION_MESSAGE );
    return 1;
}

/*****************************************************************************
 * Get the Heimdall copyright
 *****************************************************************************/
static int heimdalllua_copyright( lua_State *L )
{
    lua_pushliteral( L, COPYRIGHT_MESSAGE );
    return 1;
}

/*****************************************************************************
 * Get the Heimdall license msg/disclaimer
 *****************************************************************************/
static int heimdalllua_license( lua_State *L )
{
    lua_pushstring( L, LICENSE_MSG );
    return 1;
}

/*****************************************************************************
 * Quit Heimdall
 *****************************************************************************/
static int heimdalllua_quit( lua_State *L )
{
    heimdall_object_t *p_this = heimdalllua_get_this( L );
    /* The rc.c code also stops the playlist ... not sure if this is needed
     * though. */
    libheimdall_Quit( heimdall_object_instance(p_this) );
    return 0;
}

static int heimdalllua_mdate( lua_State *L )
{
    lua_pushnumber( L, heimdall_tick_now() );
    return 1;
}

static int heimdalllua_mwait( lua_State *L )
{
    double f = luaL_checknumber( L, 1 );

    heimdall_interrupt_t *oint = heimdalllua_set_interrupt( L );
    int ret = heimdall_mwait_i11e( llround(f) );

    heimdall_interrupt_set( oint );
    if( ret )
        return luaL_error( L, "Interrupted." );
    return 0;
}

static int heimdalllua_action_id( lua_State *L )
{
    heimdall_action_id_t i_key = heimdall_actions_get_id( luaL_checkstring( L, 1 ) );
    if (i_key == 0)
        return 0;
    lua_pushnumber( L, i_key );
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_misc_reg[] = {
    { "version", heimdalllua_version },
    { "copyright", heimdalllua_copyright },
    { "license", heimdalllua_license },

    { "action_id", heimdalllua_action_id },

    { "mdate", heimdalllua_mdate },
    { "mwait", heimdalllua_mwait },

    { "quit", heimdalllua_quit },

    { NULL, NULL }
};

void luaopen_misc( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_misc_reg );
    lua_setfield( L, -2, "misc" );
}
