/*****************************************************************************
 * sd.c: Services discovery related functions
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
 *          Fabio Ritrovato <sephiroth87 at lycaria dot org>
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

#include <assert.h>
#include <math.h>
#include <heimdall_common.h>
#include <heimdall_services_discovery.h>
#include <heimdall_charset.h>
#include <heimdall_strings.h>
#include <heimdall_hash.h>

#include "../heimdall.h"
#include "../libs.h"

static int heimdalllua_sd_delete_common( input_item_t **pp_item )
{
    assert(pp_item != NULL);

    input_item_t *p_item = *pp_item;
    if (p_item != NULL) /* item may be NULL if already removed earlier */
        input_item_Release( p_item );

    return 1;
}

static int heimdalllua_sd_remove_common( lua_State *L, input_item_t **pp_item )
{
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );

    if (pp_item == NULL)
        return luaL_error( L, "expected item" );

    input_item_t *p_item = *pp_item;
    if (*pp_item == NULL)
        return luaL_error( L, "already removed item" );

    services_discovery_RemoveItem( p_sd, p_item );
    input_item_Release( p_item );
    /* Make sure we won't try to remove it again */
    *pp_item = NULL;
    return 1;
}


/*** Input item ***/

static int heimdalllua_sd_item_delete( lua_State *L )
{
    input_item_t **pp_item = luaL_checkudata( L, 1, "input_item_t" );

    return heimdalllua_sd_delete_common( pp_item );
}

#define heimdalllua_item_luareg( a ) \
{ "set_" # a, heimdalllua_item_set_ ## a },

#define heimdalllua_item_meta( lowercase, normal ) \
static int heimdalllua_item_set_ ## lowercase ( lua_State *L )\
{\
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );\
    input_item_t **pp_node = (input_item_t **)luaL_checkudata( L, 1, "input_item_t" );\
    if( *pp_node )\
    {\
        if( lua_isstring( L, -1 ) )\
        {\
            input_item_Set ## normal ( *pp_node, lua_tostring( L, -1 ) );\
        } else\
            msg_Err( p_sd, "Error parsing set_ " # lowercase " arguments" );\
    }\
    return 1;\
}

heimdalllua_item_meta(title, Title)
heimdalllua_item_meta(artist, Artist)
heimdalllua_item_meta(genre, Genre)
heimdalllua_item_meta(copyright, Copyright)
heimdalllua_item_meta(album, Album)
heimdalllua_item_meta(tracknum, TrackNum)
heimdalllua_item_meta(description, Description)
heimdalllua_item_meta(rating, Rating)
heimdalllua_item_meta(date, Date)
heimdalllua_item_meta(setting, Setting)
heimdalllua_item_meta(url, URL)
heimdalllua_item_meta(language, Language)
heimdalllua_item_meta(nowplaying, NowPlaying)
heimdalllua_item_meta(publisher, Publisher)
heimdalllua_item_meta(encodedby, EncodedBy)
heimdalllua_item_meta(arturl, ArtworkURL)
heimdalllua_item_meta(trackid, TrackID)
heimdalllua_item_meta(tracktotal, TrackTotal)
heimdalllua_item_meta(director, Director)
heimdalllua_item_meta(season, Season)
heimdalllua_item_meta(episode, Episode)
heimdalllua_item_meta(showname, ShowName)
heimdalllua_item_meta(actors, Actors)

static const luaL_Reg heimdalllua_item_reg[] = {
    heimdalllua_item_luareg(title)
    heimdalllua_item_luareg(artist)
    heimdalllua_item_luareg(genre)
    heimdalllua_item_luareg(copyright)
    heimdalllua_item_luareg(album)
    heimdalllua_item_luareg(tracknum)
    heimdalllua_item_luareg(description)
    heimdalllua_item_luareg(rating)
    heimdalllua_item_luareg(date)
    heimdalllua_item_luareg(setting)
    heimdalllua_item_luareg(url)
    heimdalllua_item_luareg(language)
    heimdalllua_item_luareg(nowplaying)
    heimdalllua_item_luareg(publisher)
    heimdalllua_item_luareg(encodedby)
    heimdalllua_item_luareg(arturl)
    heimdalllua_item_luareg(trackid)
    heimdalllua_item_luareg(tracktotal)
    heimdalllua_item_luareg(director)
    heimdalllua_item_luareg(season)
    heimdalllua_item_luareg(episode)
    heimdalllua_item_luareg(showname)
    heimdalllua_item_luareg(actors)
    { NULL, NULL }
};

static input_item_t *heimdalllua_sd_create_item( services_discovery_t *p_sd,
                                            lua_State *L )
{
    if( !lua_istable( L, -1 ) )
    {
        msg_Err( p_sd, "Error: argument must be table" );
        return NULL;
    }

    lua_getfield( L, -1, "path" );
    if( !lua_isstring( L, -1 ) )
    {
        msg_Err( p_sd, "Error: \"%s\" parameter is required", "path" );
        return NULL;
    }

    const char *psz_path = lua_tostring( L, -1 );

    lua_getfield( L, -2, "title" );

    const char *psz_title = luaL_checkstring( L, -1 )
       ? luaL_checkstring( L, -1 )
       : psz_path;

    input_item_t *p_input = input_item_New( psz_path, psz_title );
    lua_pop( L, 2 );

    if( unlikely(p_input == NULL) )
        return NULL;

    /* The table must be at the top of the stack when calling
     * heimdalllua_read_options() */
    char **ppsz_options = NULL;
    int i_options = 0;

    lua_pushvalue( L, -1 );
    heimdalllua_read_options( p_sd, L, &i_options, &ppsz_options );
    lua_pop( L, 1 );

    input_item_AddOptions( p_input, i_options, (const char **)ppsz_options,
                           HEIMDALL_INPUT_OPTION_TRUSTED );
    while( i_options > 0 )
        free( ppsz_options[--i_options] );
    free( ppsz_options );

    heimdalllua_read_meta_data( p_sd, L, p_input );
    /* This one is to be tested... */
    heimdalllua_read_custom_meta_data( p_sd, L, p_input );
    /* The duration is given in seconds, convert to microseconds */

    lua_getfield( L, -1, "duration" );
    if( lua_isnumber( L, -1 ) )
        p_input->i_duration = llround(lua_tonumber( L, -1 ) * 1e6);
    else if( !lua_isnil( L, -1 ) )
        msg_Warn( p_sd, "Item duration should be a number (in seconds)." );
    lua_pop( L, 1 );

    /* string to build the input item uid */
    lua_getfield( L, -1, "uiddata" );
    if( lua_isstring( L, -1 ) )
    {
        char *s = strdup( luaL_checkstring( L, -1 ) );
        if ( s )
        {
            heimdall_hash_md5_t md5;
            heimdall_hash_md5_Init( &md5 );
            heimdall_hash_md5_Update( &md5, s, strlen( s ) );
            free( s );
            char tmp[HEIMDALL_HASH_MD5_DIGEST_HEX_SIZE];
            heimdall_hash_FinishHex( &md5, tmp );
            input_item_AddInfo( p_input, "uid", "md5", "%s", tmp );
        }
    }
    lua_pop( L, 1 );

    input_item_t **udata = lua_newuserdata( L, sizeof( input_item_t * ) );
    *udata = p_input;

    if( luaL_newmetatable( L, "input_item_t" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, heimdalllua_item_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, heimdalllua_sd_item_delete );
        lua_setfield( L, -2, "__gc" );
        lua_pushliteral( L, "none of your business" );
        lua_setfield( L, -2, "__metatable" );
    }
    lua_setmetatable( L, -2 );

    return p_input;
}


/*** Input item tree node ***/

static int heimdalllua_sd_node_delete( lua_State *L )
{
    input_item_t **pp_item = luaL_checkudata( L, 1, "node" );

    return heimdalllua_sd_delete_common( pp_item );
}

static int heimdalllua_sd_add_sub_common( services_discovery_t *p_sd,
                                     input_item_t **pp_node,
                                     input_item_t *p_input )
{
    if( *pp_node != NULL && p_input != NULL )
        services_discovery_AddSubItem( p_sd, *pp_node, p_input );
    return 1;
}

static int heimdalllua_node_add_subitem( lua_State *L )
{
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );
    input_item_t **pp_node = (input_item_t **)luaL_checkudata( L, 1, "node" );

    return heimdalllua_sd_add_sub_common( p_sd, pp_node,
                                     heimdalllua_sd_create_item( p_sd, L ) );
}

static const luaL_Reg heimdalllua_node_reg[];

static input_item_t *heimdalllua_sd_create_node( services_discovery_t *p_sd,
                                            lua_State *L )
{
    if( !lua_istable( L, -1 ) )
    {
        msg_Err( p_sd, "Error: argument must be table" );
        return NULL;
    }

    lua_getfield( L, -1, "title" );
    if( !lua_isstring( L, -1 ) )
    {
        msg_Err( p_sd, "Error: \"%s\" parameter is required", "title" );
        return NULL;
    }

    const char *psz_name = lua_tostring( L, -1 );
    input_item_t *p_input = input_item_NewExt( INPUT_ITEM_URI_NOP, psz_name,
                                               INPUT_DURATION_INDEFINITE,
                                               ITEM_TYPE_NODE,
                                               ITEM_NET_UNKNOWN );
    lua_pop( L, 1 );

    if( unlikely(p_input == NULL) )
        return NULL;

    lua_getfield( L, -1, "arturl" );
    if( lua_isstring( L, -1 ) && strcmp( lua_tostring( L, -1 ), "" ) )
    {
        char *psz_value = strdup( lua_tostring( L, -1 ) );
        EnsureUTF8( psz_value );
        msg_Dbg( p_sd, "ArtURL: %s", psz_value );
        /* TODO: ask for art download if not local file */
        input_item_SetArtURL( p_input, psz_value );
        free( psz_value );
    }
    lua_pop( L, 1 );

    input_item_t **udata = lua_newuserdata( L, sizeof( input_item_t * ) );
    *udata = p_input;
    if( luaL_newmetatable( L, "node" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, heimdalllua_node_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, heimdalllua_sd_node_delete );
        lua_setfield( L, -2, "__gc" );
    }
    lua_setmetatable( L, -2 );

    return p_input;
}

static int heimdalllua_node_add_subnode( lua_State *L )
{
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );
    input_item_t **pp_node = (input_item_t **)luaL_checkudata( L, 1, "node" );

    return heimdalllua_sd_add_sub_common( p_sd, pp_node,
                                     heimdalllua_sd_create_node( p_sd, L ) );
}

static const luaL_Reg heimdalllua_node_reg[] = {
    { "add_subitem", heimdalllua_node_add_subitem },
    { "add_subnode", heimdalllua_node_add_subnode },
    { NULL, NULL }
};


/*** Services discovery instance ***/

static int heimdalllua_sd_add_common( services_discovery_t *p_sd,
                                 input_item_t *p_input )
{
    if( p_input != NULL )
        services_discovery_AddItem( p_sd, p_input );
    return 1;
}

static int heimdalllua_sd_add_item( lua_State *L )
{
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );

    return heimdalllua_sd_add_common( p_sd, heimdalllua_sd_create_item( p_sd, L ) );
}

static int heimdalllua_sd_add_node( lua_State *L )
{
    services_discovery_t *p_sd = (services_discovery_t *)heimdalllua_get_this( L );

    return heimdalllua_sd_add_common( p_sd, heimdalllua_sd_create_node( p_sd, L ) );
}

static int heimdalllua_sd_remove_item( lua_State *L )
{
    input_item_t **pp_input = luaL_checkudata( L, 1, "input_item_t" );

    return heimdalllua_sd_remove_common( L, pp_input );
}

static int heimdalllua_sd_remove_node( lua_State *L )
{
    input_item_t **pp_input = luaL_checkudata( L, 1, "node" );

    return heimdalllua_sd_remove_common( L, pp_input );
}

static const luaL_Reg heimdalllua_sd_sd_reg[] = {
    { "add_item", heimdalllua_sd_add_item },
    { "add_node", heimdalllua_sd_add_node },
    { "remove_item", heimdalllua_sd_remove_item },
    { "remove_node", heimdalllua_sd_remove_node },
    { NULL, NULL }
};

void luaopen_sd_sd( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_sd_sd_reg );
    lua_setfield( L, -2, "sd" );
}
