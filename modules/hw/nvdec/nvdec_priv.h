/*****************************************************************************
 * nvdec_priv.h : NVDEC private common code
 *****************************************************************************
 * Copyright © 2019-2023 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Steve Lhomme <robux4@videolabs.io>
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

#ifndef HEIMDALL_VIDEOCHROMA_NVDEC_PRIV_H_
#define HEIMDALL_VIDEOCHROMA_NVDEC_PRIV_H_

#define FFNV_LOG_FUNC(logctx, msg, ...)        msg_Err((heimdall_object_t*)logctx, msg, __VA_ARGS__)
#define FFNV_DEBUG_LOG_FUNC(logctx, msg, ...)  msg_Dbg((heimdall_object_t*)logctx, msg, __VA_ARGS__)

#include <ffnvcodec/dynlink_loader.h>

typedef struct {

    CudaFunctions  *cudaFunctions;
    CUcontext      cuCtx;

} decoder_device_nvdec_t;

static inline decoder_device_nvdec_t *GetNVDECOpaqueDevice(heimdall_decoder_device *device)
{
    if (device == NULL || device->type != HEIMDALL_DECODER_DEVICE_NVDEC)
        return NULL;
    return device->opaque;
}

static inline int CudaCheckErr(heimdall_object_t *obj, CudaFunctions *cudaFunctions, CUresult result, const char *psz_func)
{
    if (unlikely(result != CUDA_SUCCESS)) {
        const char *psz_err, *psz_err_str;
        cudaFunctions->cuGetErrorName(result, &psz_err);
        cudaFunctions->cuGetErrorString(result, &psz_err_str);
        msg_Err(obj, "%s failed: %s (%s)", psz_func, psz_err_str, psz_err);
        return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

/* for HEIMDALL_CODEC_NVDEC_OPAQUE / HEIMDALL_CODEC_NVDEC_OPAQUE_16B */
typedef struct
{
    picture_context_t ctx;
    CUdeviceptr  devicePtr;
    unsigned int bufferPitch;
    unsigned int bufferHeight;
} pic_context_nvdec_t;

#define NVDEC_PICCONTEXT_FROM_PICCTX(pic_ctx)  \
    container_of(pic_ctx, pic_context_nvdec_t, ctx)

#endif /* include-guard */
