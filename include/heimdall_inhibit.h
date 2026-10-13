/*****************************************************************************
 * heimdall_inhibit.h: Heimdall screen saver inhibition
 *****************************************************************************
 * Copyright (C) 2009 Rémi Denis-Courmont
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

/**
 * \file
 * This file defines the interface for screen-saver inhibition modules
 */

#ifndef HEIMDALL_INHIBIT_H
# define HEIMDALL_INHIBIT_H 1

typedef struct heimdall_inhibit heimdall_inhibit_t;
typedef struct heimdall_inhibit_sys heimdall_inhibit_sys_t;

struct heimdall_window;

enum heimdall_inhibit_flags
{
    HEIMDALL_INHIBIT_NONE=0 /*< No inhibition */,
    HEIMDALL_INHIBIT_SUSPEND=0x1 /*< Processor is in use - do not suspend */,
    HEIMDALL_INHIBIT_DISPLAY=0x2 /*< Display is in use - do not blank/lock */,
#define HEIMDALL_INHIBIT_AUDIO (HEIMDALL_INHIBIT_SUSPEND)
#define HEIMDALL_INHIBIT_VIDEO (HEIMDALL_INHIBIT_SUSPEND|HEIMDALL_INHIBIT_DISPLAY)
};

struct heimdall_inhibit
{
    struct heimdall_object_t obj;

    heimdall_inhibit_sys_t *p_sys;
    void (*inhibit) (heimdall_inhibit_t *, unsigned flags);
};

static inline struct heimdall_window *heimdall_inhibit_GetWindow(heimdall_inhibit_t *ih)
{
    return (struct heimdall_window *)heimdall_object_parent(ih);
}

static inline void heimdall_inhibit_Set (heimdall_inhibit_t *ih, unsigned flags)
{
    ih->inhibit (ih, flags);
}

#endif
