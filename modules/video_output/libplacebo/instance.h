/*****************************************************************************
 * instance.h: libplacebo instance abstraction
 *****************************************************************************
 * Copyright (C) 2021 Niklas Haas
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

#ifndef HEIMDALL_LIBPLACEBO_INSTANCE_H
#define HEIMDALL_LIBPLACEBO_INSTANCE_H

#include <heimdall_common.h>
#include <heimdall_vout_display.h>

#include <libplacebo/swapchain.h>
#include <libplacebo/log.h>
#include <libplacebo/gpu.h>

struct heimdall_placebo_t;
struct heimdall_placebo_operations
{
    void (*close)(struct heimdall_placebo_t *);

    // For acquiring/releasing the context on the current thread. (Optional)
    int (*make_current)(struct heimdall_placebo_t *);
    void (*release_current)(struct heimdall_placebo_t *);

    void (*resize)(struct heimdall_placebo_t *, unsigned width, unsigned height);
};

typedef struct heimdall_placebo_system_t heimdall_placebo_system_t;

// Shared struct for libplacebo context / gpu / swapchain
typedef struct heimdall_placebo_t
{
    // fields internal to instance.c, should not be touched
    struct heimdall_object_t obj;
    heimdall_placebo_system_t *sys;

    pl_log log;
    pl_gpu gpu;
    pl_swapchain swapchain;
#if PL_API_VER >= 320
    pl_cache cache;
#endif

    const struct heimdall_placebo_operations *ops;
} heimdall_placebo_t;

heimdall_placebo_t *heimdall_placebo_Create(const vout_display_cfg_t *, const char*) HEIMDALL_USED;
void heimdall_placebo_Release(heimdall_placebo_t *);

// Needed around every `pl_gpu` / `pl_swapchain` operation
int heimdall_placebo_MakeCurrent(heimdall_placebo_t *);
void heimdall_placebo_ReleaseCurrent(heimdall_placebo_t *);

void heimdall_placebo_Resize(heimdall_placebo_t *, unsigned width, unsigned height);

#endif // HEIMDALL_LIBPLACEBO_INSTANCE_H
