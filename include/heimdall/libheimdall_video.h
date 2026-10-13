/*****************************************************************************
 * libheimdall_video.h: libheimdall video-related enumerations
 *****************************************************************************
 * Copyright (C) 1998-2010 VLC authors and VideoLAN
 * Copyright (C) 2023      Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Filippo Carone <littlejohn@videolan.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
 *          Alexandre Janniaux <ajanni@videolabs.io>
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
#ifndef HEIMDALL_LIBHEIMDALL_VIDEO_H
#define HEIMDALL_LIBHEIMDALL_VIDEO_H 1

# ifdef __cplusplus
extern "C"{
# endif

typedef enum libheimdall_video_orient_t
{
    libheimdall_video_orient_top_left,       /**< Normal. Top line represents top, left column left. */
    libheimdall_video_orient_top_right,      /**< Flipped horizontally */
    libheimdall_video_orient_bottom_left,    /**< Flipped vertically */
    libheimdall_video_orient_bottom_right,   /**< Rotated 180 degrees */
    libheimdall_video_orient_left_top,       /**< Transposed */
    libheimdall_video_orient_left_bottom,    /**< Rotated 90 degrees clockwise (or 270 anti-clockwise) */
    libheimdall_video_orient_right_top,      /**< Rotated 90 degrees anti-clockwise */
    libheimdall_video_orient_right_bottom    /**< Anti-transposed */
} libheimdall_video_orient_t;

typedef enum libheimdall_video_projection_t
{
    libheimdall_video_projection_rectangular,
    libheimdall_video_projection_equirectangular, /**< 360 spherical */

    libheimdall_video_projection_cubemap_layout_standard = 0x100,
} libheimdall_video_projection_t;

typedef enum libheimdall_video_multiview_t
{
    libheimdall_video_multiview_2d,                  /**< No stereoscopy: 2D picture. */
    libheimdall_video_multiview_stereo_sbs,          /**< Side-by-side */
    libheimdall_video_multiview_stereo_tb,           /**< Top-bottom */
    libheimdall_video_multiview_stereo_row,          /**< Row sequential */
    libheimdall_video_multiview_stereo_col,          /**< Column sequential */
    libheimdall_video_multiview_stereo_frame,        /**< Frame sequential */
    libheimdall_video_multiview_stereo_checkerboard, /**< Checkerboard pattern */
} libheimdall_video_multiview_t;

# ifdef __cplusplus
} // extern "C"
# endif

#endif
