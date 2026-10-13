/*****************************************************************************
 * d3d11_device.c : D3D11 decoder device from external ID3D11DeviceContext
 *****************************************************************************
 * Copyright © 2019 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Steve Lhomme <robux4@ycbcr.xyz>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_codec.h>

#include "d3d11_filters.h"

static void D3D11CloseDecoderDevice(heimdall_decoder_device *device)
{
    d3d11_decoder_device_t *dec_device = device->opaque;
    D3D11_ReleaseDevice( dec_device );
}

static const struct heimdall_decoder_device_operations d3d11_dev_ops = {
    .close = D3D11CloseDecoderDevice,
};

static int D3D11OpenDecoderDevice(heimdall_decoder_device *device, bool forced, heimdall_window_t *wnd)
{
    HEIMDALL_UNUSED(wnd);

    d3d11_decoder_device_t *dec_device;
    dec_device = D3D11_CreateDevice( device, NULL, true /* is_d3d11_opaque(chroma) */,
                                          forced );
    if ( dec_device == NULL )
        return HEIMDALL_EGENERIC;

    device->ops = &d3d11_dev_ops;
    device->opaque = dec_device;
    device->type = HEIMDALL_DECODER_DEVICE_D3D11VA;
    device->sys = NULL;

    return HEIMDALL_SUCCESS;
}

int D3D11OpenDecoderDeviceW8(heimdall_decoder_device *device, heimdall_window_t *wnd)
{
    return D3D11OpenDecoderDevice(device, false, wnd);
}

int D3D11OpenDecoderDeviceAny(heimdall_decoder_device *device, heimdall_window_t *wnd)
{
    return D3D11OpenDecoderDevice(device, true, wnd);
}
