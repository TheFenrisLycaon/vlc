/*****************************************************************************
 * control.h : vout internal control
 *****************************************************************************
 * Copyright (C) 2009-2010 Laurent Aimar
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

#ifndef LIBHEIMDALL_VOUT_INTERNAL_CONTROL_H
#define LIBHEIMDALL_VOUT_INTERNAL_CONTROL_H

#include <heimdall_viewpoint.h>
#include <heimdall_threads.h>

/* */
typedef struct {
    heimdall_mutex_t lock;
    heimdall_cond_t wait_request;
    heimdall_cond_t wait_available; /* available: yielding && !is_held */

    /* */
    bool forced_awake;
    bool yielding;
    bool is_held;
    unsigned pending_count;
} vout_control_t;

/* */
void vout_control_Init(vout_control_t *);

/* controls outside of the vout thread */
void vout_control_Wake(vout_control_t *);
void vout_control_Hold(vout_control_t *);
void vout_control_Release(vout_control_t *);
void vout_control_ReleaseAndWake(vout_control_t *);

/* control inside of the vout thread */
void vout_control_Wait(vout_control_t *, heimdall_tick_t deadline);

#endif
