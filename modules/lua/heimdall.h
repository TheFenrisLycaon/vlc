/*****************************************************************************
 * heimdall.h: Heimdall specific lua library functions.
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
 *          Pierre d'Herbemont <pdherbemont # videolan.org>
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

#ifndef HEIMDALL_LUA_H
#define HEIMDALL_LUA_H
/*****************************************************************************
 * Preamble
 *****************************************************************************/

#include <heimdall_common.h>
#include <heimdall_input.h>
#include <heimdall_playlist.h>
#include <heimdall_meta.h>
#include <heimdall_meta_fetcher.h>
#include <heimdall_url.h>
#include <heimdall_strings.h>
#include <heimdall_stream.h>
#include <heimdall_demux.h>

#define LUA_COMPAT_MODULE
#include <lua.h>        /* Low level lua C API */
#include <lauxlib.h>    /* Higher level C API */
#include <lualib.h>     /* Lua libs */

#if LUA_VERSION_NUM >= 502
# define lua_equal(L,idx1,idx2)    lua_compare(L,(idx1),(idx2),LUA_OPEQ)
# define lua_objlen(L,idx)         lua_rawlen(L,idx)
# define lua_strlen(L,idx)         lua_rawlen(L,idx)
#endif

#if LUA_VERSION_NUM >= 502
# undef luaL_register
# define luaL_register(L, n, l) luaL_setfuncs(L, (l), 0)
# define luaL_register_namespace(L, n, l) \
    lua_getglobal( L, n );      \
    if( lua_isnil( L, -1 ) )    \
    {                           \
        lua_pop( L, 1 );        \
        lua_newtable( L );      \
    }                           \
    luaL_setfuncs( L, (l), 0 ); \
    lua_pushvalue( L, -1 );     \
    lua_setglobal( L, n );
#else
# define luaL_register_namespace(L, n, l) luaL_register( L, n, (l) );
#endif


/*****************************************************************************
 * Module entry points
 *****************************************************************************/
int ReadMeta( demux_meta_t * );
int FetchMeta( meta_fetcher_t * );
int FindArt( meta_fetcher_t * );

int Import_LuaPlaylist( heimdall_object_t * );
void Close_LuaPlaylist( heimdall_object_t * );

#define TELNETPORT_DEFAULT 4212
int Open_LuaIntf( heimdall_object_t * );
void Close_LuaIntf( heimdall_object_t * );
int Open_LuaHTTP( heimdall_object_t * );
int Open_LuaTelnet( heimdall_object_t * );


int Open_Extension( heimdall_object_t * );
void Close_Extension( heimdall_object_t * );

int Open_LuaSD( heimdall_object_t * );
void Close_LuaSD( heimdall_object_t * );

// Script probe
int heimdalllua_probe_sd( heimdall_object_t *, const char *name );

/*****************************************************************************
 * Lua debug
 *****************************************************************************/
static inline void lua_Dbg( heimdall_object_t * p_this, const char * ppz_fmt, ... )
{
    va_list ap;
    va_start( ap, ppz_fmt );
    msg_GenericVa( p_this, HEIMDALL_MSG_DBG, ppz_fmt, ap );
    va_end( ap );
}

static inline bool lua_Disabled( heimdall_object_t *p_this )
{
    return !var_InheritBool( p_this, "lua" );
}
#define lua_Disabled( x ) lua_Disabled( HEIMDALL_OBJECT( x ) )

/*****************************************************************************
 * Functions that should be in lua ... but aren't for some obscure reason
 *****************************************************************************/
static inline bool luaL_checkboolean( lua_State *L, int narg )
{
    luaL_checktype( L, narg, LUA_TBOOLEAN ); /* can raise an error */
    return lua_toboolean( L, narg );
}

static inline int luaL_optboolean( lua_State *L, int narg, int def )
{
    return luaL_opt( L, luaL_checkboolean, narg, def );
}

static inline const char *luaL_nilorcheckstring( lua_State *L, int narg )
{
    if( lua_isnil( L, narg ) )
        return NULL;
    return luaL_checkstring( L, narg );
}

static inline char *luaL_strdupornull( lua_State *L, int narg )
{
    if( lua_isstring( L, narg ) )
        return strdup( luaL_checkstring( L, narg ) );
    return NULL;
}

void heimdalllua_set_this( lua_State *, heimdall_object_t * );
#define heimdalllua_set_this(a, b) heimdalllua_set_this(a, HEIMDALL_OBJECT(b))
heimdall_object_t * heimdalllua_get_this( lua_State * );

void heimdalllua_set_playlist_internal( lua_State *, heimdall_playlist_t * );
heimdall_playlist_t * heimdalllua_get_playlist_internal( lua_State * );

/*****************************************************************************
 * Lua function bridge
 *****************************************************************************/
#define heimdalllua_error( L ) luaL_error( L, "Heimdall lua error in file %s line %d (function %s)", __FILE__, __LINE__, __func__ )
int heimdalllua_push_ret( lua_State *, int i_error );

/*****************************************************************************
 * Will execute func on all scripts in luadirname, and stop if func returns
 * success.
 *****************************************************************************/
typedef struct luabatch_context_t luabatch_context_t;
struct luabatch_context_t
{
    input_item_t *p_item;
    meta_fetcher_scope_t e_scope;
    bool (*pf_validator)( const luabatch_context_t *, meta_fetcher_scope_t );
};

int heimdalllua_scripts_batch_execute( heimdall_object_t *p_this, const char * luadirname,
        int (*func)(heimdall_object_t *, const char *, const luabatch_context_t *),
        void * user_data );
int heimdalllua_dir_list( const char *luadirname, char ***pppsz_dir_list );
void heimdalllua_dir_list_free( char **ppsz_dir_list );
char *heimdalllua_find_file( const char *psz_luadirname, const char *psz_name );

/*****************************************************************************
 * Replace Lua file reader by Heimdall input. Allows loadings scripts in Zip pkg.
 *****************************************************************************/
int heimdalllua_dofile( heimdall_object_t *p_this, lua_State *L, const char *url );

/*****************************************************************************
 * Playlist and meta data internal utilities.
 *****************************************************************************/
void heimdalllua_read_options( heimdall_object_t *, lua_State *, int *, char *** );
#define heimdalllua_read_options( a, b, c, d ) heimdalllua_read_options( HEIMDALL_OBJECT( a ), b, c, d )
void heimdalllua_read_meta_data( heimdall_object_t *, lua_State *, input_item_t * );
#define heimdalllua_read_meta_data( a, b, c ) heimdalllua_read_meta_data( HEIMDALL_OBJECT( a ), b, c )
void heimdalllua_read_custom_meta_data( heimdall_object_t *, lua_State *,
                                   input_item_t *);
#define heimdalllua_read_custom_meta_data( a, b, c ) heimdalllua_read_custom_meta_data( HEIMDALL_OBJECT( a ), b, c )

input_item_t *heimdalllua_read_input_item(heimdall_object_t *, lua_State *);

int heimdalllua_add_modules_path( lua_State *, const char *psz_filename );

struct heimdall_interrupt;

/**
 * File descriptors table
 */
typedef struct
{
    struct heimdall_interrupt *interrupt;
    int *fdv;
    unsigned fdc;
} heimdalllua_dtable_t;

int heimdalllua_fd_init( lua_State *, heimdalllua_dtable_t * );
void heimdalllua_fd_interrupt( heimdalllua_dtable_t * );
void heimdalllua_fd_cleanup( heimdalllua_dtable_t * );
struct heimdall_interrupt *heimdalllua_set_interrupt( lua_State *L );

#endif /* HEIMDALL_LUA_H */

