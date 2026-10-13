/*****************************************************************************
 * player_vout.c: Player vout implementation
 *****************************************************************************
 * Copyright © 2018-2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>

#include <heimdall_common.h>
#include "player.h"
#include "input/resource.h"

#define heimdall_player_vout_SendEvent(player, event, ...) do { \
    heimdall_mutex_lock(&player->vout_listeners_lock); \
    heimdall_player_vout_listener_id *listener; \
    heimdall_list_foreach(listener, &player->vout_listeners, node) \
    { \
        if (listener->cbs->event) \
            listener->cbs->event(__VA_ARGS__, listener->cbs_data); \
    } \
    heimdall_mutex_unlock(&player->vout_listeners_lock); \
} while(0)

vout_thread_t *
heimdall_player_vout_Hold(heimdall_player_t *player)
{
    vout_thread_t *vout = input_resource_HoldVout(player->resource);
    return vout ? vout : input_resource_HoldDummyVout(player->resource);
}

vout_thread_t **
heimdall_player_vout_HoldAll(heimdall_player_t *player, size_t *count)
{
    vout_thread_t **vouts;
    input_resource_HoldVouts(player->resource, &vouts, count);

    if (*count == 0)
    {
        vouts = heimdall_alloc(1, sizeof(*vouts));
        if (vouts)
        {
            *count = 1;
            vouts[0] = input_resource_HoldDummyVout(player->resource);
        }
    }
    return vouts;
}

heimdall_player_vout_listener_id *
heimdall_player_vout_AddListener(heimdall_player_t *player,
                            const struct heimdall_player_vout_cbs *cbs,
                            void *cbs_data)
{
    assert(cbs);

    heimdall_player_vout_listener_id *listener = malloc(sizeof(*listener));
    if (!listener)
        return NULL;

    listener->cbs = cbs;
    listener->cbs_data = cbs_data;

    heimdall_mutex_lock(&player->vout_listeners_lock);
    heimdall_list_append(&listener->node, &player->vout_listeners);
    heimdall_mutex_unlock(&player->vout_listeners_lock);

    return listener;
}

void
heimdall_player_vout_RemoveListener(heimdall_player_t *player,
                               heimdall_player_vout_listener_id *id)
{
    assert(id);

    heimdall_mutex_lock(&player->vout_listeners_lock);
    heimdall_list_remove(&id->node);
    heimdall_mutex_unlock(&player->vout_listeners_lock);
    free(id);
}

bool
heimdall_player_vout_IsFullscreen(heimdall_player_t *player)
{
    vout_thread_t *vout = heimdall_player_vout_Hold(player);
    bool fs =  var_GetBool(vout, "fullscreen");
    vout_Release(vout);
    return fs;
}

static int
heimdall_player_VoutCallback(heimdall_object_t *this, const char *var,
                        heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    heimdall_player_t *player = data;

    if (strcmp(var, "fullscreen") == 0)
    {
        if (oldval.b_bool != newval.b_bool )
            heimdall_player_vout_SendEvent(player, on_fullscreen_changed,
                                      (vout_thread_t *)this, newval.b_bool);
    }
    else if (strcmp(var, "video-wallpaper") == 0)
    {
        if (oldval.b_bool != newval.b_bool )
            heimdall_player_vout_SendEvent(player, on_wallpaper_mode_changed,
                                      (vout_thread_t *)this, newval.b_bool);
    }
    else
        heimdall_assert_unreachable();

    return HEIMDALL_SUCCESS;
}

static const char osd_vars[][sizeof("secondary-sub-margin")] = {
    "aspect-ratio", "autoscale", "crop", "crop-bottom",
    "crop-top", "crop-left", "crop-right", "deinterlace",
    "deinterlace-mode", "sub-margin", "secondary-sub-margin", "zoom"
};

void
heimdall_player_vout_AddCallbacks(heimdall_player_t *player, vout_thread_t *vout)
{
    var_AddCallback(vout, "fullscreen", heimdall_player_VoutCallback, player);
    var_AddCallback(vout, "video-wallpaper", heimdall_player_VoutCallback, player);

    for (size_t i = 0; i < ARRAY_SIZE(osd_vars); ++i)
        var_AddCallback(vout, osd_vars[i], heimdall_player_vout_OSDCallback, player);
}

void
heimdall_player_vout_DelCallbacks(heimdall_player_t *player, vout_thread_t *vout)
{
    var_DelCallback(vout, "fullscreen", heimdall_player_VoutCallback, player);
    var_DelCallback(vout, "video-wallpaper", heimdall_player_VoutCallback, player);

    for (size_t i = 0; i < ARRAY_SIZE(osd_vars); ++i)
        var_DelCallback(vout, osd_vars[i], heimdall_player_vout_OSDCallback, player);
}

static void
heimdall_player_vout_SetVar(heimdall_player_t *player, const char *name, int type,
                       heimdall_value_t val)
{
    vout_thread_t *vout = heimdall_player_vout_Hold(player);
    var_SetChecked(vout, name, type, val);
    vout_Release(vout);
}


static void
heimdall_player_vout_TriggerOption(heimdall_player_t *player, const char *option)
{
    /* Don't use heimdall_player_vout_Hold() since there is nothing to trigger if it
     * returns a dummy vout */
    vout_thread_t *vout = input_resource_HoldVout(player->resource);
    if (vout == NULL)
        return;
    var_TriggerCallback(vout, option);
    vout_Release(vout);
}


void
heimdall_player_vout_SetFullscreen(heimdall_player_t *player, bool enabled)
{
    heimdall_player_vout_SetVar(player, "fullscreen", HEIMDALL_VAR_BOOL,
                           (heimdall_value_t) { .b_bool = enabled });
    heimdall_player_vout_SendEvent(player, on_fullscreen_changed, NULL, enabled);
}

bool
heimdall_player_vout_IsWallpaperModeEnabled(heimdall_player_t *player)
{
    vout_thread_t *vout = heimdall_player_vout_Hold(player);
    bool wm =  var_GetBool(vout, "video-wallpaper");
    vout_Release(vout);
    return wm;
}

void
heimdall_player_vout_SetWallpaperModeEnabled(heimdall_player_t *player, bool enabled)
{
    heimdall_player_vout_SetVar(player, "video-wallpaper", HEIMDALL_VAR_BOOL,
                           (heimdall_value_t) { .b_bool = enabled });
    heimdall_player_vout_SendEvent(player, on_wallpaper_mode_changed, NULL, enabled);
}

void
heimdall_player_vout_Snapshot(heimdall_player_t *player)
{
    heimdall_player_vout_TriggerOption(player, "video-snapshot");
}
