/*****************************************************************************
 * va.c: hardware acceleration plugins for avcodec
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2012-2013 Rémi Denis-Courmont
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
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_modules.h>
#include <heimdall_fourcc.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include "va.h"

bool heimdall_va_MightDecode(enum AVPixelFormat hwfmt)
{
    switch (hwfmt)
    {
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_D3D11VA_VLD:
        case AV_PIX_FMT_VDPAU:
            return true;
        default:
            return false;
    }
}

heimdall_va_t *heimdall_va_New(heimdall_object_t *obj, struct heimdall_va_cfg *cfg)
{
    struct heimdall_va_t *va = heimdall_object_create(obj, sizeof (*va));
    if (unlikely(va == NULL))
        return NULL;

    module_t **mods;
    ssize_t total = heimdall_module_match("hw decoder", NULL, false, &mods, NULL);

    for (ssize_t i = 0; i < total; i++) {
        heimdall_va_open open = heimdall_module_map(obj->logger, mods[i]);

        if (open != NULL && open(va, cfg) == HEIMDALL_SUCCESS)
        {
            free(mods);
            return va;
        }
    }

    free(mods);
    heimdall_object_delete(va);
    return NULL;
}

void heimdall_va_Delete(heimdall_va_t *va, AVCodecContext *avctx)
{
    if (va->ops->close != NULL)
        va->ops->close(va, avctx);
    heimdall_object_delete(va);
}
