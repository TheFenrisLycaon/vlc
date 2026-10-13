/*****************************************************************************
 * playlist.c
 *****************************************************************************
 * Copyright (C) 2007-2011 the VideoLAN team
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

#include <heimdall_interface.h>
#include <heimdall_playlist.h>
#include <heimdall_player.h>

#include "../heimdall.h"
#include "../libs.h"
#include "input.h"
#include "variables.h"
#include "misc.h"

void heimdalllua_set_playlist_internal(lua_State *L, heimdall_playlist_t *playlist)
{
    heimdalllua_set_object(L, heimdalllua_set_playlist_internal, playlist);
}

heimdall_playlist_t *heimdalllua_get_playlist_internal(lua_State *L)
{
    return heimdalllua_get_object(L, heimdalllua_set_playlist_internal);
}

static int heimdalllua_playlist_prev(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_playlist_Lock(playlist);
    heimdall_playlist_Prev(playlist);
    heimdall_playlist_Unlock(playlist);
    return 0;
}

static int heimdalllua_playlist_next(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_playlist_Lock(playlist);
    heimdall_playlist_Next(playlist);
    heimdall_playlist_Unlock(playlist);
    return 0;
}

static int heimdalllua_playlist_skip(lua_State *L)
{
    int n = luaL_checkinteger( L, 1 );
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    if (n < 0) {
        for (int i = 0; i < -n; i++)
            heimdall_playlist_Prev(playlist);
    } else {
        for (int i = 0; i < n; ++i)
            heimdall_playlist_Next(playlist);
    }
    return 0;
}

static int heimdalllua_playlist_play(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_playlist_Lock(playlist);
    if (heimdall_playlist_GetCurrentIndex(playlist) == -1 &&
            heimdall_playlist_Count(playlist) > 0)
        heimdall_playlist_GoTo(playlist, 0);
    heimdall_playlist_Start(playlist);
    heimdall_playlist_Unlock(playlist);
    return 0;
}

static int heimdalllua_playlist_pause(lua_State *L)
{
    /* this is in fact a toggle pause */
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);

    heimdall_player_Lock(player);
    if (heimdall_player_GetState(player) != HEIMDALL_PLAYER_STATE_PAUSED)
        heimdall_player_Pause(player);
    else
        heimdall_player_Resume(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_playlist_stop(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_playlist_Lock(playlist);
    heimdall_playlist_Stop(playlist);
    heimdall_playlist_Unlock(playlist);
    return 0;
}

static int heimdalllua_playlist_clear( lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    heimdall_playlist_Lock(playlist);
    heimdall_playlist_Clear(playlist);
    heimdall_playlist_Unlock(playlist);
    return 0;
}

static bool take_bool(lua_State *L)
{
    const char *s = luaL_checkstring(L, -1);
    lua_pop( L, 1 );
    return s && !strcmp(s, "on");
}

static int heimdalllua_playlist_repeat_(lua_State *L,
                               enum heimdall_playlist_playback_repeat enabled_mode)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    int top = lua_gettop(L);
    if (top > 1)
        return heimdalllua_error(L);

    heimdall_playlist_Lock(playlist);

    bool enable;
    if (top == 0)
    {
        /* no value provided, toggle the current */
        enum heimdall_playlist_playback_repeat repeat =
                heimdall_playlist_GetPlaybackRepeat(playlist);
        enable = repeat != enabled_mode;
    }
    else
    {
        /* use the provided value */
        enable = take_bool(L);
    }

    enum heimdall_playlist_playback_repeat new_repeat = enable
                                    ? enabled_mode
                                    : HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;

    heimdall_playlist_SetPlaybackRepeat(playlist, new_repeat);

    heimdall_playlist_Unlock(playlist);

    lua_pushboolean(L, enable);
    return 1;
}

static int heimdalllua_playlist_repeat(lua_State *L)
{
    return heimdalllua_playlist_repeat_(L, HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT);
}

static int heimdalllua_playlist_loop(lua_State *L)
{
    return heimdalllua_playlist_repeat_(L, HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL);
}

static int heimdalllua_playlist_get_repeat(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);
    enum heimdall_playlist_playback_repeat repeat =
            heimdall_playlist_GetPlaybackRepeat(playlist);
    bool result = repeat != HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
    heimdall_playlist_Unlock(playlist);

    lua_pushboolean(L, result);
    return 1;
}

static int heimdalllua_playlist_get_loop(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);
    enum heimdall_playlist_playback_repeat repeat =
            heimdall_playlist_GetPlaybackRepeat(playlist);
    bool result = repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
    heimdall_playlist_Unlock(playlist);

    lua_pushboolean(L, result);
    return 1;
}

static int heimdalllua_playlist_random(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    int top = lua_gettop(L);
    if (top > 1)
        return heimdalllua_error(L);

    heimdall_playlist_Lock(playlist);

    bool enable;
    if (top == 0)
    {
        enum heimdall_playlist_playback_order order =
                heimdall_playlist_GetPlaybackOrder(playlist);
        enable = order != HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
    }
    else
    {
        /* use the provided value */
        enable = take_bool(L);
    }

    enum heimdall_playlist_playback_order new_order = enable
                                    ? HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM
                                    : HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;

    heimdall_playlist_SetPlaybackOrder(playlist, new_order);

    heimdall_playlist_Unlock(playlist);

    lua_pushboolean(L, enable);
    return 1;
}

static int heimdalllua_playlist_get_random(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);
    enum heimdall_playlist_playback_order order =
            heimdall_playlist_GetPlaybackOrder(playlist);
    bool result = order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
    heimdall_playlist_Unlock(playlist);

    lua_pushboolean(L, result);
    return 1;
}

static int heimdalllua_playlist_gotoitem(lua_State *L)
{
    uint64_t id = luaL_checkinteger(L, 1);
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    int ret;

    heimdall_playlist_Lock(playlist);
    ssize_t index = heimdall_playlist_IndexOfId(playlist, id);
    if (index == -1)
        ret = HEIMDALL_ENOENT;
    else
    {
        heimdall_playlist_GoTo(playlist, index);
        ret = HEIMDALL_SUCCESS;
    }
    heimdall_playlist_Unlock(playlist);

    return heimdalllua_push_ret(L, ret);
}

static int heimdalllua_playlist_delete(lua_State *L)
{
    uint64_t id = luaL_checkinteger(L, 1);
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    int ret;

    heimdall_playlist_Lock(playlist);
    ssize_t index = heimdall_playlist_IndexOfId(playlist, id);
    if (index == -1)
        ret = -1;
    else
    {
        heimdall_playlist_RemoveOne(playlist, index);
        ret = HEIMDALL_SUCCESS;
    }
    heimdall_playlist_Unlock(playlist);

    return heimdalllua_push_ret(L, ret);
}

static int heimdalllua_playlist_move(lua_State *L)
{
    uint64_t item_id = luaL_checkinteger(L, 1);
    uint64_t target_id = luaL_checkinteger(L, 2);
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    int ret;

    heimdall_playlist_Lock(playlist);
    ssize_t item_index = heimdall_playlist_IndexOfId(playlist, item_id);
    ssize_t target_index = heimdall_playlist_IndexOfId(playlist, target_id);
    if (item_index == -1 || target_index == -1)
        ret = -1;
    else
    {
        /* if the current item was before the target, moving it shifts the
         * target item by one */
        size_t new_index = item_index <= target_index ? target_index
                                                      : target_index + 1;
        heimdall_playlist_MoveOne(playlist, item_index, new_index);
        ret = HEIMDALL_SUCCESS;
    }
    heimdall_playlist_Unlock(playlist);

    return heimdalllua_push_ret(L, ret);
}

static int heimdalllua_playlist_add_common(lua_State *L, bool play)
{
    heimdall_object_t *obj = heimdalllua_get_this(L);
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    int count = 0;

    /* playlist */
    if (!lua_istable(L, -1))
    {
        msg_Warn(obj, "Playlist should be a table.");
        return 0;
    }

    lua_pushnil(L);

    heimdall_playlist_Lock(playlist);

    /* playlist nil */
    while (lua_next(L, -2))
    {
        input_item_t *item = heimdalllua_read_input_item(obj, L);
        if (item != NULL)
        {
            int ret = heimdall_playlist_AppendOne(playlist, item);
            if (ret == HEIMDALL_SUCCESS)
            {
                count++;
                if (play)
                {
                    size_t last = heimdall_playlist_Count(playlist) - 1;
                    heimdall_playlist_PlayAt(playlist, last);
                }
            }
            input_item_Release(item);
        }
        /* pop the value, keep the key for the next lua_next() call */
        lua_pop(L, 1);
    }
    /* playlist */

    heimdall_playlist_Unlock(playlist);

    lua_pushinteger(L, count);
    return 1;
}

static int heimdalllua_playlist_add(lua_State *L)
{
    return heimdalllua_playlist_add_common(L, true);
}

static int heimdalllua_playlist_enqueue(lua_State *L)
{
    return heimdalllua_playlist_add_common(L, false);
}

static void push_playlist_item(lua_State *L, heimdall_playlist_item_t *item)
{
    lua_newtable(L);

    lua_pushinteger(L, heimdall_playlist_item_GetId(item));
    lua_setfield(L, -2, "id");

    input_item_t *media = heimdall_playlist_item_GetMedia(item);

    /* Apart from nb_played, these fields unfortunately duplicate
       fields already available from the input item */
    char *name = input_item_GetTitleFbName(media);
    lua_pushstring(L, name);
    free(name);
    lua_setfield(L, -2, "name");

    lua_pushstring(L, media->psz_uri);
    lua_setfield(L, -2, "path");

    if( media->i_duration < 0 )
        lua_pushnumber(L, -1);
    else
        lua_pushnumber(L, secf_from_heimdall_tick(media->i_duration));
    lua_setfield(L, -2, "duration");

    luaopen_input_item(L, media);
}

static int heimdalllua_playlist_get(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);
    uint64_t item_id = luaL_checkinteger(L, 1);

    heimdall_playlist_Lock(playlist);
    ssize_t index = heimdall_playlist_IndexOfId(playlist, item_id);
    heimdall_playlist_item_t *item = index != -1 ? heimdall_playlist_Get(playlist, index)
                                            : NULL;
    if (item)
        push_playlist_item(L, item);
    else
        lua_pushnil(L);
    heimdall_playlist_Unlock(playlist);

    return 1;
}

static int heimdalllua_playlist_list(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);

    size_t count = heimdall_playlist_Count(playlist);
    lua_createtable(L, count, 0);

    for (size_t i = 0; i < count; ++i)
    {
        push_playlist_item(L, heimdall_playlist_Get(playlist, i));
        lua_rawseti(L, -2, i + 1);
    }

    heimdall_playlist_Unlock(playlist);

    return 1;
}

static int heimdalllua_playlist_current(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);
    ssize_t current = heimdall_playlist_GetCurrentIndex(playlist);
    int id;
    if (current != -1) {
        heimdall_playlist_item_t *item = heimdall_playlist_Get(playlist, current);
        id = heimdall_playlist_item_GetId(item);
    } else
        id = -1;
    heimdall_playlist_Unlock(playlist);

    lua_pushinteger(L, id);
    return 1;
}

static int heimdalllua_playlist_current_item(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_playlist_Lock(playlist);
    ssize_t index = heimdall_playlist_GetCurrentIndex(playlist);
    heimdall_playlist_item_t *item = index != -1 ? heimdall_playlist_Get(playlist, index)
                                            : NULL;
    if (item)
        push_playlist_item(L, item);
    else
        lua_pushnil(L);
    heimdall_playlist_Unlock(playlist);

    return 1;
}

static bool heimdall_sort_key_from_string(const char *keyname,
                                     enum heimdall_playlist_sort_key *key)
{
    static const struct
    {
        const char *keyname;
        enum heimdall_playlist_sort_key key;
    } map[] = {
        { "title",    HEIMDALL_PLAYLIST_SORT_KEY_TITLE },
        { "artist",   HEIMDALL_PLAYLIST_SORT_KEY_ARTIST },
        { "genre",    HEIMDALL_PLAYLIST_SORT_KEY_GENRE },
        { "duration", HEIMDALL_PLAYLIST_SORT_KEY_DURATION },
        { "album",    HEIMDALL_PLAYLIST_SORT_KEY_ALBUM },
    };
    for (size_t i = 0; i < ARRAY_SIZE(map); ++i)
    {
        if (!strcmp(keyname, map[i].keyname))
        {
            *key = map[i].key;
            return true;
        }
    }
    return false;
}

static int heimdalllua_playlist_sort( lua_State *L )
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    const char *keyname = luaL_checkstring(L, 1);

    int ret;
    if (!strcmp(keyname, "random"))
    {
        /* sort randomly -> shuffle */
        heimdall_playlist_Lock(playlist);
        heimdall_playlist_Shuffle(playlist);
        heimdall_playlist_Unlock(playlist);
        ret = HEIMDALL_SUCCESS;
    }
    else
    {
        struct heimdall_playlist_sort_criterion criterion;
        if (!heimdall_sort_key_from_string(keyname, &criterion.key))
            return luaL_error(L, "Invalid search key.");
        criterion.order = luaL_optboolean(L, 2, 0)
                        ? HEIMDALL_PLAYLIST_SORT_ORDER_DESCENDING
                        : HEIMDALL_PLAYLIST_SORT_ORDER_ASCENDING;

        heimdall_playlist_Lock(playlist);
        ret = heimdall_playlist_Sort(playlist, &criterion, 1);
        heimdall_playlist_Unlock(playlist);
    }
    return heimdalllua_push_ret(L, ret);
}

static int heimdalllua_playlist_status(lua_State *L)
{
    heimdall_playlist_t *playlist = heimdalllua_get_playlist_internal(L);

    heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);
    heimdall_player_Lock(player);
    enum heimdall_player_state state = heimdall_player_GetState(player);
    heimdall_player_Unlock(player);

    switch (state)
    {
        case HEIMDALL_PLAYER_STATE_STOPPED:
            lua_pushliteral(L, "stopped");
            break;
        case HEIMDALL_PLAYER_STATE_STARTED:
            lua_pushliteral(L, "started");
            break;
        case HEIMDALL_PLAYER_STATE_PLAYING:
            lua_pushliteral(L, "playing");
            break;
        case HEIMDALL_PLAYER_STATE_PAUSED:
            lua_pushliteral(L, "paused");
            break;
        case HEIMDALL_PLAYER_STATE_STOPPING:
            lua_pushliteral(L, "stopping");
            break;
        default:
            lua_pushliteral(L, "unknown");
    }
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_playlist_reg[] = {
    { "prev", heimdalllua_playlist_prev },
    { "next", heimdalllua_playlist_next },
    { "skip", heimdalllua_playlist_skip },
    { "play", heimdalllua_playlist_play },
    { "pause", heimdalllua_playlist_pause },
    { "stop", heimdalllua_playlist_stop },
    { "clear", heimdalllua_playlist_clear },
    { "repeat", heimdalllua_playlist_repeat }, // repeat is a reserved lua keyword...
    { "repeat_", heimdalllua_playlist_repeat }, // ... provide repeat_ too.
    { "loop", heimdalllua_playlist_loop },
    { "random", heimdalllua_playlist_random },
    { "get_repeat", heimdalllua_playlist_get_repeat },
    { "get_loop", heimdalllua_playlist_get_loop },
    { "get_random", heimdalllua_playlist_get_random },
#if LUA_VERSION_NUM < 502
    { "goto", heimdalllua_playlist_gotoitem },
#endif
    { "gotoitem", heimdalllua_playlist_gotoitem },
    { "add", heimdalllua_playlist_add },
    { "enqueue", heimdalllua_playlist_enqueue },
    { "get", heimdalllua_playlist_get },
    { "list", heimdalllua_playlist_list },
    { "current", heimdalllua_playlist_current },
    { "current_item", heimdalllua_playlist_current_item },
    { "sort", heimdalllua_playlist_sort },
    { "status", heimdalllua_playlist_status },
    { "delete", heimdalllua_playlist_delete },
    { "move", heimdalllua_playlist_move },
    { NULL, NULL }
};

void luaopen_playlist( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_playlist_reg );
    lua_setfield( L, -2, "playlist" );
}
