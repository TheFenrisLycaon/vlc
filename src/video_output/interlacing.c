/*****************************************************************************
 * interlacing.c
 *****************************************************************************
 * Copyright (C) 2010 Laurent Aimar
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_threads.h>
#include <heimdall_vout.h>

#include "vout_private.h"
#include "vout_internal.h"

/*****************************************************************************
 * Deinterlacing
 *****************************************************************************/
/* XXX
 * You can use the non vout filter if and only if the video properties stay the
 * same (width/height/chroma/fps), at least for now.
 */
static const char deinterlace_modes[][9]= {
    "auto",
    "discard",
    "blend",
    "mean",
    "bob",
    "linear",
    "x",
    "yadif",
    "yadif2x",
    "phosphor",
    "ivtc",
};

static bool DeinterlaceIsModeValid(const char *mode)
{
    for (unsigned i = 0; i < ARRAY_SIZE(deinterlace_modes); i++) {
        if (!strcmp(deinterlace_modes[i], mode))
            return true;
    }
    return false;
}

static int DeinterlaceCallback(heimdall_object_t *object, char const *cmd,
                               heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(newval); HEIMDALL_UNUSED(data);
    vout_thread_t *vout = (vout_thread_t *)object;

    /* */
    const int  deinterlace_state = var_GetInteger(vout, "deinterlace");
    char       *mode             = var_GetString(vout,  "deinterlace-mode");
    const bool is_needed         = var_GetBool(vout,    "deinterlace-needed");
    if (!mode || !DeinterlaceIsModeValid(mode))
    {
        msg_Err(vout, "unknown deinterlace mode %s", mode);
        free(mode);
        return HEIMDALL_EGENERIC;
    }

    /* */
    var_SetString(vout, "sout-deinterlace-mode", mode);

    msg_Dbg(vout, "deinterlace %d, mode %s, is_needed %d", deinterlace_state, mode, is_needed);
    vout_ControlChangeInterlacing(vout, deinterlace_state != 0 && (is_needed || deinterlace_state >= 0));

    /* */
    free(mode);
    return HEIMDALL_SUCCESS;
}

void vout_InitInterlacingSupport(vout_thread_t *vout, vout_interlacing_state_t *sys)
{
    heimdall_value_t val;

    msg_Dbg(vout, "Deinterlacing available");

    sys->has_deint = false;

    /* Create the configuration variables */
    /* */
    var_Create(vout, "deinterlace", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    int deinterlace_state = var_GetInteger(vout, "deinterlace");

    var_Change(vout, "deinterlace", HEIMDALL_VAR_SETTEXT, _("Deinterlace"));

    const module_config_t *optd = config_FindConfig("deinterlace");
    var_Change(vout, "deinterlace", HEIMDALL_VAR_CLEARCHOICES);
    if (likely(optd != NULL))
        for (unsigned i = 0; i < optd->list_count; i++) {
            val.i_int = optd->list.i[i];
            var_Change(vout, "deinterlace", HEIMDALL_VAR_ADDCHOICE, val,
                       heimdall_gettext(optd->list_text[i]));
        }
    var_AddCallback(vout, "deinterlace", DeinterlaceCallback, NULL);
    /* */
    var_Create(vout, "deinterlace-mode", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT );
    char *deinterlace_mode = var_GetNonEmptyString(vout, "deinterlace-mode");

    var_Change(vout, "deinterlace-mode", HEIMDALL_VAR_SETTEXT,
               _("Deinterlace mode"));

    const module_config_t *optm = config_FindConfig("deinterlace-mode");
    var_Change(vout, "deinterlace-mode", HEIMDALL_VAR_CLEARCHOICES);
    if (likely(optm != NULL))
        for (unsigned i = 0; i < optm->list_count; i++) {
             if (!DeinterlaceIsModeValid(optm->list.psz[i]))
                 continue;

             val.psz_string  = (char *)optm->list.psz[i];
             var_Change(vout, "deinterlace-mode", HEIMDALL_VAR_ADDCHOICE,
                        val, heimdall_gettext(optm->list_text[i]));
         }
    var_AddCallback(vout, "deinterlace-mode", DeinterlaceCallback, NULL);
    /* */
    var_Create(vout, "deinterlace-needed", HEIMDALL_VAR_BOOL);
    var_AddCallback(vout, "deinterlace-needed", DeinterlaceCallback, NULL);

    /* */
    val.psz_string = deinterlace_mode ? deinterlace_mode : optm->orig.psz;
    var_Change(vout, "deinterlace-mode", HEIMDALL_VAR_SETVALUE, val);

    var_SetInteger(vout, "deinterlace", deinterlace_state);
    free(deinterlace_mode);

    var_Create(vout, "sout-deinterlace-mode", HEIMDALL_VAR_STRING);

    sys->is_interlaced = false;
}

void vout_ReinitInterlacingSupport(vout_thread_t *vout, vout_interlacing_state_t *sys)
{
    sys->is_interlaced = false;
    var_SetBool(vout, "deinterlace-needed", false);
}

void vout_SetInterlacingState(vout_thread_t *vout, vout_interlacing_state_t *sys, bool is_interlaced)
{
    const bool interlacing_change =
        is_interlaced != sys->is_interlaced;

    /* Wait 30s before quitting interlacing mode */
    const bool is_after_deadline =
        sys->date + HEIMDALL_TICK_FROM_SEC(30) < heimdall_tick_now();

    if (interlacing_change && (is_interlaced || is_after_deadline))
    {
        msg_Dbg(vout, "Detected %s video",
                 is_interlaced ? "interlaced" : "progressive");
        var_SetBool(vout, "deinterlace-needed", is_interlaced);
        sys->is_interlaced = is_interlaced;
    }
    if (is_interlaced)
        sys->date = heimdall_tick_now();
}
