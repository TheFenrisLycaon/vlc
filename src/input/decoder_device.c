/*****************************************************************************
 * decoder_device.c: Decoder device and video context
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

#include <heimdall_common.h>
#include <heimdall_codec.h>
#include <heimdall_modules.h>
#include "../libheimdall.h"

struct heimdall_decoder_device_priv
{
    struct heimdall_decoder_device device;
    heimdall_atomic_rc_t rc;
};

static int decoder_device_Open(void *func, bool forced, va_list ap)
{
    HEIMDALL_UNUSED(forced);
    heimdall_decoder_device_Open open = func;
    heimdall_decoder_device *device = va_arg(ap, heimdall_decoder_device *);
    heimdall_window_t *window = va_arg(ap, heimdall_window_t *);
    int ret = open(device, window);
    if (ret != HEIMDALL_SUCCESS)
        heimdall_objres_clear(&device->obj);
    return ret;
}

heimdall_decoder_device *
heimdall_decoder_device_Create(heimdall_object_t *o, heimdall_window_t *window)
{
    struct heimdall_decoder_device_priv *priv =
            heimdall_custom_create(o, sizeof (*priv), "decoder device");
    if (!priv)
        return NULL;
    char *name = var_InheritString(o, "dec-dev");
    module_t *module = heimdall_module_load(heimdall_object_logger(&priv->device),
                                       "decoder device", name,true,
                                       decoder_device_Open, &priv->device,
                                       window);
    free(name);
    if (module == NULL)
    {
        heimdall_objres_clear(HEIMDALL_OBJECT(&priv->device));
        heimdall_object_delete(&priv->device);
        return NULL;
    }
    assert(priv->device.ops != NULL);
    heimdall_atomic_rc_init(&priv->rc);
    return &priv->device;
}

heimdall_decoder_device *
heimdall_decoder_device_Hold(heimdall_decoder_device *device)
{
    struct heimdall_decoder_device_priv *priv =
            container_of(device, struct heimdall_decoder_device_priv, device);
    heimdall_atomic_rc_inc(&priv->rc);
    return device;
}

void
heimdall_decoder_device_Release(heimdall_decoder_device *device)
{
    struct heimdall_decoder_device_priv *priv =
            container_of(device, struct heimdall_decoder_device_priv, device);
    if (heimdall_atomic_rc_dec(&priv->rc))
    {
        if (device->ops->close != NULL)
            device->ops->close(device);
        heimdall_objres_clear(HEIMDALL_OBJECT(device));
        heimdall_object_delete(device);
    }
}

/* video context */

struct heimdall_video_context
{
    heimdall_atomic_rc_t    rc;
    heimdall_decoder_device *device;
    const struct heimdall_video_context_operations *ops;
    enum heimdall_video_context_type private_type;
    size_t private_size;
    uint8_t private[];
};

heimdall_video_context * heimdall_video_context_Create(heimdall_decoder_device *device,
                                          enum heimdall_video_context_type private_type,
                                          size_t private_size,
                                          const struct heimdall_video_context_operations *ops)
{
    assert(private_type != 0);
    heimdall_video_context *vctx = malloc(sizeof(*vctx) + private_size);
    if (unlikely(vctx == NULL))
        return NULL;
    heimdall_atomic_rc_init( &vctx->rc );
    vctx->private_type = private_type;
    vctx->private_size = private_size;
    vctx->device = device;
    if (vctx->device)
        heimdall_decoder_device_Hold( vctx->device );
    vctx->ops = ops;
    return vctx;
}

void *heimdall_video_context_GetPrivate(heimdall_video_context *vctx, enum heimdall_video_context_type type)
{
    if (vctx && vctx->private_type == type)
        return &vctx->private;
    return NULL;
}

enum heimdall_video_context_type heimdall_video_context_GetType(const heimdall_video_context *vctx)
{
    return vctx->private_type;
}

heimdall_video_context *heimdall_video_context_Hold(heimdall_video_context *vctx)
{
    heimdall_atomic_rc_inc( &vctx->rc );
    return vctx;
}

void heimdall_video_context_Release(heimdall_video_context *vctx)
{
    if ( heimdall_atomic_rc_dec( &vctx->rc ) )
    {
        if (vctx->device)
            heimdall_decoder_device_Release( vctx->device );
        if ( vctx->ops && vctx->ops->destroy )
            vctx->ops->destroy( heimdall_video_context_GetPrivate(vctx, vctx->private_type) );
        free(vctx);
    }
}

heimdall_decoder_device* heimdall_video_context_HoldDevice(heimdall_video_context *vctx)
{
    if (!vctx->device)
        return NULL;
    return heimdall_decoder_device_Hold( vctx->device );
}
