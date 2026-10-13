/*****************************************************************************
 * gstheimdallvideopool.h: Heimdall pictures managed by GstBufferPool
 *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Vikram Fugro <vikram.fugro@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifndef HEIMDALL_GST_VIDEO_POOL_H
#define HEIMDALL_GST_VIDEO_POOL_H

#include "gstheimdallpictureplaneallocator.h"

typedef struct _GstHeimdallVideoPool GstHeimdallVideoPool;
typedef struct _GstHeimdallVideoPoolClass GstHeimdallVideoPoolClass;

/* buffer pool functions */
#define GST_TYPE_HEIMDALL_VIDEO_POOL      (gst_heimdall_video_pool_get_type())
#define GST_IS_HEIMDALL_VIDEO_POOL(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        GST_TYPE_HEIMDALL_VIDEO_POOL))
#define GST_HEIMDALL_VIDEO_POOL(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        GST_TYPE_HEIMDALL_VIDEO_POOL, \
                                        GstHeimdallVideoPool))
#define GST_HEIMDALL_VIDEO_POOL_CAST(obj) ((GstHeimdallVideoPool*)(obj))

struct _GstHeimdallVideoPool
{
    GstBufferPool bufferpool;
    GstHeimdallPicturePlaneAllocator *p_allocator;

    GstCaps *p_caps;
    GstVideoInfo info;
    GstVideoAlignment align;
    bool b_add_metavideo;
    bool b_need_aligned;

    decoder_t *p_dec;
};

struct _GstHeimdallVideoPoolClass
{
    GstBufferPoolClass parent_class;
};

GType gst_heimdall_video_pool_get_type( void );
GstHeimdallVideoPool* gst_heimdall_video_pool_new(
        GstAllocator *p_allocator, decoder_t *p_dec );

#endif /*__GST_HEIMDALL_VIDEO_POOL_H__*/
