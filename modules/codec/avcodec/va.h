/*****************************************************************************
 * va.h: Video Acceleration API for avcodec
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir_AT_ lycaria _DOT_ org>
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

#ifndef HEIMDALL_AVCODEC_VA_H
#define HEIMDALL_AVCODEC_VA_H 1

#include "avcommon_compat.h"
#include <libavutil/pixdesc.h>

typedef struct heimdall_va_t heimdall_va_t;
typedef struct heimdall_decoder_device heimdall_decoder_device;
typedef struct heimdall_video_context heimdall_video_context;

struct heimdall_va_cfg
{
    /**
     * AVContext to set the hwaccel_context on.
     *
     * The VA can assume fields codec_id, coded_width, coded_height,
     * active_thread_type, thread_count, framerate, profile, refs (H264) and
     * hw_pix_fmt are set correctly */
    AVCodecContext *avctx;

    /** avcodec hardare PIX_FMT */
    enum AVPixelFormat hwfmt;

    /**  Description of the hardware fornat */
    const AVPixFmtDescriptor *desc;

    /** Heimdall format of the content to decode */
    const es_format_t *fmt_in;

    /** Decoder device that will be used to create the video context */
    heimdall_decoder_device *dec_device;

    /**
     * Resulting Video format
     *
     * Valid, can be changed by the module to change the size or chroma.
     */
    video_format_t *video_fmt_out;

    /**
    * Pointer to the previous video context
    *
    * Only valid if use_hwframes is true and when recreating the va module.
    * This context can be re-used (held, and set to vctx_out) if the internal
    * format/size matches the new cfg.
    */
    heimdall_video_context *vctx_prev;

    /**
     * Pointer to the used video context
     *
     * The video context must be allocated from the dec_device, filled and set
     * to this pointer.
     */
    heimdall_video_context *vctx_out;

    /**
     * Indicate if the module is using the new AVHWFramesContext API
     *
     * False, by default, set to true if the module is using this API
     */
    bool use_hwframes;

    /**
     * Request more pictures
     *
     * 0 by default, set if from the module
     */
    unsigned extra_pictures;
};

struct heimdall_va_operations {
    int (*get)(heimdall_va_t *, picture_t *pic, AVCodecContext *ctx, AVFrame *frame);
    void (*close)(heimdall_va_t *, AVCodecContext* ctx);
};

struct heimdall_va_t {
    struct heimdall_object_t obj;

    void *sys;
    const struct heimdall_va_operations *ops;
};

typedef int (*heimdall_va_open)(heimdall_va_t *, struct heimdall_va_cfg *cfg);

#define set_va_callback(activate, priority) \
    { \
        heimdall_va_open open__ = activate; \
        (void) open__; \
        set_callback(activate) \
    } \
    set_capability( "hw decoder", priority )

/**
 * Determines whether the hardware acceleration PixelFormat can be used to
 * decode pixels.
 * @param hwfmt the hardware acceleration pixel format
 * @return true if the hardware acceleration should be supported
 */
bool heimdall_va_MightDecode(enum AVPixelFormat hwfmt);

/**
 * Creates an accelerated video decoding back-end for libavcodec.
 * @param obj parent Heimdall object
 * @param cfg pointer to a configuration struct
 * @return a new Heimdall object on success, NULL on error.
 */
heimdall_va_t *heimdall_va_New(heimdall_object_t *obj, struct heimdall_va_cfg *cfg);

/**
 * Get a hardware video surface for a libavcodec frame.
 * The surface will be used as output for the hardware decoder, and possibly
 * also as a reference frame to decode other surfaces.
 *
 * The type of the surface depends on the hardware pixel format:
 * AV_PIX_FMT_D3D11VA_VLD - ID3D11VideoDecoderOutputView*
 * AV_PIX_FMT_DXVA2_VLD   - IDirect3DSurface9*
 * AV_PIX_FMT_VDPAU       - VdpVideoSurface
 * AV_PIX_FMT_VAAPI       - VASurfaceID
 *
 * @param pic pointer to Heimdall picture containing the surface [IN/OUT]
 * @param ctx pointer to the current AVCodecContext [IN]
 * @param frame pointer to the AVFrame [IN]
 *
 * @note This function needs not be reentrant.
 *
 * @return HEIMDALL_SUCCESS on success, otherwise an error code.
 */
static inline int heimdall_va_Get(heimdall_va_t *va, picture_t *pic, AVCodecContext *ctx,
                             AVFrame *frame)
{
    return va->ops->get(va, pic, ctx, frame);
}

/**
 * Destroys a libavcodec hardware acceleration back-end.
 * All allocated surfaces shall have been released beforehand.
 *
 * @param avctx pointer to the current AVCodecContext if it needs to be cleaned, NULL otherwise [IN]
 */
void heimdall_va_Delete(heimdall_va_t *, AVCodecContext * avctx);

#endif
