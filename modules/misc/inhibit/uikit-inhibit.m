/*****************************************************************************
 * Copyright © 2021 Videolabs
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
 * \file uikit-inhibit.m
 * \brief iOS display and idle sleep inhibitor using UIKit
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_inhibit.h>
#include <heimdall_window.h>

#include <UIKit/UIKit.h>

static void UpdateInhibit(heimdall_inhibit_t *ih, unsigned mask)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [UIApplication sharedApplication].idleTimerDisabled =
            (mask & HEIMDALL_INHIBIT_DISPLAY) == HEIMDALL_INHIBIT_DISPLAY;
    });
}

static int OpenInhibit(heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t *)obj;
    heimdall_window_t *wnd = heimdall_inhibit_GetWindow(ih);
    if (wnd->type != HEIMDALL_WINDOW_TYPE_NSOBJECT)
        return HEIMDALL_EGENERIC;

    UIView * view = (__bridge UIView*)wnd->handle.nsobject;

    if (unlikely(![view respondsToSelector:@selector(isKindOfClass:)]))
        return HEIMDALL_EGENERIC;

    if (![view isKindOfClass:[UIView class]])
        return HEIMDALL_EGENERIC;

    ih->inhibit = UpdateInhibit;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname("UIKit sleep inhibition")
    set_description("UIKit screen sleep inhibition for iOS and tvOS")
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("inhibit", 10)
    set_callback(OpenInhibit)
heimdall_module_end()
