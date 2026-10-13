/*****************************************************************************
 * vout_wrapper.c: "vout display" -> "video output" wrapper
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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
#include <heimdall_plugin.h>
#include <heimdall_vout.h>
#include <assert.h>
#include "vout_private.h"
#include "vout_internal.h"
#include "display.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
#ifdef _WIN32
static int  Forward(heimdall_object_t *, char const *,
                    heimdall_value_t, heimdall_value_t, void *);
#endif

static void VoutViewpointMoved(void *sys, const heimdall_viewpoint_t *vp)
{
    vout_thread_t *vout = sys;
    var_SetAddress(vout, "viewpoint-moved", (void*)vp);
}

/*****************************************************************************
 *
 *****************************************************************************/
vout_display_t *vout_OpenWrapper(vout_thread_t *vout,
                     const char *splitter_name, const vout_display_cfg_t *cfg,
                     const video_format_t *fmt, heimdall_video_context *vctx)
{
    vout_display_t *vd;
    vout_display_owner_t owner = {
        .viewpoint_moved = VoutViewpointMoved, .sys = vout,
    };
    const char *modlist;
    char *modlistbuf = NULL;

    msg_Dbg(vout, "Opening vout display wrapper");

    if (splitter_name == NULL) {
        modlist = modlistbuf = var_InheritString(vout, "vout");
        if (unlikely(modlist == NULL))
            return NULL;
    } else
        modlist = "splitter,none";

    vd = vout_display_New(HEIMDALL_OBJECT(vout), fmt, vctx, cfg, modlist, &owner);
    free(modlistbuf);

    if (vd == NULL)
        return NULL;

#ifdef _WIN32
    var_Create(vout, "video-wallpaper", HEIMDALL_VAR_BOOL|HEIMDALL_VAR_DOINHERIT);
    var_AddCallback(vout, "video-wallpaper", Forward, vd);
#endif
    var_SetBool(HEIMDALL_OBJECT(vout), "viewpoint-changeable",
                vd->fmt->projection_mode != PROJECTION_MODE_RECTANGULAR);
    return vd;
}

/*****************************************************************************
 *
 *****************************************************************************/
void vout_CloseWrapper(vout_thread_t *vout, vout_display_t *vd)
{
#ifdef _WIN32
    var_DelCallback(vout, "video-wallpaper", Forward, vd);
#else
    (void) vout;
#endif

    vout_display_Delete(vd);
}

#ifdef _WIN32
static int Forward(heimdall_object_t *object, char const *var,
                   heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    vout_display_t *vd = data;

    HEIMDALL_UNUSED(object); HEIMDALL_UNUSED(oldval);
    return var_Set(vd, var, newval);
}
#endif

