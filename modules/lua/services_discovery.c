/*****************************************************************************
 * services_discovery.c : Services discovery using lua scripts
 *****************************************************************************
 * Copyright (C) 2010 VideoLAN and AUTHORS
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Fabio Ritrovato <sephiroth87 at lycaria dot org>
 *          Rémi Duraffort <ivoire at lycaria -dot- org>
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

#include <assert.h>
#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_services_discovery.h>
#include <heimdall_queue.h>

#include "heimdall.h"
#include "libs.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void *Run( void * );
static int DoSearch( services_discovery_t *p_sd, const char *psz_query );
static int FillDescriptor( services_discovery_t *, services_discovery_descriptor_t * );
static int Control( services_discovery_t *p_sd, int i_command, va_list args );

// When successful, the returned string is stored on top of the lua
// stack and remains valid as long as it is kept in the stack.
static const char *heimdalllua_sd_description( heimdall_object_t *obj, lua_State *L,
                                          const char *filename )
{
    lua_getglobal( L, "descriptor" );
    if( !lua_isfunction( L, -1 ) )
    {
        msg_Warn( obj, "No 'descriptor' function in '%s'", filename );
        lua_pop( L, 1 );
        return NULL;
    }

    if( lua_pcall( L, 0, 1, 0 ) )
    {
        msg_Warn( obj, "Error while running script %s, "
                  "function descriptor(): %s", filename,
                  lua_tostring( L, -1 ) );
        lua_pop( L, 1 );
        return NULL;
    }

    lua_getfield( L, -1, "title" );
    if ( !lua_isstring( L, -1 ) )
    {
        msg_Warn( obj, "'descriptor' function in '%s' returned no title",
                  filename );
        lua_pop( L, 2 );
        return NULL;
    }

    return lua_tostring( L, -1 );
}

int heimdalllua_probe_sd( heimdall_object_t *obj, const char *name )
{
    heimdall_probe_t *probe = (heimdall_probe_t *)obj;

    char *filename = heimdalllua_find_file( "sd", name );
    if( filename == NULL )
    {
        // File suddenly disappeared - maybe a race condition, no problem
        msg_Err( probe, "Couldn't probe lua services discovery script \"%s\".",
                 name );
        return HEIMDALL_PROBE_CONTINUE;
    }

    lua_State *L = luaL_newstate();
    if( !L )
    {
        msg_Err( probe, "Could not create new Lua State" );
        free( filename );
        return HEIMDALL_ENOMEM;
    }
    luaL_openlibs( L );
    if( heimdalllua_add_modules_path( L, filename ) )
    {
        msg_Err( probe, "Error while setting the module search path for %s",
                 filename );
        lua_close( L );
        free( filename );
        return HEIMDALL_ENOMEM;
    }
    if( heimdalllua_dofile( obj, L, filename ) )
    {
        msg_Err( probe, "Error loading script %s: %s", filename,
                 lua_tostring( L, -1 ) );
        lua_close( L );
        free( filename );
        return HEIMDALL_PROBE_CONTINUE;
    }
    const char *description = heimdalllua_sd_description( obj, L, filename );
    if( description == NULL )
        description = name;

    int r = HEIMDALL_ENOMEM;
    char *name_esc = config_StringEscape( name );
    char *chain;
    if( asprintf( &chain, "lua{sd='%s'}", name_esc ) != -1 )
    {
        r = heimdall_sd_probe_Add( probe, chain, description, SD_CAT_INTERNET );
        free( chain );
    }
    free( name_esc );

    lua_close( L );
    free( filename );
    return r;
}

static const char * const ppsz_sd_options[] = { "sd", NULL };

/*****************************************************************************
 * Local structures
 *****************************************************************************/
typedef struct
{
    lua_State *L;
    char *psz_filename;

    heimdall_thread_t thread;
    heimdall_queue_t queue;
    bool dead;
} services_discovery_sys_t;

struct sd_query
{
    struct sd_query *next;
    char query[];
};

/*****************************************************************************
 * Open: initialize and create stuff
 *****************************************************************************/
int Open_LuaSD( heimdall_object_t *p_this )
{
    if( lua_Disabled( p_this ) )
        return HEIMDALL_EGENERIC;

    services_discovery_t *p_sd = ( services_discovery_t * )p_this;
    services_discovery_sys_t *p_sys;
    lua_State *L = NULL;
    char *psz_name;

    if( !( p_sys = malloc( sizeof( services_discovery_sys_t ) ) ) )
        return HEIMDALL_ENOMEM;

    if( !strcmp( p_sd->psz_name, "lua" ) ||
        !strcmp( p_sd->psz_name, "luasd" ) )
    {
        // We want to load the module name "lua"
        // This module can be used to load lua script not registered
        // as builtin lua SD modules.
        config_ChainParse( p_sd, "lua-", ppsz_sd_options, p_sd->p_cfg );
        psz_name = var_GetString( p_sd, "lua-sd" );
    }
    else
    {
        // We are loading a builtin lua sd module.
        psz_name = strdup(p_sd->psz_name);
    }

    p_sd->p_sys = p_sys;
    p_sd->pf_control = Control;
    p_sys->psz_filename = heimdalllua_find_file( "sd", psz_name );
    if( !p_sys->psz_filename )
    {
        msg_Err( p_sd, "Couldn't find lua services discovery script \"%s\".",
                 psz_name );
        free( psz_name );
        goto error;
    }
    free( psz_name );
    L = luaL_newstate();
    if( !L )
    {
        msg_Err( p_sd, "Could not create new Lua State" );
        goto error;
    }
    heimdalllua_set_this( L, p_sd );
    luaL_openlibs( L );

    static const luaL_Reg p_reg[] = { { NULL, NULL } };
    luaL_register_namespace( L, "heimdall", p_reg );
    luaopen_input( L );
    luaopen_msg( L );
    luaopen_object( L );
    luaopen_sd_sd( L );
    luaopen_strings( L );
    luaopen_variables( L );
    luaopen_stream( L );
    luaopen_gettext( L );
    luaopen_xml( L );
    lua_pop( L, 1 );

    if( heimdalllua_add_modules_path( L, p_sys->psz_filename ) )
    {
        msg_Warn( p_sd, "Error while setting the module search path for %s",
                  p_sys->psz_filename );
        goto error;
    }
    if( heimdalllua_dofile( HEIMDALL_OBJECT(p_sd), L, p_sys->psz_filename ) )
    {
        msg_Err( p_sd, "Error loading script %s: %s", p_sys->psz_filename,
                  lua_tostring( L, lua_gettop( L ) ) );
        lua_pop( L, 1 );
        goto error;
    }

    // No strdup(), just don't remove the string from the lua stack
    p_sd->description = heimdalllua_sd_description( HEIMDALL_OBJECT(p_sd), L,
                                               p_sys->psz_filename );
    if( p_sd->description == NULL )
        p_sd->description = p_sd->psz_name;

    p_sys->L = L;
    p_sys->dead = false;
    heimdall_queue_Init( &p_sys->queue, offsetof (struct sd_query, next) );

    if( heimdall_clone( &p_sys->thread, Run, p_sd ) )
    {
        goto error;
    }
    return HEIMDALL_SUCCESS;

error:
    if( L )
        lua_close( L );
    free( p_sys->psz_filename );
    free( p_sys );
    return HEIMDALL_EGENERIC;
}

/*****************************************************************************
 * Close: cleanup
 *****************************************************************************/
void Close_LuaSD( heimdall_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t * )p_this;
    services_discovery_sys_t *p_sys = p_sd->p_sys;

    heimdall_queue_Kill( &p_sys->queue, &p_sys->dead );
    heimdall_join( p_sys->thread, NULL );
    free( p_sys->psz_filename );
    lua_close( p_sys->L );
    free( p_sys );
}

/*****************************************************************************
 * Run: Thread entry-point
 ****************************************************************************/
static void* Run( void *data )
{
    heimdall_thread_set_name("heimdall-lua-service");

    services_discovery_t *p_sd = ( services_discovery_t * )data;
    services_discovery_sys_t *p_sys = p_sd->p_sys;
    lua_State *L = p_sys->L;

    lua_getglobal( L, "main" );
    if( !lua_isfunction( L, lua_gettop( L ) ) || lua_pcall( L, 0, 1, 0 ) )
    {
        msg_Err( p_sd, "Error while running script %s, "
                  "function main(): %s", p_sys->psz_filename,
                  lua_tostring( L, lua_gettop( L ) ) );
        lua_pop( L, 1 );
        return NULL;
    }
    msg_Dbg( p_sd, "LuaSD script loaded: %s", p_sys->psz_filename );

    /* Force garbage collection, because the core will keep the SD
     * open, but lua will never gc until lua_close(). */
    lua_gc( L, LUA_GCCOLLECT, 0 );

    /* Main loop to handle search requests */
    struct sd_query *query;

    while( (query = heimdall_queue_DequeueKillable( &p_sys->queue,
                                               &p_sys->dead )) != NULL )
    {
        /* Execute one query */
        DoSearch( p_sd, query->query );
        free( query );
        /* Force garbage collection, because the core will keep the SD
         * open, but lua will never gc until lua_close(). */
        lua_gc( L, LUA_GCCOLLECT, 0 );
    }
    return NULL;
}

/*****************************************************************************
 * Control: services discrovery control
 ****************************************************************************/
static int Control( services_discovery_t *p_sd, int i_command, va_list args )
{
    services_discovery_sys_t *p_sys = p_sd->p_sys;

    switch( i_command )
    {
    case SD_CMD_SEARCH:
    {
        const char *psz_query = va_arg( args, const char * );
        size_t len = strlen(psz_query) + 1;
        struct sd_query *query = malloc(sizeof (*query) + len);

        if (unlikely(query == NULL))
            return HEIMDALL_ENOMEM;

        query->next = NULL;
        memcpy(query->query, psz_query, len);
        heimdall_queue_Enqueue(&p_sys->queue, query);
        break;
    }

    case SD_CMD_DESCRIPTOR:
    {
        services_discovery_descriptor_t *p_desc = va_arg( args,
                                services_discovery_descriptor_t * );
        return FillDescriptor( p_sd, p_desc );
    }
    }

    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * DoSearch: search for a given query
 ****************************************************************************/
static int DoSearch( services_discovery_t *p_sd, const char *psz_query )
{
    services_discovery_sys_t *p_sys = p_sd->p_sys;
    lua_State *L = p_sys->L;

    /* Lookup for the 'search' function */
    lua_getglobal( L, "search" );
    if( !lua_isfunction( L, lua_gettop( L ) ) )
    {
        msg_Err( p_sd, "The script '%s' does not define any 'search' function",
                 p_sys->psz_filename );
        lua_pop( L, 1 );
        return HEIMDALL_EGENERIC;
    }

    /* Push the query */
    lua_pushstring( L, psz_query );

    /* Call the 'search' function */
    if( lua_pcall( L, 1, 0, 0 ) )
    {
        msg_Err( p_sd, "Error while running the script '%s': %s",
                 p_sys->psz_filename, lua_tostring( L, lua_gettop( L ) ) );
        lua_pop( L, 1 );
        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}

/** List of capabilities */
static const char *const ppsz_capabilities[] = {
    "search",
    NULL
};

/*****************************************************************************
 * FillDescriptor: call the descriptor function and fill the structure
 ****************************************************************************/
static int FillDescriptor( services_discovery_t *p_sd,
                           services_discovery_descriptor_t *p_desc )
{
    services_discovery_sys_t *p_sys = p_sd->p_sys;
    int i_ret = HEIMDALL_EGENERIC;

    /* Create a new lua thread */
    lua_State *L = luaL_newstate();

    if( heimdalllua_dofile( HEIMDALL_OBJECT(p_sd), L, p_sys->psz_filename ) )
    {
        msg_Err( p_sd, "Error loading script %s: %s", p_sys->psz_filename,
                 lua_tostring( L, -1 ) );
        goto end;
    }

    /* Call the "descriptor" function */
    lua_getglobal( L, "descriptor" );
    if( !lua_isfunction( L, -1 ) || lua_pcall( L, 0, 1, 0 ) )
    {
        msg_Warn( p_sd, "Error getting the descriptor in '%s': %s",
                  p_sys->psz_filename, lua_tostring( L, -1 ) );
        goto end;
    }

    /* Get the different fields of the returned table */
    lua_getfield( L, -1, "short_description" );
    p_desc->psz_short_desc = luaL_strdupornull( L, -1 );
    lua_pop( L, 1 );

    lua_getfield( L, -1, "icon" );
    p_desc->psz_icon_url = luaL_strdupornull( L, -1 );
    lua_pop( L, 1 );

    lua_getfield( L, -1, "url" );
    p_desc->psz_url = luaL_strdupornull( L, -1 );
    lua_pop( L, 1 );

    lua_getfield( L, -1, "capabilities" );
    p_desc->i_capabilities = 0;
    if( lua_istable( L, -1 ) )
    {
        /* List all table entries */
        lua_pushnil( L );
        while( lua_next( L, -2 ) != 0 )
        {
            /* Key is at index -2 and value at index -1 */
            const char *psz_cap = luaL_checkstring( L, -1 );
            int i_cap = 0;
            const char *psz_iter;
            for( psz_iter = *ppsz_capabilities; psz_iter;
                 psz_iter = ppsz_capabilities[ ++i_cap ] )
            {
                if( !strcmp( psz_iter, psz_cap ) )
                {
                    p_desc->i_capabilities |= 1 << i_cap;
                    break;
                }
            }

            lua_pop( L, 1 );

            if( !psz_iter )
                msg_Warn( p_sd, "Services discovery capability '%s' unknown in "
                                "script '%s'", psz_cap, p_sys->psz_filename );
        }
    }

    lua_pop( L, 1 );
    i_ret = HEIMDALL_SUCCESS;

end:
    lua_close( L );
    return i_ret;

}
