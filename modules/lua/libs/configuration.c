/*****************************************************************************
 * configuration.c: Generic lua<->heimdall config interface
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

/// Heimdall configuration interactions.
// Functions to query or change Heimdall configuration options
// @module heimdall.config

/*
 * Preamble
 */
#ifndef  _GNU_SOURCE
#   define  _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_configuration.h>

#include "../heimdall.h"
#include "../libs.h"

/// Config handling.
// Functions to query or manipulate Heimdall configuration options.
// @section config_handling

/// Get a Heimdall config options value.
//
// Get the Heimdall configuration option `name`'s value.
// @function    get
// @tparam      string name Option name to get the value from
// @treturn     string|int|bool|number The option value
// @raise       Errors for an unknown option
static int heimdalllua_config_get( lua_State *L )
{
    const char *psz_name = luaL_checkstring( L, 1 );
    switch( config_GetType( psz_name ) )
    {
        case HEIMDALL_VAR_STRING:
        {
            char *psz = config_GetPsz( psz_name );
            lua_pushstring( L, psz );
            free( psz );
            break;
        }

        case HEIMDALL_VAR_INTEGER:
            lua_pushinteger( L, config_GetInt( psz_name ) );
            break;

        case HEIMDALL_VAR_BOOL:
            lua_pushboolean( L, config_GetInt( psz_name ) );
            break;

        case HEIMDALL_VAR_FLOAT:
            lua_pushnumber( L, config_GetFloat( psz_name ) );
            break;

        default:
            return heimdalllua_error( L );

    }
    return 1;
}

/// Set a Heimdall config options value.
//
// Set the Heimdall configuration option `name`'s value.
// @function    set
// @tparam      string name Option name to set the value for
// @tparam      string|int|bool|number value Value to set the option to
// @raise       Errors for an unknown option
static int heimdalllua_config_set( lua_State *L )
{
    const char *psz_name = luaL_checkstring( L, 1 );
    switch( config_GetType( psz_name ) )
    {
        case HEIMDALL_VAR_STRING:
            config_PutPsz( psz_name, luaL_checkstring( L, 2 ) );
            break;

        case HEIMDALL_VAR_INTEGER:
            config_PutInt( psz_name, luaL_checkinteger( L, 2 ) );
            break;

        case HEIMDALL_VAR_BOOL:
            config_PutInt( psz_name, luaL_checkboolean( L, 2 ) );
            break;

        case HEIMDALL_VAR_FLOAT:
            config_PutFloat( psz_name, luaL_checknumber( L, 2 ) );
            break;

        default:
            return heimdalllua_error( L );
    }
    return 0;
}

/// Directories configuration.
// Functions to query _special_ directories for the system Heimdall is running on.
// @section config_dirs

/// Get the Heimdall data directory.
//
// Returns the path of the Heimdall data directory (`HEIMDALL_PKG_DATA_DIR`).
// @function    datadir
// @treturn     string Path to the Heimdall data directory
static int heimdalllua_datadir( lua_State *L )
{
    char *psz_data = config_GetSysPath(HEIMDALL_PKG_DATA_DIR, NULL);
    lua_pushstring( L, psz_data );
    free( psz_data );
    return 1;
}

/// Get the user's Heimdall data directory.
//
// Returns the path of the user's Heimdall data directory (`HEIMDALL_USERDATA_DIR`).
// @function    userdatadir
// @treturn     string Path to the user's Heimdall data directory
static int heimdalllua_userdatadir( lua_State *L )
{
    char *dir = config_GetUserDir( HEIMDALL_USERDATA_DIR );
    lua_pushstring( L, dir );
    free( dir );
    return 1;
}

/// Get the user's home directory.
//
// Returns the path of the user's home directory (`HEIMDALL_HOME_DIR`).
// @function    homedir
// @treturn     string Path to the user's home directory
static int heimdalllua_homedir( lua_State *L )
{
    char *home = config_GetUserDir( HEIMDALL_HOME_DIR );
    lua_pushstring( L, home );
    free( home );
    return 1;
}

/// Get the user's Heimdall config directory.
//
// Returns the path of the user's Heimdall configuration directory (`HEIMDALL_CONFIG_DIR`).
// @function    configdir
// @treturn     string Path to the user's Heimdall config directory
static int heimdalllua_configdir( lua_State *L )
{
    char *dir = config_GetUserDir( HEIMDALL_CONFIG_DIR );
    lua_pushstring( L, dir );
    free( dir );
    return 1;
}

/// Get the user's Heimdall cache directory.
//
// Returns the path of the user's Heimdall cache directory (`HEIMDALL_CACHE_DIR`).
// @function    cachedir
// @treturn     string Path to the user's Heimdall cache directory
static int heimdalllua_cachedir( lua_State *L )
{
    char *dir = config_GetUserDir( HEIMDALL_CACHE_DIR );
    lua_pushstring( L, dir );
    free( dir );
    return 1;
}

/// Get a list of possible data directories.
//
// Returns the list of possible data directories in order
// of priority, appended by `name`.
// @function    datadir_list
// @tparam      string name Name for the data directory
// @treturn     {string,...} Paths to possible data directories, sorted by priority
static int heimdalllua_datadir_list( lua_State *L )
{
    const char *psz_dirname = luaL_checkstring( L, 1 );
    char **ppsz_dir_list = NULL;
    int i = 1;

    if( heimdalllua_dir_list( psz_dirname, &ppsz_dir_list )
        != HEIMDALL_SUCCESS )
        return 0;
    lua_newtable( L );
    for( char **ppsz_dir = ppsz_dir_list; *ppsz_dir; ppsz_dir++ )
    {
        lua_pushstring( L, *ppsz_dir );
        lua_rawseti( L, -2, i );
        i ++;
    }
    heimdalllua_dir_list_free( ppsz_dir_list );
    return 1;
}

/*
 *
 */
static const luaL_Reg heimdalllua_config_reg[] = {
    { "get", heimdalllua_config_get },
    { "set", heimdalllua_config_set },
    { "datadir", heimdalllua_datadir },
    { "userdatadir", heimdalllua_userdatadir },
    { "homedir", heimdalllua_homedir },
    { "configdir", heimdalllua_configdir },
    { "cachedir", heimdalllua_cachedir },
    { "datadir_list", heimdalllua_datadir_list },
    { NULL, NULL }
};

void luaopen_config( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_config_reg );
    lua_setfield( L, -2, "config" );
}
