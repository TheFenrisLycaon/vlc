/*****************************************************************************
 * player_aout.c: Player aout implementation
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
#include <heimdall_decoder.h>
#include "player.h"
#include "input/resource.h"

#define heimdall_player_aout_SendEvent(player, event, ...) do { \
    heimdall_mutex_lock(&player->aout_listeners_lock); \
    heimdall_player_aout_listener_id *listener; \
    heimdall_list_foreach(listener, &player->aout_listeners, node) \
    { \
        if (listener->cbs->event) \
            listener->cbs->event(__VA_ARGS__, listener->cbs_data); \
    } \
    heimdall_mutex_unlock(&player->aout_listeners_lock); \
} while(0)

audio_output_t *
heimdall_player_aout_Hold(heimdall_player_t *player)
{
    return input_resource_HoldAout(player->resource);
}

heimdall_player_aout_listener_id *
heimdall_player_aout_AddListener(heimdall_player_t *player,
                            const struct heimdall_player_aout_cbs *cbs,
                            void *cbs_data)
{
    assert(cbs);

    heimdall_player_aout_listener_id *listener = malloc(sizeof(*listener));
    if (!listener)
        return NULL;

    listener->cbs = cbs;
    listener->cbs_data = cbs_data;

    heimdall_mutex_lock(&player->aout_listeners_lock);
    heimdall_list_append(&listener->node, &player->aout_listeners);
    heimdall_mutex_unlock(&player->aout_listeners_lock);

    return listener;
}

void
heimdall_player_aout_RemoveListener(heimdall_player_t *player,
                               heimdall_player_aout_listener_id *id)
{
    assert(id);

    heimdall_mutex_lock(&player->aout_listeners_lock);
    heimdall_list_remove(&id->node);
    heimdall_mutex_unlock(&player->aout_listeners_lock);
    free(id);
}

static int
heimdall_player_AoutCallback(heimdall_object_t *this, const char *var,
                        heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    heimdall_player_t *player = data;

    if (strcmp(var, "volume") == 0)
    {
        if (oldval.f_float >= 0.f && oldval.f_float != newval.f_float)
        {
            heimdall_player_aout_SendEvent(player, on_volume_changed,
                                      (audio_output_t *)this, newval.f_float);
            heimdall_player_osd_Volume(player, false);
        }
    }
    else if (strcmp(var, "mute") == 0)
    {
        if (oldval.b_bool != newval.b_bool)
        {
            heimdall_player_aout_SendEvent(player, on_mute_changed,
                                      (audio_output_t *)this, newval.b_bool);
            heimdall_player_osd_Volume(player, true);
        }
    }
    else if (strcmp(var, "device") == 0)
    {
        const char *old = oldval.psz_string;
        const char *new = newval.psz_string;
        /* support NULL values for string comparison */
        if (old != new && (!old || !new || strcmp(old, new)))
            heimdall_player_aout_SendEvent(player, on_device_changed,
                                      (audio_output_t *)this, newval.psz_string);
    }
    else
        heimdall_assert_unreachable();

    (void) this;
    return HEIMDALL_SUCCESS;
}

float
heimdall_player_aout_GetVolume(heimdall_player_t *player)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1.f;
    float vol = aout_VolumeGet(aout);
    aout_Release(aout);

    return vol;
}

int
heimdall_player_aout_SetVolume(heimdall_player_t *player, float volume)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1;
    int ret = aout_VolumeSet(aout, volume);
    aout_Release(aout);

    return ret;
}

int
heimdall_player_aout_IncrementVolume(heimdall_player_t *player, int steps, float *result)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1;
    int ret = aout_VolumeUpdate(aout, steps, result);
    aout_Release(aout);

    return ret;
}

int
heimdall_player_aout_IsMuted(heimdall_player_t *player)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1;
    int ret = aout_MuteGet(aout);
    aout_Release(aout);

    return ret;
}

int
heimdall_player_aout_Mute(heimdall_player_t *player, bool mute)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1;
    int ret = aout_MuteSet (aout, mute);
    aout_Release(aout);

    return ret;
}

int
heimdall_player_aout_EnableFilter(heimdall_player_t *player, const char *name, bool add)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return -1;
    aout_EnableFilter(aout, name, add);
    aout_Release(aout);

    return 0;
}


static void
heimdall_player_aout_AddCallbacks(heimdall_player_t *player)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return;

    var_AddCallback(aout, "volume", heimdall_player_AoutCallback, player);
    var_AddCallback(aout, "mute", heimdall_player_AoutCallback, player);
    var_AddCallback(aout, "device", heimdall_player_AoutCallback, player);

    aout_Release(aout);
}

static void
heimdall_player_aout_DelCallbacks(heimdall_player_t *player)
{
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (!aout)
        return;

    var_DelCallback(aout, "volume", heimdall_player_AoutCallback, player);
    var_DelCallback(aout, "mute", heimdall_player_AoutCallback, player);
    var_DelCallback(aout, "device", heimdall_player_AoutCallback, player);

    aout_Release(aout);
}

audio_output_t *
heimdall_player_aout_Init(heimdall_player_t *player)
{
    audio_output_t *aout = input_resource_GetAout(player->resource);
    if (aout != NULL)
    {
        heimdall_player_aout_AddCallbacks(player);
        input_resource_PutAout(player->resource, aout);
    }
    return aout;
}

void
heimdall_player_aout_Deinit(heimdall_player_t *player)
{
    heimdall_player_aout_DelCallbacks(player);
}

void
heimdall_player_aout_Reset(heimdall_player_t *player)
{
    heimdall_player_aout_Deinit(player);

    input_resource_ResetAout(player->resource);

    heimdall_player_aout_Init(player);
}

