/*****************************************************************************
 * converter_vaapi.c: OpenGL VAAPI opaque converter
 *****************************************************************************
 * Copyright (C) 2017 VLC authors and VideoLAN
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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <va/va_drmcommon.h>

#ifndef HAVE_LIBDRM 
# include <drm/drm_fourcc.h>
#else
# include <drm_fourcc.h>
#endif

#include <heimdall_common.h>
#include <heimdall_window.h>
#include <heimdall_codec.h>
#include <heimdall_plugin.h>

#include "gl_util.h"
#include "interop.h"
#include "../../hw/vaapi/heimdall_vaapi.h"

/* From https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image.txt
 * The extension is an OpenGL ES extension but can (and usually is) available on
 * OpenGL implementations. */
#ifndef GL_OES_EGL_image
#define GL_OES_EGL_image 1
typedef void *GLeglImageOES;
typedef void (*PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, GLeglImageOES image);
#endif


/* From max number of plane in libva and DRM */
#define INTEROP_MAX_PLANES 4

/**
 * It maps the formats from the DRM infrastructure to the attributes 
 * expected by OpenGL to allocate textures.
 */
static const struct {
    uint32_t drm_fourcc;
    int32_t internal;
    uint32_t format, type;
} plane_tex_cfgs[] = {
    { DRM_FORMAT_R8,           GL_RED,      GL_RED,  GL_UNSIGNED_BYTE },
    { DRM_FORMAT_GR88,         GL_RG,       GL_RG,   GL_UNSIGNED_BYTE },
    { DRM_FORMAT_R16,          GL_R16,      GL_RED,  GL_UNSIGNED_SHORT },
    { DRM_FORMAT_GR1616,       GL_RG16,     GL_RG,   GL_UNSIGNED_SHORT },
    { DRM_FORMAT_ABGR16161616, GL_RGBA16,   GL_RGBA, GL_UNSIGNED_SHORT },
    { DRM_FORMAT_XYUV8888,     GL_RGBA,     GL_RGBA, GL_UNSIGNED_BYTE },
    { DRM_FORMAT_Y412,         GL_RGBA16,   GL_RGBA, GL_UNSIGNED_SHORT },
    { DRM_FORMAT_Y410,         GL_RGB10_A2, GL_RGBA,
      GL_UNSIGNED_INT_2_10_10_10_REV },
};

struct plane_format
{
    uint32_t drm_fourcc;
    heimdall_rational_t width;
    heimdall_rational_t height;
};

struct frame_format
{
    heimdall_fourcc_t chroma;
    size_t plane_count;
    struct plane_format planes[INTEROP_MAX_PLANES];
};

struct priv
{
    VADisplay vadpy;
    VASurfaceID *va_surface_ids;
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

    unsigned va_fourcc;
    struct frame_format format;

    struct {
        picture_t *                 pic;
        /* VADRMPRIMESurfaceDescriptor carries modifier information
         * (GPU tiling, compression, etc...) */
        VADRMPRIMESurfaceDescriptor va_surface_descriptor;
        unsigned                    num_planes;
        EGLImageKHR                 egl_images[INTEROP_MAX_PLANES];
    } last;
};

static EGLImageKHR
CreatePlaneImage(const struct heimdall_gl_interop *interop, EGLint w, EGLint h,
                 uint32_t drm_fourcc, EGLint fd, EGLint offset, EGLint pitch,
                 EGLuint64KHR modifier)
{
    struct priv *priv = interop->priv;
    const EGLint attribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_LINUX_DRM_FOURCC_EXT, (EGLint)drm_fourcc,
        EGL_DMA_BUF_PLANE0_FD_EXT, fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, offset,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, pitch,
        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, modifier & 0xffffffff,
        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, modifier >> 32,
        EGL_NONE
    };

    return priv->egl.createImageKHR(priv->egl.display,
            EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
}

static void
ReleaseLastPicture(const struct heimdall_gl_interop *interop)
{
    struct priv *priv = interop->priv;

    for (unsigned i = 0; i < priv->last.num_planes; ++i)
        priv->egl.destroyImageKHR(priv->egl.display, priv->last.egl_images[i]);

    for (unsigned i = 0; i < priv->last.va_surface_descriptor.num_objects; ++i)
        close(priv->last.va_surface_descriptor.objects[i].fd);

    picture_Release(priv->last.pic);
}

static int
tc_vaegl_update(const struct heimdall_gl_interop *interop, uint32_t textures[],
                const int32_t tex_width[], const int32_t tex_height[],
                picture_t *pic, const size_t *plane_offset)
{
    (void) plane_offset;
    struct priv *priv = interop->priv;
    heimdall_object_t *o = HEIMDALL_OBJECT(interop->gl);

    if (pic == priv->last.pic)
    {
        for (unsigned i = 0; i < priv->last.num_planes; ++i)
        {
            priv->gl.BindTexture(interop->tex_target, textures[i]);
            priv->glEGLImageTargetTexture2DOES(interop->tex_target,
                                               priv->last.egl_images[i]);
        }
        return HEIMDALL_SUCCESS;
    }

    VADRMPRIMESurfaceDescriptor va_surface_descriptor;
    EGLImageKHR egl_images[INTEROP_MAX_PLANES] = { NULL };
    unsigned num_planes = 0;

    {
        VAStatus s = vaSyncSurface(priv->vadpy, heimdall_vaapi_PicGetSurface(pic));
        if (s != VA_STATUS_SUCCESS) // non-fatal. ex: VA_STATUS_ERROR_DECODING_ERROR
            msg_Warn(o, "vaSyncSurface: 0x%x %s", s, vaErrorStr(s));
    }
    if (heimdall_vaapi_ExportSurfaceHandle(o, priv->vadpy, heimdall_vaapi_PicGetSurface(pic),
                                      VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                                      VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                                      &va_surface_descriptor))
        return HEIMDALL_EGENERIC;

    num_planes = va_surface_descriptor.num_layers;
    for (unsigned i = 0; i < num_planes; ++i)
    {
        unsigned obj_idx = va_surface_descriptor.layers[i].object_index[0];

        egl_images[i] =
            CreatePlaneImage(interop, tex_width[i], tex_height[i],
                             priv->format.planes[i].drm_fourcc,
                             va_surface_descriptor.objects[obj_idx].fd,
                             va_surface_descriptor.layers[i].offset[0],
                             va_surface_descriptor.layers[i].pitch[0],
                             va_surface_descriptor.objects[obj_idx].drm_format_modifier);
        if (egl_images[i] == NULL)
            goto error;
    }

    for (size_t i = 0; i < num_planes; ++i)
    {
        priv->gl.BindTexture(interop->tex_target, textures[i]);
        priv->glEGLImageTargetTexture2DOES(interop->tex_target, egl_images[i]);
    }

    if (priv->last.pic != NULL)
        ReleaseLastPicture(interop);
    priv->last.pic = picture_Hold(pic);

    priv->last.va_surface_descriptor = va_surface_descriptor;
    priv->last.num_planes = num_planes;

    for (unsigned i = 0; i < num_planes; ++i)
        priv->last.egl_images[i] = egl_images[i];

    return HEIMDALL_SUCCESS;

error:
    for (unsigned i = 0; i < INTEROP_MAX_PLANES && egl_images[i] != NULL; ++i)
        priv->egl.destroyImageKHR(priv->egl.display, egl_images[i]);

    for (unsigned i = 0; i < va_surface_descriptor.num_objects; ++i)
        close(va_surface_descriptor.objects[i].fd);

    return HEIMDALL_EGENERIC;
}

static void
Close(struct heimdall_gl_interop *interop)
{
    struct priv *priv = interop->priv;

    if (priv->last.pic != NULL)
        ReleaseLastPicture(interop);

    free(priv);
}

static int strcasecmp_void(const void *a, const void *b)
{
    return strcasecmp(a, b);
}

static int
tc_va_check_interop_blacklist(const struct heimdall_gl_interop *interop, VADisplay *vadpy)
{
    const char *vendor = vaQueryVendorString(vadpy);
    if (vendor == NULL)
        return HEIMDALL_SUCCESS;

#define BL_SIZE_MAX 19
    static const char blacklist_prefix[][BL_SIZE_MAX] = {
        /* XXX: case insensitive and alphabetical order */
        "mesa gallium vaapi",
    };

    char vendor_prefix[BL_SIZE_MAX];
    strncpy(vendor_prefix, vendor, sizeof(vendor_prefix) - 1);
    vendor_prefix[sizeof(vendor_prefix) - 1] = '\0';

    const char *found = bsearch(vendor_prefix, blacklist_prefix,
                                ARRAY_SIZE(blacklist_prefix),
                                BL_SIZE_MAX, strcasecmp_void);
    if (found != NULL)
    {
        msg_Warn(interop->gl, "The '%s' driver is blacklisted: no interop", found);
        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}

static int
tc_va_check_derive_image(const struct heimdall_gl_interop *interop)
{
    heimdall_object_t *o = HEIMDALL_OBJECT(interop->gl);
    struct priv *priv = interop->priv;
    VASurfaceID *va_surface_ids;

    picture_pool_t *pool = heimdall_vaapi_PoolNew(o, interop->vctx, priv->vadpy, 1,
                                             &va_surface_ids,
                                             &interop->fmt_out);
    if (!pool)
        return HEIMDALL_EGENERIC;

    VADRMPRIMESurfaceDescriptor desc;

    int ret = heimdall_vaapi_ExportSurfaceHandle(o, priv->vadpy, va_surface_ids[0],
                                            VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                                            VA_EXPORT_SURFACE_READ_ONLY |
                                            VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                                            &desc);
    if (ret != HEIMDALL_SUCCESS)
        goto done;

    for (unsigned i = 0; i < desc.num_layers; ++i)
    {
        unsigned obj_idx = desc.layers[i].object_index[0];

        if (desc.layers[i].num_planes > 1)
        {
            ret = HEIMDALL_EGENERIC;
            goto done_desc;
        }

        EGLint w = (desc.width * interop->texs[i].w.num) / interop->texs[i].w.den;
        EGLint h = (desc.height * interop->texs[i].h.num) / interop->texs[i].h.den;
        EGLImageKHR egl_image =
            CreatePlaneImage(interop, w, h, priv->format.planes[i].drm_fourcc,
                             desc.objects[obj_idx].fd,
                             desc.layers[i].offset[0], desc.layers[i].pitch[0],
                             desc.objects[obj_idx].drm_format_modifier);
        if (egl_image == NULL)
        {
            msg_Warn(o, "Can't create Image KHR: kernel too old ?");
            ret = HEIMDALL_EGENERIC;
            goto done_desc;
        }
        priv->egl.destroyImageKHR(priv->egl.display, egl_image);
    }

    ret = HEIMDALL_SUCCESS;

done_desc:
    for (unsigned i = 0; i < desc.num_objects; ++i)
        close(desc.objects[i].fd);

done:
    picture_pool_Release(pool);

    return ret;
}

/**
 * Describe a Heimdall VAAPI fourcc for libva and how the planes are setup
 */
static int
DescribeChroma(heimdall_fourcc_t chroma, unsigned *va_fourcc,
               struct frame_format *format)
{
    switch (chroma)
    {
        case HEIMDALL_CODEC_VAAPI_420:
            *va_fourcc = VA_FOURCC_NV12;
            *format = (struct frame_format) {
                .chroma = HEIMDALL_CODEC_NV12,
                .plane_count = 2,
                .planes = {
                    { DRM_FORMAT_R8,   { 1, 1 }, { 1, 1 } },
                    { DRM_FORMAT_GR88, { 1, 2 }, { 1, 2 } },
                },
            };
            break;
        case HEIMDALL_CODEC_VAAPI_420_10BPP:
        case HEIMDALL_CODEC_VAAPI_420_12BPP:
            *va_fourcc = chroma == HEIMDALL_CODEC_VAAPI_420_10BPP
                ? VA_FOURCC_P010 : VA_FOURCC_P012;
            *format = (struct frame_format) {
                .chroma = chroma == HEIMDALL_CODEC_VAAPI_420_10BPP
                    ? HEIMDALL_CODEC_P010 : HEIMDALL_CODEC_P012,
                .plane_count = 2,
                .planes = {
                    { DRM_FORMAT_R16,    { 1, 1 }, { 1, 1 } },
                    { DRM_FORMAT_GR1616, { 1, 2 }, { 1, 2 } },
                },
            };
            break;
        case HEIMDALL_CODEC_VAAPI_422_10BPP:
        case HEIMDALL_CODEC_VAAPI_422_12BPP:
            *va_fourcc = chroma == HEIMDALL_CODEC_VAAPI_422_10BPP
                ? VA_FOURCC_Y210 : VA_FOURCC_Y212;
            *format = (struct frame_format) {
                .chroma = chroma == HEIMDALL_CODEC_VAAPI_422_10BPP
                    ? HEIMDALL_CODEC_Y210 : HEIMDALL_CODEC_Y212,
                .plane_count = 1,
                .planes = {
                    { DRM_FORMAT_ABGR16161616, { 1, 2 }, { 1, 1 } },
                },
            };
            break;
        case HEIMDALL_CODEC_VAAPI_444:
            *va_fourcc = VA_FOURCC_XYUV;
            *format = (struct frame_format) {
                .chroma = HEIMDALL_CODEC_VUYX,
                .plane_count = 1,
                .planes = {
                    { DRM_FORMAT_XYUV8888, { 1, 1 }, { 1, 1 } },
                },
            };
            break;
        case HEIMDALL_CODEC_VAAPI_444_10BPP:
        case HEIMDALL_CODEC_VAAPI_444_12BPP:
        {
            const bool is10bit = chroma == HEIMDALL_CODEC_VAAPI_444_10BPP;

            *va_fourcc = is10bit ? VA_FOURCC_Y410 : VA_FOURCC_Y412;
            *format = (struct frame_format) {
                .chroma = is10bit ? HEIMDALL_CODEC_Y410 : HEIMDALL_CODEC_Y412,
                .plane_count = 1,
                .planes = {
                    { is10bit ? DRM_FORMAT_Y410 : DRM_FORMAT_Y412,
                      { 1, 1 }, { 1, 1 } },
                },
            };
            break;
        }
        default:
            /* Not a vaapi chroma */
            return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}

/**
 * Setup the interop texture sampling infos from the input frame format.
 */
static int
ConfigureTextures(struct heimdall_gl_interop *interop,
                  const struct frame_format *format)
{
    if (format->plane_count > ARRAY_SIZE(interop->texs))
        return HEIMDALL_EGENERIC;

    for (size_t i = 0; i < format->plane_count; i++)
    {
        const struct plane_format *plane = &format->planes[i];
        const size_t cfg = ARRAY_SIZE(plane_tex_cfgs);
        size_t j;

        for (j = 0; j < cfg; j++)
            if (plane_tex_cfgs[j].drm_fourcc == plane->drm_fourcc)
                break;

        if (j == cfg)
        {
            msg_Dbg(interop->gl, "no texture for a %4.4s plane",
                    (const char *)&plane->drm_fourcc);
            return HEIMDALL_EGENERIC;
        }

        interop->texs[i] = (struct heimdall_gl_tex_cfg) {
            .w = plane->width,
            .h = plane->height,
            .internal = plane_tex_cfgs[j].internal,
            .format = plane_tex_cfgs[j].format,
            .type = plane_tex_cfgs[j].type,
        };
    }

    interop->tex_count = (unsigned)format->plane_count;
    return HEIMDALL_SUCCESS;
}

/**
 * Whether we can allocate the textures at the requested precision
 */
static bool
CanAllocateTexture(struct heimdall_gl_interop *interop)
{
    for (unsigned i = 0; i < interop->tex_count; ++i)
    {
        const struct heimdall_gl_tex_cfg *tex = &interop->texs[i];

        if (tex->type == GL_UNSIGNED_SHORT &&
            heimdall_gl_interop_GetTexFormatSize(interop, GL_TEXTURE_2D, tex->format,
                                            tex->internal, tex->type) != 16)
            return false;
    }

    return true;
}

static int
Open(struct heimdall_gl_interop *interop)
{
    struct priv *priv = NULL;

    if (interop->vctx == NULL)
        return HEIMDALL_EGENERIC;
    heimdall_decoder_device *dec_device = heimdall_video_context_HoldDevice(interop->vctx);
    if (dec_device->type != HEIMDALL_DECODER_DEVICE_VAAPI)
        goto error;

    struct heimdall_gl_extension_vt extension_vt;
    heimdall_gl_LoadExtensionFunctions(interop->gl, &extension_vt);

    if (!heimdall_gl_HasExtension(&extension_vt, "GL_OES_EGL_image"))
        goto error;

    priv = interop->priv = calloc(1, sizeof(struct priv));
    if (unlikely(priv == NULL))
        goto error;

    /* Non-vaapi chroma are filtered out here */
    if (DescribeChroma(interop->fmt_in.i_chroma, &priv->va_fourcc,
                       &priv->format) != HEIMDALL_SUCCESS)
        goto error;

    if (ConfigureTextures(interop, &priv->format) != HEIMDALL_SUCCESS)
        goto error;

    if (!CanAllocateTexture(interop))
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

    priv->vadpy = dec_device->opaque;
    assert(priv->vadpy != NULL);

    if (tc_va_check_interop_blacklist(interop, priv->vadpy))
        goto error;

    if (tc_va_check_derive_image(interop))
        goto error;

    /* The pictures are uploaded upside-down */
    video_format_TransformBy(&interop->fmt_out, TRANSFORM_VFLIP);

    interop->tex_target = GL_TEXTURE_2D;
    interop->fmt_out.i_chroma = priv->format.chroma;
    interop->fmt_out.space = interop->fmt_in.space;

    static const struct heimdall_gl_interop_ops ops = {
        .update_textures = tc_vaegl_update,
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
    set_description("VA-API OpenGL surface converter")
    set_capability("glinterop", 1)
    set_callback(Open)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    add_shortcut("vaapi")
heimdall_module_end ()
