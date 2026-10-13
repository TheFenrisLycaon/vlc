/*****************************************************************************
 * heimdall_spu.h: spu_t definition and functions.
 *****************************************************************************
 * Copyright (C) 1999-2010 VLC authors and VideoLAN
 * Copyright (C) 2010 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *          Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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

#ifndef HEIMDALL_SPU_H
#define HEIMDALL_SPU_H 1

#include <heimdall_common.h>
#include <heimdall_tick.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vout_display_place_t;

/**
 * \defgroup spu Sub-picture channels
 * \ingroup video_output
 * @{
 * \file
 */

struct heimdall_render_subpicture;

/**
 * Subpicture unit descriptor
 */
struct spu_t
{
    struct heimdall_object_t obj;
};

HEIMDALL_API spu_t * spu_Create( heimdall_object_t *, vout_thread_t * );
#define spu_Create(a,b) spu_Create(HEIMDALL_OBJECT(a),b)
HEIMDALL_API void spu_Destroy( spu_t * );

/**
 * This function sends a subpicture to the spu_t core.
 *
 * You cannot use the provided subpicture anymore. The spu_t core
 * will destroy it at its convenience.
 */
HEIMDALL_API void spu_PutSubpicture( spu_t *, subpicture_t * );

/**
 * This function will return an unique subpicture containing the OSD and
 * subtitles visible at the requested date.
 *
 * \param spu the subpicture unit instance
 * \param p_chroma_list is a list of supported chroma for the output (can be NULL)
 * \param p_fmt_dst is the format of the picture on which the return subpicture will be rendered.
 * \param p_fmt_src is the format of the original(source) video.
 * \param video_position position of the video inside the display or NULL to fit in p_fmt_dst
 * \param system_now the reference current time
 * \param pts the timestamp of the rendered frame
 * \param ignore_osd whether we display the OSD or not
 *
 * The returned value if non NULL must be released by subpicture_Delete().
 */
HEIMDALL_API struct heimdall_render_subpicture * spu_Render( spu_t *spu, const heimdall_fourcc_t *p_chroma_list,
                                   const video_format_t *p_fmt_dst, const video_format_t *p_fmt_src,
                                   bool spu_in_full_window,
                                   const struct vout_display_place_t *video_position,
                                   heimdall_tick_t system_now, heimdall_tick_t pts,
                                   bool ignore_osd );

/**
 * It registers a new SPU channel.
 */
HEIMDALL_API ssize_t spu_RegisterChannel( spu_t * );
HEIMDALL_API void spu_UnregisterChannel( spu_t *, size_t );

/**
 * It clears all subpictures associated to a SPU channel.
 */
HEIMDALL_API void spu_ClearChannel( spu_t *, size_t );

/**
 * It changes the sub sources list
 */
HEIMDALL_API void spu_ChangeSources( spu_t *, const char * );

/**
 * It changes the sub filters list
 */
HEIMDALL_API void spu_ChangeFilters( spu_t *, const char * );

/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* HEIMDALL_SPU_H */
