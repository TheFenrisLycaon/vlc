
/*****************************************************************************
 * fourcc.c: convert between gst <->  heimdall formats
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_codec.h>

heimdall_fourcc_t GetGstHeimdallFourcc( const char* gst );

typedef struct
{
    char gst [10];
    heimdall_fourcc_t i_fourcc;
} gst_heimdall_rawvideo_fourcc;

/*
 * Raw Video Formats
 */
static const gst_heimdall_rawvideo_fourcc raw_video_fmts[] =
{
    // NOTE: These should be sorted entries, keyed by `gst` field
    // cat entries | tr -dc "[:alnum:][:space:]_" | sort -n -k1 | xargs printf "{ \"%s\", %s },\n"
    { "I420_10BE", HEIMDALL_CODEC_I420_10B },
    { "I420_10LE", HEIMDALL_CODEC_I420_10L },
    { "I420_12BE", HEIMDALL_CODEC_I420_12B },
    { "I420_12LE", HEIMDALL_CODEC_I420_12L },
    { "I420_16BE", HEIMDALL_CODEC_I420_16B },
    { "I420_16LE", HEIMDALL_CODEC_I420_16L },
    { "I420_9BE", HEIMDALL_CODEC_I420_9B },
    { "I420_9LE", HEIMDALL_CODEC_I420_9L },
    { "I422_10BE", HEIMDALL_CODEC_I422_10B },
    { "I422_10LE", HEIMDALL_CODEC_I422_10L },
    { "I422_12BE", HEIMDALL_CODEC_I422_12B },
    { "I422_12LE", HEIMDALL_CODEC_I422_12L },
    { "I422_16BE", HEIMDALL_CODEC_I422_16B },
    { "I422_16LE", HEIMDALL_CODEC_I422_16L },
    { "I422_9BE", HEIMDALL_CODEC_I422_9B },
    { "I422_9LE", HEIMDALL_CODEC_I422_9L },
    { "I444_10BE", HEIMDALL_CODEC_I444_10B },
    { "I444_10LE", HEIMDALL_CODEC_I444_10L },
    { "I444_12BE", HEIMDALL_CODEC_I444_12B },
    { "I444_12LE", HEIMDALL_CODEC_I444_12L },
    { "I444_16BE", HEIMDALL_CODEC_I444_16B },
    { "I444_16LE", HEIMDALL_CODEC_I444_16L },
    { "I444_9BE", HEIMDALL_CODEC_I444_9B },
    { "I444_9LE", HEIMDALL_CODEC_I444_9L },
};

static int compare_func( const void* key, const void* ent )
{
    return strcmp( (char*)key, ((gst_heimdall_rawvideo_fourcc*)ent)->gst );
}

heimdall_fourcc_t GetGstHeimdallFourcc( const char* gst )
{
    const gst_heimdall_rawvideo_fourcc* found = NULL;

    if( !gst )
    {
        return HEIMDALL_CODEC_UNKNOWN;
    }

    found = bsearch( gst, raw_video_fmts,
                ARRAY_SIZE(raw_video_fmts), sizeof(gst_heimdall_rawvideo_fourcc),
                compare_func );

    if( !found )
        return HEIMDALL_CODEC_UNKNOWN;
    else
        return found->i_fourcc;
}
