/*****************************************************************************
 * player_metadata.c: Player metadata listener implementation
 *****************************************************************************
 * Copyright © 2020 VLC authors and VideoLAN
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
#include <heimdall_modules.h>
#include "player.h"
#include "../audio_output/aout_internal.h"

static void
heimdall_player_OnLoudnessEvent(heimdall_tick_t date,
                           const struct heimdall_audio_loudness *loudness,
                           void *data)
{
    heimdall_player_t *player = data;

    heimdall_mutex_lock(&player->metadata_listeners_lock);

    heimdall_player_metadata_listener_id *other_id;
    heimdall_list_foreach(other_id, &player->metadata_listeners, node)
    {
        switch (other_id->option)
        {
            case HEIMDALL_PLAYER_METADATA_LOUDNESS_MOMENTARY:
                other_id->cbs->on_momentary_loudness_changed(date,
                    loudness->loudness_momentary, other_id->cbs_data);
                break;
            case HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL:
                other_id->cbs->on_loudness_changed(date,
                    loudness, other_id->cbs_data);
                break;
            default: break;
        }
    }

    heimdall_mutex_unlock(&player->metadata_listeners_lock);
}

static int
heimdall_player_AddMetadataLoudnessListener(heimdall_player_t *player,
                                       heimdall_player_metadata_listener_id *listener_id)
{
    static const struct heimdall_audio_meter_cbs meter_cbs = {
        .on_loudness = heimdall_player_OnLoudnessEvent,
    };

    listener_id->audio_meter = NULL;

    heimdall_player_metadata_listener_id *audio_meter_listener_id = NULL;
    bool has_same_meter_module = false;

    heimdall_player_metadata_listener_id *other_id;
    heimdall_list_foreach(other_id, &player->metadata_listeners, node)
    {
        if (other_id->option == listener_id->option)
            has_same_meter_module = true;

        if (other_id->audio_meter != NULL)
        {
            assert(audio_meter_listener_id == NULL);
            audio_meter_listener_id = other_id;
        }
    }

    if (audio_meter_listener_id == NULL
     || (!has_same_meter_module && listener_id->option == HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL))
    {
        /* There are no audio meter plugins, or the audio meter plugin mode
         * need to be increased */
        audio_output_t *aout = heimdall_player_aout_Hold(player);
        if (aout == NULL)
            return HEIMDALL_EGENERIC;

        unsigned mode = listener_id->option == HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL ? 4 : 0;
        char chain[sizeof("ebur128{mode=X}")];
        snprintf(chain, ARRAY_SIZE(chain), "ebur128{mode=%1u}", mode);

        const struct heimdall_audio_meter_plugin_owner meter_plugin_owner =
        {
            .cbs = &meter_cbs,
            .sys = player,
        };

        listener_id->audio_meter = aout_AddMeterPlugin(aout, chain, &meter_plugin_owner);
        if (listener_id->audio_meter == NULL)
        {
            aout_Release(aout);
            return HEIMDALL_EGENERIC;
        }

        if (audio_meter_listener_id != NULL)
        {
            aout_RemoveMeterPlugin(aout, audio_meter_listener_id->audio_meter);
            audio_meter_listener_id->audio_meter = NULL;
        }
        aout_Release(aout);
    }

    return HEIMDALL_SUCCESS;

}

static void
heimdall_player_RemoveMetadataLoudnessListener(heimdall_player_t *player,
                                          heimdall_player_metadata_listener_id *listener_id)
{
    if (listener_id->audio_meter == NULL)
        return; /* This listener is not the owner of the audio meter plugin */

    /* Attach the audio meter plugin to an other listener */
    heimdall_player_metadata_listener_id *other_id;
    heimdall_list_foreach(other_id, &player->metadata_listeners, node)
    {
        if (other_id == listener_id)
            continue;

        if (other_id->option == HEIMDALL_PLAYER_METADATA_LOUDNESS_MOMENTARY
         || other_id->option == HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL)
        {
            other_id->audio_meter = listener_id->audio_meter;
            listener_id->audio_meter = NULL;
            return;
        }
    }

    /* There are no other listeners, remove the audio meter */
    audio_output_t *aout = heimdall_player_aout_Hold(player);
    if (aout != NULL)
    {
        aout_RemoveMeterPlugin(aout, listener_id->audio_meter);
        aout_Release(aout);
    }
}

heimdall_player_metadata_listener_id *
heimdall_player_AddMetadataListener(heimdall_player_t *player,
                               enum heimdall_player_metadata_option option,
                               const union heimdall_player_metadata_cbs *cbs,
                               void *cbs_data)
{
    heimdall_player_assert_locked(player);
    assert(cbs);

    heimdall_player_metadata_listener_id *listener_id = malloc(sizeof(*listener_id));
    if (listener_id == NULL)
        return NULL;

    listener_id->cbs = cbs;
    listener_id->cbs_data = cbs_data;
    listener_id->option = option;

    heimdall_mutex_lock(&player->metadata_listeners_lock);

    int ret;
    switch (option)
    {
        case HEIMDALL_PLAYER_METADATA_LOUDNESS_MOMENTARY:
        case HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL:
            ret = heimdall_player_AddMetadataLoudnessListener(player, listener_id);
            break;
        default: heimdall_assert_unreachable();
    }

    if (ret == HEIMDALL_EGENERIC)
    {
        free(listener_id);
        heimdall_mutex_unlock(&player->metadata_listeners_lock);
        return NULL;
    }

    heimdall_list_append(&listener_id->node, &player->metadata_listeners);

    heimdall_mutex_unlock(&player->metadata_listeners_lock);

    return listener_id;
}

void
heimdall_player_RemoveMetadataListener(heimdall_player_t *player,
                                    heimdall_player_metadata_listener_id *listener_id)
{
    heimdall_player_assert_locked(player);
    assert(listener_id);

    heimdall_mutex_lock(&player->metadata_listeners_lock);

    switch (listener_id->option)
    {
        case HEIMDALL_PLAYER_METADATA_LOUDNESS_MOMENTARY:
        case HEIMDALL_PLAYER_METADATA_LOUDNESS_FULL:
            heimdall_player_RemoveMetadataLoudnessListener(player, listener_id);
            break;
        default: heimdall_assert_unreachable();
    }

    heimdall_list_remove(&listener_id->node);
    free(listener_id);

    heimdall_mutex_unlock(&player->metadata_listeners_lock);
}
