/*****************************************************************************
 * egl_surfacetexture.c: OpenGL offscreen provider with SurfaceTexture
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
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_modules.h>
#include <heimdall_picture.h>
#include <heimdall_filter.h>
#include <heimdall_opengl.h>
#include <heimdall_vout_display.h>
#include <heimdall_picture_pool.h>
#include <heimdall_atomic.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../video_output/android/utils.h"
#include "../video_output/opengl/gl_api.h"
#include "../video_output/opengl/gl_util.h"

#define BUFFER_COUNT 3

struct video_ctx
{
    android_video_context_t android;
    picture_pool_t *pool;
    picture_t *pictures[BUFFER_COUNT];

    EGLDisplay display;
    EGLContext context;
};

struct picture_ctx
{
    struct picture_context_t context;
    EGLSurface surface;

    struct heimdall_asurfacetexture *texture;
};

#define PRIV(pic_ctx) container_of(pic_ctx, struct picture_ctx, context)

struct surfacetexture_sys
{
    android_video_context_t *avctx;

    video_format_t fmt_out;

    size_t current_flip;

    EGLConfig cfgv;

    picture_t *current_picture;
};

static inline struct video_ctx *GetVCtx(heimdall_gl_t *gl)
{
    return heimdall_video_context_GetPrivate(gl->offscreen_vctx_out,
                                        HEIMDALL_VIDEO_CONTEXT_AWINDOW);
}

static int MakeCurrent(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;
    struct video_ctx *vctx = GetVCtx(gl);

    /* We must always have a surface mapped. */
    assert(sys->current_picture);
    assert(sys->current_picture->context);

    struct picture_ctx *ctx = PRIV(sys->current_picture->context);

    if (eglMakeCurrent (vctx->display, ctx->surface, ctx->surface,
                        vctx->context) != EGL_TRUE)
        return HEIMDALL_EGENERIC;

    return HEIMDALL_SUCCESS;
}

static void ReleaseCurrent(heimdall_gl_t *gl)
{
    struct video_ctx *vctx = GetVCtx(gl);

    eglMakeCurrent(vctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                    EGL_NO_CONTEXT);
}

static void *GetSymbol(heimdall_gl_t *gl, const char *procname)
{
    (void) gl;
    return (void *)eglGetProcAddress(procname);
}

static picture_context_t *CopyPictureContext(picture_context_t *input)
{
    heimdall_video_context_Hold(input->vctx);
    return input;
}

static void DestroyPictureContext(picture_context_t *input)
{
    (void) input;
    /* video context is already released by picture_Release. */
}

static picture_context_t *CreatePictureContext(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;
    struct picture_ctx *ctx = malloc(sizeof(*ctx));
    struct video_ctx *vctx = GetVCtx(gl);

    ctx->texture = heimdall_asurfacetexture_New(gl->device->opaque, false);
    if (ctx->texture == NULL)
        goto error;

    struct ANativeWindow *window = ctx->texture->window;
    ANativeWindow_setBuffersGeometry(window, sys->fmt_out.i_width, sys->fmt_out.i_height,
                            AHARDWAREBUFFER_FORMAT_BLOB);

    /* Create a drawing surface */
    ctx->surface = eglCreateWindowSurface(vctx->display, sys->cfgv, window,
                                          NULL);
    if (ctx->surface == EGL_NO_SURFACE)
    {
        msg_Err(gl, "cannot create EGL window surface");
        heimdall_asurfacetexture_Delete(ctx->texture);
        goto error;
    }

    ctx->context.vctx = gl->offscreen_vctx_out;
    ctx->context.copy = CopyPictureContext;
    ctx->context.destroy = DestroyPictureContext;

    return &ctx->context;

error:
    free(ctx);
    return NULL;
}

static int InitEGL(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;
    struct video_ctx *vctx = GetVCtx(gl);

    vctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (vctx->display == EGL_NO_DISPLAY)
        return HEIMDALL_EGENERIC;

    /* Initialize EGL display */
    EGLint major, minor;
    if (eglInitialize(vctx->display, &major, &minor) != EGL_TRUE)
        goto error;
    msg_Dbg(gl, "EGL version %s by %s, API %s",
            eglQueryString(vctx->display, EGL_VERSION),
            eglQueryString(vctx->display, EGL_VENDOR),
            "OpenGL ES2"
            );

    const EGLint conf_attr[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE,
    };

    EGLConfig *cfgv = &sys->cfgv;

    EGLint cfgc;
    if (eglChooseConfig(vctx->display, conf_attr, cfgv, 1, &cfgc) != EGL_TRUE
            || cfgc == 0)
    {
        msg_Err(gl, "cannot choose EGL configuration");
        goto error;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        msg_Err(gl, "cannot bind EGL OPENGL ES API");
        goto error;
    }

    const EGLint ctx_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext ctx
        = eglCreateContext(vctx->display, sys->cfgv, EGL_NO_CONTEXT, ctx_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        msg_Err(gl, "cannot create EGL context");
        goto error;
    }

    vctx->context = ctx;

    return HEIMDALL_SUCCESS;

error:
    eglTerminate(vctx->display);
    return HEIMDALL_EGENERIC;
}

static picture_t *SwapOffscreen(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;
    struct video_ctx *vctx = GetVCtx(gl);

    picture_t *current_picture = sys->current_picture;
    assert(current_picture);
    assert(current_picture->context);

    /* Swap currently active surface */
    struct picture_ctx *current_ctx = PRIV(current_picture->context);

    eglSwapBuffers(vctx->display, current_ctx->surface);

    /* Change currently active surface to the next available one */
    picture_t *picture = picture_pool_Wait(vctx->pool);
    assert(picture->context);
    struct picture_ctx *ctx = PRIV(current_picture->context);

    eglMakeCurrent(vctx->display, ctx->surface, ctx->surface, vctx->context);
    sys->current_picture = picture;

    return current_picture;
}

static void Close(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;

    picture_Release(sys->current_picture);
    heimdall_video_context_Release(gl->offscreen_vctx_out);
    free(sys);
}

static void DestroyVideoContext(void *priv)
{
    struct video_ctx *vctx = priv;
    for (size_t i = 0; i < ARRAY_SIZE(vctx->pictures); ++i)
    {
        picture_t *picture = vctx->pictures[i];
        struct picture_ctx *ctx = PRIV(picture->context);

        eglDestroySurface(vctx->display, ctx->surface);
        heimdall_asurfacetexture_Delete(ctx->texture);

        /* Delete context without calling the picture destructors */
        free(ctx);
        picture->context = NULL;
    }

    /* Picture pool will release the pictures too */
    picture_pool_Release(vctx->pool);

    /* Must be not be called from Close(), otherwise it might cause deadlocks
     * on eglDestroySurface() */
    eglDestroyContext(vctx->display, vctx->context);

    eglTerminate(vctx->display);
}

static struct heimdall_asurfacetexture *
PictureContextGetTexture(picture_context_t *context)
{
    assert(context);

    struct picture_ctx *ctx = PRIV(context);
    return ctx->texture;
}

static int InitPicturePool(heimdall_gl_t *gl)
{
    struct surfacetexture_sys *sys = gl->sys;
    struct video_ctx *vctx = GetVCtx(gl);

    size_t i;
    for (i = 0; i < ARRAY_SIZE(vctx->pictures); ++i)
    {
        picture_t *pic = picture_NewFromFormat(&sys->fmt_out);
        if (!pic)
            goto error;

        pic->context = CreatePictureContext(gl);
        if (!pic->context)
        {
            picture_Release(pic);
            goto error;
        }

        vctx->pictures[i] = pic;
    }

    vctx->pool = picture_pool_New(ARRAY_SIZE(vctx->pictures), vctx->pictures);
    if (!vctx->pool)
        goto error;

    return HEIMDALL_SUCCESS;

error:
    while (i--)
        picture_Release(vctx->pictures[i]);
    return HEIMDALL_EGENERIC;
}

static int Open(heimdall_gl_t *gl, unsigned width, unsigned height,
                const struct heimdall_gl_cfg *gl_cfg)
{
    if (gl_cfg->need_alpha)
    {
        msg_Err(gl, "Cannot support alpha yet");
        return HEIMDALL_ENOTSUP;
    }

    if (gl->device == NULL || gl->device->type != HEIMDALL_DECODER_DEVICE_AWINDOW)
    {
        msg_Err(gl, "Wrong decoder device");
        return HEIMDALL_EGENERIC;
    }

    struct surfacetexture_sys *sys = malloc(sizeof *sys);
    if (sys == NULL)
        return HEIMDALL_ENOMEM;

    sys->fmt_out.i_visible_width
        = sys->fmt_out.i_width
        = width;
    sys->fmt_out.i_visible_height
        = sys->fmt_out.i_height
        = height;

    sys->fmt_out.i_chroma
        = gl->offscreen_chroma_out
        = HEIMDALL_CODEC_ANDROID_OPAQUE;
    gl->sys = sys;

    static const struct heimdall_video_context_operations ops = {
        .destroy = DestroyVideoContext,
    };

    gl->offscreen_vctx_out =
        heimdall_video_context_Create(gl->device, HEIMDALL_VIDEO_CONTEXT_AWINDOW,
                                 sizeof(struct video_ctx), &ops);
    if (gl->offscreen_vctx_out == NULL)
        goto error1;

    if (InitEGL(gl) != HEIMDALL_SUCCESS)
    {
        msg_Err(gl, "Failed to create opengl context\n");
        goto error2;
    }

    static const struct heimdall_gl_operations gl_ops =
    {
        .make_current = MakeCurrent,
        .release_current = ReleaseCurrent,
        .swap_offscreen = SwapOffscreen,
        .get_proc_address = GetSymbol,
        .close = Close,
    };
    gl->ops = &gl_ops;

    struct video_ctx *vctx = GetVCtx(gl);

    if (InitPicturePool(gl) != HEIMDALL_SUCCESS)
        goto error3;

    sys->current_picture = picture_pool_Get(vctx->pool);
    assert(sys->current_picture);
    assert(sys->current_picture->context);
    assert(sys->current_picture->context->vctx);

    vctx->android.texture = NULL;
    vctx->android.render = NULL;
    vctx->android.render_ts = NULL;
    vctx->android.get_texture = PictureContextGetTexture;

    int ret = heimdall_gl_MakeCurrent(gl);
    if (ret != HEIMDALL_SUCCESS)
        goto error4;

    struct heimdall_gl_extension_vt extension_vt;
    heimdall_gl_LoadExtensionFunctions(gl, &extension_vt);

    bool has_image_external =
        heimdall_gl_HasExtension(&extension_vt, "GL_OES_EGL_image_external");
    heimdall_gl_ReleaseCurrent(gl);

    if (!has_image_external)
    {
        msg_Warn(gl, "GL_OES_EGL_image_external is not available,"
                " disabling egl_surfacetexture.");
        goto error4;
    }

    return HEIMDALL_SUCCESS;

error4:
    picture_Release(sys->current_picture);
error3:
    eglTerminate(vctx->display);
error2:
    heimdall_video_context_Release(gl->offscreen_vctx_out);
error1:
    free(sys);
    return HEIMDALL_EGENERIC;
}

heimdall_module_begin()
    set_shortname( N_("egl_surfacetexture") )
    set_description( N_("EGL Android SurfaceTexture offscreen opengl provider") )
    set_callback_opengl_es2_offscreen(Open, 100)

    add_shortcut( "egl_surfacetexture" )
heimdall_module_end()
