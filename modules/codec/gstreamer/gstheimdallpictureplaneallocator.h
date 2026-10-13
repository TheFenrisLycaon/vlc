/*****************************************************************************
 * gstheimdallpictureplaneallocator.h: Heimdall pictures wrapped by GstAllocator
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
#ifndef HEIMDALL_GST_PICTURE_PLANE_ALLOCATOR_H_
#define HEIMDALL_GST_PICTURE_PLANE_ALLOCATOR_H_

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>

#include <heimdall_common.h>
#include <heimdall_codec.h>

typedef struct _GstHeimdallPicturePlane GstHeimdallPicturePlane;
typedef struct _GstHeimdallPicturePlaneAllocator GstHeimdallPicturePlaneAllocator;
typedef struct _GstHeimdallPicturePlaneAllocatorClass \
                                    GstHeimdallPicturePlaneAllocatorClass;

/* allocator functions */
#define GST_TYPE_HEIMDALL_PICTURE_PLANE_ALLOCATOR      \
    (gst_heimdall_picture_plane_allocator_get_type())
#define GST_IS_HEIMDALL_PICTURE_PLANE_ALLOCATOR(obj)   \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_HEIMDALL_PICTURE_PLANE_ALLOCATOR))
#define GST_HEIMDALL_PICTURE_PLANE_ALLOCATOR(obj)      \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_HEIMDALL_PICTURE_PLANE_ALLOCATOR, \
    GstHeimdallPicturePlaneAllocator))
#define GST_HEIMDALL_PICTURE_PLANE_ALLOCATOR_CAST(obj) \
    ((GstHeimdallPicturePlaneAllocator*)(obj))

struct _GstHeimdallPicturePlane
{
    GstMemory parent;

    decoder_t *p_dec;
    picture_t *p_pic;
    plane_t *p_plane;
};

struct _GstHeimdallPicturePlaneAllocator
{
    GstAllocator parent;
    picture_t pic_info;

    decoder_t *p_dec;
};

struct _GstHeimdallPicturePlaneAllocatorClass
{
    GstAllocatorClass parent_class;
};

GType gst_heimdall_picture_plane_allocator_get_type( void );
GstHeimdallPicturePlaneAllocator* gst_heimdall_picture_plane_allocator_new(
        decoder_t *p_dec );
bool gst_heimdall_picture_plane_allocator_query_format(
        GstHeimdallPicturePlaneAllocator *p_gallocator, GstVideoInfo *p_info,
        GstVideoAlignment *p_align, GstCaps *p_caps );
bool gst_heimdall_set_vout_fmt( GstVideoInfo *p_info, GstVideoAlignment *p_align,
        GstCaps *p_caps, decoder_t *p_dec );
heimdall_fourcc_t gst_heimdall_to_map_format( const char* psz_fourcc );
void gst_heimdall_dec_ensure_empty_queue( decoder_t* p_dec );
bool gst_heimdall_picture_plane_allocator_hold( GstHeimdallPicturePlaneAllocator
        *p_allocator, GstBuffer *p_buffer );
void gst_heimdall_picture_plane_allocator_release(
        GstHeimdallPicturePlaneAllocator *p_allocator, GstBuffer *p_buffer );
bool gst_heimdall_picture_plane_allocator_alloc(
        GstHeimdallPicturePlaneAllocator *p_allocator,
        GstBuffer *p_buffer );
#endif
