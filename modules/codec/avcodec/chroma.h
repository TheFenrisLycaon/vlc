/*****************************************************************************
 * chroma.h: decoder and encoder using libavcodec
 *****************************************************************************
 * Copyright (C) 2001-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

/* Heimdall <-> avutil tables */

#ifndef HEIMDALL_AVUTIL_CHROMA_H_
#define HEIMDALL_AVUTIL_CHROMA_H_

#include <libavutil/pixfmt.h>

struct heimdall_chroma_ffmpeg
{
    heimdall_fourcc_t  i_chroma;
    enum AVPixelFormat i_chroma_id;
    video_color_range_t range;
};

const struct heimdall_chroma_ffmpeg *GetHeimdallChromaFfmpegTable( size_t *count );

enum AVPixelFormat FindFfmpegChroma( heimdall_fourcc_t, bool *uv_flipped );

int GetHeimdallChroma( video_format_t *fmt, enum AVPixelFormat i_ffmpeg_chroma );

#endif
