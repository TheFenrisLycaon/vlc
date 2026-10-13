/*****************************************************************************
 * stream_filter.c :  Lua playlist stream filter module
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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <heimdall_common.h>
#include <heimdall_access.h>

#include "heimdall.h"
#include "libs.h"

/*****************************************************************************
 * Demux specific functions
 *****************************************************************************/
struct heimdalllua_playlist
{
    lua_State *L;
    char *filename;
    char *access;
    const char *path;
};

static int heimdalllua_demux_peek( lua_State *L )
{
    stream_t *s = (stream_t *)heimdalllua_get_this(L);
    int n = luaL_checkinteger( L, 1 );
    const uint8_t *p_peek;

    ssize_t val = heimdall_stream_Peek(s->s, &p_peek, n);
    if (val > 0)
        lua_pushlstring(L, (const char *)p_peek, val);
    else
        lua_pushnil( L );
    return 1;
}

static int heimdalllua_demux_read( lua_State *L )
{
    stream_t *s = (stream_t *)heimdalllua_get_this(L);
    int n = luaL_checkinteger( L, 1 );
    char *buf = malloc(n);

    if (buf != NULL)
    {
        ssize_t val = heimdall_stream_Read(s->s, buf, n);
        if (val > 0)
            lua_pushlstring(L, buf, val);
        else
            lua_pushnil( L );
        free(buf);
    }
    else
        lua_pushnil( L );

    return 1;
}

static int heimdalllua_demux_readline( lua_State *L )
{
    stream_t *s = (stream_t *)heimdalllua_get_this(L);
    char *line = heimdall_stream_ReadLine(s->s);

    if (line != NULL)
    {
        lua_pushstring(L, line);
        free(line);
    }
    else
        lua_pushnil( L );

    return 1;
}

/*****************************************************************************
 * Called through lua_scripts_batch_execute to call 'probe' on
 * the script pointed by psz_filename.
 *****************************************************************************/
static int probe_luascript(heimdall_object_t *obj, const char *filename,
                           const luabatch_context_t *ctx)
{
    stream_t *s = (stream_t *)obj;
    struct heimdalllua_playlist *sys = s->p_sys;

    /* Initialise Lua state structure */
    lua_State *L = luaL_newstate();
    if( !L )
        return HEIMDALL_ENOMEM;

    sys->L = L;

    /* Load Lua libraries */
    luaL_openlibs( L ); /* FIXME: Don't open all the libs? */

    heimdalllua_set_this(L, s);

    /* Functions to register */
    static const luaL_Reg p_reg[] =
    {
        { "peek", heimdalllua_demux_peek },
        { NULL, NULL }
    };
    luaL_register_namespace( L, "heimdall", p_reg );
    luaopen_msg( L );
    luaopen_strings( L );
    luaopen_stream( L );
    luaopen_variables( L );
    luaopen_xml( L );

    if (sys->path != NULL)
        lua_pushstring(L, sys->path);
    else
        lua_pushnil(L);
    lua_setfield( L, -2, "path" );

    if (sys->access != NULL)
        lua_pushstring(L, sys->access);
    else
        lua_pushnil(L);
    lua_setfield( L, -2, "access" );

    lua_pop( L, 1 );

    /* Setup the module search path */
    if (heimdalllua_add_modules_path(L, filename))
    {
        msg_Warn(s, "error setting the module search path for %s", filename);
        goto error;
    }

    /* Load and run the script(s) */
    if (heimdalllua_dofile(HEIMDALL_OBJECT(s), L, filename))
    {
        msg_Warn(s, "error loading script %s: %s", filename,
                 lua_tostring(L, lua_gettop(L)));
        goto error;
    }

    lua_getglobal( L, "probe" );
    if( !lua_isfunction( L, -1 ) )
    {
        msg_Warn(s, "error running script %s: function %s(): %s",
                 filename, "probe", "not found");
        goto error;
    }

    if( lua_pcall( L, 0, 1, 0 ) )
    {
        msg_Warn(s, "error running script %s: function %s(): %s",
                 filename, "probe", lua_tostring(L, lua_gettop(L)));
        goto error;
    }

    if( lua_gettop( L ) )
    {
        if( lua_toboolean( L, 1 ) )
        {
            msg_Dbg(s, "Lua playlist script %s's "
                    "probe() function was successful", filename );
            lua_pop( L, 1 );
            sys->filename = strdup(filename);
            return HEIMDALL_SUCCESS;
        }
    }

    (void) ctx;
error:
    lua_pop( L, 1 );
    lua_close(sys->L);
    return HEIMDALL_EGENERIC;
}

static int ReadDir(stream_t *s, input_item_node_t *node)
{
    struct heimdalllua_playlist *sys = s->p_sys;
    lua_State *L = sys->L;

    /* Functions to register for parse() function call only */
    static const luaL_Reg p_reg_parse[] =
    {
        { "read", heimdalllua_demux_read },
        { "readline", heimdalllua_demux_readline },
        { NULL, NULL }
    };
    luaL_register_namespace( L, "heimdall", p_reg_parse );

    lua_getglobal( L, "parse" );

    if( !lua_isfunction( L, -1 ) )
    {
        msg_Warn(s, "error running script %s: function %s(): %s",
                 sys->filename, "parse", "not found");
        return HEIMDALL_EINVAL;
    }

    if( lua_pcall( L, 0, 1, 0 ) )
    {
        msg_Warn(s, "error running script %s: function %s(): %s",
                 sys->filename, "parse", lua_tostring(L, lua_gettop(L)));
        return HEIMDALL_EINVAL;
    }

    if (!lua_gettop(L))
    {
        msg_Err(s, "script went completely foobar");
        return HEIMDALL_EINVAL;
    }

    if (!lua_istable(L, -1))
    {
        msg_Warn(s, "Playlist should be a table.");
        return HEIMDALL_EINVAL;
    }

    lua_pushnil(L);

    /* playlist nil */
    while (lua_next(L, -2))
    {
        input_item_t *item = heimdalllua_read_input_item(HEIMDALL_OBJECT(s), L);
        if (item != NULL)
        {
            /* copy the original URL to the meta data,
             * if "URL" is still empty */
            char *url = input_item_GetURL(item);
            if (url == NULL && s->psz_url != NULL)
                input_item_SetURL(item, s->psz_url);
            free(url);

            input_item_node_AppendItem(node, item);
            input_item_Release(item);
        }
        /* pop the value, keep the key for the next lua_next() call */
        lua_pop(L, 1);
    }
    /* playlist */

    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Import_LuaPlaylist: main import function
 *****************************************************************************/
int Import_LuaPlaylist(heimdall_object_t *obj)
{
    stream_t *s = (stream_t *)obj;

    if( lua_Disabled( obj ) )
        return HEIMDALL_EGENERIC;

    struct heimdalllua_playlist *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    s->p_sys = sys;
    sys->access = NULL;
    sys->path = NULL;

    if (s->psz_url != NULL)
    {   /* Backward compatibility hack: Lua scripts expect the URI scheme and
         * the rest of the URI separately. */
        const char *p = strstr(s->psz_url, "://");
        if (p != NULL)
        {
            sys->access = strndup(s->psz_url, p - s->psz_url);
            sys->path = p + 3;
        }
    }

    int ret = heimdalllua_scripts_batch_execute(HEIMDALL_OBJECT(s), "playlist",
                                           probe_luascript, NULL);
    if (ret != HEIMDALL_SUCCESS)
    {
        free(sys->access);
        free(sys);
        return ret;
    }

    s->pf_readdir = ReadDir;
    s->pf_control = access_vaDirectoryControlHelper;
    return HEIMDALL_SUCCESS;
}

void Close_LuaPlaylist(heimdall_object_t *obj)
{
    stream_t *s = (stream_t *)obj;
    struct heimdalllua_playlist *sys = s->p_sys;

    free(sys->filename);
    assert(sys->L != NULL);
    lua_close(sys->L);
    free(sys->access);
    free(sys);
}
