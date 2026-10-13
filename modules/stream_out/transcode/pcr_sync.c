/*****************************************************************************
 * pcr_sync.c
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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
#include "config.h"
#endif

#include <assert.h>
#include <stdint.h>

#include <heimdall_common.h>

#include <heimdall_frame.h>
#include <heimdall_list.h>
#include <heimdall_tick.h>
#include <heimdall_vector.h>

#include "pcr_sync.h"

struct es_dts_entry
{
    heimdall_tick_t dts;
    bool passed;
    heimdall_tick_t discontinuity;
};

typedef struct
{
    heimdall_tick_t pcr;
    struct HEIMDALL_VECTOR(struct es_dts_entry) es_last_dts_entries;
    size_t entries_left;
    bool no_frame_before;

    struct heimdall_list node;
} pcr_event_t;

static inline void pcr_event_Delete(pcr_event_t *ev)
{
    heimdall_vector_destroy(&ev->es_last_dts_entries);
    free(ev);
}

struct es_data
{
    bool is_deleted;
    heimdall_tick_t last_input_dts;
    heimdall_tick_t last_output_dts;
    heimdall_tick_t discontinuity;
    pcr_event_t *last_pcr_event;
};

static inline struct es_data es_data_Init(void)
{
    return (struct es_data){.is_deleted = false,
                            .last_input_dts = HEIMDALL_TICK_INVALID,
                            .last_output_dts = HEIMDALL_TICK_INVALID,
                            .discontinuity = HEIMDALL_TICK_INVALID};
}

struct es_data_vec HEIMDALL_VECTOR(struct es_data);

struct heimdall_pcr_sync
{
    struct heimdall_list pcr_events;
    struct es_data_vec es_data;
    heimdall_mutex_t lock;
};

heimdall_pcr_sync_t *heimdall_pcr_sync_New(void)
{
    heimdall_pcr_sync_t *ret = malloc(sizeof(*ret));
    if (unlikely(ret == NULL))
        return NULL;

    heimdall_vector_init(&ret->es_data);
    heimdall_list_init(&ret->pcr_events);
    heimdall_mutex_init(&ret->lock);
    return ret;
}

void heimdall_pcr_sync_Delete(heimdall_pcr_sync_t *pcr_sync)
{
    heimdall_vector_destroy(&pcr_sync->es_data);

    pcr_event_t *it;
    heimdall_list_foreach(it, &pcr_sync->pcr_events, node) { pcr_event_Delete(it); }

    free(pcr_sync);
}

static bool pcr_sync_ShouldFastForwardPCR(heimdall_pcr_sync_t *pcr_sync)
{
    struct es_data it;
    heimdall_vector_foreach(it, &pcr_sync->es_data)
    {
        if (!it.is_deleted && it.last_output_dts != it.last_input_dts)
            return false;
    }
    return heimdall_list_is_empty(&pcr_sync->pcr_events);
}

static bool pcr_sync_HadFrameInputSinceLastPCR(heimdall_pcr_sync_t *pcr_sync)
{
    const pcr_event_t *pcr_event =
        heimdall_list_last_entry_or_null(&pcr_sync->pcr_events, pcr_event_t, node);

    if (pcr_event == NULL)
        return true;

    for (unsigned int i = 0; i < pcr_sync->es_data.size; ++i)
    {
        const struct es_data *curr = &pcr_sync->es_data.data[i];
        if (curr->is_deleted || curr->last_input_dts == HEIMDALL_TICK_INVALID)
            continue;

        const bool is_oob = i >= pcr_event->es_last_dts_entries.size;
        if (!is_oob && curr->last_input_dts != pcr_event->es_last_dts_entries.data[i].dts)
            return true;
        else if (is_oob)
            return true;
    }
    return false;
}

static int pcr_sync_SignalPCRLocked(heimdall_pcr_sync_t *pcr_sync, heimdall_tick_t pcr)
{
    if (pcr_sync_ShouldFastForwardPCR(pcr_sync))
        return HEIMDALL_PCR_SYNC_FORWARD_PCR;

    pcr_event_t *event = malloc(sizeof(*event));
    if (unlikely(event == NULL))
        return HEIMDALL_ENOMEM;

    heimdall_vector_init(&event->es_last_dts_entries);
    const bool alloc_succeeded =
        heimdall_vector_reserve(&event->es_last_dts_entries, pcr_sync->es_data.size);
    if (unlikely(alloc_succeeded == false))
    {
        free(event);
        return HEIMDALL_ENOMEM;
    }

    size_t entries_left = 0;
    struct es_data data;
    heimdall_vector_foreach(data, &pcr_sync->es_data)
    {
        if (!data.is_deleted)
        {
            heimdall_vector_push(&event->es_last_dts_entries,
                            ((struct es_dts_entry){.dts = data.last_input_dts,
                                                   .discontinuity = data.discontinuity}));
            if (data.last_input_dts != HEIMDALL_TICK_INVALID)
                ++entries_left;
            data.discontinuity = HEIMDALL_TICK_INVALID;
        }
    }

    event->pcr = pcr;
    event->entries_left = entries_left;
    event->no_frame_before = !pcr_sync_HadFrameInputSinceLastPCR(pcr_sync);

    heimdall_list_append(&event->node, &pcr_sync->pcr_events);

    return HEIMDALL_SUCCESS;
}

int heimdall_pcr_sync_SignalPCR(heimdall_pcr_sync_t *pcr_sync, heimdall_tick_t pcr)
{
    heimdall_mutex_lock(&pcr_sync->lock);
    const int result = pcr_sync_SignalPCRLocked(pcr_sync, pcr);
    heimdall_mutex_unlock(&pcr_sync->lock);
    return result;
}

void heimdall_pcr_sync_SignalFrame(heimdall_pcr_sync_t *pcr_sync, unsigned int id, const heimdall_frame_t *frame)
{
    heimdall_mutex_lock(&pcr_sync->lock);
    assert(id < pcr_sync->es_data.size);

    struct es_data *data = &pcr_sync->es_data.data[id];
    assert(!data->is_deleted);
    if (frame->i_dts != HEIMDALL_TICK_INVALID)
        data->last_input_dts = frame->i_dts;
    if (frame->i_flags & HEIMDALL_FRAME_FLAG_DISCONTINUITY)
    {
        assert(frame->i_dts != HEIMDALL_TICK_INVALID);
        data->discontinuity = frame->i_dts;
    }

    heimdall_mutex_unlock(&pcr_sync->lock);
}

int heimdall_pcr_sync_NewESID(heimdall_pcr_sync_t *pcr_sync, unsigned int *id)
{
    heimdall_mutex_lock(&pcr_sync->lock);

    for (unsigned int i = 0; i < pcr_sync->es_data.size; ++i)
    {
        if (pcr_sync->es_data.data[i].is_deleted)
        {
            pcr_sync->es_data.data[i] = es_data_Init();
            heimdall_mutex_unlock(&pcr_sync->lock);
            *id = i;
            return HEIMDALL_SUCCESS;
        }
    }

    const bool push_succeeded = heimdall_vector_push(&pcr_sync->es_data, es_data_Init());
    const size_t ids_count = pcr_sync->es_data.size;

    heimdall_mutex_unlock(&pcr_sync->lock);

    if (push_succeeded)
    {
        *id = ids_count - 1;
        return HEIMDALL_SUCCESS;
    }
    return HEIMDALL_ENOMEM;
}

void heimdall_pcr_sync_DelESID(heimdall_pcr_sync_t *pcr_sync, unsigned int id)
{
    heimdall_mutex_lock(&pcr_sync->lock);

    assert(id < pcr_sync->es_data.size);
    pcr_sync->es_data.data[id].is_deleted = true;

    heimdall_mutex_unlock(&pcr_sync->lock);
}

#define pcr_event_FirstEntry(head)                                                                 \
    heimdall_list_first_entry_or_null(head, pcr_event_t, node)
#define pcr_event_NextEntry(head, entry)                                                           \
    heimdall_list_next_entry_or_null(head, entry, pcr_event_t, node)

heimdall_tick_t
heimdall_pcr_sync_SignalFrameOutput(heimdall_pcr_sync_t *pcr_sync, unsigned int id, const heimdall_frame_t *frame)
{
    heimdall_mutex_lock(&pcr_sync->lock);

    struct es_data *es = &pcr_sync->es_data.data[id];
    assert(!es->is_deleted);
    es->last_output_dts = frame->i_dts;

    pcr_event_t *pcr_event = (es->last_pcr_event == NULL)
                                 ? pcr_event_FirstEntry(&pcr_sync->pcr_events)
                                 : es->last_pcr_event;
    if (pcr_event == NULL)
        goto no_pcr;

    assert(id < pcr_event->es_last_dts_entries.size);

    const heimdall_tick_t pcr = pcr_event->pcr;

    if (pcr_event->no_frame_before)
    {
        es->last_pcr_event = pcr_event_NextEntry(&pcr_sync->pcr_events, pcr_event);
        goto return_pcr;
    }

    assert(pcr_event->entries_left != 0);

    struct es_dts_entry *dts_entry = &pcr_event->es_last_dts_entries.data[id];

    if (dts_entry->discontinuity != HEIMDALL_TICK_INVALID && frame->i_dts > dts_entry->discontinuity)
        goto no_pcr;

    if (frame->i_dts < dts_entry->dts)
        goto no_pcr;

    if (dts_entry->passed)
        goto no_pcr;

    dts_entry->passed = true;
    es->last_pcr_event = pcr_event_NextEntry(&pcr_sync->pcr_events, pcr_event);
    if (--pcr_event->entries_left != 0)
        goto no_pcr;

return_pcr:
    heimdall_list_remove(&pcr_event->node);
    pcr_event_Delete(pcr_event);

    heimdall_mutex_unlock(&pcr_sync->lock);
    return pcr;

no_pcr:
    heimdall_mutex_unlock(&pcr_sync->lock);
    return HEIMDALL_TICK_INVALID;
}
