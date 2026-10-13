/*****************************************************************************
 * hotkeys.c: Hotkey handling for heimdall
 *****************************************************************************
 * Copyright (C) 2005-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x.nl>
 *          Victorien Le Couviour--Tuffet <victorien.lecouviour.tuffet@gmail.com>
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

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_configuration.h>
#include <heimdall_plugin.h>
#include <heimdall_interface.h>
#include <heimdall_input_item.h>
#include <heimdall_mouse.h>
#include <heimdall_viewpoint.h>
#include <heimdall_player.h>
#include <heimdall_playlist.h>
#include <heimdall_actions.h>
#include <heimdall_spu.h>
#include <heimdall_executor.h>
#include <heimdall_vout_osd.h>
#include "math.h"

struct intf_sys_t
{
    heimdall_playlist_t *playlist;
    heimdall_player_listener_id *player_listener;
    heimdall_array_t vouts;
    struct
    {
        bool btn_pressed;
        int x, y;
    } vrnav;

    struct
    {
        heimdall_tick_t audio_time;
        heimdall_tick_t subtitle_time;
    } subsync;
    enum heimdall_vout_order spu_channel_order;

    heimdall_executor_t *executor;
};

static void handle_action(intf_thread_t *, heimdall_action_id_t);

/*****************************
 * interface action handling *
 *****************************/

#define INTF_ACTION_HANDLER(name) \
static inline void \
action_handler_Intf##name(intf_thread_t *intf, heimdall_action_id_t action_id)

INTF_ACTION_HANDLER()
{
    char const *varname;
    switch (action_id)
    {
        case ACTIONID_QUIT:
            libheimdall_Quit(heimdall_object_instance(intf));
            return;
        case ACTIONID_INTF_TOGGLE_FSC:
        case ACTIONID_INTF_HIDE:
            varname = "intf-toggle-fscontrol";
            break;
        case ACTIONID_INTF_BOSS:
            varname = "intf-boss";
            break;
        case ACTIONID_INTF_POPUP_MENU:
            varname = "intf-popupmenu";
            break;
        default:
            heimdall_assert_unreachable();
    }
    var_TriggerCallback(heimdall_object_instance(intf), varname);
}

INTF_ACTION_HANDLER(ActionCombo)
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer(intf->p_sys->playlist);
    vout_thread_t *vout = heimdall_player_vout_Hold(player);
    bool vrnav = var_GetBool(vout, "viewpoint-changeable");
    vout_Release(vout);
    switch (action_id)
    {
        case ACTIONID_COMBO_VOL_FOV_DOWN:
            action_id = !vrnav
                ? ACTIONID_VOL_DOWN
                : ACTIONID_VIEWPOINT_FOV_OUT;
            break;
        case ACTIONID_COMBO_VOL_FOV_UP:
            action_id = !vrnav
                ? ACTIONID_VOL_UP
                : ACTIONID_VIEWPOINT_FOV_IN;
            break;
        default:
            heimdall_assert_unreachable();
    }
    handle_action(intf, action_id);
}

/****************************
 * playlist action handling *
 ****************************/

#define PLAYLIST_ACTION_HANDLER(name) \
static inline void \
action_handler_Playlist##name(intf_thread_t *intf, heimdall_playlist_t *playlist, \
                              heimdall_action_id_t action_id)

PLAYLIST_ACTION_HANDLER(Interact)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_PLAY_CLEAR:
            heimdall_playlist_Clear(playlist);
            break;
        case ACTIONID_PREV:
            heimdall_playlist_Prev(playlist);
            break;
        case ACTIONID_NEXT:
            heimdall_playlist_Next(playlist);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

PLAYLIST_ACTION_HANDLER(Playback)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_LOOP:
        {
            enum heimdall_playlist_playback_repeat repeat_mode =
                heimdall_playlist_GetPlaybackRepeat(playlist);
            switch (repeat_mode)
            {
                case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE:
                    repeat_mode = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
                    break;
                case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL:
                    repeat_mode = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
                    break;
                case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT:
                    repeat_mode = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
                    break;
            }
            heimdall_playlist_SetPlaybackRepeat(playlist, repeat_mode);
            break;
        }
        case ACTIONID_RANDOM:
        {
            enum heimdall_playlist_playback_order order_mode =
                heimdall_playlist_GetPlaybackOrder(playlist);
            order_mode =
                order_mode == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL
                    ? HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM
                    : HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;
            heimdall_playlist_SetPlaybackOrder(playlist, order_mode);
            break;
        }
        default:
            heimdall_assert_unreachable();
    }
}

static inline void
playlist_bookmark_Set(intf_thread_t *intf,
                      heimdall_playlist_t *playlist, char *name, int id)
{
    var_Create(intf, name, HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);
    input_item_t *item = heimdall_player_GetCurrentMedia(player);
    if (item)
    {
        char *uri = input_item_GetURI(item);
        config_PutPsz(name, uri);
        msg_Info(intf, "setting playlist bookmark %i to %s", id, uri);
        free(uri);
    }
}

static inline void
playlist_bookmark_Play(intf_thread_t *intf,
                       heimdall_playlist_t *playlist, char *name)
{
    char *bookmark_uri = var_CreateGetString(intf, name);
    size_t count = heimdall_playlist_Count(playlist);
    size_t i;
    for (i = 0; i < count; ++i)
    {
        heimdall_playlist_item_t *plitem = heimdall_playlist_Get(playlist, i);
        input_item_t *item = heimdall_playlist_item_GetMedia(plitem);
        char *item_uri = input_item_GetURI(item);
        if (!strcmp(bookmark_uri, item_uri))
            break;
        free(item_uri);
    }
    if (i != count)
        heimdall_playlist_PlayAt(playlist, i);
    free(bookmark_uri);
}

INTF_ACTION_HANDLER(PlaylistBookmark)
{
    bool set = action_id >= ACTIONID_SET_BOOKMARK1 &&
               action_id <= ACTIONID_SET_BOOKMARK10;
    int id = (set ? 1 - ACTIONID_SET_BOOKMARK1 : 1 - ACTIONID_PLAY_BOOKMARK1);
    id += action_id;
    char *bookmark_name;
    if (asprintf(&bookmark_name, "bookmark%i", id) == -1)
        return;
    heimdall_playlist_t *playlist = intf->p_sys->playlist;
    if (set)
        playlist_bookmark_Set(intf, playlist, bookmark_name, id);
    else
        playlist_bookmark_Play(intf, playlist, bookmark_name);
    free(bookmark_name);
}

/**************************
 * player action handling *
 **************************/

#define PLAYER_ACTION_HANDLER(name) \
static inline void action_handler_Player##name(intf_thread_t *intf, \
                                               heimdall_player_t *player, \
                                               heimdall_action_id_t action_id)

PLAYER_ACTION_HANDLER(State)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_PLAY_PAUSE:
        {
            enum heimdall_player_state state = heimdall_player_GetState(player);
            if (state == HEIMDALL_PLAYER_STATE_PAUSED)
                heimdall_player_Resume(player);
            else
                heimdall_player_Pause(player);
            break;
        }
        case ACTIONID_PLAY:
            heimdall_player_Start(player);
            break;
        case ACTIONID_PAUSE:
            heimdall_player_Pause(player);
            break;
        case ACTIONID_STOP:
            heimdall_player_Stop(player);
            break;
        case ACTIONID_FRAME_NEXT:
            heimdall_player_NextVideoFrame(player);
            break;
        case ACTIONID_FRAME_PREVIOUS:
            heimdall_player_PreviousVideoFrame(player);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

INTF_ACTION_HANDLER(PlayerSeek)
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer(intf->p_sys->playlist);
    if (!heimdall_player_CanSeek(player))
        return;
    char const *varname;
    int sign = +1;
    switch (action_id)
    {
        case ACTIONID_JUMP_BACKWARD_EXTRASHORT:
            sign = -1;
            /* fall through */
        case ACTIONID_JUMP_FORWARD_EXTRASHORT:
            varname = "extrashort-jump-size";
            break;
        case ACTIONID_JUMP_BACKWARD_SHORT:
            sign = -1;
            /* fall through */
        case ACTIONID_JUMP_FORWARD_SHORT:
            varname = "short-jump-size";
            break;
        case ACTIONID_JUMP_BACKWARD_MEDIUM:
            sign = -1;
            /* fall through */
        case ACTIONID_JUMP_FORWARD_MEDIUM:
            varname = "medium-jump-size";
            break;
        case ACTIONID_JUMP_BACKWARD_LONG:
            sign = -1;
            /* fall through */
        case ACTIONID_JUMP_FORWARD_LONG:
            varname = "long-jump-size";
            break;
        default:
            heimdall_assert_unreachable();
    }
    int jmpsz = var_InheritInteger(heimdall_object_instance(intf), varname);
    if (jmpsz >= 0)
        heimdall_player_JumpTime(player, heimdall_tick_from_sec(jmpsz * sign));
}

PLAYER_ACTION_HANDLER(Position)
{
    HEIMDALL_UNUSED(action_id); HEIMDALL_UNUSED(intf);
    vout_thread_t *vout = heimdall_player_vout_Hold(player);
    input_item_t* item = heimdall_player_GetCurrentMedia(player);
    if (item && vout_OSDEpg(vout, item))
        heimdall_player_DisplayPosition(player);
    vout_Release(vout);
}

PLAYER_ACTION_HANDLER(NavigateMedia)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_PROGRAM_SID_PREV:
            heimdall_player_SelectPrevProgram(player);
            break;
        case ACTIONID_PROGRAM_SID_NEXT:
            heimdall_player_SelectNextProgram(player);
            break;
        case ACTIONID_TITLE_PREV:
            heimdall_player_SelectPrevTitle(player);
            break;
        case ACTIONID_TITLE_NEXT:
            heimdall_player_SelectNextTitle(player);
            break;
        case ACTIONID_CHAPTER_PREV:
            heimdall_player_SelectPrevChapter(player);
            break;
        case ACTIONID_CHAPTER_NEXT:
            heimdall_player_SelectNextChapter(player);
            break;
        case ACTIONID_DISC_MENU:
            heimdall_player_Navigate(player, HEIMDALL_PLAYER_NAV_MENU);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

PLAYER_ACTION_HANDLER(Track)
{
    switch (action_id)
    {
        case ACTIONID_AUDIO_TRACK:
            heimdall_player_SelectNextTrack(player, AUDIO_ES, HEIMDALL_VOUT_ORDER_PRIMARY);
            break;
        case ACTIONID_SUBTITLE_REVERSE_TRACK:
        case ACTIONID_SUBTITLE_TRACK:
            heimdall_player_CycleTrack(player, SPU_ES, intf->p_sys->spu_channel_order,
                                  action_id == ACTIONID_SUBTITLE_TRACK);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

PLAYER_ACTION_HANDLER(Delay)
{
    intf_sys_t *sys = intf->p_sys;
    enum { AUDIODELAY, SUBDELAY } type;
    int delta = 50;
    switch (action_id)
    {
        case ACTIONID_AUDIODELAY_DOWN:
            delta = -50;
            /* fall-through */
        case ACTIONID_AUDIODELAY_UP:
            type = AUDIODELAY;
            break;
        case ACTIONID_SUBDELAY_DOWN:
            delta = -50;
            /* fall-through */
        case ACTIONID_SUBDELAY_UP:
            type = SUBDELAY;
            break;
        default:
            heimdall_assert_unreachable();
    }
    enum heimdall_player_whence whence = HEIMDALL_PLAYER_WHENCE_RELATIVE;
    delta = HEIMDALL_TICK_FROM_MS(delta);
    if (type == AUDIODELAY)
        heimdall_player_SetAudioDelay(player, delta, whence);
    else
    {
        if (sys->spu_channel_order == HEIMDALL_VOUT_ORDER_PRIMARY)
            heimdall_player_SetSubtitleDelay(player, delta, whence);
        else
        {
            size_t count = heimdall_player_GetTrackCount(player, SPU_ES);
            for (size_t i = 0; i < count; ++i)
            {
                const struct heimdall_player_track *track =
                    heimdall_player_GetTrackAt(player, SPU_ES, i);

                enum heimdall_vout_order order;
                if (heimdall_player_GetEsIdVout(player, track->es_id, &order)
                 && order == HEIMDALL_VOUT_ORDER_SECONDARY)
                {
                    heimdall_player_SetEsIdDelay(player, track->es_id, delta, whence);
                    break;
                }
            }
        }
    }
}

static inline float
AdjustRateFine(float rate, int const dir)
{
    float rate_min = INPUT_RATE_MIN;
    float rate_max = INPUT_RATE_MAX;
    int sign = rate < 0 ? -1 : 1;
    rate = floor(fabs(rate) / 0.1 + dir + 0.05) * 0.1;
    if (rate < rate_min)
       rate = rate_min;
    else if (rate > rate_max)
        rate = rate_max;
    return rate * sign;
}

PLAYER_ACTION_HANDLER(Rate)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_RATE_SLOWER:
            heimdall_player_DecrementRate(player);
            break;
        case ACTIONID_RATE_FASTER:
            heimdall_player_IncrementRate(player);
            break;
        default:
        {
            float rate;
            switch (action_id)
            {
                case ACTIONID_RATE_NORMAL:
                    rate = 1.f;
                    break;
                case ACTIONID_RATE_SLOWER_FINE:
                case ACTIONID_RATE_FASTER_FINE:
                {
                    int const dir = action_id == ACTIONID_RATE_SLOWER_FINE ?
                        -1 : +1;
                    rate = heimdall_player_GetRate(player);
                    rate = AdjustRateFine(rate, dir);
                    break;
                }
                default:
                    heimdall_assert_unreachable();
            }
            heimdall_player_ChangeRate(player, rate);
            break;
        }
    }
}

PLAYER_ACTION_HANDLER(ToggleSubtitle)
{
    HEIMDALL_UNUSED(action_id); HEIMDALL_UNUSED(intf);
    heimdall_player_ToggleSubtitle(player);
}

PLAYER_ACTION_HANDLER(ControlSubtitleSecondary)
{
    HEIMDALL_UNUSED(action_id);
    intf_sys_t *sys = intf->p_sys;

    sys->spu_channel_order =
        sys->spu_channel_order == HEIMDALL_VOUT_ORDER_PRIMARY ?
        HEIMDALL_VOUT_ORDER_SECONDARY : HEIMDALL_VOUT_ORDER_PRIMARY;

    heimdall_player_osd_Message(player, _("%s subtitle control"),
        sys->spu_channel_order == HEIMDALL_VOUT_ORDER_PRIMARY ? "Primary" : "Secondary");
}

PLAYER_ACTION_HANDLER(SyncSubtitle)
{
    intf_sys_t *sys = intf->p_sys;

    switch (action_id)
    {
        case ACTIONID_SUBSYNC_MARKAUDIO:
            sys->subsync.audio_time = heimdall_tick_now();
            heimdall_player_osd_Message(player, _("Sub sync: bookmarked audio time"));
            break;
        case ACTIONID_SUBSYNC_MARKSUB:
            sys->subsync.subtitle_time = heimdall_tick_now();
            heimdall_player_osd_Message(player, _("Sub sync: bookmarked subtitle time"));
            break;
        case ACTIONID_SUBSYNC_APPLY:
        {
            if (sys->subsync.audio_time == HEIMDALL_TICK_INVALID ||
                sys->subsync.subtitle_time == HEIMDALL_TICK_INVALID)
            {
                heimdall_player_osd_Message(player, _("Sub sync: set bookmarks first!"));
                break;
            }
            heimdall_tick_t delay =
                sys->subsync.audio_time - sys->subsync.subtitle_time;
            sys->subsync.audio_time = HEIMDALL_TICK_INVALID;
            sys->subsync.subtitle_time = HEIMDALL_TICK_INVALID;

            heimdall_tick_t previous_delay = heimdall_player_GetSubtitleDelay(player);
            heimdall_player_SetSubtitleDelay(player, delay, HEIMDALL_PLAYER_WHENCE_RELATIVE);

            long long delay_ms = MS_FROM_HEIMDALL_TICK(delay);
            long long totdelay_ms =  MS_FROM_HEIMDALL_TICK(previous_delay + delay);
            heimdall_player_osd_Message(player, _("Sub sync: corrected %"PRId64
                                   " ms (total delay = %"PRId64" ms)"),
                                       delay_ms, totdelay_ms);
            break;
        }
        case ACTIONID_SUBSYNC_RESET:
            sys->subsync.audio_time = HEIMDALL_TICK_INVALID;
            sys->subsync.subtitle_time = HEIMDALL_TICK_INVALID;
            heimdall_player_SetSubtitleDelay(player, 0, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);
            heimdall_player_osd_Message(player, _("Sub sync: delay reset"));
            break;
        default:
            heimdall_assert_unreachable();
    }
}

PLAYER_ACTION_HANDLER(Navigate)
{
    HEIMDALL_UNUSED(intf);
    enum heimdall_player_nav nav;
    switch (action_id)
    {
#define PLAYER_NAV_FROM_ACTION(navval) \
        case ACTIONID_NAV_##navval: \
            nav = HEIMDALL_PLAYER_NAV_##navval; \
            break;
        PLAYER_NAV_FROM_ACTION(ACTIVATE)
        PLAYER_NAV_FROM_ACTION(UP)
        PLAYER_NAV_FROM_ACTION(DOWN)
        PLAYER_NAV_FROM_ACTION(LEFT)
        PLAYER_NAV_FROM_ACTION(RIGHT)
#undef PLAYER_NAV_FROM_ACTION
        default:
            heimdall_assert_unreachable();
    }
    heimdall_player_Navigate(player, nav);
}

PLAYER_ACTION_HANDLER(Viewpoint)
{
    HEIMDALL_UNUSED(intf);
    heimdall_viewpoint_t viewpoint = { .fov = 0.f };
    switch (action_id)
    {
        case ACTIONID_VIEWPOINT_FOV_IN:
            viewpoint.fov = -1.f;
            break;
        case ACTIONID_VIEWPOINT_FOV_OUT:
            viewpoint.fov = +1.f;
            break;
        case ACTIONID_VIEWPOINT_ROLL_CLOCK:
            heimdall_viewpoint_from_euler(&viewpoint,
                                     0.f, 0.f, -1.f);
            break;
        case ACTIONID_VIEWPOINT_ROLL_ANTICLOCK:
            heimdall_viewpoint_from_euler(&viewpoint,
                                     0.f, 0.f, 1.f);
            break;
        default:
            heimdall_assert_unreachable();
    }
    heimdall_player_UpdateViewpoint(player, &viewpoint,
                               HEIMDALL_PLAYER_WHENCE_RELATIVE);
}

PLAYER_ACTION_HANDLER(Record)
{
    HEIMDALL_UNUSED(intf);
    HEIMDALL_UNUSED(action_id);
    heimdall_player_ToggleRecording(player);
}

static int
AudioDeviceCycle(audio_output_t *aout, char **p_name)
{
    int ret = -1;

    char *device = aout_DeviceGet(aout);
    if (!device)
        goto end;
    char **ids, **names;
    int n = aout_DevicesList(aout, &ids, &names);
    if (n == -1)
    {
        free(device);
        goto end;
    }

    int index;
    for (index = 0; index < n; ++index)
        if (!strcmp(ids[index], device))
        {
            index = (index + 1) % n;
            break;
        }
    ret = aout_DeviceSet(aout, ids[index]);
    if (p_name != NULL)
        *p_name = strdup(names[index]);

    free(device);
    for (int i = 0; i < n; ++i)
    {
        free(ids[i]);
        free(names[i]);
    }
    free(ids);
    free(names);
end:
    return ret;
}

PLAYER_ACTION_HANDLER(Aout)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_VOL_DOWN:
            heimdall_player_aout_DecrementVolume(player, 1, NULL);
            break;
        case ACTIONID_VOL_UP:
            heimdall_player_aout_IncrementVolume(player, 1, NULL);
            break;
        case ACTIONID_VOL_MUTE:
            heimdall_player_aout_ToggleMute(player);
            break;
        case ACTIONID_AUDIODEVICE_CYCLE:
        {
            audio_output_t *aout = heimdall_player_aout_Hold(player);
            if (!aout)
                break;
            char *devname;
            if (!AudioDeviceCycle(aout, &devname))
            {
                heimdall_player_osd_Message(player, _("Audio device: %s"), devname);
                free(devname);
            }
            aout_Release(aout);
            break;
        }
        default:
            heimdall_assert_unreachable();
    }
}

struct snapshot_task
{
    struct heimdall_runnable runnable;
    vout_thread_t *vout;
};

static void RunSnapshot(void *userdata)
{
    struct snapshot_task *task = userdata;

    var_TriggerCallback(task->vout, "video-snapshot");

    vout_Release(task->vout);
    free(task);
}

static void TakeSnapshotAsync(struct intf_sys_t *sys, heimdall_player_t *player)
{
    if (!sys->executor)
    {
        /* Create the executor only on the first request */
        sys->executor = heimdall_executor_New(1);
        if (!sys->executor)
            return;
    }

    struct snapshot_task *task = malloc(sizeof(*task));
    if (!task)
        return;

    task->vout = heimdall_player_vout_Hold(player);
    task->runnable.run = RunSnapshot;
    task->runnable.userdata = task;
    heimdall_executor_Submit(sys->executor, &task->runnable);
}

PLAYER_ACTION_HANDLER(Vouts)
{
    HEIMDALL_UNUSED(intf);
    switch (action_id)
    {
        case ACTIONID_TOGGLE_FULLSCREEN:
            heimdall_player_vout_ToggleFullscreen(player);
            break;
        case ACTIONID_LEAVE_FULLSCREEN:
            heimdall_player_vout_SetFullscreen(player, false);
            break;
        case ACTIONID_SNAPSHOT:
            TakeSnapshotAsync(intf->p_sys, player);
            break;
        case ACTIONID_WALLPAPER:
            heimdall_player_vout_ToggleWallpaperMode(player);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

/************************
 * vout action handling *
 ************************/

#define VOUT_ACTION_HANDLER(name) \
static inline void \
action_handler_Vout##name(intf_thread_t *intf, vout_thread_t *vout, heimdall_action_id_t action_id)

static inline void
vout_CycleVariable(vout_thread_t *vout,
                   char const *varname, int vartype, bool next)
{
    heimdall_value_t val;

    if (var_Get(vout, varname, &val) != HEIMDALL_SUCCESS)
        return;

    size_t num_choices;
    heimdall_value_t *choices;
    var_Change(vout, varname, HEIMDALL_VAR_GETCHOICES,
               &num_choices, &choices, NULL);

    heimdall_value_t *choice = choices;
    for (size_t curidx = 0; curidx < num_choices; ++curidx, ++choice)
        if ((vartype == HEIMDALL_VAR_FLOAT &&
             choice->f_float == val.f_float) ||
            (vartype == HEIMDALL_VAR_STRING &&
             !strcmp(choice->psz_string, val.psz_string)))
        {
            curidx += next ? +1 : -1;
            if (next && curidx == num_choices)
                curidx = 0;
            else if (!next && curidx == (size_t)-1)
                curidx = num_choices - 1;
            choice = choices + curidx;
            break;
        }
    if (choice == choices + num_choices)
        choice = choices;
    if (vartype == HEIMDALL_VAR_FLOAT)
        var_SetFloat(vout, varname, choice->f_float);
    else if (vartype == HEIMDALL_VAR_STRING)
        var_SetString(vout, varname, choice->psz_string);

    if (vartype == HEIMDALL_VAR_STRING)
    {
        free(val.psz_string);
        for (size_t i = 0; i < num_choices; ++i)
            free(choices[i].psz_string);
    }
    free(choices);
}

#define vout_CycleVariable(vout, varname, vartype, next) \
    do \
    { \
        static_assert(vartype == HEIMDALL_VAR_FLOAT || \
                      vartype == HEIMDALL_VAR_STRING, \
                      "vartype must be either HEIMDALL_VAR_FLOAT or HEIMDALL_VAR_STRING"); \
        vout_CycleVariable(vout, varname, vartype, next); \
    } while (0)

VOUT_ACTION_HANDLER(AspectRatio)
{
    HEIMDALL_UNUSED(action_id); HEIMDALL_UNUSED(intf);
    vout_CycleVariable(vout, "aspect-ratio", HEIMDALL_VAR_STRING, true);
}

VOUT_ACTION_HANDLER(Crop)
{
    HEIMDALL_UNUSED(intf);
    if (action_id == ACTIONID_CROP)
    {
        vout_CycleVariable(vout, "crop", HEIMDALL_VAR_STRING, true);
        return;
    }
    char const *varname;
    int delta;
    switch (action_id)
    {
#define CASE_CROP(crop, var) \
        case ACTIONID_CROP_##crop: \
        case ACTIONID_UNCROP_##crop: \
            varname = "crop-"#var; \
            delta = action_id == ACTIONID_CROP_##crop? +1 : -1; \
            break;
                CASE_CROP(TOP, top)
                CASE_CROP(BOTTOM, bottom)
                CASE_CROP(LEFT, left)
                CASE_CROP(RIGHT, right)
#undef CASE_CROP
        default:
            heimdall_assert_unreachable();
    }
    int crop = var_GetInteger(vout, varname);
    var_SetInteger(vout, varname, crop + delta);
}

VOUT_ACTION_HANDLER(Zoom)
{
    HEIMDALL_UNUSED(intf);
    char const *varname = "zoom";
    switch (action_id)
    {
        case ACTIONID_TOGGLE_AUTOSCALE:
            if (var_GetFloat(vout, varname) != 1.f)
                var_SetFloat(vout, varname, 1.f);
            else
                var_ToggleBool(vout, "autoscale");
            break;
        case ACTIONID_SCALE_DOWN:
        case ACTIONID_SCALE_UP:
        {
            float zoom = var_GetFloat(vout, varname);
            float delta = action_id == ACTIONID_SCALE_DOWN ?
                -.1f : +.1f;
            if ((zoom >= .3f || delta > 0) && (zoom <= 10.f || delta < 0))
                var_SetFloat(vout, varname, zoom + delta);
            break;
        }
        case ACTIONID_ZOOM:
        case ACTIONID_UNZOOM:
            vout_CycleVariable(vout, varname, HEIMDALL_VAR_FLOAT,
                               action_id == ACTIONID_ZOOM);
            break;
        default:
        {
            static float const zoom_modes[] = { .25f, .5f, 1.f, 2.f };
            var_SetFloat(vout, varname,
                         zoom_modes[action_id - ACTIONID_ZOOM_QUARTER]);
            break;
        }
    }
}

VOUT_ACTION_HANDLER(Deinterlace)
{
    HEIMDALL_UNUSED(intf);
    if (action_id == ACTIONID_DEINTERLACE)
        var_SetInteger(vout, "deinterlace",
                       var_GetInteger(vout, "deinterlace") ? 0 : 1);
    else if (action_id == ACTIONID_DEINTERLACE_MODE)
        vout_CycleVariable(vout, "deinterlace-mode", HEIMDALL_VAR_STRING, true);
}

VOUT_ACTION_HANDLER(SubtitleDisplay)
{
    intf_sys_t *sys = intf->p_sys;
    const char* psz_sub_margin = sys->spu_channel_order == HEIMDALL_VOUT_ORDER_PRIMARY ?
        "sub-margin" : "secondary-sub-margin";
    switch (action_id)
    {
        case ACTIONID_SUBPOS_DOWN:
            var_DecInteger(vout, psz_sub_margin);
            break;
        case ACTIONID_SUBPOS_UP:
            var_IncInteger(vout, psz_sub_margin);
            break;
        default:
        {
            // FIXME all vouts
            heimdall_player_t *player = heimdall_playlist_GetPlayer(sys->playlist);
            int scale;
            if (action_id == ACTIONID_SUBTITLE_TEXT_SCALE_NORMAL)
                scale = 100;
            else
            {
                scale = heimdall_player_GetSubtitleTextScale(player);
                int delta = (scale > 100 ? scale - 100 : 100 - scale) / 25;
                delta = delta <= 1 ? 10 : 25;
                if (action_id == ACTIONID_SUBTITLE_TEXT_SCALE_DOWN)
                    delta = -delta;
                scale += delta;
                scale -= scale % delta;
                scale = HEIMDALL_CLIP(scale, 25, 500);
            }
            heimdall_player_SetSubtitleTextScale(player, scale);
            break;
        }
    }
}

VOUT_ACTION_HANDLER(Projection)
{
    (void)intf; (void)action_id;
    var_ToggleBool(vout, "override-projection");
}

/****************
 * action table *
 ****************/

struct heimdall_action
{
    enum
    {
        NULL_ACTION = -1,
        INTF_ACTION,
        PLAYLIST_ACTION,
        PLAYER_ACTION,
        VOUT_ACTION,
    } type;
    struct
    {
        heimdall_action_id_t first;
        heimdall_action_id_t last;
    } range;
    union
    {
        void *fptr;
        void (*pf_intf)(intf_thread_t *, heimdall_action_id_t);
        void (*pf_playlist)(intf_thread_t *, heimdall_playlist_t *, heimdall_action_id_t);
        void (*pf_player)(intf_thread_t *, heimdall_player_t *, heimdall_action_id_t);
        void (*pf_vout)(intf_thread_t *, vout_thread_t *vout, heimdall_action_id_t);
    } handler;
    bool pl_need_lock;
};

static struct heimdall_action const actions[] =
{
#define HEIMDALL_ACTION(typeval, first, last, name, lock) \
    { \
        .type = typeval, \
        .range = { ACTIONID_##first, ACTIONID_##last }, \
        .handler.fptr = action_handler_##name, \
        .pl_need_lock = lock \
    },
#define HEIMDALL_ACTION_INTF(first, last, name, lock) \
    HEIMDALL_ACTION(INTF_ACTION, first, last, Intf ## name, lock)
#define HEIMDALL_ACTION_PLAYLIST(first, last, name) \
    HEIMDALL_ACTION(PLAYLIST_ACTION, first, last, Playlist ## name, true)
#define HEIMDALL_ACTION_PLAYER(first, last, name, lock) \
    HEIMDALL_ACTION(PLAYER_ACTION, first, last, Player ## name, lock)
#define HEIMDALL_ACTION_VOUT(first, last, name) \
    HEIMDALL_ACTION(VOUT_ACTION, first, last, Vout ## name, false)

    /* interface actions */
    HEIMDALL_ACTION_INTF(QUIT, INTF_POPUP_MENU, , false)
    HEIMDALL_ACTION_INTF(COMBO_VOL_FOV_DOWN, COMBO_VOL_FOV_UP, ActionCombo, false)
    /* playlist actions */
    HEIMDALL_ACTION_PLAYLIST(PLAY_CLEAR, NEXT, Interact)
    HEIMDALL_ACTION_PLAYLIST(LOOP, RANDOM, Playback)
    HEIMDALL_ACTION_INTF(SET_BOOKMARK1, PLAY_BOOKMARK10, PlaylistBookmark, true)
    /* player actions */
    HEIMDALL_ACTION_PLAYER(PLAY_PAUSE, FRAME_NEXT, State, true)
    HEIMDALL_ACTION_PLAYER(PLAY_PAUSE, FRAME_PREVIOUS, State, true)
    HEIMDALL_ACTION_INTF(JUMP_BACKWARD_EXTRASHORT, JUMP_FORWARD_LONG, PlayerSeek, true)
    HEIMDALL_ACTION_PLAYER(POSITION, POSITION, Position, true)
    HEIMDALL_ACTION_PLAYER(PROGRAM_SID_PREV, DISC_MENU, NavigateMedia, true)
    HEIMDALL_ACTION_PLAYER(AUDIO_TRACK, SUBTITLE_TRACK, Track, true)
    HEIMDALL_ACTION_PLAYER(AUDIODELAY_DOWN, SUBDELAY_UP, Delay, true)
    HEIMDALL_ACTION_PLAYER(RATE_NORMAL, RATE_FASTER_FINE, Rate, true)
    HEIMDALL_ACTION_PLAYER(SUBTITLE_TOGGLE, SUBTITLE_TOGGLE, ToggleSubtitle, true)
    HEIMDALL_ACTION_PLAYER(SUBTITLE_CONTROL_SECONDARY, SUBTITLE_CONTROL_SECONDARY,
                      ControlSubtitleSecondary, true)
    HEIMDALL_ACTION_PLAYER(SUBSYNC_MARKAUDIO, SUBSYNC_RESET, SyncSubtitle, true)
    HEIMDALL_ACTION_PLAYER(NAV_ACTIVATE, NAV_RIGHT, Navigate, true)
    HEIMDALL_ACTION_PLAYER(VIEWPOINT_FOV_IN, VIEWPOINT_ROLL_ANTICLOCK, Viewpoint, true)
    HEIMDALL_ACTION_PLAYER(RECORD, RECORD, Record, true)
    HEIMDALL_ACTION_PLAYER(VOL_DOWN, AUDIODEVICE_CYCLE, Aout, false)
    HEIMDALL_ACTION_PLAYER(TOGGLE_FULLSCREEN, WALLPAPER, Vouts, false)
    /* vout actions */
    HEIMDALL_ACTION_VOUT(ASPECT_RATIO, ASPECT_RATIO, AspectRatio)
    HEIMDALL_ACTION_VOUT(CROP, UNCROP_RIGHT, Crop)
    HEIMDALL_ACTION_VOUT(TOGGLE_AUTOSCALE, ZOOM_DOUBLE, Zoom)
    HEIMDALL_ACTION_VOUT(DEINTERLACE, DEINTERLACE_MODE, Deinterlace)
    HEIMDALL_ACTION_VOUT(SUBPOS_DOWN, SUBTITLE_TEXT_SCALE_UP, SubtitleDisplay)
    HEIMDALL_ACTION_VOUT(PROJECTION_TOGGLE, PROJECTION_TOGGLE, Projection)
    /* null action */
    { .type = NULL_ACTION }

#undef HEIMDALL_ACTION_VOUT
#undef HEIMDALL_ACTION_PLAYER
#undef HEIMDALL_ACTION_PLAYLIST
#undef HEIMDALL_ACTION_INTF
#undef HEIMDALL_ACTION
};

static void
handle_action(intf_thread_t *intf, heimdall_action_id_t action_id)
{
    size_t action_idx;
    for (action_idx = 0; actions[action_idx].type != NULL_ACTION; ++action_idx)
        if (actions[action_idx].range.first <= action_id &&
            actions[action_idx].range.last >= action_id)
            break;
    if (actions[action_idx].type == NULL_ACTION)
    {
        msg_Warn(intf, "no handler for action %d", action_id);
        return;
    }
    struct heimdall_action const *action = actions + action_idx;

    heimdall_playlist_t *playlist = intf->p_sys->playlist;
    if (action->pl_need_lock)
        heimdall_playlist_Lock(playlist);

    switch (action->type)
    {
        case INTF_ACTION:
            action->handler.pf_intf(intf, action_id);
            break;
        case PLAYLIST_ACTION:
            action->handler.pf_playlist(intf, playlist, action_id);
            break;
        case PLAYER_ACTION:
        case VOUT_ACTION:
        {
            heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);
            if (action->type == PLAYER_ACTION)
                action->handler.pf_player(intf, player, action_id);
            else
            {
                vout_thread_t *vout = heimdall_player_vout_Hold(player);
                action->handler.pf_vout(intf, vout, action_id);
                vout_Release(vout);
            }
            break;
        }
        default:
            heimdall_assert_unreachable();
    }

    if (action->pl_need_lock)
        heimdall_playlist_Unlock(playlist);
}

/******************
 * vout callbacks *
 ******************/

static int
MouseButtonCallback(heimdall_object_t *obj, char const *var,
                    heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(var);

    intf_thread_t *intf = data;
    intf_sys_t *sys = intf->p_sys;
    vout_thread_t *vout = (vout_thread_t *)obj;

    if ((newval.i_int & (1 << MOUSE_BUTTON_LEFT)) &&
        var_GetBool(vout, "viewpoint-changeable"))
    {
        if (!sys->vrnav.btn_pressed)
        {
            sys->vrnav.btn_pressed = true;
            var_GetCoords(vout, "mouse-moved", &sys->vrnav.x, &sys->vrnav.y);
        }
    }
    else
        sys->vrnav.btn_pressed = false;

    unsigned pressed = newval.i_int & ~oldval.i_int;
    if (pressed & (1 << MOUSE_BUTTON_LEFT))
        var_SetBool(heimdall_object_instance(intf), "intf-popupmenu", false);
    if (pressed & (1 << MOUSE_BUTTON_CENTER))
        var_TriggerCallback(heimdall_object_instance(intf), "intf-toggle-fscontrol");
#ifndef _WIN32
    if (pressed & (1 << MOUSE_BUTTON_RIGHT))
#else
    if ((oldval.i_int & (1 << MOUSE_BUTTON_RIGHT))
     && !(newval.i_int & (1 << MOUSE_BUTTON_RIGHT)))
#endif
        var_SetBool(heimdall_object_instance(intf), "intf-popupmenu", true);
    for (int i = MOUSE_BUTTON_WHEEL_UP; i <= MOUSE_BUTTON_WHEEL_RIGHT; i++)
        if (pressed & (1 << i))
        {
            int keycode = KEY_MOUSEWHEEL_FROM_BUTTON(i);
            var_SetInteger(heimdall_object_instance(intf), "key-pressed", keycode);
        }

    return HEIMDALL_SUCCESS;
}

static int
MouseMovedCallback(heimdall_object_t *obj, char const *var,
                   heimdall_value_t ov, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(obj); HEIMDALL_UNUSED(var); HEIMDALL_UNUSED(ov);
    intf_sys_t *sys = data;
    heimdall_player_t *player = heimdall_playlist_GetPlayer(sys->playlist);
    if (sys->vrnav.btn_pressed)
    {
        int i_horizontal = newval.coords.x - sys->vrnav.x;
        int i_vertical   = newval.coords.y - sys->vrnav.y;
        heimdall_viewpoint_t viewpoint = { .fov = 0.f };
        heimdall_viewpoint_from_euler(&viewpoint,
            -i_horizontal * 0.05f, /* yaw */
            -i_vertical   * 0.05f, /* pitch */
            0);                    /* roll */
        heimdall_player_Lock(player);
        heimdall_player_UpdateViewpoint(player, &viewpoint,
                                   HEIMDALL_PLAYER_WHENCE_RELATIVE);
        heimdall_player_Unlock(player);
        sys->vrnav.x = newval.coords.x;
        sys->vrnav.y = newval.coords.y;
    }
    return HEIMDALL_SUCCESS;
}

static int
ViewpointMovedCallback(heimdall_object_t *obj, char const *var,
                       heimdall_value_t ov, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(obj); HEIMDALL_UNUSED(var); HEIMDALL_UNUSED(ov);
    heimdall_player_t *player = data;
    heimdall_player_Lock(player);
    heimdall_player_UpdateViewpoint(player, (heimdall_viewpoint_t *)newval.p_address,
                               HEIMDALL_PLAYER_WHENCE_RELATIVE);
    heimdall_player_Unlock(player);
    return HEIMDALL_SUCCESS;
}

static void
player_on_vout_started(heimdall_player_t *player, vout_thread_t *vout, void *data)
{
    intf_thread_t *intf = data;
    intf_sys_t *sys = intf->p_sys;

    if (unlikely(heimdall_array_append(&sys->vouts, vout) != 0))
        return;

    var_AddCallback(vout, "mouse-button-down", MouseButtonCallback, intf);
    var_AddCallback(vout, "mouse-moved", MouseMovedCallback, sys);

    if (var_GetBool(vout, "viewpoint-changeable"))
        var_AddCallback(vout, "viewpoint-moved",
                        ViewpointMovedCallback, player);
}

static void
player_on_vout_stopped(heimdall_player_t *player, vout_thread_t *vout, void *data)
{
    intf_thread_t *intf = data;
    intf_sys_t *sys = intf->p_sys;
    ssize_t idx = heimdall_array_index_of_item(&sys->vouts, vout);

    if (unlikely(idx < 0))
        return;

    heimdall_array_remove(&sys->vouts, idx);

    if (var_GetBool(vout, "viewpoint-changeable"))
        var_DelCallback(vout, "viewpoint-moved",
                        ViewpointMovedCallback, player);

    var_DelCallback(vout, "mouse-moved", MouseMovedCallback, sys);
    var_DelCallback(vout, "mouse-button-down", MouseButtonCallback, intf);
}

static void
player_on_vout_changed(heimdall_player_t *player,
                       enum heimdall_player_vout_action action, vout_thread_t *vout,
                       enum heimdall_vout_order order, heimdall_es_id_t *es_id,
                       void *data)
{
    HEIMDALL_UNUSED(order);

    if (heimdall_es_id_GetCat(es_id) != VIDEO_ES)
        return;

    switch (action)
    {
        case HEIMDALL_PLAYER_VOUT_STARTED:
            player_on_vout_started(player, vout, data);
            break;
        case HEIMDALL_PLAYER_VOUT_STOPPED:
            player_on_vout_stopped(player, vout, data);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static int
ActionCallback(heimdall_object_t *obj, char const *var,
               heimdall_value_t ov, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(obj); HEIMDALL_UNUSED(var); HEIMDALL_UNUSED(ov);
    handle_action(data, newval.i_int);
    return HEIMDALL_SUCCESS;
}

/****************************
 * module opening / closing *
 ****************************/

static int
Open(heimdall_object_t *this)
{
    intf_thread_t *intf = (intf_thread_t *)this;
    intf_sys_t *sys = malloc(sizeof(intf_sys_t));
    if (!sys)
        return HEIMDALL_ENOMEM;
    sys->vrnav.btn_pressed = false;
    sys->playlist = heimdall_intf_GetMainPlaylist(intf);
    heimdall_array_init(&sys->vouts);
    sys->subsync.audio_time = sys->subsync.subtitle_time = HEIMDALL_TICK_INVALID;
    sys->spu_channel_order = HEIMDALL_VOUT_ORDER_PRIMARY;
    static struct heimdall_player_cbs const player_cbs =
    {
        .on_vout_changed = player_on_vout_changed,
    };
    heimdall_player_t *player = heimdall_playlist_GetPlayer(sys->playlist);
    heimdall_player_Lock(player);
    sys->player_listener = heimdall_player_AddListener(player, &player_cbs, intf);
    heimdall_player_Unlock(player);
    if (!sys->player_listener)
    {
        free(sys);
        return HEIMDALL_EGENERIC;
    }
    var_AddCallback(heimdall_object_instance(intf), "key-action", ActionCallback, intf);

    sys->executor = NULL;

    intf->p_sys = sys;
    return HEIMDALL_SUCCESS;
}

static void
Close(heimdall_object_t *this)
{
    intf_thread_t *intf = (intf_thread_t *)this;
    intf_sys_t *sys = intf->p_sys;
    heimdall_player_t *player = heimdall_playlist_GetPlayer(sys->playlist);
    heimdall_player_Lock(player);
    heimdall_player_RemoveListener(player, sys->player_listener);

    while (sys->vouts.i_count > 0)
        player_on_vout_stopped(player, heimdall_array_item_at_index(&sys->vouts, 0),
                               intf);
    heimdall_player_Unlock(player);

    var_DelCallback(heimdall_object_instance(intf), "key-action", ActionCallback, intf);

    if (sys->executor)
    {
        heimdall_executor_WaitIdle(sys->executor);
        heimdall_executor_Delete(sys->executor);
    }

    free(sys);
}

static void AutoRun(libheimdall_int_t *libheimdall)
{
    intf_Create(libheimdall, MODULE_STRING);
}

heimdall_module_begin ()
    set_shortname(N_("Hotkeys"))
    set_description(N_("Hotkeys management interface"))
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    set_subcategory(SUBCAT_INTERFACE_HOTKEYS)

    add_submodule()
    set_capability("autorun", 20)
    set_callback(AutoRun)
heimdall_module_end ()
