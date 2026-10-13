/*****************************************************************************
 * pcr_helper.c:
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

#include "pcr_helper.h"

#include <heimdall_list.h>
#include <heimdall_tick.h>

struct transcode_track_pcr_helper
{
    heimdall_tick_t max_delay;
    /// Represents the media time held by the frame processing unit.
    heimdall_tick_t held_media_time;
    heimdall_tick_t input_media_time;
    heimdall_tick_t last_dts_output;

    struct heimdall_list delayed_frames_data;

    heimdall_pcr_sync_t *sync_ref;
    unsigned int pcr_sync_es_id;
};

typedef struct
{
    heimdall_tick_t length;
    heimdall_tick_t dts;
    heimdall_tick_t media_time;
    struct heimdall_list node;
} delayed_frame_data_t;

transcode_track_pcr_helper_t *transcode_track_pcr_helper_New(heimdall_pcr_sync_t *sync_ref,
                                                             heimdall_tick_t max_delay)
{
    transcode_track_pcr_helper_t *ret = malloc(sizeof(*ret));
    if (unlikely(ret == NULL))
        return NULL;

    if (heimdall_pcr_sync_NewESID(sync_ref, &ret->pcr_sync_es_id) != HEIMDALL_SUCCESS)
    {
        free(ret);
        return NULL;
    }

    ret->max_delay = max_delay;
    ret->held_media_time = 0;
    ret->input_media_time = 0;
    ret->last_dts_output = HEIMDALL_TICK_INVALID;

    heimdall_list_init(&ret->delayed_frames_data);
    ret->sync_ref = sync_ref;

    return ret;
}

void transcode_track_pcr_helper_Delete(transcode_track_pcr_helper_t *pcr_helper)
{
    delayed_frame_data_t *it;
    heimdall_list_foreach(it, &pcr_helper->delayed_frames_data, node)
    {
        heimdall_list_remove(&it->node);
        free(it);
    }

    heimdall_pcr_sync_DelESID(pcr_helper->sync_ref, pcr_helper->pcr_sync_es_id);

    free(pcr_helper);
}

static inline heimdall_tick_t
transcode_track_pcr_helper_GetFramePCR(transcode_track_pcr_helper_t *pcr_helper,
                                       heimdall_tick_t frame_dts)
{
    heimdall_tick_t pcr = HEIMDALL_TICK_INVALID;
    heimdall_tick_t it = HEIMDALL_TICK_INVALID;

    // XXX: `heimdall_pcr_sync_SignalFrameOutput` only needs DTS for now. Passing a frame by only filling
    // the DTS is enough.
    const heimdall_frame_t fake_frame = {.i_dts = frame_dts};

    while ((it = heimdall_pcr_sync_SignalFrameOutput(pcr_helper->sync_ref, pcr_helper->pcr_sync_es_id,
                                                &fake_frame)) != HEIMDALL_TICK_INVALID)
    {
        pcr = it;
    }
    return pcr;
}

int transcode_track_pcr_helper_SignalEnteringFrame(transcode_track_pcr_helper_t *pcr_helper,
                                                   const heimdall_frame_t *frame,
                                                   heimdall_tick_t *dropped_frame_ts)
{
    delayed_frame_data_t *bdata = malloc(sizeof(*bdata));
    if (unlikely(bdata == NULL))
        return HEIMDALL_ENOMEM;

    pcr_helper->input_media_time += frame->i_length;
    pcr_helper->held_media_time += frame->i_length;

    bdata->length = frame->i_length;
    bdata->dts = frame->i_dts;
    bdata->media_time = pcr_helper->input_media_time;

    heimdall_pcr_sync_SignalFrame(pcr_helper->sync_ref, pcr_helper->pcr_sync_es_id, frame);

    heimdall_list_append(&bdata->node, &pcr_helper->delayed_frames_data);

    // Something went wrong in the delaying unit.
    // Exceeding this limit usually means the frame was dropped. So in our case, act like it went
    // through.
    // TODO needs to be properly unit-tested.
    if (pcr_helper->held_media_time > pcr_helper->max_delay)
    {
        delayed_frame_data_t *first_bdata = heimdall_list_first_entry_or_null(
            &pcr_helper->delayed_frames_data, delayed_frame_data_t, node);
        assert(first_bdata != NULL);

        const heimdall_tick_t pcr = transcode_track_pcr_helper_GetFramePCR(pcr_helper, first_bdata->dts);
        *dropped_frame_ts = pcr_helper->last_dts_output == HEIMDALL_TICK_INVALID
                                ? pcr
                                : __MIN(pcr_helper->last_dts_output, first_bdata->dts);

        pcr_helper->held_media_time -= first_bdata->length;

        heimdall_list_remove(&first_bdata->node);
        free(first_bdata);
    }
    else
    {
        *dropped_frame_ts = HEIMDALL_TICK_INVALID;
    }
    return HEIMDALL_SUCCESS;
}

int transcode_track_pcr_helper_SignalLeavingFrame(transcode_track_pcr_helper_t *pcr_helper,
                                                  const heimdall_frame_t *frame,
                                                  heimdall_tick_t *pcr)
{
    *pcr = HEIMDALL_TICK_INVALID;

    if (heimdall_list_is_empty(&pcr_helper->delayed_frames_data))
        return HEIMDALL_EGENERIC;

    pcr_helper->last_dts_output = frame->i_dts;

    pcr_helper->held_media_time -= frame->i_length;
    const heimdall_tick_t output_media_time =
        pcr_helper->input_media_time - pcr_helper->held_media_time;

    delayed_frame_data_t *frame_data;
    heimdall_list_foreach(frame_data, &pcr_helper->delayed_frames_data, node)
    {
        if (output_media_time < frame_data->media_time)
            break;

        const heimdall_tick_t current_pcr =
            transcode_track_pcr_helper_GetFramePCR(pcr_helper, frame_data->dts);
        if (current_pcr != HEIMDALL_TICK_INVALID)
            *pcr = current_pcr;

        heimdall_list_remove(&frame_data->node);
        free(frame_data);
    }

    if (*pcr != HEIMDALL_TICK_INVALID)
        *pcr = __MIN(frame->i_dts, *pcr);

    return HEIMDALL_SUCCESS;
}
