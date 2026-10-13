/**
 * @file splitter.c
 * @brief Video splitter video output module for Heimdall media player
 */
/*****************************************************************************
 * Copyright © 2009 Laurent Aimar
 * Copyright © 2009-2018 Rémi Denis-Courmont
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

#include <assert.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_modules.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_vout_display.h>
#include <heimdall_video_splitter.h>

struct heimdall_vidsplit_part {
    heimdall_window_t *window;
    vout_display_t *display;
    heimdall_sem_t lock;
    unsigned width;
    unsigned height;
};

typedef struct vout_display_sys_t {
    video_splitter_t splitter;
    heimdall_mutex_t lock;

    picture_t **pictures;
    struct heimdall_vidsplit_part *parts;
} vout_display_sys_t;

static void heimdall_vidsplit_Prepare(vout_display_t *vd, picture_t *pic,
                                 const heimdall_render_subpicture *subpic, heimdall_tick_t date)
{
    vout_display_sys_t *sys = vd->sys;

    picture_Hold(pic);
    (void) subpic;

    heimdall_mutex_lock(&sys->lock);
    if (video_splitter_Filter(&sys->splitter, sys->pictures, pic)) {
        heimdall_mutex_unlock(&sys->lock);

        for (int i = 0; i < sys->splitter.i_output; i++)
            sys->pictures[i] = NULL;
        return;
    }
    heimdall_mutex_unlock(&sys->lock);

    for (int i = 0; i < sys->splitter.i_output; i++) {
        struct heimdall_vidsplit_part *part = &sys->parts[i];

        heimdall_sem_wait(&part->lock);
        sys->pictures[i] = vout_display_Prepare(part->display,
                                                sys->pictures[i], NULL, date);
    }
}

static void heimdall_vidsplit_Display(vout_display_t *vd, picture_t *picture)
{
    vout_display_sys_t *sys = vd->sys;

    for (int i = 0; i < sys->splitter.i_output; i++) {
        struct heimdall_vidsplit_part *part = &sys->parts[i];

        if (sys->pictures[i] != NULL)
        {
            vout_display_Display(part->display, sys->pictures[i]);
            picture_Release(sys->pictures[i]);
        }
        heimdall_sem_post(&part->lock);
    }

    (void) picture;
}

static void heimdall_vidsplit_Close(vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;
    int n = sys->splitter.i_output;

    for (int i = 0; i < n; i++) {
        struct heimdall_vidsplit_part *part = &sys->parts[i];
        vout_display_t *display;

        heimdall_sem_wait(&part->lock);
        display = part->display;
        part->display = NULL;
        heimdall_sem_post(&part->lock);

        if (display != NULL)
            vout_display_Delete(display);

        heimdall_window_Disable(part->window);
        heimdall_window_Delete(part->window);
    }

    module_unneed(&sys->splitter, sys->splitter.p_module);
    video_format_Clean(&sys->splitter.fmt);
    heimdall_object_delete(&sys->splitter);
}

static void heimdall_vidsplit_window_Resized(heimdall_window_t *wnd,
                                        unsigned width, unsigned height,
                                        heimdall_window_ack_cb cb, void *opaque)
{
    struct heimdall_vidsplit_part *part = wnd->owner.sys;

    heimdall_sem_wait(&part->lock);
    part->width = width;
    part->height = height;

    if (part->display != NULL)
        vout_display_SetSize(part->display, width, height);

    if (cb != NULL)
        cb(wnd, width, height, opaque);
    heimdall_sem_post(&part->lock);
}

static void heimdall_vidsplit_window_Closed(heimdall_window_t *wnd)
{
    struct heimdall_vidsplit_part *part = wnd->owner.sys;
    vout_display_t *display;

    heimdall_sem_wait(&part->lock);
    display = part->display;
    part->display = NULL;
    heimdall_sem_post(&part->lock);

    if (display != NULL)
        vout_display_Delete(display);
}

static void heimdall_vidsplit_window_MouseEvent(heimdall_window_t *wnd,
                                           const heimdall_window_mouse_event_t *e)
{
    struct heimdall_vidsplit_part *part = wnd->owner.sys;
    vout_display_t *vd = (vout_display_t *)heimdall_object_parent(wnd);
    vout_display_sys_t *sys = vd->sys;
    heimdall_window_mouse_event_t ev = *e;

    heimdall_mutex_lock(&sys->lock);
    if (video_splitter_Mouse(&sys->splitter, part - sys->parts,
                             &ev) == HEIMDALL_SUCCESS)
        heimdall_window_SendMouseEvent(vd->cfg->window, &ev);
    heimdall_mutex_unlock(&sys->lock);
}

static void heimdall_vidsplit_window_KeyboardEvent(heimdall_window_t *wnd, unsigned key)
{
    vout_display_t *vd = (vout_display_t *)heimdall_object_parent(wnd);
    vout_display_sys_t *sys = vd->sys;

    heimdall_mutex_lock(&sys->lock);
    heimdall_window_ReportKeyPress(vd->cfg->window, key);
    heimdall_mutex_unlock(&sys->lock);
}

static const struct heimdall_window_callbacks heimdall_vidsplit_window_cbs = {
    .resized = heimdall_vidsplit_window_Resized,
    .closed = heimdall_vidsplit_window_Closed,
    .mouse_event = heimdall_vidsplit_window_MouseEvent,
    .keyboard_event = heimdall_vidsplit_window_KeyboardEvent,
};

static heimdall_window_t *video_splitter_CreateWindow(heimdall_object_t *obj,
    const vout_display_cfg_t *restrict vdcfg,
    const video_format_t *restrict source, void *sys)
{
    heimdall_window_cfg_t cfg = {
        .is_decorated = true,
    };
    heimdall_window_owner_t owner = {
        .cbs = &heimdall_vidsplit_window_cbs,
        .sys = sys,
    };

    vout_display_GetDefaultDisplaySize(&cfg.width, &cfg.height, source,
                                       &vdcfg->display);

    heimdall_window_t *window = heimdall_window_New(obj, NULL, &owner, &cfg);
    if (window != NULL) {
        if (heimdall_window_Enable(window)) {
            heimdall_window_Delete(window);
            window = NULL;
        }
    }
    return window;
}

static const struct heimdall_display_operations ops = {
    .close = heimdall_vidsplit_Close,
    .prepare = heimdall_vidsplit_Prepare,
    .display = heimdall_vidsplit_Display,
};

static int heimdall_vidsplit_Open(vout_display_t *vd,
                             video_format_t *fmtp, heimdall_video_context *ctx)
{
    heimdall_object_t *obj = HEIMDALL_OBJECT(vd);

    if (vout_display_cfg_IsWindowed(vd->cfg))
        return HEIMDALL_EGENERIC;

    char *name = var_InheritString(obj, "video-splitter");
    if (name == NULL)
        return HEIMDALL_EGENERIC;

    vout_display_sys_t *sys = heimdall_object_create(obj, sizeof (*sys));
    if (unlikely(sys == NULL)) {
        free(name);
        return HEIMDALL_ENOMEM;
    }
    vd->sys = sys;

    video_splitter_t *splitter = &sys->splitter;

    heimdall_mutex_init(&sys->lock);
    video_format_Copy(&splitter->fmt, vd->source);
    splitter->fmt.orientation = ORIENT_NORMAL;
    fmtp->orientation = ORIENT_NORMAL;

    splitter->p_module = module_need(splitter, "video splitter", name, true);
    free(name);
    if (splitter->p_module == NULL) {
        video_format_Clean(&splitter->fmt);
        heimdall_object_delete(splitter);
        return HEIMDALL_EGENERIC;
    }

    sys->pictures = heimdall_obj_malloc(obj, splitter->i_output
                                        * sizeof (*sys->pictures));
    sys->parts = heimdall_obj_malloc(obj,
                                splitter->i_output * sizeof (*sys->parts));
    if (unlikely(sys->pictures == NULL || sys->parts == NULL)) {
        splitter->i_output = 0;
        heimdall_vidsplit_Close(vd);
        return HEIMDALL_ENOMEM;
    }

    for (int i = 0; i < splitter->i_output; i++) {
        const video_splitter_output_t *output = &splitter->p_output[i];
        vout_display_cfg_t vdcfg = {
            .display = {
                .width = 0,
                .height = 0,
                .sar = { 1, 1 },
                .align = { 0, 0 } /* TODO */,
                .fitting = HEIMDALL_VIDEO_FIT_SMALLER,
                .zoom = { 1, 1 },
                .full_fill = true,
            },
        };
        const char *modname = output->psz_module;
        struct heimdall_vidsplit_part *part = &sys->parts[i];

        heimdall_sem_init(&part->lock, 1);
        part->display = NULL;
        part->width = 1;
        part->height = 1;

        part->window = video_splitter_CreateWindow(obj, &vdcfg, &output->fmt,
                                                   part);
        if (part->window == NULL) {
            splitter->i_output = i;
            heimdall_vidsplit_Close(vd);
            return HEIMDALL_EGENERIC;
        }

        vdcfg.window = part->window;
        heimdall_sem_wait(&part->lock);
        vdcfg.display.width = part->width;
        vdcfg.display.height = part->height;

        vout_display_t *display = vout_display_New(obj, &output->fmt, ctx, &vdcfg,
                                                   modname, NULL);
        if (display == NULL) {
            heimdall_sem_post(&part->lock);
            heimdall_window_Disable(part->window);
            heimdall_window_Delete(part->window);
            splitter->i_output = i;
            heimdall_vidsplit_Close(vd);
            return HEIMDALL_EGENERIC;
        }

        part->display = display;
        heimdall_sem_post(&part->lock);
    }

    vd->ops = &ops;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    add_shortcut("splitter")
    set_shortname(N_("Splitter"))
    set_description(N_("Video splitter display plugin"))
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_callback_display(heimdall_vidsplit_Open, 0)
    add_module("video-splitter", "video splitter", "none",
               N_("Video splitter module"), NULL)
heimdall_module_end()
