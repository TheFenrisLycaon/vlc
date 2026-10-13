/*****************************************************************************
 * platform.h: Vulkan platform abstraction
 *****************************************************************************
 * Copyright (C) 2018 Niklas Haas
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

#ifndef HEIMDALL_VULKAN_PLATFORM_H
#define HEIMDALL_VULKAN_PLATFORM_H

#include <heimdall_common.h>
#include <heimdall_window.h>

#include "instance.h"

struct heimdall_vk_platform_t;
struct heimdall_vk_platform_operations
{
    void (*close)(struct heimdall_vk_platform_t *);
    int (*create_surface)(struct heimdall_vk_platform_t *, const heimdall_vk_instance_t *, VkSurfaceKHR *);
};


// Struct for platform-specific Vulkan state
typedef struct heimdall_vk_platform_t
{
    // set by platform.c
    struct heimdall_object_t obj;
    struct heimdall_window *window;
    module_t *module;

    // set by the platform
    void *platform_sys;
    const char *platform_ext;
    const struct heimdall_vk_platform_operations *ops;
} heimdall_vk_platform_t;

heimdall_vk_platform_t *heimdall_vk_platform_Create(struct heimdall_window *, const char *) HEIMDALL_USED;
void heimdall_vk_platform_Release(heimdall_vk_platform_t *);

// Create a vulkan surface and store it to `surface_out`
static inline int heimdall_vk_CreateSurface(heimdall_vk_platform_t * vk,
                                       const heimdall_vk_instance_t *instance,
                                       VkSurfaceKHR *surface_out)
{
    return vk->ops->create_surface(vk, instance, surface_out);
}

static inline void heimdall_vk_DestroySurface(const heimdall_vk_instance_t *inst,
                                         VkSurfaceKHR surface)
{
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)
        inst->get_proc_address(inst->instance, "vkDestroySurfaceKHR");

    DestroySurfaceKHR(inst->instance, surface, NULL);
}

#endif // HEIMDALL_VULKAN_PLATFORM_H
