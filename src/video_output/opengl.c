/*****************************************************************************
 * opengl.c: Heimdall GL API
 *****************************************************************************
 * Copyright (C) 2011 Rémi Denis-Courmont
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
#include <heimdall_atomic.h>
#include <heimdall_opengl.h>
#include <heimdall_codec.h>
#include <heimdall_vout_display.h>
#include "../libheimdall.h"
#include <heimdall_modules.h>

static const struct heimdall_gl_cfg gl_cfg_default = {
    .need_alpha = false
};

struct heimdall_gl_priv_t
{
    heimdall_gl_t gl;
};

static int heimdall_gl_start(void *func, bool forced, va_list ap)
{
    heimdall_gl_activate activate = func;
    heimdall_gl_t *gl = va_arg(ap, heimdall_gl_t *);
    unsigned width = va_arg(ap, unsigned);
    unsigned height = va_arg(ap, unsigned);
    const struct heimdall_gl_cfg *gl_cfg = va_arg(ap, const struct heimdall_gl_cfg *);

    int ret = activate(gl, width, height, gl_cfg);
    if (ret)
        heimdall_objres_clear(HEIMDALL_OBJECT(gl));
    (void) forced;
    return ret;
}

heimdall_gl_t *heimdall_gl_Create(const struct vout_display_cfg *restrict cfg,
                        unsigned flags, const char *name,
                        const struct heimdall_gl_cfg * gl_cfg)
{
    heimdall_window_t *wnd = cfg->window;
    struct heimdall_gl_priv_t *glpriv;
    const char *type;
    if (gl_cfg == NULL)
        gl_cfg = &gl_cfg_default;

    enum heimdall_gl_api_type api_type;

    switch (flags /*& HEIMDALL_OPENGL_API_MASK*/)
    {
        case HEIMDALL_OPENGL:
            type = "opengl";
            api_type = HEIMDALL_OPENGL;
            break;
        case HEIMDALL_OPENGL_ES2:
            type = "opengl es2";
            api_type = HEIMDALL_OPENGL_ES2;
            break;
        default:
            return NULL;
    }

    glpriv = heimdall_custom_create(HEIMDALL_OBJECT(wnd), sizeof (*glpriv), "gl");
    if (unlikely(glpriv == NULL))
        return NULL;

    heimdall_gl_t *gl = &glpriv->gl;
    gl->api_type = api_type;
    gl->orientation = ORIENT_NORMAL;
    gl->surface = wnd;
    gl->device = NULL;

    gl->module = heimdall_module_load(heimdall_object_logger(gl), type, name, true,
                                 heimdall_gl_start, gl,
                                 cfg->display.width, cfg->display.height, gl_cfg);
    if (gl->module == NULL)
    {
        heimdall_object_delete(gl);
        return NULL;
    }

    assert(gl->ops);
    assert(gl->ops->make_current);
    assert(gl->ops->release_current);
    assert(gl->ops->swap);
    assert(gl->ops->get_proc_address);

    return &glpriv->gl;
}

heimdall_gl_t *heimdall_gl_CreateOffscreen(heimdall_object_t *parent,
                                 struct heimdall_decoder_device *device,
                                 unsigned width, unsigned height,
                                 unsigned flags, const char *name,
                                 const struct heimdall_gl_cfg *gl_cfg)
{
    struct heimdall_gl_priv_t *glpriv;
    const char *type;

    enum heimdall_gl_api_type api_type;
    if (gl_cfg == NULL)
        gl_cfg = &gl_cfg_default;

    switch (flags /*& HEIMDALL_OPENGL_API_MASK*/)
    {
        case HEIMDALL_OPENGL:
            type = "opengl offscreen";
            api_type = HEIMDALL_OPENGL;
            break;
        case HEIMDALL_OPENGL_ES2:
            type = "opengl es2 offscreen";
            api_type = HEIMDALL_OPENGL_ES2;
            break;
        default:
            return NULL;
    }

    glpriv = heimdall_custom_create(parent, sizeof (*glpriv), "gl");
    if (unlikely(glpriv == NULL))
        return NULL;

    heimdall_gl_t *gl = &glpriv->gl;

    gl->api_type = api_type;
    gl->orientation = ORIENT_NORMAL;

    gl->offscreen_chroma_out = HEIMDALL_CODEC_UNKNOWN;
    gl->offscreen_vctx_out = NULL;

    gl->surface = NULL;
    gl->device = device ? heimdall_decoder_device_Hold(device) : NULL;
    gl->module = heimdall_module_load(heimdall_object_logger(gl), type, name, true,
                                 heimdall_gl_start, gl, width, height, gl_cfg);
    if (gl->module == NULL)
    {
        if (gl->device)
            heimdall_decoder_device_Release(gl->device);
        heimdall_object_delete(gl);
        return NULL;
    }

    /* The implementation must initialize the output chroma */
    assert(gl->offscreen_chroma_out != HEIMDALL_CODEC_UNKNOWN);

    assert(gl->ops);
    assert(gl->ops->make_current);
    assert(gl->ops->release_current);
    assert(gl->ops->swap_offscreen);
    assert(gl->ops->get_proc_address);

    return &glpriv->gl;
}

void heimdall_gl_Delete(heimdall_gl_t *gl)
{
    if (gl->ops->close != NULL)
        gl->ops->close(gl);

    if (gl->device)
        heimdall_decoder_device_Release(gl->device);

    heimdall_objres_clear(HEIMDALL_OBJECT(gl));
    heimdall_object_delete(gl);
}

#include <heimdall_window.h>

typedef struct heimdall_gl_surface
{
    int width;
    int height;
    heimdall_mutex_t lock;
} heimdall_gl_surface_t;

static void heimdall_gl_surface_ResizeNotify(heimdall_window_t *surface,
                                        unsigned width, unsigned height,
                                        heimdall_window_ack_cb cb, void *opaque)
{
    heimdall_gl_surface_t *sys = surface->owner.sys;

    msg_Dbg(surface, "resized to %ux%u", width, height);

    heimdall_mutex_lock(&sys->lock);
    sys->width = width;
    sys->height = height;

    if (cb != NULL)
        cb(surface, width, height, opaque);
    heimdall_mutex_unlock(&sys->lock);
}

heimdall_gl_t *heimdall_gl_surface_Create(heimdall_object_t *obj,
                                const heimdall_window_cfg_t *cfg,
                                struct heimdall_window **restrict wp,
                                const struct heimdall_gl_cfg *gl_cfg)
{
    heimdall_gl_surface_t *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return NULL;

    sys->width = cfg->width;
    sys->height = cfg->height;
    heimdall_mutex_init(&sys->lock);

    static const struct heimdall_window_callbacks cbs = {
        .resized = heimdall_gl_surface_ResizeNotify,
    };
    heimdall_window_owner_t owner = {
        .cbs = &cbs,
        .sys = sys,
    };
    char *modlist = var_InheritString(obj, "window");

    heimdall_window_t *surface = heimdall_window_New(obj, modlist, &owner, cfg);
    free(modlist);
    if (surface == NULL)
        goto error;
    if (heimdall_window_Enable(surface)) {
        heimdall_window_Delete(surface);
        goto error;
    }
    if (wp != NULL)
        *wp = surface;

    /* TODO: support ES? */
    struct vout_display_cfg dcfg = {
        .window = surface,
        .display = { .width = cfg->width, cfg->height },
    };

    heimdall_mutex_lock(&sys->lock);
    if (sys->width >= 0 && sys->height >= 0) {
        dcfg.display.width = sys->width;
        dcfg.display.height = sys->height;
        sys->width = -1;
        sys->height = -1;
    }
    heimdall_mutex_unlock(&sys->lock);

    heimdall_gl_t *gl = heimdall_gl_Create(&dcfg, HEIMDALL_OPENGL, NULL, gl_cfg);
    if (gl == NULL) {
        heimdall_window_Disable(surface);
        heimdall_window_Delete(surface);
        goto error;
    }

    return gl;

error:
    free(sys);
    return NULL;
}

/**
 * Checks if the dimensions of the surface used by the OpenGL context have
 * changed (since the previous call), and  the OpenGL viewport should be
 * updated.
 * \return true if at least one dimension has changed, false otherwise
 * \warning This function is intrinsically race-prone.
 * The dimensions can change asynchronously.
 */
bool heimdall_gl_surface_CheckSize(heimdall_gl_t *gl, unsigned *restrict width,
                              unsigned *restrict height)
{
    heimdall_window_t *surface = gl->surface;
    heimdall_gl_surface_t *sys = surface->owner.sys;
    bool ret = false;

    heimdall_mutex_lock(&sys->lock);
    if (sys->width >= 0 && sys->height >= 0)
    {
        *width = sys->width;
        *height = sys->height;
        sys->width = -1;
        sys->height = -1;

        heimdall_gl_Resize(gl, *width, *height);
        ret = true;
    }
    heimdall_mutex_unlock(&sys->lock);
    return ret;
}

void heimdall_gl_surface_Destroy(heimdall_gl_t *gl)
{
    heimdall_window_t *surface = gl->surface;
    heimdall_gl_surface_t *sys = surface->owner.sys;

    heimdall_gl_Delete(gl);
    heimdall_window_Disable(surface);
    heimdall_window_Delete(surface);
    free(sys);
}
