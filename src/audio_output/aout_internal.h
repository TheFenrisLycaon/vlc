/*****************************************************************************
 * aout_internal.h : internal defines for audio output
 *****************************************************************************
 * Copyright (C) 2002 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef LIBHEIMDALL_AOUT_INTERNAL_H
# define LIBHEIMDALL_AOUT_INTERNAL_H 1

# include <stdatomic.h>

# include <heimdall_atomic.h>
# include <heimdall_filter.h>
# include <heimdall_list.h>
# include <heimdall_viewpoint.h>
# include "../clock/clock.h"

/* Max input rate factor (1/4 -> 4) */
# define AOUT_MAX_INPUT_RATE (4)

#define AOUT_RESTART_FILTERS        0x1
#define AOUT_RESTART_OUTPUT         (AOUT_RESTART_FILTERS|0x2)
#define AOUT_RESTART_OUTPUT_DEC     (AOUT_RESTART_OUTPUT|0x4)

enum {
    AOUT_RESAMPLING_NONE=0,
    AOUT_RESAMPLING_UP,
    AOUT_RESAMPLING_DOWN
};

typedef struct aout_volume aout_volume_t;
typedef struct heimdall_aout_stream heimdall_aout_stream;

typedef struct
{
    heimdall_mutex_t lock;
    module_t *module; /**< Output plugin (or NULL if inactive) */
    bool bitexact;

    heimdall_aout_stream *main_stream;

    struct
    {
        heimdall_mutex_t lock;
        struct heimdall_list list;
        unsigned count;
    } dev;

    struct
    {
        atomic_bool update;
        heimdall_mutex_t lock;
        heimdall_viewpoint_t value;
    } vp;

    int requested_stereo_mode; /**< Requested stereo mode set by the user */
    int requested_mix_mode; /**< Requested mix mode set by the user */

    struct heimdall_audio_meter meter;

    heimdall_atomic_rc_t rc;
} aout_owner_t;

typedef struct
{
    audio_output_t output;
    aout_owner_t   owner;
} aout_instance_t;

static inline aout_instance_t *aout_instance (audio_output_t *aout)
{
    return container_of(aout, aout_instance_t, output);
}

static inline aout_owner_t *aout_owner (audio_output_t *aout)
{
    aout_instance_t *instance = aout_instance(aout);
    return &instance->owner;
}

/****************************************************************************
 * Prototypes
 *****************************************************************************/

/* From mixer.c : */
aout_volume_t *aout_volume_New(heimdall_object_t *, const audio_replay_gain_t *);
#define aout_volume_New(o, g) aout_volume_New(HEIMDALL_OBJECT(o), g)
int aout_volume_SetFormat(aout_volume_t *, heimdall_fourcc_t);
void aout_volume_SetVolume(aout_volume_t *, float);
int aout_volume_Amplify(aout_volume_t *, block_t *);
void aout_volume_Delete(aout_volume_t *);


/* From output.c : */
audio_output_t *aout_New (heimdall_object_t *);
#define aout_New(a) aout_New(HEIMDALL_OBJECT(a))

/**
 * Starts an audio output stream.
 *
 * \param aout the audio output instance to initialize from
 * \param stream the audio output stream to initialize from
 * \param fmt the format to request to the output
 * \param input_profile the audio profile to request from the audio output
 * \param filter_fmt the format requested from the filters
 * \param filters_cfg the configuration to load the audio filters from
 *
 * \warning The caller must NOT hold the audio output lock.
 */
int aout_OutputNew(audio_output_t *aout, heimdall_aout_stream *stream,
                   audio_sample_format_t *fmt, int input_profile,
                   audio_sample_format_t *filter_fmt,
                   aout_filters_cfg_t *filters_cfg);
/**
 * Stops the audio output stream (undoes aout_OutputNew()).
 * \note This can only be called after a successful aout_OutputNew().
 * \warning The caller must NOT hold the audio output lock.
 */
void aout_OutputDelete( audio_output_t * p_aout );

heimdall_audio_meter_plugin *
aout_AddMeterPlugin(audio_output_t *aout, const char *chain,
                    const struct heimdall_audio_meter_plugin_owner *owner);

void
aout_RemoveMeterPlugin(audio_output_t *aout, heimdall_audio_meter_plugin *plugin);

/* From common.c : */
void aout_FormatsPrint(heimdall_object_t *, const char *,
                       const audio_sample_format_t *,
                       const audio_sample_format_t *);
#define aout_FormatsPrint(o, t, a, b) \
        aout_FormatsPrint(HEIMDALL_OBJECT(o), t, a, b)

/* From dec.c */
#define AOUT_DEC_SUCCESS 0
#define AOUT_DEC_CHANGED 1
#define AOUT_DEC_FAILED HEIMDALL_EGENERIC

struct heimdall_aout_stream_cfg
{
    const audio_sample_format_t *fmt;
    int profile;
    struct heimdall_clock_t *clock;
    const char *str_id;
    const audio_replay_gain_t *replay_gain;
};

heimdall_aout_stream *heimdall_aout_stream_New(audio_output_t *p_aout,
                                     const struct heimdall_aout_stream_cfg *cfg);
void heimdall_aout_stream_Delete(heimdall_aout_stream *);
int heimdall_aout_stream_Play(heimdall_aout_stream *stream, block_t *block);
void heimdall_aout_stream_GetResetStats(heimdall_aout_stream *stream, unsigned *, unsigned *);
void heimdall_aout_stream_ChangePause(heimdall_aout_stream *stream, bool b_paused, heimdall_tick_t i_date);
void heimdall_aout_stream_ChangeRate(heimdall_aout_stream *stream, float rate);
void heimdall_aout_stream_ChangeDelay(heimdall_aout_stream *stream, heimdall_tick_t delay);
void heimdall_aout_stream_Flush(heimdall_aout_stream *stream);
void heimdall_aout_stream_Drain(heimdall_aout_stream *stream);
/* Contrary to other heimdall_aout_stream_*() functions, this function can be called from
 * any threads */
bool heimdall_aout_stream_IsDrained(heimdall_aout_stream *stream);
/* Called from output.c */
void heimdall_aout_stream_NotifyTiming(heimdall_aout_stream *stream, heimdall_tick_t system_ts,
                                  heimdall_tick_t audio_ts);
void heimdall_aout_stream_NotifyDrained(heimdall_aout_stream *stream);
void heimdall_aout_stream_NotifyGain(heimdall_aout_stream *stream, float gain);

void heimdall_aout_stream_RequestRestart(heimdall_aout_stream *stream, unsigned);
void heimdall_aout_stream_RequestRetiming(heimdall_aout_stream *stream, heimdall_tick_t system_ts,
                                     heimdall_tick_t audio_ts);

void aout_InputRequestRestart(audio_output_t *aout);

static inline void aout_SetWavePhysicalChannels(audio_sample_format_t *fmt)
{
    static const uint32_t wave_channels[] = {
        AOUT_CHAN_LEFT, AOUT_CHAN_RIGHT, AOUT_CHAN_CENTER,
        AOUT_CHAN_LFE, AOUT_CHAN_REARLEFT, AOUT_CHAN_REARRIGHT,
        AOUT_CHAN_MIDDLELEFT, AOUT_CHAN_MIDDLERIGHT, AOUT_CHAN_REARCENTER };

    fmt->i_physical_channels = 0;
    for (int i = 0; i < fmt->i_channels && i < AOUT_CHAN_MAX; ++i)
        fmt->i_physical_channels |= wave_channels[i];
    aout_FormatPrepare(fmt);
}

/* From filters.c */

/* Extended version of aout_FiltersNew
 *
 * The clock, that is not mandatory, will be used to create a new slave clock
 * for the filter visualization plugins.
 */
aout_filters_t *aout_FiltersNewWithClock(heimdall_object_t *, heimdall_clock_t *,
                                         const audio_sample_format_t *,
                                         const audio_sample_format_t *,
                                         const aout_filters_cfg_t *cfg) HEIMDALL_USED;
void aout_FiltersResetClock(aout_filters_t *filters);
void aout_FiltersSetClockDelay(aout_filters_t *filters, heimdall_tick_t delay);
bool aout_FiltersCanResample (aout_filters_t *filters);
filter_t *aout_filter_Create(heimdall_object_t *obj, const filter_owner_t *restrict owner,
                             const char *type, const char *name,
                             const audio_sample_format_t *infmt,
                             const audio_sample_format_t *outfmt,
                             config_chain_t *cfg, bool const_fmt);

#endif /* !LIBHEIMDALL_AOUT_INTERNAL_H */
