/*****************************************************************************
 * var.c: object variables for input thread
 *****************************************************************************
 * Copyright (C) 2004-2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <assert.h>

#include "input_internal.h"

/*****************************************************************************
 * input_ConfigVarInit:
 *  Create all config object variables
 *****************************************************************************/
void heimdall_object_InitInputConfig(heimdall_object_t *obj,
                                bool playback, bool do_inherit)
{
    /* Create Object Variables for private use only */
    int inherit_flag = do_inherit ? HEIMDALL_VAR_DOINHERIT : 0;

    if (playback)
    {
        var_Create(obj, "video", HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "audio", HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "spu", HEIMDALL_VAR_BOOL | inherit_flag);

        var_Create(obj, "video-track", HEIMDALL_VAR_INTEGER|inherit_flag);
        var_Create(obj, "audio-track", HEIMDALL_VAR_INTEGER|inherit_flag);
        var_Create(obj, "sub-track", HEIMDALL_VAR_INTEGER|inherit_flag);

        var_Create(obj, "audio-language",
                    HEIMDALL_VAR_STRING|inherit_flag);
        var_Create(obj, "sub-language",
                    HEIMDALL_VAR_STRING|inherit_flag);
        var_Create(obj, "menu-language",
                    HEIMDALL_VAR_STRING|inherit_flag);

        var_Create(obj, "video-track-id",
                    HEIMDALL_VAR_STRING|inherit_flag);
        var_Create(obj, "audio-track-id",
                    HEIMDALL_VAR_STRING|inherit_flag);
        var_Create(obj, "sub-track-id",
                    HEIMDALL_VAR_STRING|inherit_flag);

        var_Create(obj, "sub-file", HEIMDALL_VAR_STRING | inherit_flag);
        var_Create(obj, "sub-autodetect-file", HEIMDALL_VAR_BOOL |
                    inherit_flag);
        var_Create(obj, "sub-autodetect-path", HEIMDALL_VAR_STRING |
                    inherit_flag);
        var_Create(obj, "sub-autodetect-fuzzy", HEIMDALL_VAR_INTEGER |
                    inherit_flag);

        var_Create(obj, "sout", HEIMDALL_VAR_STRING | inherit_flag);
        var_Create(obj, "sout-all",   HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "sout-audio", HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "sout-video", HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "sout-spu", HEIMDALL_VAR_BOOL | inherit_flag);
        var_Create(obj, "sout-keep",  HEIMDALL_VAR_BOOL | inherit_flag);

        var_Create(obj, "start-time", HEIMDALL_VAR_FLOAT|inherit_flag);
        var_Create(obj, "stop-time", HEIMDALL_VAR_FLOAT|inherit_flag);
        var_Create(obj, "run-time", HEIMDALL_VAR_FLOAT|inherit_flag);
        var_Create(obj, "input-fast-seek", HEIMDALL_VAR_BOOL|inherit_flag);

        var_Create(obj, "input-slave",
                    HEIMDALL_VAR_STRING | inherit_flag);

        var_Create(obj, "audio-desync",
                    HEIMDALL_VAR_INTEGER | inherit_flag);
        var_Create(obj, "cr-average",
                    HEIMDALL_VAR_INTEGER | inherit_flag);
        var_Create(obj, "clock-synchro",
                    HEIMDALL_VAR_INTEGER | inherit_flag);

        var_Create(obj, "bookmarks", HEIMDALL_VAR_STRING | inherit_flag);
        var_Create(obj, "programs", HEIMDALL_VAR_STRING | inherit_flag);
        var_Create(obj, "program", HEIMDALL_VAR_INTEGER | inherit_flag);
        var_Create(obj, "rate", HEIMDALL_VAR_FLOAT | inherit_flag);
    }

    /* */
    var_Create(obj, "input-record-native", HEIMDALL_VAR_BOOL | inherit_flag);

    /* */
    var_Create(obj, "demux", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "demux-filter", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "stream-filter", HEIMDALL_VAR_STRING | inherit_flag);

    /* Meta */
    var_Create(obj, "meta-title", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-author", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-artist", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-genre", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-copyright", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-description", HEIMDALL_VAR_STRING|inherit_flag);
    var_Create(obj, "meta-date", HEIMDALL_VAR_STRING | inherit_flag);
    var_Create(obj, "meta-url", HEIMDALL_VAR_STRING | inherit_flag);

    /* Inherited by demux/subtitle.c */
    var_Create(obj, "sub-original-fps", HEIMDALL_VAR_FLOAT);

    /* used by Medialibrary */
    var_Create(obj, "save-recentplay", HEIMDALL_VAR_BOOL | inherit_flag);
}
