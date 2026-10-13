/*****************************************************************************
 * rv32.c: conversion plugin to RV32 format.
 *****************************************************************************
 * Copyright (C) 2005 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Cyril Deguet <asmax@videolan.org>
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
#include <heimdall_plugin.h>
#include <heimdall_filter.h>
#include <heimdall_picture.h>
#include <heimdall_chroma_probe.h>

/****************************************************************************
 * Local prototypes
 ****************************************************************************/
static int  OpenFilter ( filter_t * );
static picture_t *Filter( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static void ProbeChroma(heimdall_chroma_conv_vec *vec)
{
    heimdall_chroma_conv_add(vec, 1, HEIMDALL_CODEC_BGR24, HEIMDALL_CODEC_RGBA, false);
}

heimdall_module_begin ()
    set_description( N_("RV32 conversion filter") )
    set_callback_video_converter( OpenFilter, 1 )
    add_submodule()
        set_callback_chroma_conv_probe(ProbeChroma)
heimdall_module_end ()

static const struct heimdall_filter_operations filter_ops = {
    .filter_video = Filter,
};

/*****************************************************************************
 * OpenFilter: probe the filter and return score
 *****************************************************************************/
static int OpenFilter( filter_t *p_filter )
{
    /* XXX Only support RV24 -> RV32 conversion */
    if( p_filter->fmt_in.video.i_chroma != HEIMDALL_CODEC_BGR24 ||
        p_filter->fmt_out.video.i_chroma != HEIMDALL_CODEC_RGBA )
    {
        return HEIMDALL_EGENERIC;
    }

    if( p_filter->fmt_in.video.i_width != p_filter->fmt_out.video.i_width
     || p_filter->fmt_in.video.i_height != p_filter->fmt_out.video.i_height
     || p_filter->fmt_in.video.orientation != p_filter->fmt_out.video.orientation)
        return -1;

    p_filter->ops = &filter_ops;

    return HEIMDALL_SUCCESS;
}

/****************************************************************************
 * Filter: the whole thing
 ****************************************************************************/
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    picture_t *p_pic_dst;
    int i_plane, i;
    unsigned int j;

    /* Request output picture */
    p_pic_dst = filter_NewPicture( p_filter );
    if( !p_pic_dst )
    {
        picture_Release( p_pic );
        return NULL;
    }

    /* Convert RV24 to RV32 */
    for( i_plane = 0; i_plane < p_pic_dst->i_planes; i_plane++ )
    {
        uint8_t *p_src = p_pic->p[i_plane].p_pixels;
        uint8_t *p_dst = p_pic_dst->p[i_plane].p_pixels;
        unsigned int i_width = p_filter->fmt_out.video.i_width;

        for( i = 0; i < p_pic_dst->p[i_plane].i_lines; i++ )
        {
            for( j = 0; j < i_width; j++ )
            {
                *(p_dst++) = p_src[2];
                *(p_dst++) = p_src[1];
                *(p_dst++) = p_src[0];
                *(p_dst++) = 0xff;  /* Alpha */
                p_src += 3;
            }
            p_src += p_pic->p[i_plane].i_pitch - 3 * i_width;
            p_dst += p_pic_dst->p[i_plane].i_pitch - 4 * i_width;
        }
    }

    picture_CopyProperties( p_pic_dst, p_pic );
    picture_Release( p_pic );

    return p_pic_dst;
}

