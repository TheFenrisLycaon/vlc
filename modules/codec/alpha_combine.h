// SPDX-License-Identifier: LGPL-2.1-or-later

// alpha_combine.h : helper to combine D3D11 planes to generate pictures with alpha
// Copyright © 2023 VideoLabs, VLC authors and VideoLAN
// Copyright (C) 2026 TheFenrisLycaon

// Authors: Steve Lhomme <robux4@videolabs.io>

#ifndef HEIMDALL_ALPHA_COMBINE_H
#define HEIMDALL_ALPHA_COMBINE_H 1

#include <heimdall_common.h>
#include <heimdall_codec.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
picture_t *CombineD3D11(decoder_t *bdec, picture_t *opaque, picture_t *alpha, heimdall_video_context*);
int SetupD3D11(decoder_t *bdec, heimdall_video_context *vctx, heimdall_video_context **vctx_out);
#endif // _WIN32

#ifdef __cplusplus
}
#endif

#endif // HEIMDALL_ALPHA_COMBINE_H
