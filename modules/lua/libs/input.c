/*****************************************************************************
 * input.c
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
#include <heimdall_es.h>
#include <heimdall_meta.h>
#include <heimdall_url.h>
#include <heimdall_playlist.h>
#include <heimdall_player.h>

#include <assert.h>

#include "../heimdall.h"
#include "input.h"
#include "../libs.h"
#include "../extension.h"

vout_thread_t *heimdalllua_get_vout_internal(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);
    return heimdall_player_vout_Hold(player);
}

audio_output_t *heimdalllua_get_aout_internal(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);
    return heimdall_player_aout_Hold(player);
}

static input_item_t* heimdalllua_input_item_get_internal( lua_State *L );

static int heimdalllua_input_item_info( lua_State *L )
{
    input_item_t *p_item = heimdalllua_input_item_get_internal( L );
    int i_cat = 0;

    info_category_t *p_category;
    heimdall_list_foreach( p_category, &p_item->categories, node )
        i_cat++;

    lua_createtable( L, 0, i_cat );

    heimdall_list_foreach( p_category, &p_item->categories, node )
    {
        info_t *p_info;

        if (info_category_IsHidden(p_category))
            continue;

        lua_pushstring( L, p_category->psz_name );
        lua_newtable( L );
        info_foreach(p_info, &p_category->infos)
        {
            lua_pushstring( L, p_info->psz_name );
            lua_pushstring( L, p_info->psz_value );
            lua_settable( L, -3 );
        }
        lua_settable( L, -3 );
    }
    return 1;
}

static int heimdalllua_input_is_playing( lua_State *L )
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    bool started = heimdall_player_IsStarted(player);
    heimdall_player_Unlock(player);
    lua_pushboolean(L, started);
    return 1;
}

static int heimdalllua_player_get_title_index(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    ssize_t idx = heimdall_player_GetSelectedTitleIdx(player);
    heimdall_player_Unlock(player);

    lua_pushinteger(L, idx);
    return 1;
}

static int heimdalllua_player_get_titles_count(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    struct heimdall_player_title_list *titles = heimdall_player_GetTitleList(player);
    size_t count = titles ? heimdall_player_title_list_GetCount(titles) : 0;
    heimdall_player_Unlock(player);

    lua_pushinteger(L, count);
    return 1;
}

static int heimdalllua_player_title_next(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectNextTitle(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_title_prev(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectPrevTitle(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_title_goto(lua_State *L)
{
    int idx = luaL_checkinteger(L, 1);

    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectTitleIdx(player, idx);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_get_chapter_index(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    ssize_t idx = heimdall_player_GetSelectedChapterIdx(player);
    heimdall_player_Unlock(player);

    lua_pushinteger(L, idx);
    return 1;
}

static int heimdalllua_player_get_chapters_count(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    const struct heimdall_player_title *current_title =
        heimdall_player_GetSelectedTitle(player);

    size_t count = current_title ? current_title->chapter_count : 0;
    heimdall_player_Unlock(player);

    lua_pushinteger(L, count);
    return 1;
}

static int heimdalllua_player_chapter_next(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectNextChapter(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_chapter_prev(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectPrevChapter(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_chapter_goto(lua_State *L)
{
    int idx = luaL_checkinteger(L, 1);

    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SelectChapterIdx(player, idx);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_get_time(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_tick_t time = heimdall_player_GetTime(player);
    heimdall_player_Unlock(player);

    lua_pushinteger(L, US_FROM_HEIMDALL_TICK(time));
    return 1;
}

static int heimdalllua_player_get_position(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    float pos = heimdall_player_GetPosition(player);
    heimdall_player_Unlock(player);

    lua_pushnumber(L, pos);
    return 1;
}

static int heimdalllua_player_get_rate(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    float rate = heimdall_player_GetRate(player);
    heimdall_player_Unlock(player);

    lua_pushnumber(L, rate);
    return 1;
}

static int heimdalllua_player_set_rate(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    float rate = luaL_checknumber(L, 1);

    heimdall_player_Lock(player);
    heimdall_player_ChangeRate(player, rate);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_increment_rate(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_IncrementRate(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_decrement_rate(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_DecrementRate(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_get_tracks_(lua_State *L,
                                     enum es_format_category_e cat)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);

    size_t count = heimdall_player_GetTrackCount(player, cat);
    lua_createtable(L, count, 0);

    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_player_track *track =
                heimdall_player_GetTrackAt(player, cat, i);
        if (!track) {
            continue;
        }

        lua_newtable(L);

        lua_pushinteger(L, heimdall_es_id_GetInputId(track->es_id));
        lua_setfield(L, -2, "id");

        lua_pushstring(L, track->name);
        lua_setfield(L, -2, "name");

        lua_pushboolean(L, track->selected);
        lua_setfield(L, -2, "selected");

        lua_rawseti(L, -2, i + 1);
    }

    heimdall_player_Unlock(player);

    return 1;
}

static int heimdalllua_player_get_video_tracks(lua_State *L)
{
    return heimdalllua_player_get_tracks_(L, VIDEO_ES);
}

static int heimdalllua_player_get_audio_tracks(lua_State *L)
{
    return heimdalllua_player_get_tracks_(L, AUDIO_ES);
}

static int heimdalllua_player_get_spu_tracks(lua_State *L)
{
    return heimdalllua_player_get_tracks_(L, SPU_ES);
}

static const struct heimdall_player_track *
FindTrack(heimdall_player_t *player, enum es_format_category_e cat, int id)
{
    size_t count = heimdall_player_GetTrackCount(player, cat);
    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_player_track *track =
                heimdall_player_GetTrackAt(player, cat, i);
        if (id == heimdall_es_id_GetInputId(track->es_id))
            return track;
    }
    return NULL;
}

static int heimdalllua_player_toggle_track_(lua_State *L,
                                       enum es_format_category_e cat,
                                       int id)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);

    if (id != -1) {
        const struct heimdall_player_track *track = FindTrack(player, cat, id);

        if (track != NULL) {
            if (track->selected)
                heimdall_player_UnselectTrack(player, track);
            else
                heimdall_player_SelectTrack(player, track,
                                       HEIMDALL_PLAYER_SELECT_EXCLUSIVE);
        }
    } else
        heimdall_player_UnselectTrackCategory(player, cat);

    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_toggle_video_track(lua_State *L)
{
    int id = luaL_checkinteger(L, 1);
    return heimdalllua_player_toggle_track_(L, VIDEO_ES, id);
}

static int heimdalllua_player_toggle_audio_track(lua_State *L)
{
    int id = luaL_checkinteger(L, 1);
    return heimdalllua_player_toggle_track_(L, AUDIO_ES, id);
}

static int heimdalllua_player_toggle_spu_track(lua_State *L)
{
    int id = luaL_checkinteger(L, 1);
    return heimdalllua_player_toggle_track_(L, SPU_ES, id);
}

static int heimdalllua_player_next_video_frame(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_NextVideoFrame(player);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_seek_by_pos_(lua_State *L,
                                      enum heimdall_player_whence whence)
{
    float position = luaL_checknumber(L, 1);

    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SeekByPos(player, position, HEIMDALL_PLAYER_SEEK_PRECISE, whence);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_seek_by_pos_absolute(lua_State *L)
{
    return heimdalllua_player_seek_by_pos_(L, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
}

static int heimdalllua_player_seek_by_pos_relative(lua_State *L)
{
    return heimdalllua_player_seek_by_pos_(L, HEIMDALL_PLAYER_WHENCE_RELATIVE);
}

static int heimdalllua_player_seek_by_time_(lua_State *L,
                                       enum heimdall_player_whence whence)
{
    int usec = luaL_checkinteger(L, 1);
    heimdall_tick_t time = HEIMDALL_TICK_FROM_US(usec);

    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_player_SeekByTime(player, time, HEIMDALL_PLAYER_SEEK_PRECISE, whence);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_seek_by_time_absolute(lua_State *L)
{
    return heimdalllua_player_seek_by_time_(L, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
}

static int heimdalllua_player_seek_by_time_relative(lua_State *L)
{
    return heimdalllua_player_seek_by_time_(L, HEIMDALL_PLAYER_WHENCE_RELATIVE);
}

static int heimdalllua_player_get_audio_delay(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_tick_t delay = heimdall_player_GetAudioDelay(player);
    heimdall_player_Unlock(player);

    double delay_sec = secf_from_heimdall_tick(delay);

    lua_pushnumber(L, delay_sec);
    return 1;
}

static int heimdalllua_player_set_audio_delay(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    double delay_sec = luaL_checknumber(L, 1);
    heimdall_tick_t delay = heimdall_tick_from_sec(delay_sec);

    heimdall_player_Lock(player);
    heimdall_player_SetAudioDelay(player, delay, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_player_get_subtitle_delay(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    heimdall_tick_t delay = heimdall_player_GetSubtitleDelay(player);
    heimdall_player_Unlock(player);

    double delay_sec = secf_from_heimdall_tick(delay);

    lua_pushnumber(L, delay_sec);
    return 1;
}

static int heimdalllua_player_set_subtitle_delay(lua_State *L)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    double delay_sec = luaL_checknumber(L, 1);
    heimdall_tick_t delay = heimdall_tick_from_sec(delay_sec);

    heimdall_player_Lock(player);
    heimdall_player_SetSubtitleDelay(player, delay, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
    heimdall_player_Unlock(player);

    return 0;
}

static int heimdalllua_input_metas_internal( lua_State *L, input_item_t *p_item )
{
    if( !p_item )
    {
        lua_pushnil( L );
        return 1;
    }

    lua_newtable( L );
    const char *psz_meta;

    char* psz_uri = input_item_GetURI( p_item );
    char* psz_filename = psz_uri ? strrchr( psz_uri, '/' ) : NULL;

    if( psz_filename && psz_filename[1] == '\0' )
    {
        /* trailing slash, get the preceeding data */
        psz_filename[0] = '\0';
        psz_filename = strrchr( psz_uri, '/' );
    }

    if( psz_filename )
    {
        /* url decode, without leading slash */
        psz_filename = heimdall_uri_decode( psz_filename + 1 );
    }

    lua_pushstring( L, psz_filename );
    lua_setfield( L, -2, "filename" );

    free( psz_uri );

#define PUSH_META( n, m ) \
    psz_meta = heimdall_meta_Get( p_item->p_meta, heimdall_meta_ ## n ); \
    lua_pushstring( L, psz_meta ); \
    lua_setfield( L, -2, m )

    heimdall_mutex_lock(&p_item->lock);

    if (p_item->p_meta)
    {
        PUSH_META( Title, "title" );
        PUSH_META( Artist, "artist" );
        PUSH_META( Genre, "genre" );
        PUSH_META( Copyright, "copyright" );
        PUSH_META( Album, "album" );
        PUSH_META( TrackNumber, "track_number" );
        PUSH_META( Description, "description" );
        PUSH_META( Rating, "rating" );
        PUSH_META( Date, "date" );
        PUSH_META( Setting, "setting" );
        PUSH_META( URL, "url" );
        PUSH_META( Language, "language" );
        PUSH_META( NowPlaying, "now_playing" );
        PUSH_META( Publisher, "publisher" );
        PUSH_META( EncodedBy, "encoded_by" );
        PUSH_META( ArtworkURL, "artwork_url" );
        PUSH_META( TrackID, "track_id" );
        PUSH_META( TrackTotal, "track_total" );
        PUSH_META( Director, "director" );
        PUSH_META( Season, "season" );
        PUSH_META( Episode, "episode" );
        PUSH_META( ShowName, "show_name" );
        PUSH_META( Actors, "actors" );

#undef PUSH_META

        char **names = heimdall_meta_CopyExtraNames(p_item->p_meta);
        for(int i = 0; names[i]; i++)
        {
            const char *meta = heimdall_meta_GetExtra(p_item->p_meta, names[i]);
            lua_pushstring( L, meta );
            lua_setfield( L, -2, names[i] );
            free(names[i]);
        }
        free(names);
    }
    heimdall_mutex_unlock(&p_item->lock);

    return 1;
}

static int heimdalllua_input_item_stats( lua_State *L )
{
    input_item_t *p_item = heimdalllua_input_item_get_internal( L );
    lua_newtable( L );
    if( p_item == NULL )
        return 1;

    heimdall_mutex_lock( &p_item->lock );
    input_stats_t *p_stats = p_item->p_stats;
    if( p_stats != NULL )
    {
#define STATS_INT( n ) lua_pushinteger( L, p_item->p_stats->i_ ## n ); \
                       lua_setfield( L, -2, #n );
#define STATS_FLOAT( n ) lua_pushnumber( L, p_item->p_stats->f_ ## n ); \
                         lua_setfield( L, -2, #n );
        STATS_INT( read_packets )
        STATS_INT( read_bytes )
        STATS_FLOAT( input_bitrate )
        STATS_INT( demux_read_packets )
        STATS_INT( demux_read_bytes )
        STATS_FLOAT( demux_bitrate )
        STATS_INT( demux_corrupted )
        STATS_INT( demux_discontinuity )
        STATS_INT( decoded_audio )
        STATS_INT( decoded_video )
        STATS_INT( displayed_pictures )
        STATS_INT( late_pictures )
        STATS_INT( lost_pictures )
        STATS_INT( played_abuffers )
        STATS_INT( lost_abuffers )
#undef STATS_INT
#undef STATS_FLOAT
    }
    heimdall_mutex_unlock( &p_item->lock );
    return 1;
}

static int heimdalllua_input_add_subtitle(lua_State *L, bool b_path)
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    if (!lua_isstring(L, 1))
        return luaL_error( L, "heimdall.player.add_subtitle() usage: (path, autoselect=false)" );

    bool autoselect = false;
    if (lua_gettop(L) >= 2)
        autoselect = lua_toboolean(L, 2);

    const char *sub = luaL_checkstring(L, 1);
    char *mrl;
    if (b_path)
        mrl = heimdall_path2uri(sub, NULL);

    const char *uri = b_path ? mrl : sub;
    heimdall_player_Lock(player);
    heimdall_player_AddAssociatedMedia(player, SPU_ES, uri, autoselect, true, false);
    heimdall_player_Unlock(player);
    if (b_path)
        free(mrl);

    return 1;
}

static int heimdalllua_input_add_subtitle_path( lua_State *L )
{
    return heimdalllua_input_add_subtitle( L, true );
}

static int heimdalllua_input_add_subtitle_mrl( lua_State *L )
{
    return heimdalllua_input_add_subtitle( L, false );
}

/*****************************************************************************
 * Input items
 *****************************************************************************/

static input_item_t* heimdalllua_input_item_get_internal( lua_State *L )
{
    input_item_t **pp_item = luaL_checkudata( L, 1, "input_item" );
    input_item_t *p_item = *pp_item;

    if( !p_item )
        luaL_error( L, "script went completely foobar" );

    return p_item;
}

/* Garbage collection of an input_item_t */
static int heimdalllua_input_item_delete( lua_State *L )
{
    input_item_t **pp_item = luaL_checkudata( L, 1, "input_item" );
    input_item_t *p_item = *pp_item;

    if( !p_item )
        return luaL_error( L, "script went completely foobar" );

    *pp_item = NULL;
    input_item_Release( p_item );

    return 1;
}

static int heimdalllua_input_item_get_current( lua_State *L )
{
    heimdall_player_t *player = heimdalllua_get_player_internal(L);

    heimdall_player_Lock(player);
    input_item_t *media = heimdall_player_GetCurrentMedia(player);
    if (media)
        heimdalllua_input_item_get(L, media);
    else
        lua_pushnil(L);
    heimdall_player_Unlock(player);

    return 1;
}

static int heimdalllua_input_item_metas( lua_State *L )
{
    heimdalllua_input_metas_internal( L, heimdalllua_input_item_get_internal( L ) );
    return 1;
}

static int heimdalllua_input_item_is_preparsed( lua_State *L )
{
    lua_pushboolean( L, input_item_IsPreparsed( heimdalllua_input_item_get_internal( L ) ) );
    return 1;
}

static int heimdalllua_input_item_uri( lua_State *L )
{
    char *uri = input_item_GetURI( heimdalllua_input_item_get_internal( L ) );
    lua_pushstring( L, uri );
    free( uri );
    return 1;
}

static int heimdalllua_input_item_name( lua_State *L )
{
    char *name = input_item_GetName( heimdalllua_input_item_get_internal( L ) );
    lua_pushstring( L, name );
    free( name );
    return 1;
}

static int heimdalllua_input_item_duration( lua_State *L )
{
    heimdall_tick_t duration = input_item_GetDuration( heimdalllua_input_item_get_internal( L ) );
    lua_pushnumber( L, secf_from_heimdall_tick(duration) );
    return 1;
}

static int heimdalllua_input_item_set_meta( lua_State *L )
{
    input_item_t *p_item = heimdalllua_input_item_get_internal( L );
    lua_settop( L, 1 + 2 ); // two arguments
    const char *psz_name = luaL_checkstring( L, 2 ),
               *psz_value = luaL_checkstring( L, 3 );

#define META_TYPE( n, s ) { s, heimdall_meta_ ## n },
    static const struct
    {
        const char psz_name[15];
        unsigned char type;
    } pp_meta_types[] = {
        META_TYPE( Title, "title" )
        META_TYPE( Artist, "artist" )
        META_TYPE( Genre, "genre" )
        META_TYPE( Copyright, "copyright" )
        META_TYPE( Album, "album" )
        META_TYPE( TrackNumber, "track_number" )
        META_TYPE( Description, "description" )
        META_TYPE( Rating, "rating" )
        META_TYPE( Date, "date" )
        META_TYPE( Setting, "setting" )
        META_TYPE( URL, "url" )
        META_TYPE( Language, "language" )
        META_TYPE( NowPlaying, "now_playing" )
        META_TYPE( ESNowPlaying, "now_playing" )
        META_TYPE( Publisher, "publisher" )
        META_TYPE( EncodedBy, "encoded_by" )
        META_TYPE( ArtworkURL, "artwork_url" )
        META_TYPE( TrackID, "track_id" )
        META_TYPE( TrackTotal, "track_total" )
        META_TYPE( Director, "director" )
        META_TYPE( Season, "season" )
        META_TYPE( Episode, "episode" )
        META_TYPE( ShowName, "show_name" )
        META_TYPE( Actors, "actors" )
        META_TYPE( AlbumArtist, "album_artist" )
        META_TYPE( DiscNumber, "disc_number" )
        META_TYPE( DiscTotal, "disc_total" )
        META_TYPE( Compilation, "compilation" )
    };
#undef META_TYPE

    static_assert( sizeof(pp_meta_types)
                      == HEIMDALL_META_TYPE_COUNT * sizeof(pp_meta_types[0]),
                   "Inconsistent meta data types" );
    heimdall_meta_type_t type = heimdall_meta_Title;
    for( unsigned i = 0; i < HEIMDALL_META_TYPE_COUNT; i++ )
    {
        if( !strcasecmp( pp_meta_types[i].psz_name, psz_name ) )
        {
            type = pp_meta_types[i].type;
            input_item_SetMeta( p_item, type, psz_value );
            return 1;
        }
    }

    heimdall_meta_SetExtra( p_item->p_meta, psz_name, psz_value );
    return 1;
}

/*****************************************************************************
 * Lua bindings
 *****************************************************************************/
static const luaL_Reg heimdalllua_input_reg[] = {
    { "is_playing", heimdalllua_input_is_playing },
    { "item", heimdalllua_input_item_get_current },
    { "add_subtitle", heimdalllua_input_add_subtitle_path },
    { "add_subtitle_mrl", heimdalllua_input_add_subtitle_mrl },
    { "get_title_index", heimdalllua_player_get_title_index },
    { "get_titles_count", heimdalllua_player_get_titles_count },
    { "title_next", heimdalllua_player_title_next },
    { "title_prev", heimdalllua_player_title_prev },
    { "title_goto", heimdalllua_player_title_goto },
    { "get_chapter_index", heimdalllua_player_get_chapter_index },
    { "get_chapters_count", heimdalllua_player_get_chapters_count },
    { "chapter_next", heimdalllua_player_chapter_next },
    { "chapter_prev", heimdalllua_player_chapter_prev },
    { "chapter_goto", heimdalllua_player_chapter_goto },
    { "get_time", heimdalllua_player_get_time },
    { "get_position", heimdalllua_player_get_position },
    { "get_rate", heimdalllua_player_get_rate },
    { "set_rate", heimdalllua_player_set_rate },
    { "increment_rate", heimdalllua_player_increment_rate },
    { "decrement_rate", heimdalllua_player_decrement_rate },
    { "get_video_tracks", heimdalllua_player_get_video_tracks },
    { "get_audio_tracks", heimdalllua_player_get_audio_tracks },
    { "get_spu_tracks", heimdalllua_player_get_spu_tracks },
    { "toggle_video_track", heimdalllua_player_toggle_video_track },
    { "toggle_audio_track", heimdalllua_player_toggle_audio_track },
    { "toggle_spu_track", heimdalllua_player_toggle_spu_track },
    { "next_video_frame", heimdalllua_player_next_video_frame },
    { "seek_by_pos_absolute", heimdalllua_player_seek_by_pos_absolute },
    { "seek_by_pos_relative", heimdalllua_player_seek_by_pos_relative },
    { "seek_by_time_absolute", heimdalllua_player_seek_by_time_absolute },
    { "seek_by_time_relative", heimdalllua_player_seek_by_time_relative },
    { "get_audio_delay", heimdalllua_player_get_audio_delay },
    { "set_audio_delay", heimdalllua_player_set_audio_delay },
    { "get_subtitle_delay", heimdalllua_player_get_subtitle_delay },
    { "set_subtitle_delay", heimdalllua_player_set_subtitle_delay },
    { NULL, NULL }
};

void luaopen_input( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_input_reg );
    lua_setfield( L, -2, "player" );
}

static const luaL_Reg heimdalllua_input_item_reg[] = {
    { "is_preparsed", heimdalllua_input_item_is_preparsed },
    { "metas", heimdalllua_input_item_metas },
    { "set_meta", heimdalllua_input_item_set_meta },
    { "uri", heimdalllua_input_item_uri },
    { "name", heimdalllua_input_item_name },
    { "duration", heimdalllua_input_item_duration },
    { "stats", heimdalllua_input_item_stats },
    { "info", heimdalllua_input_item_info },
    { NULL, NULL }
};

int heimdalllua_input_item_get( lua_State *L, input_item_t *p_item )
{
    input_item_Hold( p_item );
    input_item_t **pp = lua_newuserdata( L, sizeof( input_item_t* ) );
    *pp = p_item;

    if( luaL_newmetatable( L, "input_item" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, heimdalllua_input_item_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, heimdalllua_input_item_delete );
        lua_setfield( L, -2, "__gc" );
    }

    lua_setmetatable(L, -2);

    return 1;
}


void luaopen_input_item( lua_State *L, input_item_t *item )
{
    assert(item);
    heimdalllua_input_item_get( L, item );
    lua_setfield( L, -2, "item" );
}
