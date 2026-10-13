/*****************************************************************************
 * playlist.c : remote control stdin/stdout module for heimdall
 *****************************************************************************
 * Copyright (C) 2004-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Peter Surda <shurdeek@panorama.sth.ac.at>
 *         Jean-Paul Saman <jpsaman #_at_# m2x _replaceWith#dot_ nl>
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
#include <heimdall_interface.h>
#include <heimdall_input_item.h>
#include <heimdall_playlist.h>
#include <heimdall_url.h>

#include "cli.h"

#ifndef HAVE_WORDEXP
/*****************************************************************************
 * parse_MRL: build a input item from a full mrl
 *****************************************************************************
 * MRL format: "simplified-mrl [:option-name[=option-value]]"
 * We don't check for '"' or '\'', we just assume that a ':' that follows a
 * space is a new option. Should be good enough for our purpose.
 *****************************************************************************/
static input_item_t *parse_MRL(const char *mrl)
{
#define SKIPSPACE( p ) { while( *p == ' ' || *p == '\t' ) p++; }
#define SKIPTRAILINGSPACE( p, d ) \
    { char *e = d; while (e > p && (*(e-1)==' ' || *(e-1)=='\t')) {e--; *e=0 ;} }

    input_item_t *p_item = NULL;
    char *psz_item = NULL, *psz_item_mrl = NULL, *psz_orig, *psz_mrl;
    char **ppsz_options = NULL;
    int i_options = 0;

    if (mrl == NULL)
        return 0;

    psz_mrl = psz_orig = strdup( mrl );
    if (psz_mrl == NULL)
        return NULL;

    while (*psz_mrl)
    {
        SKIPSPACE(psz_mrl);
        psz_item = psz_mrl;

        for (; *psz_mrl; psz_mrl++)
        {
            if ((*psz_mrl == ' ' || *psz_mrl == '\t') && psz_mrl[1] == ':')
            {
                /* We have a complete item */
                break;
            }
            if ((*psz_mrl == ' ' || *psz_mrl == '\t') &&
                (psz_mrl[1] == '"' || psz_mrl[1] == '\'') && psz_mrl[2] == ':')
            {
                /* We have a complete item */
                break;
            }
        }

        if (*psz_mrl)
        {
            *psz_mrl = 0;
            psz_mrl++;
        }
        SKIPTRAILINGSPACE(psz_item, psz_item + strlen(psz_item));

        /* Remove '"' and '\'' if necessary */
        if (*psz_item == '"' && psz_item[strlen(psz_item)-1] == '"')
        {
            psz_item++;
            psz_item[strlen(psz_item) - 1] = 0;
        }
        if (*psz_item == '\'' && psz_item[strlen(psz_item)-1] == '\'')
        {
            psz_item++;
            psz_item[strlen(psz_item)-1] = 0;
        }

        if (psz_item_mrl == NULL)
        {
            if (strstr( psz_item, "://" ) != NULL)
                psz_item_mrl = strdup(psz_item);
            else
                psz_item_mrl = heimdall_path2uri(psz_item, NULL);
            if (psz_item_mrl == NULL)
            {
                free(psz_orig);
                return NULL;
            }
        }
        else if (*psz_item)
        {
            i_options++;
            ppsz_options = xrealloc(ppsz_options, i_options * sizeof(char *));
            ppsz_options[i_options - 1] = &psz_item[1];
        }

        if (*psz_mrl)
            SKIPSPACE(psz_mrl);
    }

    /* Now create a playlist item */
    if (psz_item_mrl != NULL)
    {
        p_item = input_item_New(psz_item_mrl, NULL);
        for (int i = 0; i < i_options; i++)
            input_item_AddOption(p_item, ppsz_options[i],
                                 HEIMDALL_INPUT_OPTION_TRUSTED);
        free(psz_item_mrl);
    }

    if (i_options)
        free(ppsz_options);
    free(psz_orig);

    return p_item;
}
#endif

static void print_playlist(struct cli_client *cl, heimdall_playlist_t *playlist)
{
    size_t count = heimdall_playlist_Count(playlist);
    size_t current = heimdall_playlist_GetCurrentIndex(playlist);

    for (size_t i = 0; i < count; ++i)
    {
        heimdall_playlist_item_t *plitem = heimdall_playlist_Get(playlist, i);
        input_item_t *item = heimdall_playlist_item_GetMedia(plitem);
        heimdall_tick_t len = item->i_duration;
        char selected = (i == current) ? '*' : ' ';

        if (len != INPUT_DURATION_INDEFINITE && len != HEIMDALL_TICK_INVALID)
        {
            char buf[MSTRTIME_MAX_SIZE];
            heimdall_tick_to_str(buf, len);
            cli_printf(cl, "| %c%zu %s (%s)", selected, i, item->psz_name, buf);
        }
        else
            cli_printf(cl, "| %c%zu %s", selected, i, item->psz_name);
    }
}

static int PlaylistDoVoid(struct cli_client *cl, void *data,
                          int (*cb)(heimdall_playlist_t *))
{
    heimdall_playlist_t *playlist = data;
    int ret;

    heimdall_playlist_Lock(playlist);
    ret = cb(playlist);
    heimdall_playlist_Unlock(playlist);
    (void) cl;
    return ret;
}

static int PlaylistPrev(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, heimdall_playlist_Prev);
}

static int PlaylistNext(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, heimdall_playlist_Next);
}

static int PlaylistPlay(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, heimdall_playlist_Start);
}

static int PlaylistDoStop(heimdall_playlist_t *playlist)
{
    heimdall_playlist_Stop(playlist);
    return 0;
}

static int PlaylistStop(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, PlaylistDoStop);
}

static int PlaylistDoClear(heimdall_playlist_t *playlist)
{
    heimdall_playlist_Stop(playlist);
    heimdall_playlist_Clear(playlist);
    return 0;
}

static int PlaylistClear(struct cli_client *cl, const char *const *args,
                         size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, PlaylistDoClear);
}

static int PlaylistDoSort(heimdall_playlist_t *playlist)
{
    struct heimdall_playlist_sort_criterion criteria =
    {
        .key = HEIMDALL_PLAYLIST_SORT_KEY_ARTIST,
        .order = HEIMDALL_PLAYLIST_SORT_ORDER_ASCENDING
    };

    return heimdall_playlist_Sort(playlist, &criteria, 1);
}

static int PlaylistSort(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    (void) args; (void) count;
    return PlaylistDoVoid(cl, data, PlaylistDoSort);
}

static int PlaylistList(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    heimdall_playlist_t *playlist = data;

    cli_printf(cl, "+----[ Playlist ]");
    heimdall_playlist_Lock(playlist);
    print_playlist(cl, playlist);
    heimdall_playlist_Unlock(playlist);
    cli_printf(cl, "+----[ End of playlist ]");
    (void) args; (void) count;
    return 0;
}

static int PlaylistRepeatCommon(struct cli_client *cl, const char *const *args,
                                size_t count, void *data,
                                enum heimdall_playlist_playback_repeat on_mode)

{
    heimdall_playlist_t *playlist = data;

    heimdall_playlist_Lock(playlist);

    enum heimdall_playlist_playback_repeat cur_mode =
                                      heimdall_playlist_GetPlaybackRepeat(playlist);
    enum heimdall_playlist_playback_repeat new_mode;

    if (cur_mode == on_mode)
        new_mode = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
    else
        new_mode = on_mode;

    if (count > 1)
    {
        if (strcmp(args[1], "on") == 0)
            new_mode = on_mode;
        if (strcmp(args[1], "off") == 0)
            new_mode = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
    }

    if (new_mode != cur_mode)
        heimdall_playlist_SetPlaybackRepeat(playlist, new_mode);

    heimdall_playlist_Unlock(playlist);
    (void) cl;
    return 0;
}

static int PlaylistRepeat(struct cli_client *cl, const char *const *args,
                          size_t count, void *data)
{
    return PlaylistRepeatCommon(cl, args, count, data,
                                HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT);
}

static int PlaylistLoop(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    return PlaylistRepeatCommon(cl, args, count, data,
                                HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL);
}

static int PlaylistRandom(struct cli_client *cl, const char *const *args,
                          size_t count, void *data)
{
    heimdall_playlist_t *playlist = data;

    heimdall_playlist_Lock(playlist);

    enum heimdall_playlist_playback_order cur_mode =
                                       heimdall_playlist_GetPlaybackOrder(playlist);
    enum heimdall_playlist_playback_order new_mode;

    if (cur_mode == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
        new_mode = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;
    else
        new_mode = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;

    if (count > 1)
    {
        if (strcmp(args[1], "on") == 0)
            new_mode = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
        if (strcmp(args[1], "off") == 0)
            new_mode = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;
    }

    if (new_mode != cur_mode)
        heimdall_playlist_SetPlaybackOrder(playlist, new_mode);

    heimdall_playlist_Unlock(playlist);
    (void) cl;
    return 0;
}

static int PlaylistGoto(struct cli_client *cl, const char *const *args,
                        size_t n_args, void *data)
{
    heimdall_playlist_t *playlist = data;
    const char *arg = n_args > 1 ? args[1] : "";
    unsigned long long index = atoll(arg);

    heimdall_playlist_Lock(playlist);

    int ret = heimdall_playlist_PlayAt(playlist, index);
    if (ret) {
        size_t count = heimdall_playlist_Count(playlist);

        cli_printf(cl,
                   heimdall_ngettext("Playlist has only %zu element",
                                "Playlist has only %zu elements", count),
                   count);
    }

    heimdall_playlist_Unlock(playlist);
    return ret;
}

static int PlaylistAddCommon(struct cli_client *cl, const char *const *args,
                             size_t n_args, void *data, bool play)
{
    heimdall_playlist_t *playlist = data;
    size_t count;
    int ret = 0;

    heimdall_playlist_Lock(playlist);
    count = heimdall_playlist_Count(playlist);
#ifdef HAVE_WORDEXP

    for (size_t i = 1; i < n_args;)
    {
        input_item_t *item;

        if (strstr(args[i], "://" ) != NULL)
            item = input_item_New(args[i], NULL);
        else
        {
            char *url = heimdall_path2uri(args[i], NULL);

            if (url != NULL)
            {
                item = input_item_New(url, NULL);
                free(url);
            }
            else
                item = NULL;
        }

        i++;

        /* Check if following argument(s) are input item options prefixed with
         * a colon.
         */
        while (i < n_args && args[i][0] == ':')
        {
            if (likely(item != NULL)
             && input_item_AddOption(item, args[i] + 1,
                                     HEIMDALL_INPUT_OPTION_TRUSTED))
            {
                input_item_Release(item);
                item = NULL;
            }
            i++;
        }

        if (unlikely(item == NULL))
        {
            ret = HEIMDALL_ENOMEM;
            continue;
        }

        if (heimdall_playlist_InsertOne(playlist, count, item) == HEIMDALL_SUCCESS)
        {
            if (play)
                heimdall_playlist_PlayAt(playlist, count);

            count++;
        }

        input_item_Release(item);
    }
    (void) cl;
#else
    const char *arg = n_args > 1 ? args[1] : "";

    input_item_t *item = parse_MRL( arg );

    if (item != NULL)
    {
        cli_printf(cl, "Trying to %s %s to playlist.",
                   play ? "add" : "enqueue", arg);

        if (heimdall_playlist_InsertOne(playlist, count, item) == HEIMDALL_SUCCESS
         && play)
            heimdall_playlist_PlayAt(playlist, count);

        input_item_Release(item);
    }
#endif
    heimdall_playlist_Unlock(playlist);
    return ret;
}

static int PlaylistAdd(struct cli_client *cl, const char *const *args,
                       size_t count, void *data)
{
    return PlaylistAddCommon(cl, args, count, data, true);
}

static int PlaylistEnqueue(struct cli_client *cl, const char *const *args,
                           size_t count, void *data)
{
    return PlaylistAddCommon(cl, args, count, data, false);
}

static int PlaylistMove(struct cli_client *cl, const char *const *args,
                        size_t count, void *data)
{
    heimdall_playlist_t *playlist = data;
    int ret;

    if (count != 3)
    {
        cli_printf(cl, "%s expects two parameters", args[0]);
        return HEIMDALL_EGENERIC /*EINVAL*/;
    }

    size_t from = strtoul(args[1], NULL, 0);
    size_t to = strtoul(args[2], NULL, 0);

    heimdall_playlist_Lock(playlist);
    size_t size = heimdall_playlist_Count(playlist);

    if (from < size && to < size)
    {
        heimdall_playlist_Move(playlist, from, 1, to);
        ret = 0;
    }
    else
    {
        cli_printf(cl, heimdall_ngettext("Playlist has only %zu element",
                                    "Playlist has only %zu elements", size),
                  size);
        ret = HEIMDALL_ENOENT;
    }
    heimdall_playlist_Unlock(playlist);
    return ret;
}

static void ItemPrint(struct cli_client *cl, input_item_t *item)
{
    heimdall_meta_t *meta;
    info_category_t *category;
    char **extras;

    heimdall_mutex_lock(&item->lock);
    meta = item->p_meta;
    cli_printf(cl, "+----[ %s ]", "Meta data");
    cli_printf(cl, "| ");

    for (int i = 0; i < HEIMDALL_META_TYPE_COUNT; i++) {
        const char *s = heimdall_meta_Get(meta, i);

        if (s != NULL)
            cli_printf(cl, "| %s: %s", heimdall_meta_TypeToString(i), s);
    }

    extras = heimdall_meta_CopyExtraNames(meta);
    if (extras != NULL) {
        for (size_t i = 0; extras[i] != NULL; i++) {
             cli_printf(cl, "| %s: %s", extras[i],
                        heimdall_meta_GetExtra(meta, extras[i]));
             free(extras[i]);
        }
        free(extras);
    }

    cli_printf(cl, "| ");

    heimdall_list_foreach(category, &item->categories, node) {
        info_t *info;

        if (info_category_IsHidden(category))
            continue;

        cli_printf(cl, "+----[ %s ]", category->psz_name);
        cli_printf(cl, "| ");
        info_foreach(info, &category->infos)
            cli_printf(cl, "| %s: %s", info->psz_name, info->psz_value);
        cli_printf(cl, "| ");
    }
    cli_printf(cl, "+----[ %s ]", "end of stream info");
    heimdall_mutex_unlock(&item->lock);
}

static int PlaylistItemInfo(struct cli_client *cl, const char *const *args,
                            size_t count, void *data)
{
    heimdall_playlist_t *playlist = data;
    ssize_t idx;
    input_item_t *item;

    heimdall_playlist_Lock(playlist);
    if (count >= 2)
        idx = atoi(args[1]);
    else
        idx = heimdall_playlist_GetCurrentIndex(playlist);

    if ((size_t)idx < heimdall_playlist_Count(playlist))
        item = heimdall_playlist_item_GetMedia(heimdall_playlist_Get(playlist, idx));
    else
        item = NULL;

    if (item != NULL)
        ItemPrint(cl, item);
    else
        cli_printf(cl, "no input");
    heimdall_playlist_Unlock(playlist);
    (void) args; (void) count;
    return (item != NULL) ? 0 : HEIMDALL_ENOENT;
}

static const struct cli_handler cmds[] =
{
    { "playlist", PlaylistList },
    { "sort", PlaylistSort },
    { "play", PlaylistPlay },
    { "stop", PlaylistStop },
    { "clear", PlaylistClear },
    { "prev", PlaylistPrev },
    { "next", PlaylistNext },
    { "add", PlaylistAdd },
    { "repeat", PlaylistRepeat },
    { "loop", PlaylistLoop },
    { "random", PlaylistRandom },
    { "enqueue", PlaylistEnqueue },
    { "goto", PlaylistGoto },
    { "move", PlaylistMove },
    { "info", PlaylistItemInfo },
};

void RegisterPlaylist(intf_thread_t *intf)
{
    RegisterHandlers(intf, cmds, ARRAY_SIZE(cmds),
                     heimdall_intf_GetMainPlaylist(intf));
}
