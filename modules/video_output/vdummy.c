/*****************************************************************************
 * vdummy.c: Dummy video output display method for testing purposes
 *****************************************************************************
 * Copyright (C) 2000-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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
#include <heimdall_threads.h>
#include <heimdall_plugin.h>
#include <heimdall_vout_display.h>

#define CHROMA_TEXT N_("Dummy image chroma format")
#define CHROMA_LONGTEXT N_( \
    "Force the dummy video output to create images using a specific chroma " \
    "format instead of trying to improve performances by using the most " \
    "efficient one.")

static int OpenDummy(vout_display_t *vd,
                     video_format_t *fmtp, heimdall_video_context *context);
static int OpenStats(vout_display_t *vd,
                     video_format_t *fmtp, heimdall_video_context *context);

heimdall_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy video output") )
    set_callback_display( OpenDummy, 0 )
    add_shortcut( "dummy" )

    set_subcategory( SUBCAT_VIDEO_VOUT )
    add_string( "dummy-chroma", NULL, CHROMA_TEXT, CHROMA_LONGTEXT )

    add_submodule ()
    set_description( N_("Statistics video output") )
    add_shortcut( "stats" )
    set_callback_display( OpenStats, 0 )
heimdall_module_end ()


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void            DisplayStat(vout_display_t *, picture_t *);

/*****************************************************************************
 * OpenVideo: activates dummy vout display method
 *****************************************************************************/
static void Open(vout_display_t *vd, video_format_t *fmt)
{
    /* p_vd->info is not modified */

    char *chroma = var_InheritString(vd, "dummy-chroma");
    if (chroma) {
        heimdall_fourcc_t fcc = heimdall_fourcc_GetCodecFromString(VIDEO_ES, chroma);
        if (fcc != 0) {
            msg_Dbg(vd, "forcing chroma 0x%.8x (%4.4s)", fcc, (char*)&fcc);
            fmt->i_chroma = fcc;
        }
        free(chroma);
    }
}

static void Display(vout_display_t *vd, picture_t *picture)
{
    (void) vd;
    (void) picture;
}

static const struct heimdall_display_operations ops_dummy = {
    .display = Display,
};

static int OpenDummy(vout_display_t *vd,
                     video_format_t *fmtp, heimdall_video_context *context)
{
    (void) context;
    Open(vd, fmtp);
    vd->ops = &ops_dummy;
    return HEIMDALL_SUCCESS;
}

static const struct heimdall_display_operations ops_stats = {
    .display = DisplayStat,
};

static int OpenStats(vout_display_t *vd,
                     video_format_t *fmtp, heimdall_video_context *context)
{
    (void) context;
    Open(vd, fmtp);
    vd->ops = &ops_stats;
    return HEIMDALL_SUCCESS;
}

static void DisplayStat(vout_display_t *vd, picture_t *picture)
{
    plane_t *p = picture->p;

    HEIMDALL_UNUSED(vd);

    if (picture->format.i_width * picture->format.i_height >= sizeof (heimdall_tick_t)
     && (p->i_pitch * p->i_lines) >= (ssize_t)sizeof (heimdall_tick_t)) {
        heimdall_tick_t date;
        memcpy(&date, p->p_pixels, sizeof(date));
        msg_Dbg(vd, "VOUT got %"PRIu64" ms offset",
                MS_FROM_HEIMDALL_TICK(heimdall_tick_now() - date));
    }
}
