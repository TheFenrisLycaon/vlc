// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * d3d11_tonemap: Direct3D11 VideoProcessor to handle tonemapping
 *****************************************************************************
 * Copyright © 2024 Videolabs, VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Steve Lhomme <robux4@videolabs.io>
 *****************************************************************************/

#ifndef HEIMDALL_D3D11_TONEMAP_H
#define HEIMDALL_D3D11_TONEMAP_H

#include "d3d11_quad.h"
#include "../../video_chroma/d3d11_fmt.h"
#include <heimdall_vout_display.h>

#ifdef __cplusplus
extern "C" {
#endif

struct d3d11_tonemapper;

struct d3d11_tonemapper *D3D11_TonemapperCreate(heimdall_object_t *, d3d11_device_t *,
                                                const video_format_t * in);
void D3D11_TonemapperDestroy(struct d3d11_tonemapper *);
HRESULT D3D11_TonemapperProcess(heimdall_object_t *, struct d3d11_tonemapper *, picture_sys_d3d11_t *);
picture_sys_d3d11_t *D3D11_TonemapperGetOutput(struct d3d11_tonemapper *);

#ifdef __cplusplus
}
#endif

#endif // HEIMDALL_D3D11_TONEMAP_H
