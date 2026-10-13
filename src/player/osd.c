/*****************************************************************************
 * player_osd.c: Player osd implementation
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
#include <heimdall_memstream.h>
#include <heimdall_vout_osd.h>
#include "player.h"
#include "input/resource.h"

static vout_thread_t **
heimdall_player_osd_HoldAll(heimdall_player_t *player, size_t *count)
{
    vout_thread_t **vouts;
    input_resource_HoldVouts(player->resource, &vouts, count);

    for (size_t i = 0; i < *count; ++i)
    {
        vout_FlushSubpictureChannel(vouts[i], VOUT_SPU_CHANNEL_OSD);
        vout_FlushSubpictureChannel(vouts[i], VOUT_SPU_CHANNEL_OSD_HSLIDER);
        vout_FlushSubpictureChannel(vouts[i], VOUT_SPU_CHANNEL_OSD_HSLIDER);
    }
    return vouts;
}

static void
heimdall_player_osd_ReleaseAll(heimdall_player_t *player, vout_thread_t **vouts,
                            size_t count)
{
    for (size_t i = 0; i < count; ++i)
        vout_Release(vouts[i]);
    free(vouts);
    (void) player;
}

static inline void
vouts_osd_Message(vout_thread_t **vouts, size_t count, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    for (size_t i = 0; i < count; ++i)
    {
        va_list acpy;
        va_copy(acpy, args);
        vout_OSDMessageVa(vouts[i], VOUT_SPU_CHANNEL_OSD, fmt, acpy);
        va_end(acpy);
    }
    va_end(args);
}

static inline void
vouts_osd_Icon(vout_thread_t **vouts, size_t count, heimdall_osd_widget_type type)
{
    for (size_t i = 0; i < count; ++i)
        vout_OSDIcon(vouts[i], VOUT_SPU_CHANNEL_OSD, type);
}

static inline void
vouts_osd_Slider(vout_thread_t **vouts, size_t count, int position, heimdall_osd_widget_type type)
{
    int channel = type == OSD_HOR_SLIDER ?
        VOUT_SPU_CHANNEL_OSD_HSLIDER : VOUT_SPU_CHANNEL_OSD_VSLIDER;
    for (size_t i = 0; i < count; ++i)
        vout_OSDSlider(vouts[i], channel, position, type);
}

void
heimdall_player_osd_Message(heimdall_player_t *player, const char *fmt, ...)
{
    size_t count;
    vout_thread_t **vouts = heimdall_player_osd_HoldAll(player, &count);

    va_list args;
    va_start(args, fmt);
    for (size_t i = 0; i < count; ++i)
    {
        va_list acpy;
        va_copy(acpy, args);
        vout_OSDMessageVa(vouts[i], VOUT_SPU_CHANNEL_OSD, fmt, acpy);
        va_end(acpy);
    }
    va_end(args);

    heimdall_player_osd_ReleaseAll(player, vouts, count);
}

void
heimdall_player_osd_Icon(heimdall_player_t *player, heimdall_osd_widget_type type)
{
    size_t count;
    vout_thread_t **vouts = heimdall_player_osd_HoldAll(player, &count);

    vouts_osd_Icon(vouts, count, type);

    heimdall_player_osd_ReleaseAll(player, vouts, count);
}

void
heimdall_player_osd_Position(heimdall_player_t *player,
                        struct heimdall_player_input *input, heimdall_tick_t time,
                        double position)
{
    if (input->length != HEIMDALL_TICK_INVALID)
    {
        if (time == HEIMDALL_TICK_INVALID)
            time = position * input->length;
        else
            position = time / (double) input->length;
    }

    size_t count;
    vout_thread_t **vouts = heimdall_player_osd_HoldAll(player, &count);

    if (time != HEIMDALL_TICK_INVALID)
    {
        char time_text[MSTRTIME_MAX_SIZE];
        heimdall_tick_to_str(time_text, time);
        if (input->length != HEIMDALL_TICK_INVALID)
        {
            char len_text[MSTRTIME_MAX_SIZE];
            heimdall_tick_to_str(len_text, input->length);
            vouts_osd_Message(vouts, count, "%s / %s", time_text, len_text);
        }
        else
            vouts_osd_Message(vouts, count, "%s", time_text);
    }

    if (heimdall_player_vout_IsFullscreen(player))
        vouts_osd_Slider(vouts, count, position * 100, OSD_HOR_SLIDER);
    heimdall_player_osd_ReleaseAll(player, vouts, count);
}

void
heimdall_player_osd_Volume(heimdall_player_t *player, bool mute_action)
{
    size_t count;
    vout_thread_t **vouts = heimdall_player_osd_HoldAll(player, &count);

    bool mute = heimdall_player_aout_IsMuted(player);
    int volume = lroundf(heimdall_player_aout_GetVolume(player) * 100.f);
    if (mute_action && mute)
        vouts_osd_Icon(vouts, count, OSD_MUTE_ICON);
    else
    {
        if (heimdall_player_vout_IsFullscreen(player))
            vouts_osd_Slider(vouts, count, volume, OSD_VERT_SLIDER);
        vouts_osd_Message(vouts, count, _("Volume: %ld%%"), volume);
    }

    heimdall_player_osd_ReleaseAll(player, vouts, count);
}

void
heimdall_player_osd_Track(heimdall_player_t *player, heimdall_es_id_t *id, bool select)
{
    enum es_format_category_e cat = heimdall_es_id_GetCat(id);
    const struct heimdall_player_track *track = heimdall_player_GetTrack(player, id);
    if (!track && select)
        return;

    const char *cat_name = es_format_category_to_string(cat);
    assert(cat_name);
    const char *track_name = select ? track->name : _("N/A");
    heimdall_player_osd_Message(player, _("%s track: %s"), cat_name, track_name);
}

void
heimdall_player_osd_Tracks(heimdall_player_t *player, heimdall_es_id_t * const *selected, heimdall_es_id_t *unselect)
{
    if (!selected || !selected[0])
        return;

    const enum es_format_category_e cat = heimdall_es_id_GetCat(selected[0]);
    const char *cat_name = es_format_category_to_string(cat);
    int tracks_count = 0;
    struct heimdall_memstream stream;

    for (size_t i = 0; selected[i] != NULL; i++)
    {
        if (unselect != NULL
         && !strcmp(heimdall_es_id_GetStrId(selected[i]), heimdall_es_id_GetStrId(unselect)))
            continue;

        const struct heimdall_player_track *track =
            heimdall_player_GetTrack(player, selected[i]);
        /* The track can be NULL if it was terminated by the playback.
         * In this case, we have nothing to display for it. */
        if (unlikely(track == NULL))
            continue;

        /* Open the stream for the first track, and exit in case of error.
         * On the next tracks, add comma to separate the track names. */
        if (tracks_count == 0)
        {
            if (heimdall_memstream_open(&stream) != 0)
                return;
        }
        else
            heimdall_memstream_puts(&stream, ", ");

        heimdall_memstream_puts(&stream, track->name);
        tracks_count++;
    }

    if (tracks_count == 0)
    {
        heimdall_player_osd_Message(player, _("%s track: %s"), cat_name, _("N/A"));
        return;
    }

    /* The heimdall_memstream is opened only if tracks_count != 0. */
    if (heimdall_memstream_close(&stream) != 0)
        return;

    if (tracks_count == 1)
        heimdall_player_osd_Message(player, _("%s track: %s"), cat_name,
                               stream.ptr);
    else
        heimdall_player_osd_Message(player, _("%s tracks: %s"), cat_name,
                               stream.ptr);
    free(stream.ptr);
}

void
heimdall_player_osd_Program(heimdall_player_t *player, const char *name)
{
    heimdall_player_osd_Message(player, _("Program Service ID: %s"), name);
}

static bool
vout_osd_PrintVariableText(vout_thread_t *vout, const char *varname, int vartype,
                           heimdall_value_t varval, const char *osdfmt)
{
    bool found = false;
    bool isvarstring = vartype == HEIMDALL_VAR_STRING;
    size_t num_choices;
    heimdall_value_t *choices;
    char **choices_text;
    var_Change(vout, varname, HEIMDALL_VAR_GETCHOICES,
               &num_choices, &choices, &choices_text);
    for (size_t i = 0; i < num_choices; ++i)
    {
        if (!found)
            if ((isvarstring &&
                 strcmp(choices[i].psz_string, varval.psz_string) == 0) ||
                (!isvarstring && choices[i].f_float == varval.f_float))
            {
                vouts_osd_Message(&vout, 1, osdfmt, choices_text[i]);
                found = true;
            }
        if (isvarstring)
            free(choices[i].psz_string);
        free(choices_text[i]);
    }
    free(choices);
    free(choices_text);
    return found;
}

int
heimdall_player_vout_OSDCallback(heimdall_object_t *this, const char *var,
                            heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(oldval);

    vout_thread_t *vout = (vout_thread_t *)this;

    if (strcmp(var, "aspect-ratio") == 0)
        vout_osd_PrintVariableText(vout, var, HEIMDALL_VAR_STRING,
                                   newval, _("Aspect ratio: %s"));

    else if (strcmp(var, "autoscale") == 0)
        vouts_osd_Message(&vout, 1, newval.b_bool ?
                          _("Scaled to screen") : _("Original size"));

    else if (strcmp(var, "crop") == 0)
        vout_osd_PrintVariableText(vout, var, HEIMDALL_VAR_STRING, newval,
                                   _("Crop: %s"));

    else if (strcmp(var, "crop-bottom") == 0)
        vouts_osd_Message(&vout, 1, _("Bottom crop: %d px"), newval.i_int);

    else if (strcmp(var, "crop-top") == 0)
        vouts_osd_Message(&vout, 1, _("Top crop: %d px"), newval.i_int);

    else if (strcmp(var, "crop-left") == 0)
        vouts_osd_Message(&vout, 1, _("Left crop: %d px"), newval.i_int);

    else if (strcmp(var, "crop-right") == 0)
        vouts_osd_Message(&vout, 1, _("Right crop: %d px"), newval.i_int);

    else if (strcmp(var, "deinterlace") == 0 ||
             strcmp(var, "deinterlace-mode") == 0)
    {
        bool varmode = strcmp(var, "deinterlace-mode") == 0;
        int on = !varmode ?
            newval.i_int : var_GetInteger(vout, "deinterlace");
        char *mode = varmode ?
            newval.psz_string : var_GetString(vout, "deinterlace-mode");
        vouts_osd_Message(&vout, 1, _("Deinterlace %s (%s)"),
                          on == 1 ? _("On") : _("Off"), mode);
        if (!varmode)
            free(mode);
    }

    else if (strcmp(var, "sub-margin") == 0 && newval.i_int != oldval.i_int)
        vouts_osd_Message(&vout, 1, _("Subtitle position %d px"), newval.i_int);

    else if (strcmp(var, "secondary-sub-margin") == 0 && newval.i_int != oldval.i_int)
        vouts_osd_Message(&vout, 1, _("Secondary subtitle position %d px"), newval.i_int);

    else if (strcmp(var, "sub-text-scale") == 0)
        vouts_osd_Message(&vout, 1, _("Subtitle text scale %d%%"), newval.i_int);

    else if (strcmp(var, "zoom") == 0)
    {
        if (newval.f_float == 1.f)
            vouts_osd_Message(&vout, 1, _("Zooming reset"));
        else
        {
            bool found =  vout_osd_PrintVariableText(vout, var, HEIMDALL_VAR_FLOAT,
                                                     newval, _("Zoom mode: %s"));
            if (!found)
                vouts_osd_Message(&vout, 1, _("Zoom: x%.2f"), newval.f_float);
        }
    }

    (void) data;
    return HEIMDALL_SUCCESS;
}
