/*****************************************************************************
 * egl_display.c
 *****************************************************************************
 * Copyright (C) 2021 Videolabs
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
# include "config.h"
#endif

#include <assert.h>
#include <heimdall_common.h>
#include <heimdall_modules.h>

#include "egl_display.h"

struct heimdall_egl_display *
heimdall_egl_display_New(heimdall_object_t *parent, const char *name)
{
    struct heimdall_egl_display *display =
        heimdall_object_create(parent, sizeof(*display));
    if (!display)
        return NULL;

    display->ops = NULL;
    display->display = EGL_NO_DISPLAY;

    if (!name || !*name)
        name = "any";

    module_t **mods;
    ssize_t total = heimdall_module_match("egl display", name, true, &mods, NULL);
    for (ssize_t i = 0; i < total; ++i)
    {
        heimdall_egl_display_open_fn *open = heimdall_module_map(parent->logger, mods[i]);

        if (open && open(display) == HEIMDALL_SUCCESS)
        {
            assert(display->display != EGL_NO_DISPLAY);
            free(mods);
            return display;
        }
    }

    free(mods);
    heimdall_object_delete(display);
    return NULL;
}

void heimdall_egl_display_Delete(struct heimdall_egl_display *display)
{
    if (display->ops && display->ops->close) {
        display->ops->close(display);
    }

    heimdall_object_delete(display);
}
