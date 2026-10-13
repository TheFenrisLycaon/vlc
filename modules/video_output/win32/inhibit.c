/*****************************************************************************
 * inhibit.c: Windows video output common code
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Steve Lhomme <robux4@ycbcr.xyz>
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

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_inhibit.h>
#include <heimdall_plugin.h>

#include <assert.h>
#include <windows.h>

struct heimdall_inhibit_sys
{
    heimdall_mutex_t mutex;
    heimdall_cond_t cond;
    heimdall_thread_t thread;
    unsigned int mask;
    bool exit;
};

static void Inhibit (heimdall_inhibit_t *ih, unsigned mask)
{
    heimdall_inhibit_sys_t *sys = ih->p_sys;
    heimdall_mutex_lock(&sys->mutex);
    sys->mask = mask;
    heimdall_cond_signal(&sys->cond);
    heimdall_mutex_unlock(&sys->mutex);
}

static void* Run(void* obj)
{
    heimdall_thread_set_name("heimdall-inhibit-win");

    heimdall_inhibit_t *ih = (heimdall_inhibit_t*)obj;
    heimdall_inhibit_sys_t *sys = ih->p_sys;

    heimdall_mutex_lock(&sys->mutex);
    while (!sys->exit)
    {
        EXECUTION_STATE state = ES_CONTINUOUS;

        if (sys->mask & HEIMDALL_INHIBIT_DISPLAY)
            /* Prevent monitor from powering off */
            state |= ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED;

        SetThreadExecutionState(state);
        heimdall_cond_wait(&sys->cond, &sys->mutex);
    }
    heimdall_mutex_unlock(&sys->mutex);
    SetThreadExecutionState(ES_CONTINUOUS);
    return NULL;
}

static void CloseInhibit (heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t*)obj;
    heimdall_inhibit_sys_t* sys = ih->p_sys;
    heimdall_mutex_lock(&sys->mutex);
    sys->exit = true;
    heimdall_cond_signal(&sys->cond);
    heimdall_mutex_unlock(&sys->mutex);
    heimdall_join(sys->thread, NULL);
}

static int OpenInhibit (heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t *)obj;
    heimdall_inhibit_sys_t *sys = ih->p_sys =
            heimdall_obj_malloc(obj, sizeof(heimdall_inhibit_sys_t));
    if (unlikely(ih->p_sys == NULL))
        return HEIMDALL_ENOMEM;

    heimdall_mutex_init(&sys->mutex);
    heimdall_cond_init(&sys->cond);
    sys->mask = 0;
    sys->exit = false;

    /* SetThreadExecutionState always needs to be called from the same thread */
    if (heimdall_clone(&sys->thread, Run, ih))
        return HEIMDALL_EGENERIC;

    ih->inhibit = Inhibit;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin ()
    set_shortname (N_("Windows screensaver"))
    set_description (N_("Windows screen saver inhibition"))
    set_subcategory (SUBCAT_ADVANCED_MISC)
    set_capability ("inhibit", 10)
    set_callbacks (OpenInhibit, CloseInhibit)
heimdall_module_end ()
