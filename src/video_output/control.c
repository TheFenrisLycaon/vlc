/*****************************************************************************
 * control.c : vout internal control
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_vout.h>
#include "vout_internal.h"
#include "control.h"

/* */
void vout_control_Init(vout_control_t *ctrl)
{
    heimdall_mutex_init(&ctrl->lock);
    heimdall_cond_init(&ctrl->wait_request);
    heimdall_cond_init(&ctrl->wait_available);

    ctrl->is_held = false;
    ctrl->yielding = false;
    ctrl->forced_awake = false;
    ctrl->pending_count = 0;
}

void vout_control_Wake(vout_control_t *ctrl)
{
    heimdall_mutex_lock(&ctrl->lock);
    ctrl->forced_awake = true;
    heimdall_cond_signal(&ctrl->wait_request);
    heimdall_mutex_unlock(&ctrl->lock);
}

void vout_control_Hold(vout_control_t *ctrl)
{
    heimdall_mutex_lock(&ctrl->lock);
    ++ctrl->pending_count;
    heimdall_cond_signal(&ctrl->wait_request);
    while (ctrl->is_held || !ctrl->yielding)
        heimdall_cond_wait(&ctrl->wait_available, &ctrl->lock);
    ctrl->is_held = true;
    --ctrl->pending_count;
    if (ctrl->pending_count == 0)
        heimdall_cond_signal(&ctrl->wait_request);
    heimdall_mutex_unlock(&ctrl->lock);
}

static void vout_control_ReleaseUnlocked(vout_control_t *ctrl)
{
    assert(ctrl->is_held);
    ctrl->is_held = false;
    heimdall_cond_signal(&ctrl->wait_available);
}

void vout_control_Release(vout_control_t *ctrl)
{
    heimdall_mutex_lock(&ctrl->lock);
    vout_control_ReleaseUnlocked(ctrl);
    heimdall_mutex_unlock(&ctrl->lock);
}

void vout_control_ReleaseAndWake(vout_control_t *ctrl)
{
    heimdall_mutex_lock(&ctrl->lock);
    vout_control_ReleaseUnlocked(ctrl);
    ctrl->forced_awake = true;
    heimdall_cond_signal(&ctrl->wait_request);
    heimdall_mutex_unlock(&ctrl->lock);
}

void vout_control_Wait(vout_control_t *ctrl, heimdall_tick_t deadline)
{
    heimdall_mutex_lock(&ctrl->lock);

    ctrl->yielding = true;

    while (ctrl->pending_count != 0)
    {
        /* Let vout_control_Hold() callers pass */
        heimdall_cond_signal(&ctrl->wait_available);
        heimdall_cond_wait(&ctrl->wait_request, &ctrl->lock);
    }

    if (deadline != HEIMDALL_TICK_INVALID)
    {
        do
        {
            if (ctrl->forced_awake)
                break;

            heimdall_cond_signal(&ctrl->wait_available);
        }
        while (heimdall_cond_timedwait(&ctrl->wait_request, &ctrl->lock,
                                  deadline) == 0);
    }

    ctrl->yielding = false;

    while (ctrl->is_held)
        heimdall_cond_wait(&ctrl->wait_available, &ctrl->lock);

    ctrl->forced_awake = false;
    heimdall_mutex_unlock(&ctrl->lock);
}

