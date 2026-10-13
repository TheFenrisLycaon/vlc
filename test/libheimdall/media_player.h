/*
 * media_player.h - media player test common definitions
 *
 */

/**********************************************************************
 *  Copyright (C) 2023 VideoLAN and its authors                       *
 *                                                                    *
 *  This program is free software; you can redistribute and/or modify *
 *  it under the terms of the GNU General Public License as published *
 *  by the Free Software Foundation; version 2 of the license, or (at *
 *  your option) any later version.                                   *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *  See the GNU General Public License for more details.              *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, you can get it from:             *
 *  http://www.gnu.org/copyleft/gpl.html                              *
 *  Copyright (C) 2026 TheFenrisLycaon
 **********************************************************************/

#ifndef TEST_MEDIA_PLAYER_H
#define TEST_MEDIA_PLAYER_H

#include <heimdall_list.h>

struct mp_event
{
    struct heimdall_list node;
    enum {
        EVENT_TYPE_STATE,
        EVENT_TYPE_TRACK_LIST,
        EVENT_TYPE_TRACK_SELECTION,
        EVENT_TYPE_PROGRAM_LIST,
        EVENT_TYPE_PROGRAM_SELECTION,
        EVENT_TYPE_RECORDING_CHANGED,
    } type;
    union {
        libheimdall_state_t state;
        struct {
            libheimdall_list_action_t action;
            libheimdall_track_type_t type;
            char *id;
        } track_list;
        struct {
            libheimdall_track_type_t type;
            char *unselected_id;
            char *selected_id;
        } track_selection;
        struct {
            libheimdall_list_action_t action;
            int group_id;
        } program_list;
        struct {
            int unselected_group_id;
            int selected_group_id;
        } program_selection;
        struct {
            bool recording;
            char *file_path;
        } recording_changed;
    };
};

struct mp_event_ctx
{
    libheimdall_media_player_t *mp;

    struct heimdall_list events;
};

static inline void mp_event_delete(struct mp_event *ev)
{
    switch (ev->type)
    {
        case EVENT_TYPE_TRACK_LIST:
            free(ev->track_list.id);
            break;
        case EVENT_TYPE_TRACK_SELECTION:
            free(ev->track_selection.unselected_id);
            free(ev->track_selection.selected_id);
            break;
        case EVENT_TYPE_RECORDING_CHANGED:
            free(ev->recording_changed.file_path);
        default: break;
    }
    free(ev);
}

static inline void mp_event_ctx_init(struct mp_event_ctx *ctx)
{
    ctx->mp = NULL;
    heimdall_list_init(&ctx->events);
}

static inline void mp_event_ctx_set_mp(struct mp_event_ctx *ctx,
                                       libheimdall_media_player_t *mp)
{
    ctx->mp = mp;
}

static inline void mp_event_ctx_destroy(struct mp_event_ctx *ctx)
{
    struct mp_event *ev;
    heimdall_list_foreach(ev, &ctx->events, node)
        mp_event_delete(ev);
}

static inline struct mp_event *mp_event_ctx_wait_event(struct mp_event_ctx *ctx)
{
    assert(ctx->mp != NULL);
    libheimdall_media_player_lock(ctx->mp);
    struct mp_event *event;

    while ((event = heimdall_list_first_entry_or_null(&ctx->events, struct mp_event,
                                                 node)) == NULL)
        libheimdall_media_player_wait(ctx->mp);

    heimdall_list_remove(&event->node);
    libheimdall_media_player_unlock(ctx->mp);

    return event;
}

static inline void mp_event_ctx_push_event(struct mp_event_ctx *ctx,
                                           const struct mp_event *ev)
{
    struct mp_event *dup = malloc(sizeof *dup);
    assert(dup != NULL);
    *dup = *ev;

    /* The lock is already held from the event, but it's a good occasion to
     * test recursive locks */
    assert(ctx->mp != NULL);
    libheimdall_media_player_lock(ctx->mp);
    heimdall_list_append(&dup->node, &ctx->events);
    libheimdall_media_player_signal(ctx->mp);
    libheimdall_media_player_unlock(ctx->mp);
}

static inline void mp_event_ctx_wait_state(struct mp_event_ctx *ctx,
                                           libheimdall_state_t state)
{
    for (;;)
    {
        struct mp_event *ev = mp_event_ctx_wait_event(ctx);
        if (ev->type == EVENT_TYPE_STATE && ev->state == state)
        {
            mp_event_delete(ev);
            break;
        }
        else
            mp_event_delete(ev);
    }
}

static inline void mp_on_state_changed(void *opaque, libheimdall_state_t state)
{
    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_STATE,
        .state = state,
    });
}

static inline void mp_on_track_list_changed(void *opaque, libheimdall_list_action_t action,
                                            libheimdall_track_type_t type, const char *id)
{
    char *id_dup = strdup(id);
    assert(id_dup);

    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_TRACK_LIST,
        .track_list.action = action,
        .track_list.type = type,
        .track_list.id = id_dup,
    });
}

static inline void mp_on_track_selection_changed(void *opaque, libheimdall_track_type_t type,
                                                 const char *unselected_id,
                                                 const char *selected_id)
{
    char *unselected_id_dup = NULL;
    if (unselected_id != NULL)
    {
        unselected_id_dup = strdup(unselected_id);
        assert(unselected_id_dup);
    }
    char *selected_id_dup = NULL;
    if (selected_id != NULL)
    {
        selected_id_dup = strdup(selected_id);
        assert(selected_id_dup);
    }
    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_TRACK_SELECTION,
        .track_selection.type = type,
        .track_selection.unselected_id = unselected_id_dup,
        .track_selection.selected_id = selected_id_dup,
    });
}

static inline void mp_on_program_list_changed(void *opaque,
                                              libheimdall_list_action_t action,
                                              int group_id)
{
    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_PROGRAM_LIST,
        .program_list.action = action,
        .program_list.group_id = group_id,
    });
}

static inline void mp_on_program_selection_changed(void *opaque,
                                                   int unselected_group_id,
                                                   int selected_group_id)
{
    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_PROGRAM_SELECTION,
        .program_selection.unselected_group_id = unselected_group_id,
        .program_selection.selected_group_id = selected_group_id,
    });
}

static inline void mp_on_recording_changed(void *opaque, bool recording,
                                           const char *file_path)
{

    char *file_path_dup;
    if (file_path != NULL)
    {
        file_path_dup = strdup(file_path);
        assert(file_path_dup != NULL);
    }
    else
        file_path_dup = NULL;

    mp_event_ctx_push_event(opaque, &(struct mp_event) {
        .type = EVENT_TYPE_RECORDING_CHANGED,
        .recording_changed.recording = recording,
        .recording_changed.file_path = file_path_dup,
    });
}

#endif /* TEST_MEDIA_PLAYER_H */
