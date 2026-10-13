/*****************************************************************************
 * meter.c : audio meter
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_aout.h>
#include "aout_internal.h"

struct heimdall_audio_meter_plugin
{
    char *name;
    config_chain_t *cfg;
    filter_t *filter;
    heimdall_tick_t last_date;

    struct heimdall_audio_meter_plugin_owner owner;

    struct heimdall_list node;
};

void
(heimdall_audio_meter_Init)(struct heimdall_audio_meter *meter, heimdall_object_t *obj)
{
    heimdall_mutex_init(&meter->lock);
    meter->parent = obj;
    meter->fmt = NULL;
    heimdall_list_init(&meter->plugins);
}

void
heimdall_audio_meter_Destroy(struct heimdall_audio_meter *meter)
{
    heimdall_audio_meter_plugin *plugin;
    heimdall_list_foreach(plugin, &meter->plugins, node)
        heimdall_audio_meter_RemovePlugin(meter, plugin);
}

static void
heimdall_audio_meter_OnLoudnessChanged(filter_t *filter,
                             const struct heimdall_audio_loudness *loudness)
{
    heimdall_audio_meter_plugin *plugin = filter->owner.sys;

    if (plugin->owner.cbs->on_loudness != NULL)
        plugin->owner.cbs->on_loudness(plugin->last_date, loudness, plugin->owner.sys);
}

static filter_t *
heimdall_audio_meter_CreatePluginFilter(struct heimdall_audio_meter *meter, heimdall_audio_meter_plugin *plugin)
{
    static const struct filter_audio_callbacks audio_cbs = {
        .meter_loudness = { .on_changed = heimdall_audio_meter_OnLoudnessChanged }
    };

    const filter_owner_t owner = {
        .audio = &audio_cbs,
        .sys = plugin,
    };

    return aout_filter_Create(meter->parent, &owner, "audio meter", plugin->name,
                              meter->fmt, meter->fmt, plugin->cfg, true);
}

heimdall_audio_meter_plugin *
heimdall_audio_meter_AddPlugin(struct heimdall_audio_meter *meter, const char *chain,
                          const struct heimdall_audio_meter_plugin_owner *owner)
{
    assert(owner != NULL && owner->cbs != NULL);

    heimdall_audio_meter_plugin *plugin = malloc(sizeof(*plugin));
    if (plugin == NULL)
        return NULL;
    plugin->owner = *owner;
    plugin->last_date = HEIMDALL_TICK_INVALID;
    plugin->name = NULL;
    plugin->cfg = NULL;
    plugin->filter = NULL;

    free(config_ChainCreate(&plugin->name, &plugin->cfg, chain));
    if (plugin->name == NULL)
        goto error;

    heimdall_mutex_lock(&meter->lock);
    if (meter->fmt != NULL)
    {
        plugin->filter = heimdall_audio_meter_CreatePluginFilter(meter, plugin);
        if (plugin->filter == NULL)
        {
            heimdall_mutex_unlock(&meter->lock);
            goto error;
        }

        assert(plugin->filter->ops->drain_audio == NULL); /* Not supported */
    }

    heimdall_list_append(&plugin->node, &meter->plugins);
    heimdall_mutex_unlock(&meter->lock);

    return plugin;

error:
    free(plugin->name);
    if (plugin->cfg != NULL)
        config_ChainDestroy(plugin->cfg);
    free(plugin);
    return NULL;
}

void
heimdall_audio_meter_RemovePlugin(struct heimdall_audio_meter *meter, heimdall_audio_meter_plugin *plugin)
{
    heimdall_mutex_lock(&meter->lock);

    if (plugin->filter != NULL)
    {
        heimdall_filter_Delete(plugin->filter);
    }

    if (plugin->cfg != NULL)
        config_ChainDestroy(plugin->cfg);
    free(plugin->name);

    heimdall_list_remove(&plugin->node);
    free(plugin);

    heimdall_mutex_unlock(&meter->lock);
}

int
heimdall_audio_meter_Reset(struct heimdall_audio_meter *meter, const audio_sample_format_t *fmt)
{
    int ret = HEIMDALL_SUCCESS;

    heimdall_mutex_lock(&meter->lock);

    meter->fmt = fmt;

    /* Reload every plugins using the new fmt */
    heimdall_audio_meter_plugin *plugin;
    heimdall_list_foreach(plugin, &meter->plugins, node)
    {
        if (plugin->filter != NULL)
        {
            heimdall_filter_Delete(plugin->filter);
            plugin->filter = NULL;
        }
        plugin->last_date = HEIMDALL_TICK_INVALID;

        if (meter->fmt != NULL)
        {
            plugin->filter = heimdall_audio_meter_CreatePluginFilter(meter, plugin);
            if (plugin->filter == NULL)
            {
                ret = HEIMDALL_EGENERIC;
                break;
            }
        }
    }

    heimdall_mutex_unlock(&meter->lock);

    return ret;
}

void
heimdall_audio_meter_Process(struct heimdall_audio_meter *meter, block_t *block, heimdall_tick_t date)
{
    heimdall_mutex_lock(&meter->lock);

    heimdall_audio_meter_plugin *plugin;
    heimdall_list_foreach(plugin, &meter->plugins, node)
    {
        filter_t *filter = plugin->filter;

        if (filter != NULL)
        {
            plugin->last_date = date + block->i_length;

            block_t *same_block = filter->ops->filter_audio(filter, block);
            assert(same_block == block); (void) same_block;
        }
    }

    heimdall_mutex_unlock(&meter->lock);
}

void
heimdall_audio_meter_Flush(struct heimdall_audio_meter *meter)
{
    heimdall_mutex_lock(&meter->lock);

    heimdall_audio_meter_plugin *plugin;
    heimdall_list_foreach(plugin, &meter->plugins, node)
    {
        filter_t *filter = plugin->filter;
        if (filter != NULL)
            filter_Flush(filter);
    }

    heimdall_mutex_unlock(&meter->lock);
}
