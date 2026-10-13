// SPDX-License-Identifier: LGPL-2.1-or-later

// amf_helper.h: AMD Advanced Media Framework helper
// Copyright © 2024 VideoLabs, VLC authors and VideoLAN
// Copyright (C) 2026 TheFenrisLycaon

// Authors: Steve Lhomme <robux4@videolabs.io>

#ifndef HEIMDALL_AMF_HELPER_H
#define HEIMDALL_AMF_HELPER_H

#include <AMF/core/Context.h>
#include <AMF/core/Factory.h>

#ifdef _WIN32
#include "../../video_chroma/d3d11_fmt.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define AMT_TYPE(t)  amf::t
#else
#define AMT_TYPE(t)  t
#endif

struct heimdall_amf_context
{
    AMT_TYPE(AMFFactory) *pFactory;
    AMT_TYPE(AMFContext) *Context;
    amf_uint64           Version;
    void            *Private;
};

int heimdall_AMFCreateContext(struct heimdall_amf_context *);
void heimdall_AMFReleaseContext(struct heimdall_amf_context *);

#ifdef _WIN32
static inline AMT_TYPE(AMF_SURFACE_FORMAT) DXGIToAMF(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
        case DXGI_FORMAT_NV12: return AMT_TYPE(AMF_SURFACE_NV12);
        case DXGI_FORMAT_P010: return AMT_TYPE(AMF_SURFACE_P010);
        case DXGI_FORMAT_P016: return AMT_TYPE(AMF_SURFACE_P016);
        case DXGI_FORMAT_B8G8R8A8_UNORM: return AMT_TYPE(AMF_SURFACE_BGRA);
        case DXGI_FORMAT_R8G8B8A8_UNORM: return AMT_TYPE(AMF_SURFACE_RGBA);
        case DXGI_FORMAT_R10G10B10A2_UNORM: return AMT_TYPE(AMF_SURFACE_R10G10B10A2);
        default: return AMT_TYPE(AMF_SURFACE_UNKNOWN);
    }
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HEIMDALL_AMF_HELPER_H
