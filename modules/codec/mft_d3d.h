/*****************************************************************************
 * mft_d3d.h : Media Foundation Transform audio/video decoder D3D helpers
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Steve Lhomme <slhomme@videolabs.io>
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

#ifndef HEIMDALL_MFT_D3D_H
#define HEIMDALL_MFT_D3D_H

#include <windows.h>

#include <wrl/client.h>

#include "../video_chroma/dxgi_fmt.h"

#if !defined(_GAMING_XBOX_SCARLETT) && !defined(_GAMING_XBOX_XBOXONE)
#define IGraphicsUnknown IUnknown
#endif // !_GAMING_XBOX_XBOXONE && !_GAMING_XBOX_XBOXONE

struct IMFTransform;
struct IMFDXGIDeviceManager;
struct IMFDXGIBuffer;

struct heimdall_logger;
struct picture_context_t;
struct es_format_t;
struct heimdall_video_context;
struct heimdall_decoder_device;

class heimdall_mft_ref
{
public:
    virtual void AddRef() = 0;
    /**
     * \return true if the object was deleted
     */
    virtual bool Release() = 0;
};

class heimdall_mf_d3d
{
public:
    ~heimdall_mf_d3d();

    void Init();

    bool CanUseD3D() const;

    HRESULT SetD3D(heimdall_logger *logger, IGraphicsUnknown *d3d_device, Microsoft::WRL::ComPtr<IMFTransform> &);

    void ReleaseD3D(Microsoft::WRL::ComPtr<IMFTransform> &);

private:
    UINT dxgi_token = 0;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> dxgi_manager;
    HANDLE d3d_handle = INVALID_HANDLE_VALUE;
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
    HRESULT (WINAPI *fptr_MFCreateDXGIDeviceManager)(UINT *resetToken, IMFDXGIDeviceManager **ppDeviceManager) = nullptr;
#endif
};

class MFHW_d3d
{
public:
    MFHW_d3d();
    virtual ~MFHW_d3d() = default;
    virtual HRESULT SetD3D(heimdall_logger *, heimdall_decoder_device & dec_dev, Microsoft::WRL::ComPtr<IMFTransform> & mft) = 0;
    virtual void Release(Microsoft::WRL::ComPtr<IMFTransform> & mft);


    virtual picture_context_t *CreatePicContext(heimdall_logger *, Microsoft::WRL::ComPtr<IMFDXGIBuffer> &,
                                                heimdall_mft_ref *) = 0;

    virtual HRESULT SetupVideoContext(heimdall_logger *, Microsoft::WRL::ComPtr<IMFDXGIBuffer> &, es_format_t & fmt_out) = 0;

    heimdall_video_context *vctx_out = nullptr;
    heimdall_decoder_device *dec_dev = nullptr;

protected:
    HRESULT SetD3D(heimdall_logger *, heimdall_decoder_device &, IGraphicsUnknown *, Microsoft::WRL::ComPtr<IMFTransform> &);

private:
    heimdall_mf_d3d d3d;
};

#endif // HEIMDALL_MFT_D3D_H
