/*****************************************************************************
 * kms_drm.c : Direct rendering management plugin for heimdall
 *****************************************************************************
 * Copyright © 2018 Intel Corporation
 * Copyright © 2021 Videolabs
 * Copyright © 2022 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_LIBDRM
# include <drm/drm_mode.h>
#else
# include <drm_mode.h>
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_vout_display.h>
#include <heimdall_picture.h>
#include <heimdall_window.h>
#include "heimdall_drm.h"

#include <assert.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

#define DRM_CHROMA_TEXT "Image format used by DRM"
#define DRM_CHROMA_LONGTEXT "Chroma fourcc override for DRM framebuffer format selection"

/*
 * how many hw buffers are allocated for page flipping. I think
 * 3 is enough so we shouldn't get unexpected stall from kernel.
 */
#define   MAXHWBUF 3

typedef struct vout_display_sys_t {
    picture_t       *buffers[MAXHWBUF];

    unsigned int    front_buf;
/*
 * modeset information
 */
    uint32_t        plane_id;
} vout_display_sys_t;

static void Prepare(vout_display_t *vd, picture_t *pic,
                    const struct heimdall_render_subpicture *subpic,
                    heimdall_tick_t date)
{
    HEIMDALL_UNUSED(subpic); HEIMDALL_UNUSED(date);
    vout_display_sys_t *sys = vd->sys;

    picture_Copy(sys->buffers[sys->front_buf], pic);
}

static void Display(vout_display_t *vd, picture_t *picture)
{
    HEIMDALL_UNUSED(picture);
    vout_display_sys_t *sys = vd->sys;
    heimdall_window_t *wnd = vd->cfg->window;
    const video_format_t *fmt = vd->fmt;
    picture_t *pic = sys->buffers[sys->front_buf];

    struct drm_mode_set_plane sp = {
        .plane_id = sys->plane_id,
        .crtc_id = wnd->handle.crtc,
        .fb_id = heimdall_drm_dumb_get_fb_id(pic),
        .crtc_x = vd->place->x,
        .crtc_y = vd->place->y,
        .crtc_w = vd->place->width,
        .crtc_h = vd->place->height,
        /* Source values as U16.16 fixed point */
        .src_x = fmt->i_x_offset << 16,
        .src_y = fmt->i_y_offset << 16,
        .src_w = fmt->i_visible_width << 16,
        .src_h = fmt->i_visible_height << 16,
    };

    if (heimdall_drm_ioctl(wnd->display.drm_fd, DRM_IOCTL_MODE_SETPLANE, &sp) < 0) {
        msg_Err(vd, "DRM plane setting error: %s", heimdall_strerror_c(errno));
        return;
    }

    sys->front_buf++;
    if (sys->front_buf == MAXHWBUF)
        sys->front_buf = 0;
}

/**
 * Terminate an output method created by Open
 */
static void Close(vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;

    for (size_t i = 0; i < ARRAY_SIZE(sys->buffers); i++)
        picture_Release(sys->buffers[i]);
}

static const struct heimdall_display_operations ops = {
    .close = Close,
    .prepare = Prepare,
    .display = Display,
};

/**
 * This function allocates and initializes a KMS vout method.
 */
static int Open(vout_display_t *vd,
                video_format_t *fmtp, heimdall_video_context *context)
{
    heimdall_window_t *wnd = vd->cfg->window;
    uint_fast32_t drm_fourcc = 0;
    video_format_t fmt;

    if (wnd->type != HEIMDALL_WINDOW_TYPE_KMS)
        return HEIMDALL_EGENERIC;

    /*
     * Allocate instance and initialize some members
     */
    vout_display_sys_t *sys = heimdall_obj_malloc(HEIMDALL_OBJECT(vd), sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    char *chroma = var_InheritString(vd, "kms-drm-chroma");
    if (chroma) {
        memcpy(&drm_fourcc, chroma, strnlen(chroma, sizeof (drm_fourcc)));
        msg_Dbg(vd, "Setting DRM chroma to '%4s'", chroma);
        free(chroma);
    }

    int fd = wnd->display.drm_fd;

    int crtc_index = heimdall_drm_get_crtc_index(fd, wnd->handle.crtc);
    if (crtc_index < 0) {
        msg_Err(vd, "DRM CRTC object ID %"PRIu32" error: %s",
                wnd->handle.crtc, heimdall_strerror_c(errno));
        return -errno;
    }

    msg_Dbg(vd, "using DRM CRTC object ID %"PRIu32", index %d",
            wnd->handle.crtc, crtc_index);

    size_t nfmt;
    sys->plane_id = heimdall_drm_get_crtc_primary_plane(fd, crtc_index, &nfmt);
    if (sys->plane_id == 0) {
        /* Most likely the window provider failed to set universal mode, or
         * failed to lease a primary plane. */
        msg_Err(vd, "DRM primary plane not found: %s", heimdall_strerror_c(errno));
        return -errno;
    }

    msg_Dbg(vd, "using DRM plane ID %"PRIu32, sys->plane_id);

    if (drm_fourcc == 0) {
        drm_fourcc = heimdall_drm_find_best_format(fd, sys->plane_id, nfmt,
                                              vd->source->i_chroma);
        if (drm_fourcc == 0) {
            msg_Err(vd, "DRM plane format error: %s", heimdall_strerror_c(errno));
            return -errno;
        }
    }

    msg_Dbg(vd, "using DRM pixel format %4.4s (0x%08"PRIXFAST32")",
            (char *)&drm_fourcc, drm_fourcc);

    video_format_ApplyRotation(&fmt, vd->source);
    heimdall_fourcc_t heimdall_fourcc = heimdall_fourcc_drm(drm_fourcc);
    if (heimdall_fourcc == 0) {
        /* This can only occur if $heimdall-drm-chroma is unknown. */
        assert(chroma != NULL);
        msg_Err(vd, "unknown DRM pixel format %4.4s (0x%08"PRIXFAST32")",
                (char *)&drm_fourcc, drm_fourcc);
        return -ENOTSUP;
    }
    fmt.i_chroma = heimdall_fourcc;

    for (size_t i = 0; i < ARRAY_SIZE(sys->buffers); i++) {
        sys->buffers[i] = heimdall_drm_dumb_alloc_fb(vd->obj.logger, fd, &fmt);
        if (sys->buffers[i] == NULL) {
            while (i > 0)
                picture_Release(sys->buffers[--i]);
            return -ENOBUFS;
        }
    }

    sys->front_buf = 0;
    *fmtp = fmt;
    vd->sys = sys;
    vd->ops = &ops;

    (void) context;
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
heimdall_module_begin ()
    set_shortname("drm")
    /* Keep kms here for compatibility with previous video output. */
    add_shortcut("drm", "kms_drm", "kms")
    set_subcategory(SUBCAT_VIDEO_VOUT)

    add_obsolete_string("kms-heimdall-chroma") /* since 4.0.0 */
    add_string( "kms-drm-chroma", NULL, DRM_CHROMA_TEXT, DRM_CHROMA_LONGTEXT)
    set_description("Direct rendering management video output")
    set_callback_display(Open, 30)
heimdall_module_end ()
