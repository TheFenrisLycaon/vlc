/*****************************************************************************
 * dxgi_swapchain.h: DXGI swapchain handled by the display module
 *****************************************************************************
 * Copyright (C) 2014-2021 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Martell Malone <martellmalone@gmail.com>
 *          Steve Lhomme <robux4@gmail.com>
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

#ifndef HEIMDALL_DXGI_SWAPCHAIN_H
#define HEIMDALL_DXGI_SWAPCHAIN_H

#include <heimdall_common.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_media_player.h>

#include <wrl/client.h>

#include <dxgi1_5.h>
#include "../../video_chroma/dxgi_fmt.h"


#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(ppType) IID_PPV_ARGS(ppType)
#endif

#define DXGI_SWAP_FRAME_COUNT   3

struct dxgi_swapchain;

struct dxgi_swapchain *DXGI_CreateLocalSwapchainHandleHwnd(heimdall_object_t *, HWND);

struct dxgi_swapchain *DXGI_CreateLocalSwapchainHandleDComp(heimdall_object_t *,
                                           void /*IDCompositionDevice*/ * dcompDevice,
                                           void /*IDCompositionVisual*/ * dcompVisual);

Microsoft::WRL::ComPtr<IDXGISwapChain1> & DXGI_GetSwapChain1( struct dxgi_swapchain * );
Microsoft::WRL::ComPtr<IDXGISwapChain4> & DXGI_GetSwapChain4( struct dxgi_swapchain * );
const d3d_format_t  *DXGI_GetPixelFormat( struct dxgi_swapchain * );

void DXGI_SelectSwapchainColorspace( struct dxgi_swapchain *, const libheimdall_video_render_cfg_t *, bool match_display );
void DXGI_LocalSwapchainCleanupDevice( struct dxgi_swapchain * );
void DXGI_SwapchainUpdateOutput( struct dxgi_swapchain *, libheimdall_video_output_cfg_t * );
bool DXGI_UpdateSwapChain( struct dxgi_swapchain *, IDXGIAdapter *,
                           IUnknown *pFactoryDevice,
                           const d3d_format_t *, const libheimdall_video_render_cfg_t *,
                           bool match_display );

void DXGI_LocalSwapchainSwap( struct dxgi_swapchain * );
void DXGI_LocalSwapchainSetMetadata( struct dxgi_swapchain *, libheimdall_video_metadata_type_t, const void * );

#endif /* HEIMDALL_DXGI_SWAPCHAIN_H */
