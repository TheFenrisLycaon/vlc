/*****************************************************************************
 * player.h: Player internal interface
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

#ifndef HEIMDALL_PLAYER_INTERNAL_H
#define HEIMDALL_PLAYER_INTERNAL_H

#include <heimdall_player.h>
#include <heimdall_list.h>
#include <heimdall_vector.h>
#include <heimdall_atomic.h>
#include <heimdall_media_library.h>
#include <heimdall_vout_osd.h>

#include "input/input_internal.h"

struct heimdall_player_track_priv
{
    struct heimdall_player_track t;
    vout_thread_t *vout; /* weak reference */
    heimdall_tick_t delay;
    /* only valid if selected and if category is VIDEO_ES or SPU_ES */
    enum heimdall_vout_order vout_order;
    /* Used to save or not the track selection */
    bool selected_by_user;
};

typedef struct HEIMDALL_VECTOR(struct heimdall_player_program *)
    heimdall_player_program_vector;

typedef struct HEIMDALL_VECTOR(struct heimdall_player_track_priv *)
    heimdall_player_track_vector;

struct heimdall_player_title_list
{
    heimdall_atomic_rc_t rc;
    size_t count;
    struct heimdall_player_title array[];
};

struct heimdall_player_input
{
    input_thread_t *thread;
    heimdall_player_t *player;
    bool started;

    /* Monitor the OPENING_S -> PLAYING_S transition. */
    bool playing;

    enum heimdall_player_state state;
    enum heimdall_player_error error;
    float rate;
    int capabilities;
    heimdall_tick_t length;
    bool live;

    double position;
    heimdall_tick_t time;
    heimdall_tick_t normal_time;

    heimdall_tick_t pause_date;

    bool recording;

    float signal_quality;
    float signal_strength;
    float cache;

    struct input_stats_t stats;

    heimdall_tick_t cat_delays[DATA_ES];

    heimdall_player_program_vector program_vector;
    heimdall_player_track_vector video_track_vector;
    heimdall_player_track_vector audio_track_vector;
    heimdall_player_track_vector spu_track_vector;
    const struct heimdall_player_track_priv *teletext_source;

    struct heimdall_player_title_list *titles;

    size_t title_selected;
    size_t chapter_selected;

    struct heimdall_list node;

    bool teletext_enabled;
    bool teletext_transparent;
    unsigned teletext_page;

    struct
    {
        heimdall_tick_t time;
        double pos;
        bool set;
    } abloop_state[2];

    struct
    {
        heimdall_ml_playback_states_all states;
        enum
        {
            HEIMDALL_RESTOREPOINT_TITLE,
            HEIMDALL_RESTOREPOINT_POSITION,
            HEIMDALL_RESTOREPOINT_NONE,
        } restore;
        float pos;
        bool restore_states;
        bool delay_restore;
        /* Keep a trace of tracks as they appear since they won't be available
         * for probing when the input gets stopped
         */
        bool has_video_tracks;
        bool has_audio_tracks;
    } ml;

    unsigned repeat;
};

struct heimdall_player_listener_id
{
    const struct heimdall_player_cbs *cbs;
    void *cbs_data;
    struct heimdall_list node;
};

struct heimdall_player_metadata_listener_id
{
    const union heimdall_player_metadata_cbs *cbs;
    void *cbs_data;

    enum heimdall_player_metadata_option option;
    union
    {
        heimdall_audio_meter_plugin *audio_meter;
    };
    struct heimdall_list node;
};

struct heimdall_player_vout_listener_id
{
    const struct heimdall_player_vout_cbs *cbs;
    void *cbs_data;
    struct heimdall_list node;
};

struct heimdall_player_aout_listener_id
{
    const struct heimdall_player_aout_cbs *cbs;
    void *cbs_data;
    struct heimdall_list node;
};

enum heimdall_player_timer_source_type
{
    HEIMDALL_PLAYER_TIMER_TYPE_BEST,
    HEIMDALL_PLAYER_TIMER_TYPE_SMPTE,
    HEIMDALL_PLAYER_TIMER_TYPE_COUNT
};

struct heimdall_player_timer_id
{
    heimdall_tick_t period;
    heimdall_tick_t last_update_date;

    union
    {
        const struct heimdall_player_timer_cbs *cbs;
        const struct heimdall_player_timer_smpte_cbs *smpte_cbs;
    };
    void *data;

    struct heimdall_list node;
};

struct heimdall_player_timer_source
{
    struct heimdall_list listeners; /* list of struct heimdall_player_timer_id */
    heimdall_es_id_t *es; /* weak reference */
    struct heimdall_player_timer_point point;
    bool seeking;

    /* Date of the last reported pause, to report a pause signaled by both
     * the input and the output only once */
    heimdall_tick_t reported_pause_date;

    union
    {
        struct {
            unsigned long last_framenum;
            unsigned frame_rate;
            unsigned frame_rate_base;
            unsigned frame_resolution;
            unsigned df_fps;
            int df;
            int frames_per_10mins;
        } smpte;
    };
};

enum heimdall_player_timer_event
{
    HEIMDALL_PLAYER_TIMER_EVENT_PLAYING,
    HEIMDALL_PLAYER_TIMER_EVENT_PAUSED,
    HEIMDALL_PLAYER_TIMER_EVENT_DISCONTINUITY,
    HEIMDALL_PLAYER_TIMER_EVENT_STOPPING,
};

struct heimdall_player_timer
{
    heimdall_mutex_t lock;

    bool input_live;
    heimdall_tick_t input_length;
    heimdall_tick_t input_normal_time;
    heimdall_tick_t last_ts;
    heimdall_tick_t start_offset;
    double input_position;

    heimdall_tick_t seek_ts;
    double seek_position;
    enum
    {
        UPDATE_STATE_RESUMED,
        UPDATE_STATE_PAUSED,
        UPDATE_STATE_RESUMING,
    } update_state;
    heimdall_tick_t pause_date;
    /* Date of the last input PLAYING event, to detect ES PAUSED events
     * delivered after their pause was resumed */
    heimdall_tick_t last_resume_date;
    bool stopping;

    struct heimdall_player_timer_source sources[HEIMDALL_PLAYER_TIMER_TYPE_COUNT];
#define best_source sources[HEIMDALL_PLAYER_TIMER_TYPE_BEST]
#define smpte_source sources[HEIMDALL_PLAYER_TIMER_TYPE_SMPTE]
};

struct heimdall_player_t
{
    struct heimdall_object_t obj;
    heimdall_mutex_t lock;
    heimdall_mutex_t metadata_listeners_lock;
    heimdall_mutex_t aout_listeners_lock;
    heimdall_mutex_t vout_listeners_lock;
    heimdall_cond_t start_delay_cond;

    bool start_paused;
    bool play_and_pause;

    bool pause_on_cork;
    bool corked;

    struct heimdall_list listeners;
    struct heimdall_list metadata_listeners;
    struct heimdall_list aout_listeners;
    struct heimdall_list vout_listeners;

    input_resource_t *resource;
    heimdall_renderer_item_t *renderer;

    input_item_t *media;
    struct heimdall_player_input *input;

    bool releasing_media;
    input_item_t *next_media;

    char *video_string_ids;
    char *audio_string_ids;
    char *sub_string_ids;

    enum heimdall_player_state global_state;
    bool started;

    /**
     * Playing a tiny stream (either empty, or with unreported errors) in a loop
     * would cause high CPU usage. To mitigate the problem, temporize if
     * several EOS are received too quickly.
     */
#define HEIMDALL_PLAYER_EOS_BURST_THRESHOLD HEIMDALL_TICK_FROM_MS(250)
    heimdall_tick_t last_eos;
    unsigned eos_burst_count;
    unsigned repeat;

    bool deleting;
    struct
    {
        heimdall_thread_t thread;
        heimdall_cond_t wait;
        heimdall_cond_t notify;
        struct heimdall_list stop_inputs;
        struct heimdall_list stopping_inputs;
        struct heimdall_list joinable_inputs;
    } mainloop;

    struct heimdall_player_timer timer;
};

#ifndef NDEBUG
/*
 * Assert that the player mutex is locked.
 *
 * This is exposed in this internal header because the playlist and its
 * associated player share the lock to avoid lock-order inversion issues.
 */
static inline void
heimdall_player_assert_locked(heimdall_player_t *player)
{
    assert(player);
    heimdall_mutex_assert(&player->lock);
}
#else
#define heimdall_player_assert_locked(x) ((void) (0))
#endif

static inline struct heimdall_player_input *
heimdall_player_get_input_locked(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);
    return player->input;
}

#define heimdall_player_SendEvent(player, event, ...) do { \
    heimdall_player_listener_id *listener; \
    heimdall_list_foreach(listener, &player->listeners, node) \
    { \
        if (listener->cbs->event) \
            listener->cbs->event(player, ##__VA_ARGS__, listener->cbs_data); \
    } \
} while(0)

#define heimdall_player_SendEventCount(player, event, count, ...) do { \
    heimdall_player_listener_id *listener; \
    count = 0; \
    heimdall_list_foreach(listener, &player->listeners, node) \
    { \
        if (listener->cbs->event) { \
            listener->cbs->event(player, ##__VA_ARGS__, listener->cbs_data); \
            count++; \
        } \
    } \
} while(0)

static inline const char *
es_format_category_to_string(enum es_format_category_e cat)
{
    switch (cat)
    {
        case VIDEO_ES: return "Video";
        case AUDIO_ES: return "Audio";
        case SPU_ES: return "Subtitle";
        default: return NULL;
    }
}

/*
 * player.c
 */

heimdall_object_t *
heimdall_player_GetObject(heimdall_player_t *player);

int
heimdall_player_OpenNextMedia(heimdall_player_t *player);

void
heimdall_player_destructor_AddStoppingInput(heimdall_player_t *player,
                                       struct heimdall_player_input *input);

void
heimdall_player_destructor_AddJoinableInput(heimdall_player_t *player,
                                       struct heimdall_player_input *input);

void
heimdall_player_SignalAtoBLoop(heimdall_player_t *player);

/*
 * player_track.c
 */

struct heimdall_player_program *
heimdall_player_program_New(int id, const char *name);

int
heimdall_player_program_Update(struct heimdall_player_program *prgm, int id,
                          const char *name);

struct heimdall_player_program *
heimdall_player_program_vector_FindById(heimdall_player_program_vector *vec, int id,
                                   size_t *idx);

struct heimdall_player_track_priv *
heimdall_player_track_priv_New(heimdall_es_id_t *id, const char *name, const es_format_t *fmt);

void
heimdall_player_track_priv_Delete(struct heimdall_player_track_priv *trackpriv);

int
heimdall_player_track_priv_Update(struct heimdall_player_track_priv *trackpriv,
                             const char *name, const es_format_t *fmt);

struct heimdall_player_track_priv *
heimdall_player_track_vector_FindById(heimdall_player_track_vector *vec, heimdall_es_id_t *id,
                                 size_t *idx);

int
heimdall_player_GetFirstSelectedTrackId(const heimdall_player_track_vector* tracks);

/*
 * player_title.c
 */

struct heimdall_player_title_list *
heimdall_player_title_list_Create(input_title_t *const *array, size_t count,
                             int title_offset, int chapter_offset);

/*
 * player_input.c
 */

static inline heimdall_player_track_vector *
heimdall_player_input_GetTrackVector(struct heimdall_player_input *input,
                                enum es_format_category_e cat)
{
    switch (cat)
    {
        case VIDEO_ES:
            return &input->video_track_vector;
        case AUDIO_ES:
            return &input->audio_track_vector;
        case SPU_ES:
            return &input->spu_track_vector;
        default:
            return NULL;
    }
}

struct heimdall_player_track_priv *
heimdall_player_input_FindTrackById(struct heimdall_player_input *input, heimdall_es_id_t *id,
                               size_t *idx);

struct heimdall_player_input *
heimdall_player_input_New(heimdall_player_t *player, input_item_t *item);

void
heimdall_player_input_Delete(struct heimdall_player_input *input);

void
heimdall_player_input_SelectTracksByStringIds(struct heimdall_player_input *input,
                                         enum es_format_category_e cat,
                                         const char *str_ids);

char *
heimdall_player_input_GetSelectedTrackStringIds(struct heimdall_player_input *input,
                                           enum es_format_category_e cat) HEIMDALL_MALLOC;

heimdall_tick_t
heimdall_player_input_GetTime(struct heimdall_player_input *input, bool seeking,
                         heimdall_tick_t system_now);

double
heimdall_player_input_GetPos(struct heimdall_player_input *input, bool seeking,
                        heimdall_tick_t system_now);

int
heimdall_player_input_Start(struct heimdall_player_input *input);

void
heimdall_player_input_SeekByPos(struct heimdall_player_input *input, double position,
                           enum heimdall_player_seek_speed speed,
                           enum heimdall_player_whence whence);

void
heimdall_player_input_SeekByTime(struct heimdall_player_input *input, heimdall_tick_t time,
                            enum heimdall_player_seek_speed speed,
                            enum heimdall_player_whence whence);

void
heimdall_player_input_UpdateViewpoint(struct heimdall_player_input *input,
                                 const heimdall_viewpoint_t *viewpoint,
                                 enum heimdall_player_whence whence);

void
heimdall_player_input_HandleState(struct heimdall_player_input *, enum heimdall_player_state,
                             heimdall_tick_t state_date);

bool
heimdall_player_input_HandleAtoBLoop(struct heimdall_player_input *input, bool forced);

/*
 * player_timer.c
*/

void
heimdall_player_InitTimer(heimdall_player_t *player);

void
heimdall_player_DestroyTimer(heimdall_player_t *player);

void
heimdall_player_ResetTimer(heimdall_player_t *player);

void
heimdall_player_UpdateTimerEvent(heimdall_player_t *player, heimdall_es_id_t *es_source,
                            enum heimdall_player_timer_event event,
                            heimdall_tick_t system_date);

void
heimdall_player_UpdateTimerSeekState(heimdall_player_t *player, heimdall_tick_t time,
                                double position);

void
heimdall_player_UpdateTimer(heimdall_player_t *player, heimdall_es_id_t *es_source,
                       bool es_source_is_master,
                       const struct heimdall_player_timer_point *point,
                       heimdall_tick_t normal_time,
                       unsigned frame_rate, unsigned frame_rate_base,
                       heimdall_tick_t start_offset);

void
heimdall_player_RemoveTimerSource(heimdall_player_t *player, heimdall_es_id_t *es_source);

int
heimdall_player_GetTimerPoint(heimdall_player_t *player, bool *seeking,
                         heimdall_tick_t system_now,
                         heimdall_tick_t *out_ts, double *out_pos);

/*
 * player_vout.c
 */

void
heimdall_player_vout_AddCallbacks(heimdall_player_t *player, vout_thread_t *vout);

void
heimdall_player_vout_DelCallbacks(heimdall_player_t *player, vout_thread_t *vout);

/*
 * player_aout.c
 */

audio_output_t *
heimdall_player_aout_Init(heimdall_player_t *player);

void
heimdall_player_aout_Deinit(heimdall_player_t *player);

/*
 * player_osd.c
 */

void
heimdall_player_osd_Message(heimdall_player_t *player, const char *fmt, ...);

void
heimdall_player_osd_Icon(heimdall_player_t *player, heimdall_osd_widget_type type);

void
heimdall_player_osd_Position(heimdall_player_t *player,
                        struct heimdall_player_input *input, heimdall_tick_t time,
                        double position);
void
heimdall_player_osd_Volume(heimdall_player_t *player, bool mute_action);

int
heimdall_player_vout_OSDCallback(heimdall_object_t *this, const char *var,
                            heimdall_value_t oldval, heimdall_value_t newval, void *data);

void
heimdall_player_osd_Track(heimdall_player_t *player, heimdall_es_id_t *id, bool select);

void
heimdall_player_osd_Tracks(heimdall_player_t *player, heimdall_es_id_t * const*selected, heimdall_es_id_t *unselect);

void
heimdall_player_osd_Program(heimdall_player_t *player, const char *name);

/*
 * player/medialib.c
 */

void
heimdall_player_input_RestoreMlStates(struct heimdall_player_input* input, bool force_pos);

void
heimdall_player_UpdateMLStates(heimdall_player_t *player, struct heimdall_player_input* input);

#endif
