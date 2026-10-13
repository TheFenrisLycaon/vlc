/*****************************************************************************
 * vout_wrapper.h: definitions for vout wrappers (temporary)
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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

#ifndef LIBHEIMDALL_VOUT_WRAPPER_H
#define LIBHEIMDALL_VOUT_WRAPPER_H 1

#include <heimdall_vout_display.h>

/* XXX DO NOT use it outside the vout module wrapper XXX */
struct vout_crop;

picture_t * vout_ConvertForDisplay(vout_display_t *, picture_t *);
void vout_FilterFlush(vout_display_t *);

void vout_SetDisplayFitting(vout_display_t *, enum heimdall_video_fitting);
void vout_SetDisplayZoom(vout_display_t *, unsigned num, unsigned den);
void vout_SetDisplayAspect(vout_display_t *, heimdall_rational_t dar);
void vout_SetDisplayCrop(vout_display_t *, const struct vout_crop *);
void vout_SetDisplayViewpoint(vout_display_t *, const heimdall_viewpoint_t *);
int vout_SetDisplayFormat(vout_display_t *, const video_format_t *fmt,
                          heimdall_video_context *vctx);

/* The owner/caller is responsible for managing the lifetime of this ICC
 * profile and always updating the display state to a consistent value */
void vout_SetDisplayIccProfile(vout_display_t *, const heimdall_icc_profile_t *);
void vout_SetDisplayProjection(vout_display_t *, video_projection_mode_t);
void vout_SetDisplayStereo(vout_display_t *, heimdall_stereoscopic_mode_t);

#endif /* LIBHEIMDALL_VOUT_WRAPPER_H */

