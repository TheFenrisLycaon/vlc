/*****************************************************************************
 * ftcache.h : Font Face and glyph cache freetype2
 *****************************************************************************
 * Copyright (C) 2020 - VideoLabs, VLC authors and VideoLAN
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef FTCACHE_H
#define FTCACHE_H

#include FT_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heimdall_ftcache_t heimdall_ftcache_t;

typedef struct
{
    char *psz_filename;
    unsigned idx;
    unsigned int charmap_index;

} heimdall_face_id_t;

heimdall_ftcache_t * heimdall_ftcache_New( heimdall_object_t *, FT_Library, unsigned );
void heimdall_ftcache_Delete( heimdall_ftcache_t * );

/* Glyphs managed by the cache. Always use heimdall_ftcache_Init/Release */
typedef struct
{
    FT_Glyph p_glyph;
    FTC_Node ref;
} heimdall_ftcache_glyph_t;

void heimdall_ftcache_Glyph_Init( heimdall_ftcache_glyph_t * );
void heimdall_ftcache_Glyph_Release( heimdall_ftcache_t *, heimdall_ftcache_glyph_t * );

heimdall_face_id_t * heimdall_ftcache_GetFaceID( heimdall_ftcache_t *, const char *psz_fontfile, int i_idx );

typedef struct
{
    int width_px;
    int height_px;
} heimdall_ftcache_metrics_t;

heimdall_face_id_t * heimdall_ftcache_GetActiveFaceInfo( heimdall_ftcache_t *, heimdall_ftcache_metrics_t * );
int heimdall_ftcache_GetGlyphForCurrentFace( heimdall_ftcache_t *, FT_UInt charmap_index,
                                        heimdall_ftcache_glyph_t *, FT_Long * );
FT_UInt heimdall_ftcache_LookupCMapIndex( heimdall_ftcache_t *, heimdall_face_id_t *faceid, FT_UInt codepoint );
/* Big fat warning : Do not store FT_Face.
 * Faces are fully managed by cache and possibly invalidated when changing face */
FT_Face heimdall_ftcache_LoadFaceByID( heimdall_ftcache_t *, heimdall_face_id_t *faceid,
                                  const heimdall_ftcache_metrics_t * );
int heimdall_ftcache_LoadFaceByIDNoSize( heimdall_ftcache_t *ftcache, heimdall_face_id_t *faceid );

/* Custom modified glyphs cache. Similar to native caching.
 * Stores and returns a refcounted copy of the original glyph. */
typedef struct heimdall_ftcache_custom_glyph_ref_Rec_ * heimdall_ftcache_custom_glyph_ref_t;
typedef struct
{
    FT_Glyph p_glyph;
    heimdall_ftcache_custom_glyph_ref_t ref;
} heimdall_ftcache_custom_glyph_t;

FT_Glyph heimdall_ftcache_GetOutlinedGlyph( heimdall_ftcache_t *ftcache, const heimdall_face_id_t *faceid,
                                       FT_UInt index, const heimdall_ftcache_metrics_t *,
                                       FT_Long style, int radius, const FT_Glyph sourceglyph,
                                       int(*createOutline)(FT_Glyph, FT_Glyph *, void *), void *,
                                       heimdall_ftcache_custom_glyph_ref_t * );

void heimdall_ftcache_Custom_Glyph_Init( heimdall_ftcache_custom_glyph_t * );
void heimdall_ftcache_Custom_Glyph_Release( heimdall_ftcache_custom_glyph_t * );

#ifdef __cplusplus
}
#endif

#endif
