/*****************************************************************************
 * interop_gst_mem.c: OpenGL GStreamer Memory opaque converter
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Yann Lochet <yann@l0chet.fr>
 * Heavily inspired by interop_vaapi.c
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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/video-format.h>
#include <gst/allocators/gstdmabuf.h>

#include <heimdall_common.h>
#include <heimdall_window.h>
#include <heimdall_codec.h>
#include <heimdall_plugin.h>

#include "gl_util.h"
#include "interop.h"

#include "../../codec/gstreamer/gst_mem.h"

#define DRM_FORMAT_MOD_LINEAR 0ULL

struct priv
{
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

    struct
    {
        EGLDisplay display;
        EGLDisplay (*getCurrentDisplay)(void);
        const char *(*queryString)(EGLDisplay, EGLint);
        EGLImage (*createImageKHR)(EGLDisplay, EGLContext, EGLenum target, EGLClientBuffer buffer,
                const EGLint *attrib_list);
        void (*destroyImageKHR)(EGLDisplay, EGLImage image);
    } egl;

    struct
    {
        PFNGLBINDTEXTUREPROC BindTexture;
    } gl;

    EGLint drm_fourccs[3];
};

static void
egl_image_destroy(const struct heimdall_gl_interop *interop, EGLImageKHR image)
{
    struct priv *priv = interop->priv;
    priv->egl.destroyImageKHR(priv->egl.display, image);
}

static int
egl_update(const struct heimdall_gl_interop *interop, uint32_t textures[],
           const int32_t tex_width[], const int32_t tex_height[],
           picture_t *pic, const size_t *plane_offset)
{
    (void) plane_offset;
    struct priv *priv = interop->priv;

    struct gst_mem_pic_context *pctx = container_of( pic->context,
                                                     struct gst_mem_pic_context, s );
    GstBuffer *p_buf = pctx->p_buf;
    GstVideoMeta *p_meta = gst_buffer_get_video_meta(p_buf);
    GstMemory *p_mem = gst_buffer_peek_memory(p_buf, 0);
    int dmabuf_fd = gst_dmabuf_memory_get_fd(p_mem);

    int egl_color_space;
    switch (interop->fmt_in.space)
    {
        case COLOR_SPACE_BT601:
            egl_color_space = EGL_ITU_REC601_EXT;
            break;
        case COLOR_SPACE_BT709:
            egl_color_space = EGL_ITU_REC709_EXT;
            break;
        case COLOR_SPACE_BT2020:
            egl_color_space = EGL_ITU_REC2020_EXT;
            break;
        default:
            goto error;
    }

    int egl_color_range;
    switch (interop->fmt_in.color_range)
    {
        case COLOR_RANGE_FULL:
            egl_color_range = EGL_YUV_FULL_RANGE_EXT;
            break;
        case COLOR_RANGE_LIMITED:
            egl_color_range = EGL_YUV_NARROW_RANGE_EXT;
            break;
        default:
            heimdall_assert_unreachable();
    }

    const EGLint attribs[] = {
        EGL_WIDTH, tex_width[0],
        EGL_HEIGHT, tex_height[0],
        EGL_LINUX_DRM_FOURCC_EXT, priv->drm_fourccs[0],
        EGL_DMA_BUF_PLANE0_FD_EXT, dmabuf_fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, p_meta->offset[0],
        EGL_DMA_BUF_PLANE0_PITCH_EXT, p_meta->stride[0],
        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, DRM_FORMAT_MOD_LINEAR & 0xffffffff,
        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (DRM_FORMAT_MOD_LINEAR >> 32) & 0xffffffff,
        EGL_DMA_BUF_PLANE1_FD_EXT, dmabuf_fd,
        EGL_DMA_BUF_PLANE1_OFFSET_EXT, p_meta->offset[1],
        EGL_DMA_BUF_PLANE1_PITCH_EXT, p_meta->stride[1],
        EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, DRM_FORMAT_MOD_LINEAR & 0xffffffff,
        EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (DRM_FORMAT_MOD_LINEAR >> 32) & 0xffffffff,
        EGL_YUV_COLOR_SPACE_HINT_EXT, egl_color_space,
        EGL_SAMPLE_RANGE_HINT_EXT, egl_color_range,
        EGL_NONE
    };

    EGLImageKHR egl_image = priv->egl.createImageKHR(priv->egl.display, EGL_NO_CONTEXT,
                                                     EGL_LINUX_DMA_BUF_EXT, NULL,
                                                     attribs);

    if( egl_image == NULL )
        goto error;

    priv->gl.BindTexture(interop->tex_target, textures[0]);

    priv->glEGLImageTargetTexture2DOES(interop->tex_target, egl_image);

    egl_image_destroy(interop, egl_image);

    return HEIMDALL_SUCCESS;

error:
    return HEIMDALL_EGENERIC;
}

static void
Close(struct heimdall_gl_interop *interop)
{
    struct priv *priv = interop->priv;

    free(priv);
}

static int
Open(struct heimdall_gl_interop *interop)
{
    struct priv *priv = NULL;

    if (interop->vctx == NULL)
        return HEIMDALL_EGENERIC;
    heimdall_decoder_device *dec_device = heimdall_video_context_HoldDevice(interop->vctx);
    if (dec_device->type != HEIMDALL_DECODER_DEVICE_GSTDECODE)
        goto error;

    priv = interop->priv = calloc(1, sizeof(struct priv));
    if (unlikely(priv == NULL))
        goto error;

    switch (interop->fmt_in.i_chroma)
    {
        case HEIMDALL_CODEC_GST_MEM_OPAQUE:
            interop->tex_count = 1;
            interop->texs[0] = (struct heimdall_gl_tex_cfg) {
                .w = {1, 1},
                .h = {1, 1},
                .internal = GL_RGBA,
                .format = GL_RGBA,
                .type = GL_UNSIGNED_BYTE,
            };

            priv->drm_fourccs[0] = HEIMDALL_FOURCC('N', 'V', '1', '2');
            break;
        default:
            goto error;
    }

    struct heimdall_gl_extension_vt extension_vt;
    heimdall_gl_LoadExtensionFunctions(interop->gl, &extension_vt);

    /* GL_OES_EGL_image_external is required for GL_TEXTURE_EXTERNAL_OES */
    if (!heimdall_gl_HasExtension(&extension_vt, "GL_OES_EGL_image_external"))
        goto error;

    priv->egl.getCurrentDisplay = heimdall_gl_GetProcAddress(interop->gl, "eglGetCurrentDisplay");
    if (priv->egl.getCurrentDisplay == EGL_NO_DISPLAY)
        goto error;

    priv->egl.display = priv->egl.getCurrentDisplay();
    if (priv->egl.display == EGL_NO_DISPLAY)
        goto error;

    priv->egl.queryString = heimdall_gl_GetProcAddress(interop->gl, "eglQueryString");
    if (priv->egl.queryString == NULL)
        goto error;

    /* EGL_EXT_image_dma_buf_import implies EGL_KHR_image_base */
    const char *eglexts = priv->egl.queryString(priv->egl.display, EGL_EXTENSIONS);
    if (eglexts == NULL || !heimdall_gl_StrHasToken(eglexts, "EGL_EXT_image_dma_buf_import"))
        goto error;

    priv->egl.createImageKHR =
        heimdall_gl_GetProcAddress(interop->gl, "eglCreateImageKHR");
    if (priv->egl.createImageKHR == NULL)
        goto error;

    priv->egl.destroyImageKHR =
        heimdall_gl_GetProcAddress(interop->gl, "eglDestroyImageKHR");
    if (priv->egl.destroyImageKHR == NULL)
        goto error;

    priv->glEGLImageTargetTexture2DOES =
        heimdall_gl_GetProcAddress(interop->gl, "glEGLImageTargetTexture2DOES");
    if (priv->glEGLImageTargetTexture2DOES == NULL)
        goto error;

    priv->gl.BindTexture =
        heimdall_gl_GetProcAddress(interop->gl, "glBindTexture");
    if (priv->gl.BindTexture == NULL)
        goto error;

    /* The pictures are uploaded upside-down */
    video_format_TransformBy(&interop->fmt_out, TRANSFORM_VFLIP);

    interop->tex_target = GL_TEXTURE_EXTERNAL_OES;
    interop->fmt_out.i_chroma = HEIMDALL_CODEC_RGBA;
    interop->fmt_out.space = COLOR_SPACE_UNDEF;

    static const struct heimdall_gl_interop_ops ops = {
        .update_textures = egl_update,
        .close = Close,
    };
    interop->ops = &ops;

    heimdall_decoder_device_Release(dec_device);

    return HEIMDALL_SUCCESS;

error:
    heimdall_decoder_device_Release(dec_device);
    free(priv);
    return HEIMDALL_EGENERIC;
}

heimdall_module_begin ()
    set_description("GST_MEM OpenGL surface converter")
    set_capability("glinterop", 1)
    set_callback(Open)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    add_shortcut("gst_mem")
heimdall_module_end ()
