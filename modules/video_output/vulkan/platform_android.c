/**
 * @file platform_android.c
 * @brief Vulkan platform-specific code for Android
 */
/*****************************************************************************
 * Copyright © 2018 Niklas Haas, Thomas Guillem
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>

#include "platform.h"
#include "../android/utils.h"

static void ClosePlatform(heimdall_vk_platform_t *vk);
static int CreateSurface(heimdall_vk_platform_t *vk, const heimdall_vk_instance_t *, VkSurfaceKHR *);
static const struct heimdall_vk_platform_operations platform_ops =
{
    .close = ClosePlatform,
    .create_surface = CreateSurface,
};

static int InitPlatform(heimdall_vk_platform_t *vk)
{
    if (vk->window->type != HEIMDALL_WINDOW_TYPE_ANDROID_NATIVE)
        return HEIMDALL_EGENERIC;

    vk->platform_ext = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
    vk->ops = &platform_ops;
    return HEIMDALL_SUCCESS;
}

static void ClosePlatform(heimdall_vk_platform_t *vk)
{
    AWindowHandler_releaseANativeWindow(vk->window->display.anativewindow,
                                        vk->window->handle.android_id);
}

static int CreateSurface(heimdall_vk_platform_t *vk, const heimdall_vk_instance_t *inst,
                         VkSurfaceKHR *surface_out)
{
    ANativeWindow *anw =
        AWindowHandler_getANativeWindow(vk->window->display.anativewindow,
                                        vk->window->handle.android_id);
    PFN_vkCreateAndroidSurfaceKHR CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)
        inst->get_proc_address(inst->instance, "vkCreateAndroidSurfaceKHR");

    VkAndroidSurfaceCreateInfoKHR ainfo = {
         .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
         .pNext = NULL,
         .flags = 0,
         .window = anw,
    };

    VkResult res = CreateAndroidSurfaceKHR(inst->instance, &ainfo, NULL, surface_out);
    if (res != VK_SUCCESS) {
        msg_Err(vk, "Failed creating Android surface");
        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname("Vulkan Android")
    set_description(N_("Android platform support for Vulkan"))
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_capability("vulkan platform", 50)
    set_callback(InitPlatform)
    add_shortcut("vk_android")
heimdall_module_end()
