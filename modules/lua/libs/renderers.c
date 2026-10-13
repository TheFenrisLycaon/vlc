/*****************************************************************************
 * renderers.c
 *****************************************************************************
 * Copyright (C) 2020 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_player.h>
#include <heimdall_renderer_discovery.h>
#include <heimdall_vector.h>

#include "../heimdall.h"
#include "../libs.h"

#include "misc.h"

typedef struct heimdalllua_renderer_item
{
    uint32_t id;
    heimdall_renderer_item_t* item;
} heimdalllua_renderer_item;

struct rd_items_vec HEIMDALL_VECTOR(heimdalllua_renderer_item);

typedef struct heimdalllua_rd_sys_t
{
    struct heimdall_renderer_discovery_owner owner;
    heimdall_renderer_discovery_t* rd;
    struct rd_items_vec items;
    heimdall_mutex_t mutex;
    uint32_t last_renderer_id;
} heimdalllua_rd_sys_t;

static bool heimdalllua_renderer_compare( heimdall_renderer_item_t* rhs,
                                     heimdall_renderer_item_t* lhs )
{
    return !strcmp( heimdall_renderer_item_name( rhs ), heimdall_renderer_item_name( lhs ) ) &&
           !strcmp( heimdall_renderer_item_type( rhs ), heimdall_renderer_item_type( lhs ) );
}

static int heimdalllua_rd_list( lua_State* L )
{
    heimdalllua_rd_sys_t* sys = (heimdalllua_rd_sys_t*)luaL_checkudata( L, 1, "renderer_discoverer" );

    heimdall_renderer_item_t *activeRenderer;
    heimdall_player_t* player = heimdalllua_get_player_internal( L );
    heimdall_player_Lock( player );
    activeRenderer = heimdall_player_GetRenderer( player );
    if ( activeRenderer )
        heimdall_renderer_item_hold( activeRenderer );
    heimdall_player_Unlock( player );

    lua_createtable( L, sys->items.size, 0 );
    heimdall_mutex_lock( &sys->mutex );
    for ( size_t i = 0; i < sys->items.size; ++i )
    {
        lua_newtable( L );

        lua_pushinteger( L, sys->items.data[i].id );
        lua_setfield( L, -2, "id" );
        lua_pushstring( L, heimdall_renderer_item_name( sys->items.data[i].item ) );
        lua_setfield( L, -2, "name" );
        lua_pushstring( L, heimdall_renderer_item_type( sys->items.data[i].item ) );
        lua_setfield( L, -2, "type" );

        bool selected = activeRenderer != NULL &&
                heimdalllua_renderer_compare( activeRenderer, sys->items.data[i].item );
        lua_pushboolean( L, selected );
        lua_setfield( L, -2, "selected" );

        lua_rawseti( L, -2, i + 1 );
    }
    heimdall_mutex_unlock( &sys->mutex );

    if ( activeRenderer )
        heimdall_renderer_item_release( activeRenderer );

    return 1;
}

static int heimdalllua_rd_select( lua_State* L )
{
    heimdall_player_t* player = heimdalllua_get_player_internal( L );
    if ( !player )
        return 0;
    heimdalllua_rd_sys_t* sys = (heimdalllua_rd_sys_t*)luaL_checkudata( L, 1, "renderer_discoverer" );
    lua_Integer id = luaL_checkinteger( L, 2 );
    if ( id < 0 )
    {
        heimdall_player_Lock( player );
        heimdall_player_SetRenderer( player, NULL );
        heimdall_player_Unlock( player );
    }
    heimdall_mutex_lock( &sys->mutex );
    for ( size_t i = 0; i < sys->items.size; ++i )
    {
        if ( sys->items.data[i].id != id )
            continue;
        heimdall_player_Lock( player );
        heimdall_player_SetRenderer( player, sys->items.data[i].item );
        heimdall_player_Unlock( player );
    }
    heimdall_mutex_unlock( &sys->mutex );
    return 0;
}

static const luaL_Reg heimdalllua_rd_obj_reg[] = {
    { "list", heimdalllua_rd_list},
    { "select", heimdalllua_rd_select },
    { NULL, NULL }
};

static int heimdalllua_rd_obj_delete( lua_State *L )
{
    heimdalllua_rd_sys_t* sys = (heimdalllua_rd_sys_t*)luaL_checkudata( L, 1, "renderer_discoverer" );
    heimdall_rd_release( sys->rd );
    for ( size_t i = 0; i < sys->items.size; ++i )
        heimdall_renderer_item_release( sys->items.data[i].item );
    heimdall_vector_destroy( &sys->items );
    return 0;
}

static void heimdalllua_rd_on_item_added( struct heimdall_renderer_discovery_t* rd,
                                      struct heimdall_renderer_item_t* item )
{
    heimdalllua_rd_sys_t* sys = rd->owner.sys;
    heimdall_mutex_lock( &sys->mutex );
    heimdalllua_renderer_item i =
    {
        sys->last_renderer_id++,
        heimdall_renderer_item_hold( item ),
    };
    heimdall_vector_push( &sys->items, i );
    heimdall_mutex_unlock( &sys->mutex );
}

static void heimdalllua_rd_on_item_removed( struct heimdall_renderer_discovery_t* rd,
                                        struct heimdall_renderer_item_t* item )
{
    heimdalllua_rd_sys_t* sys = rd->owner.sys;

    heimdall_mutex_lock( &sys->mutex );
    for ( size_t i = 0; i < sys->items.size; ++i )
    {
        if ( !heimdalllua_renderer_compare( item, sys->items.data[i].item ) )
            continue;
        heimdall_vector_remove( &sys->items, i );
        break;
    }
    heimdall_mutex_unlock( &sys->mutex );
}

static int heimdalllua_rd_create( lua_State* L )
{
    heimdalllua_rd_sys_t* sys = lua_newuserdata( L, sizeof( *sys ) );
    if ( !sys )
        return 0;
    sys->owner.sys = sys;
    sys->owner.item_added = heimdalllua_rd_on_item_added;
    sys->owner.item_removed = heimdalllua_rd_on_item_removed;
    heimdall_vector_init( &sys->items );
    heimdall_mutex_init( &sys->mutex );
    sys->last_renderer_id = 0;

    heimdall_object_t *this = heimdalllua_get_this( L );
    const char* name = luaL_checkstring( L, 1 );

    sys->rd = heimdall_rd_new( this, name, &sys->owner );
    if ( !sys->rd )
    {
        heimdall_vector_destroy( &sys->items );
        return 0;
    }

    if ( luaL_newmetatable( L, "renderer_discoverer" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, heimdalllua_rd_obj_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, heimdalllua_rd_obj_delete );
        lua_setfield( L, -2, "__gc" );
    }
    lua_setmetatable( L, -2 );

    return 1;
}

static const luaL_Reg heimdalllua_rd_reg[] = {
    { "create", heimdalllua_rd_create },
    { NULL, NULL }
};

void luaopen_rd( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_rd_reg );
    lua_setfield( L, -2, "rd" );
}
