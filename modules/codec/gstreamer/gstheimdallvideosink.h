/*****************************************************************************
 * gstheimdallvideosink.h: Heimdall gstreamer video sink
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
#ifndef HEIMDALL_GST_VIDEO_SINK_H
#define HEIMDALL_GST_VIDEO_SINK_H

#include <gst/gst.h>
#include <gst/gstallocator.h>

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/base/gstbasesink.h>

#include <heimdall_codec.h>

typedef struct _GstHeimdallVideoSink GstHeimdallVideoSink;
typedef struct _GstHeimdallVideoSinkClass GstHeimdallVideoSinkClass;

#define GST_TYPE_HEIMDALL_VIDEO_SINK \
    (gst_heimdall_video_sink_get_type())
#define GST_HEIMDALL_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HEIMDALL_VIDEO_SINK, \
        GstHeimdallVideoSink))
#define GST_HEIMDALL_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HEIMDALL_VIDEO_SINK, \
        GstHeimdallVideoSinkClass))
#define GST_IS_HEIMDALL_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HEIMDALL_VIDEO_SINK))
#define GST_IS_HEIMDALL_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HEIMDALL_VIDEO_SINK))

struct _GstHeimdallVideoSink
{
    GstBaseSink parent;

    GstAllocator *p_allocator;
    GstVideoInfo vinfo;
    gboolean b_use_pool;

    decoder_t *p_dec;

    //FIXME: caps_signal
    gboolean (*new_caps) ( GstElement *p_ele, GstCaps *p_caps,
            gpointer p_data );
};

struct _GstHeimdallVideoSinkClass
{
    GstBaseSinkClass parent_class;

    //FIXME: caps_signal
#if 0
    gboolean (*new_caps) ( GstElement *p_ele, GstCaps *p_caps,
            gpointer p_data );
#endif
    void (*new_buffer) ( GstElement *p_ele, GstBuffer *p_buffer,
            gpointer p_data );
};

GType gst_heimdall_video_sink_get_type (void);

gboolean gst_heimdall_video_sink_query_caps( GstQuery *p_query );

#endif /* __GST_HEIMDALL_VIDEO_SINK_H__ */
