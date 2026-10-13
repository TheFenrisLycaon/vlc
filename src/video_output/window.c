/*****************************************************************************
 * window.c: generic window management
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright © 2009-2021 Rémi Denis-Courmont
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_window.h>
#include <heimdall_modules.h>
#include "inhibit.h"
#include "../libheimdall.h"

typedef struct
{
    heimdall_window_t wnd;
    heimdall_window_cfg_t cfg;
    module_t *module;
    bool inhibit_windowed;

    /* Screensaver inhibition state (protected by lock) */
    heimdall_inhibit_t *inhibit;
    bool active;
    bool fullscreen;
    heimdall_mutex_t lock;
} window_t;

static int heimdall_window_start(void *func, bool forced, va_list ap)
{
    int (*activate)(heimdall_window_t *) = func;
    heimdall_window_t *wnd = va_arg(ap, heimdall_window_t *);

    int ret = activate(wnd);
    if (ret)
        heimdall_objres_clear(HEIMDALL_OBJECT(wnd));
    (void) forced;
    return ret;
}

heimdall_window_t *heimdall_window_New(heimdall_object_t *obj, const char *module,
                               const heimdall_window_owner_t *owner,
                               const heimdall_window_cfg_t *restrict cfg)
{
    window_t *w = heimdall_custom_create(obj, sizeof(*w), "window");
    heimdall_window_t *window = &w->wnd;

    memset(&window->handle, 0, sizeof(window->handle));
    window->info.has_double_click = false;
    window->sys = NULL;
    assert(owner != NULL);
    window->owner = *owner;

    w->cfg.is_fullscreen = false;
    w->cfg.is_decorated = true;
    w->cfg.width = 0;
    w->cfg.height = 0;

    int dss = var_InheritInteger(obj, "disable-screensaver");

    w->inhibit = NULL;
    w->inhibit_windowed = dss == 1;
    w->active = false;
    w->fullscreen = false;
    heimdall_mutex_init(&w->lock);

    w->module = heimdall_module_load(heimdall_object_logger(window), "vout window", module,
                                false, heimdall_window_start, window);
    if (!w->module) {
        heimdall_object_delete(window);
        return NULL;
    }

    /* Hook for screensaver inhibition */
    if (dss > 0) {
        heimdall_inhibit_t *inh = heimdall_inhibit_Create(HEIMDALL_OBJECT(window));

        heimdall_mutex_lock(&w->lock);
        w->inhibit = inh;
        heimdall_mutex_unlock(&w->lock);
    }

    /* Apply initial configuration */
    if (cfg != NULL) {
        if (cfg->is_fullscreen)
            heimdall_window_SetFullScreen(window, NULL);
        if (cfg->width != 0 && cfg->height != 0)
            heimdall_window_SetSize(window, cfg->width, cfg->height);

        /* This will be applied whence the window is enabled. */
        w->cfg.is_decorated = cfg->is_decorated;
    }

    return window;
}

int heimdall_window_Enable(heimdall_window_t *window)
{
    window_t *w = container_of(window, window_t, wnd);

    if (window->ops->enable != NULL) {
        int err = window->ops->enable(window, &w->cfg);
        if (err)
            return err;
    }

    heimdall_window_SetInhibition(window, true);
    return HEIMDALL_SUCCESS;
}

void heimdall_window_Disable(heimdall_window_t *window)
{
    heimdall_window_SetInhibition(window, false);

    if (window->ops->disable != NULL)
        window->ops->disable(window);
}

void heimdall_window_SetSize(heimdall_window_t *window, unsigned width,
                         unsigned height)
{
    window_t *w = container_of(window, window_t, wnd);

    w->cfg.width = width;
    w->cfg.height = height;

    if (window->ops->resize != NULL)
        window->ops->resize(window, width, height);
}

void heimdall_window_Delete(heimdall_window_t *window)
{
    if (!window)
        return;

    window_t *w = container_of(window, window_t, wnd);

    if (w->inhibit != NULL) {
        heimdall_inhibit_t *inh = w->inhibit;

        assert(!w->active);
        heimdall_mutex_lock(&w->lock);
        w->inhibit = NULL;
        heimdall_mutex_unlock(&w->lock);

        heimdall_inhibit_Destroy(inh);
    }

    if (window->ops->destroy != NULL)
        window->ops->destroy(window);

    heimdall_objres_clear(HEIMDALL_OBJECT(window));
    heimdall_object_delete(window);
}

static void heimdall_window_UpdateInhibitionUnlocked(heimdall_window_t *window)
{
    window_t *w = container_of(window, window_t, wnd);
    unsigned flags = HEIMDALL_INHIBIT_NONE;

    heimdall_mutex_assert(&w->lock);

    if (w->active && (w->inhibit_windowed || w->fullscreen))
        flags = HEIMDALL_INHIBIT_VIDEO;
    if (w->inhibit != NULL)
        heimdall_inhibit_Set(w->inhibit, flags);
}

void heimdall_window_SetInhibition(heimdall_window_t *window, bool enabled)
{
    window_t *w = container_of(window, window_t, wnd);

    heimdall_mutex_lock(&w->lock);
    w->active = enabled;

    heimdall_window_UpdateInhibitionUnlocked(window);
    heimdall_mutex_unlock(&w->lock);
}

void heimdall_window_ReportWindowed(heimdall_window_t *window)
{
    window_t *w = container_of(window, window_t, wnd);

    if (!w->inhibit_windowed) {
        heimdall_mutex_lock(&w->lock);
        w->fullscreen = false;

        heimdall_window_UpdateInhibitionUnlocked(window);
        heimdall_mutex_unlock(&w->lock);
    }

    if (window->owner.cbs->windowed != NULL)
        window->owner.cbs->windowed(window);
}

void heimdall_window_ReportFullscreen(heimdall_window_t *window, const char *id)
{
    window_t *w = container_of(window, window_t, wnd);

    if (!w->inhibit_windowed) {
        heimdall_mutex_lock(&w->lock);
        w->fullscreen = true;
        heimdall_window_UpdateInhibitionUnlocked(window);
        heimdall_mutex_unlock(&w->lock);
    }

    if (window->owner.cbs->fullscreened != NULL)
        window->owner.cbs->fullscreened(window, id);
}

void heimdall_window_UnsetFullScreen(heimdall_window_t *window)
{
    window_t *w = container_of(window, window_t, wnd);

    w->cfg.is_fullscreen = false;

    if (window->ops->unset_fullscreen != NULL)
        window->ops->unset_fullscreen(window);
}

void heimdall_window_SetFullScreen(heimdall_window_t *window, const char *id)
{
    window_t *w = container_of(window, window_t, wnd);

    w->cfg.is_fullscreen = true;

    if (window->ops->set_fullscreen != NULL)
        window->ops->set_fullscreen(window, id);
}
