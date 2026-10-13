/*****************************************************************************
 * ftcache.c : Font Face and glyph cache freetype2
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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_filter.h>
#include <heimdall_text_style.h>

/* Freetype */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "ftcache.h"
#include "platform_fonts.h"
#include "lru.h"

struct heimdall_ftcache_t
{
    heimdall_object_t *obj;
    /** Font face cache */
    heimdall_dictionary_t  face_ids;
    FTC_ScalerRec     scaler;
    FTC_Manager       cachemanager;
    FTC_ImageCache    image_cache;
    FTC_CMapCache     charmap_cache;
    /* Derived glyph cache */
    heimdall_lru *         glyphs_lrucache;
    /* current face properties */
    FT_Long           style_flags;
};

struct heimdall_ftcache_custom_glyph_ref_Rec_
{
    FT_Glyph glyph;
    unsigned refcount;
};

heimdall_face_id_t * heimdall_ftcache_GetFaceID( heimdall_ftcache_t *ftcache,
                                       const char *psz_fontfile, int i_idx )
{
    char *psz_key;
    if( asprintf( &psz_key, "%s#%d",
                  psz_fontfile, i_idx ) < 0 )
        return NULL;

    heimdall_face_id_t *faceid = heimdall_dictionary_value_for_key( &ftcache->face_ids, psz_key );
    if( !faceid )
    {
        faceid = malloc(sizeof(heimdall_face_id_t));
        if( faceid )
        {
            faceid->idx = i_idx;
            faceid->psz_filename = strdup(psz_fontfile);
            if( faceid->psz_filename )
            {
                faceid->charmap_index = -1;
                heimdall_dictionary_insert( &ftcache->face_ids, psz_key, faceid );
            }
            else
            {
                free( faceid );
                faceid = NULL;
            }
        }
    }
    free( psz_key );
    return faceid;
}

heimdall_face_id_t * heimdall_ftcache_GetActiveFaceInfo( heimdall_ftcache_t *ftcache,
                                               heimdall_ftcache_metrics_t *metrics )
{
    if( metrics )
    {
        metrics->width_px = ftcache->scaler.width;
        metrics->height_px = ftcache->scaler.height;
    }
    return (heimdall_face_id_t *) ftcache->scaler.face_id;
}

static int heimdall_ftcache_SetCharmap( heimdall_face_id_t *faceid, FT_Face p_face )
{
    if( FT_Select_Charmap( p_face, FT_ENCODING_UNICODE ) )
        /* We've loaded a font face which is unhelpful for actually rendering text */
        return HEIMDALL_EGENERIC;

    faceid->charmap_index =
            p_face->charmap ? FT_Get_Charmap_Index( p_face->charmap ) : 0;

    return HEIMDALL_SUCCESS;
}

FT_Face heimdall_ftcache_LoadFaceByID( heimdall_ftcache_t *ftcache, heimdall_face_id_t *faceid,
                                  const heimdall_ftcache_metrics_t *metrics )
{
    ftcache->scaler.face_id = faceid;
    ftcache->scaler.width = metrics->width_px;
    ftcache->scaler.height = metrics->height_px;

    FT_Size size;
    if(FTC_Manager_LookupSize( ftcache->cachemanager, &ftcache->scaler, &size ))
    {
        ftcache->scaler.face_id = NULL;
        ftcache->scaler.width = 0;
        ftcache->scaler.height = 0;
        return NULL;
    }

    return size->face;
}

int heimdall_ftcache_LoadFaceByIDNoSize( heimdall_ftcache_t *ftcache, heimdall_face_id_t *faceid )
{
    FT_Face face;
    if( FTC_Manager_LookupFace( ftcache->cachemanager, faceid, &face ) )
        return HEIMDALL_EGENERIC;

    ftcache->style_flags = face->style_flags;
    ftcache->scaler.face_id = faceid;
    ftcache->scaler.width = 0;
    ftcache->scaler.height = 0;
    return HEIMDALL_SUCCESS;
}

FT_UInt heimdall_ftcache_LookupCMapIndex( heimdall_ftcache_t *ftcache, heimdall_face_id_t *faceid, FT_UInt codepoint )
{
    return FTC_CMapCache_Lookup( ftcache->charmap_cache, faceid,
                                 faceid->charmap_index, codepoint );
}

int heimdall_ftcache_GetGlyphForCurrentFace( heimdall_ftcache_t *ftcache, FT_UInt index,
                                        heimdall_ftcache_glyph_t *glyph, FT_Long *styleflags )
{
    int ret = FTC_ImageCache_LookupScaler( ftcache->image_cache,
                                           &ftcache->scaler,
                                           FT_LOAD_NO_BITMAP | FT_LOAD_DEFAULT,
                                           index, &glyph->p_glyph, &glyph->ref );
    if( !ret && styleflags )
        *styleflags = ftcache->style_flags;
    return ret;
}

static FT_Error RequestFace( FTC_FaceID face_id, FT_Library library,
                             FT_Pointer req_data, FT_Face* aface )
{
    heimdall_ftcache_t *ftcache = (heimdall_ftcache_t *) req_data;
    heimdall_face_id_t *faceid = (heimdall_face_id_t *) face_id;
    HEIMDALL_UNUSED(library);

    *aface = doLoadFace( ftcache->obj, faceid->psz_filename, faceid->idx);

    if( !*aface || heimdall_ftcache_SetCharmap( faceid, *aface ) )
        return -1;

    return 0;
}

static void LRUGlyphRefRelease( void *priv, void *v )
{
    heimdall_ftcache_custom_glyph_ref_t ref = (heimdall_ftcache_custom_glyph_ref_t) v;
    HEIMDALL_UNUSED(priv);
    assert(ref->refcount);
    if( --ref->refcount == 0 )
    {
        FT_Done_Glyph( ref->glyph );
        free( ref );
    }
}

static void FreeFaceID( void *p_faceid, void *p_obj )
{
    HEIMDALL_UNUSED(p_obj);
    heimdall_face_id_t *faceid = p_faceid;
    free(faceid->psz_filename);
    free(faceid);
}

void heimdall_ftcache_Delete( heimdall_ftcache_t *ftcache )
{
    if( ftcache->glyphs_lrucache )
        heimdall_lru_Release( ftcache->glyphs_lrucache );

    if( ftcache->cachemanager )
        FTC_Manager_Done( ftcache->cachemanager );

    /* Fonts dicts */
    heimdall_dictionary_clear( &ftcache->face_ids, FreeFaceID, NULL );

    free( ftcache );
}

heimdall_ftcache_t * heimdall_ftcache_New( heimdall_object_t *obj, FT_Library p_library,
                                 unsigned maxkb )
{
    heimdall_ftcache_t *ftcache = calloc(1, sizeof(*ftcache));
    if(!ftcache)
        return NULL;
    ftcache->obj = obj;

    /* Dictionaries for fonts */
    heimdall_dictionary_init( &ftcache->face_ids, 50 );

    ftcache->glyphs_lrucache = heimdall_lru_New( 128, LRUGlyphRefRelease, ftcache );

    if(!ftcache->glyphs_lrucache ||
       FTC_Manager_New( p_library, 4, 8, maxkb << 10,
                        RequestFace, ftcache, &ftcache->cachemanager ) ||
       FTC_ImageCache_New( ftcache->cachemanager, &ftcache->image_cache ) ||
       FTC_CMapCache_New( ftcache->cachemanager, &ftcache->charmap_cache ))
    {
        heimdall_ftcache_Delete( ftcache );
        return NULL;
    }

    ftcache->scaler.pixel = 1;
    ftcache->scaler.x_res = 0;
    ftcache->scaler.y_res = 0;

    return ftcache;
}

void heimdall_ftcache_Glyph_Release( heimdall_ftcache_t *ftcache, heimdall_ftcache_glyph_t *g )
{
    if( g->ref )
    {
        FTC_Node_Unref( g->ref, ftcache->cachemanager );
        g->ref = NULL;
        g->p_glyph = NULL;
    }
    else if( g->p_glyph ) /* glyph is not cache reference */
    {
        FT_Done_Glyph( g->p_glyph );
        g->p_glyph = NULL;
    }
}

void heimdall_ftcache_Glyph_Init( heimdall_ftcache_glyph_t *g )
{
    g->p_glyph = NULL;
    g->ref = NULL;
}

static heimdall_ftcache_custom_glyph_ref_t
heimdall_ftcache_GetCustomGlyph( heimdall_ftcache_t *ftcache, const char *psz_key )
{
    heimdall_ftcache_custom_glyph_ref_t ref = heimdall_lru_Get( ftcache->glyphs_lrucache, psz_key );
    if( ref )
        ref->refcount++;
    return ref;
}

static heimdall_ftcache_custom_glyph_ref_t
heimdall_ftcache_AddCustomGlyph( heimdall_ftcache_t *ftcache, const char *psz_key, FT_Glyph glyph )
{
    assert(!heimdall_lru_Get( ftcache->glyphs_lrucache, psz_key ));
    heimdall_ftcache_custom_glyph_ref_t ref = malloc( sizeof(*ref) );
    if( ref )
    {
        ref->refcount = 2;
        ref->glyph = glyph;
        heimdall_lru_Insert( ftcache->glyphs_lrucache, psz_key, ref );
    }
    return ref;
}

void heimdall_ftcache_Custom_Glyph_Release( heimdall_ftcache_custom_glyph_t *g )
{
    if( g->ref )
    {
        assert(g->ref->refcount);
        if( --g->ref->refcount == 0 )
        {
            FT_Done_Glyph( g->ref->glyph );
            free( g->ref );
        }
        heimdall_ftcache_Custom_Glyph_Init( g );
    }
}

void heimdall_ftcache_Custom_Glyph_Init( heimdall_ftcache_custom_glyph_t *g )
{
    g->p_glyph = NULL;
    g->ref = NULL;
}

FT_Glyph heimdall_ftcache_GetOutlinedGlyph( heimdall_ftcache_t *ftcache, const heimdall_face_id_t *faceid,
                                       FT_UInt index, const heimdall_ftcache_metrics_t *metrics,
                                       FT_Long style, int radius, const FT_Glyph sourceglyph,
                                       int(*createOutline)(FT_Glyph, FT_Glyph *, void *),
                                       void *priv,
                                       heimdall_ftcache_custom_glyph_ref_t *p_ref )
{
    char *psz_key;
    if( asprintf( &psz_key, "%s#%d#%d#%d,%d,%d,%lx,%d",
                  faceid->psz_filename, faceid->idx,
                  faceid->charmap_index, index,
                  metrics->width_px, metrics->height_px, style, radius ) < 0 )
    {
        *p_ref = NULL;
        return NULL;
    }

    FT_Glyph glyph = NULL;
    *p_ref = heimdall_ftcache_GetCustomGlyph( ftcache, psz_key );
    if( *p_ref )
    {
        glyph = (*p_ref)->glyph;
    }
    else
    {
        if( !createOutline( sourceglyph, &glyph, priv ) )
            *p_ref = heimdall_ftcache_AddCustomGlyph( ftcache, psz_key, glyph );
        if( !*p_ref )
        {
            FT_Done_Glyph( glyph );
            return NULL;
        }
    }
    free( psz_key );
    return glyph;
}
