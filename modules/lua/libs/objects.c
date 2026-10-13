/*****************************************************************************
 * objects.c: Generic lua<->heimdall object wrapper
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

#include <heimdall_common.h>
#include <heimdall_vout.h>
#include <heimdall_playlist.h>
#include <heimdall_player.h>

#include "../heimdall.h"
#include "../libs.h"
#include "input.h"

/*****************************************************************************
 * Generic heimdall_object_t wrapper creation
 *****************************************************************************/

typedef struct heimdalllua_object {
    void *object;
    void (*release)(void *);
} heimdalllua_object_t;

static int heimdalllua_release_heimdall_object(lua_State *L)
{
    heimdalllua_object_t *p_luaobj = luaL_checkudata(L, 1, "heimdall_object");

    lua_pop(L, 1);
    if (p_luaobj->release)
        p_luaobj->release(p_luaobj->object);
    return 0;
}

static int heimdalllua_push_heimdall_object(lua_State *L, void *p_obj,
                                  void (*release)(void *))
{
    heimdalllua_object_t *udata =
        (heimdalllua_object_t *)lua_newuserdata(L, sizeof (heimdalllua_object_t));

    udata->object = p_obj;
    udata->release = release;

    if (luaL_newmetatable(L, "heimdall_object"))
    {
        /* Hide the metatable */
        lua_pushliteral(L, "none of your business");
        lua_setfield(L, -2, "__metatable");
        lua_pushcfunction(L, heimdalllua_release_heimdall_object);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);
    return 1;
}

static int heimdalllua_get_libheimdall( lua_State *L )
{
    libheimdall_int_t *p_libheimdall = heimdall_object_instance(heimdalllua_get_this( L ));
    heimdalllua_push_heimdall_object(L, p_libheimdall, NULL);
    return 1;
}

static int heimdalllua_get_playlist( lua_State *L )
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    if (playlist)
        lua_pushlightuserdata(L, playlist);
    else
        lua_pushnil(L);
    return 1;
}

static int heimdalllua_get_player( lua_State *L )
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);
    if (player)
        lua_pushlightuserdata(L, player);
    else
        lua_pushnil(L);
    return 1;
}

static void do_vout_Release( void *vout )
{
    vout_Release( vout );
}

static int heimdalllua_get_vout( lua_State *L )
{
    vout_thread_t *vout = heimdalllua_get_vout_internal(L);
    if (vout)
        heimdalllua_push_heimdall_object(L, vout, do_vout_Release);
    else
        lua_pushnil(L);
    return 1;
}

static void do_aout_Release( void *aout )
{
    aout_Release( aout );
}

static int heimdalllua_get_aout( lua_State *L )
{
    audio_output_t *aout = heimdalllua_get_aout_internal(L);
    if (aout)
        heimdalllua_push_heimdall_object(L, aout, do_aout_Release);
    else
        lua_pushnil(L);
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_object_reg[] = {
    { "playlist", heimdalllua_get_playlist },
    { "player", heimdalllua_get_player },
    { "libheimdall", heimdalllua_get_libheimdall },
    { "vout", heimdalllua_get_vout},
    { "aout", heimdalllua_get_aout},
    { NULL, NULL }
};

void luaopen_object( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_object_reg );
    lua_setfield( L, -2, "object" );
}
