/*****************************************************************************
 * heimdall_meta.h: Stream meta-data
 *****************************************************************************
 * Copyright (C) 2004 VLC authors and VideoLAN
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

#ifndef HEIMDALL_META_H
#define HEIMDALL_META_H 1

/**
 * \file
 * This file defines functions and structures for stream meta-data in heimdall
 *
 */

typedef enum heimdall_meta_type_t
{
    heimdall_meta_Title,
    heimdall_meta_Artist,
    heimdall_meta_Genre,
    heimdall_meta_Copyright,
    heimdall_meta_Album,
    heimdall_meta_TrackNumber,
    heimdall_meta_Description,
    heimdall_meta_Rating,
    heimdall_meta_Date,
    heimdall_meta_Setting,
    heimdall_meta_URL,
    heimdall_meta_Language,
    heimdall_meta_NowPlaying,
    heimdall_meta_ESNowPlaying,
    heimdall_meta_Publisher,
    heimdall_meta_EncodedBy,
    heimdall_meta_ArtworkURL,
    heimdall_meta_TrackID,
    heimdall_meta_TrackTotal,
    heimdall_meta_Director,
    heimdall_meta_Season,
    heimdall_meta_Episode,
    heimdall_meta_ShowName,
    heimdall_meta_Actors,
    heimdall_meta_AlbumArtist,
    heimdall_meta_DiscNumber,
    heimdall_meta_DiscTotal,
    heimdall_meta_Compilation,
} heimdall_meta_type_t;

#define HEIMDALL_META_TYPE_COUNT 28

typedef enum
{
    HEIMDALL_META_PRIORITY_BASIC,
    HEIMDALL_META_PRIORITY_PLAYLIST,
    HEIMDALL_META_PRIORITY_INBAND
} heimdall_meta_priority_t;

#define ITEM_PREPARSED       1
#define ITEM_ART_FETCHED     2
#define ITEM_ART_NOTFOUND    4

/**
 * Basic function to deal with meta
 */
struct heimdall_meta_t;

HEIMDALL_API heimdall_meta_t * heimdall_meta_New( void ) HEIMDALL_USED;
HEIMDALL_API void heimdall_meta_Delete( heimdall_meta_t *m );
HEIMDALL_API void heimdall_meta_SetWithPriority( heimdall_meta_t *p_meta, heimdall_meta_type_t meta_type, const char *psz_val, heimdall_meta_priority_t priority );
HEIMDALL_API const char *heimdall_meta_GetWithPriority(const heimdall_meta_t *p_meta, heimdall_meta_type_t meta_type, heimdall_meta_priority_t *restrict priority);

/**
 * set the meta extra
 *
 * \param m heimdall_meta_t
 * \param psz_name meta extra name (nonnullable)
 * \param psz_value meta extra value (nullable)
 *  Removed from meta extra if set to NULL
 * \param priority metadata priority (in terms of heimdall_meta_priority_t)
 */
HEIMDALL_API void heimdall_meta_SetExtraWithPriority( heimdall_meta_t *m, const char *psz_name, const char *psz_value, heimdall_meta_priority_t priority );
HEIMDALL_API const char *heimdall_meta_GetExtraWithPriority(const heimdall_meta_t *m, const char *name, heimdall_meta_priority_t *priority);
HEIMDALL_API unsigned heimdall_meta_GetExtraCount( const heimdall_meta_t *m );

/**
 * Allocate a copy of all extra meta names and a table with it.
 * Be sure to free both the returned pointers and its name.
 */
HEIMDALL_API char ** heimdall_meta_CopyExtraNames( const heimdall_meta_t *m ) HEIMDALL_USED;

HEIMDALL_API void heimdall_meta_Merge( heimdall_meta_t *dst, const heimdall_meta_t *src );

HEIMDALL_API int heimdall_meta_GetStatus( const heimdall_meta_t *m );
HEIMDALL_API void heimdall_meta_SetStatus( heimdall_meta_t *m, int status );

HEIMDALL_API const char *heimdall_meta_TypeToString(heimdall_meta_type_t meta_type);

/**
 * Returns a localizes string describing the meta
 */
HEIMDALL_API const char * heimdall_meta_TypeToLocalizedString( heimdall_meta_type_t meta_type );

typedef struct meta_export_t
{
    struct heimdall_object_t obj;
    input_item_t *p_item;
    const char *psz_file;
} meta_export_t;

HEIMDALL_API int input_item_WriteMeta(heimdall_object_t *, input_item_t *);

#define heimdall_meta_Set( meta, meta_type, b )             heimdall_meta_SetWithPriority( meta, meta_type, b, HEIMDALL_META_PRIORITY_BASIC )
#define heimdall_meta_SetExtra( meta, psz_name, psz_value ) heimdall_meta_SetExtraWithPriority( meta, psz_name, psz_value, HEIMDALL_META_PRIORITY_BASIC )

#define heimdall_meta_Get(meta, meta_type) heimdall_meta_GetWithPriority(meta, meta_type, NULL)
#define heimdall_meta_GetExtra(meta, psz_name) heimdall_meta_GetExtraWithPriority(meta, psz_name, NULL)

/* Setters for meta.
 * Warning: Make sure to use the input_item meta setters (defined in heimdall_input_item.h)
 * instead of those one. */
#define heimdall_meta_SetTitle( meta, b )       heimdall_meta_Set( meta, heimdall_meta_Title, b )
#define heimdall_meta_SetArtist( meta, b )      heimdall_meta_Set( meta, heimdall_meta_Artist, b )
#define heimdall_meta_SetGenre( meta, b )       heimdall_meta_Set( meta, heimdall_meta_Genre, b )
#define heimdall_meta_SetCopyright( meta, b )   heimdall_meta_Set( meta, heimdall_meta_Copyright, b )
#define heimdall_meta_SetAlbum( meta, b )       heimdall_meta_Set( meta, heimdall_meta_Album, b )
#define heimdall_meta_SetTrackNum( meta, b )    heimdall_meta_Set( meta, heimdall_meta_TrackNumber, b )
#define heimdall_meta_SetDescription( meta, b ) heimdall_meta_Set( meta, heimdall_meta_Description, b )
#define heimdall_meta_SetRating( meta, b )      heimdall_meta_Set( meta, heimdall_meta_Rating, b )
#define heimdall_meta_SetDate( meta, b )        heimdall_meta_Set( meta, heimdall_meta_Date, b )
#define heimdall_meta_SetSetting( meta, b )     heimdall_meta_Set( meta, heimdall_meta_Setting, b )
#define heimdall_meta_SetURL( meta, b )         heimdall_meta_Set( meta, heimdall_meta_URL, b )
#define heimdall_meta_SetLanguage( meta, b )    heimdall_meta_Set( meta, heimdall_meta_Language, b )
#define heimdall_meta_SetNowPlaying( meta, b )  heimdall_meta_Set( meta, heimdall_meta_NowPlaying, b )
#define heimdall_meta_SetPublisher( meta, b )   heimdall_meta_Set( meta, heimdall_meta_Publisher, b )
#define heimdall_meta_SetEncodedBy( meta, b )   heimdall_meta_Set( meta, heimdall_meta_EncodedBy, b )
#define heimdall_meta_SetArtURL( meta, b )      heimdall_meta_Set( meta, heimdall_meta_ArtworkURL, b )
#define heimdall_meta_SetTrackID( meta, b )     heimdall_meta_Set( meta, heimdall_meta_TrackID, b )
#define heimdall_meta_SetTrackTotal( meta, b )  heimdall_meta_Set( meta, heimdall_meta_TrackTotal, b )
#define heimdall_meta_SetDirector( meta, b )    heimdall_meta_Set( meta, heimdall_meta_Director, b )
#define heimdall_meta_SetSeason( meta, b )      heimdall_meta_Set( meta, heimdall_meta_Season, b )
#define heimdall_meta_SetEpisode( meta, b )     heimdall_meta_Set( meta, heimdall_meta_Episode, b )
#define heimdall_meta_SetShowName( meta, b )    heimdall_meta_Set( meta, heimdall_meta_ShowName, b )
#define heimdall_meta_SetActors( meta, b )      heimdall_meta_Set( meta, heimdall_meta_Actors, b )
#define heimdall_meta_SetAlbumArtist( meta, b ) heimdall_meta_Set( meta, heimdall_meta_AlbumArtist, b )
#define heimdall_meta_SetDiscNumber( meta, b )  heimdall_meta_Set( meta, heimdall_meta_DiscNumber, b )
#define heimdall_meta_SetCompilation( meta, b ) heimdall_meta_Set( meta, heimdall_meta_Compilation, b )

#define HEIMDALL_META_TITLE              heimdall_meta_TypeToLocalizedString( heimdall_meta_Title )
#define HEIMDALL_META_ARTIST             heimdall_meta_TypeToLocalizedString( heimdall_meta_Artist )
#define HEIMDALL_META_GENRE              heimdall_meta_TypeToLocalizedString( heimdall_meta_Genre )
#define HEIMDALL_META_COPYRIGHT          heimdall_meta_TypeToLocalizedString( heimdall_meta_Copyright )
#define HEIMDALL_META_ALBUM              heimdall_meta_TypeToLocalizedString( heimdall_meta_Album )
#define HEIMDALL_META_TRACK_NUMBER       heimdall_meta_TypeToLocalizedString( heimdall_meta_TrackNumber )
#define HEIMDALL_META_DESCRIPTION        heimdall_meta_TypeToLocalizedString( heimdall_meta_Description )
#define HEIMDALL_META_RATING             heimdall_meta_TypeToLocalizedString( heimdall_meta_Rating )
#define HEIMDALL_META_DATE               heimdall_meta_TypeToLocalizedString( heimdall_meta_Date )
#define HEIMDALL_META_SETTING            heimdall_meta_TypeToLocalizedString( heimdall_meta_Setting )
#define HEIMDALL_META_URL                heimdall_meta_TypeToLocalizedString( heimdall_meta_URL )
#define HEIMDALL_META_LANGUAGE           heimdall_meta_TypeToLocalizedString( heimdall_meta_Language )
#define HEIMDALL_META_NOW_PLAYING        heimdall_meta_TypeToLocalizedString( heimdall_meta_NowPlaying )
#define HEIMDALL_META_PUBLISHER          heimdall_meta_TypeToLocalizedString( heimdall_meta_Publisher )
#define HEIMDALL_META_ENCODED_BY         heimdall_meta_TypeToLocalizedString( heimdall_meta_EncodedBy )
#define HEIMDALL_META_ART_URL            heimdall_meta_TypeToLocalizedString( heimdall_meta_ArtworkURL )
#define HEIMDALL_META_TRACKID            heimdall_meta_TypeToLocalizedString( heimdall_meta_TrackID )
#define HEIMDALL_META_DIRECTOR           heimdall_meta_TypeToLocalizedString( heimdall_meta_Director )
#define HEIMDALL_META_SEASON             heimdall_meta_TypeToLocalizedString( heimdall_meta_Season )
#define HEIMDALL_META_EPISODE            heimdall_meta_TypeToLocalizedString( heimdall_meta_Episode )
#define HEIMDALL_META_SHOW_NAME          heimdall_meta_TypeToLocalizedString( heimdall_meta_ShowName )
#define HEIMDALL_META_ACTORS             heimdall_meta_TypeToLocalizedString( heimdall_meta_Actors )
#define HEIMDALL_META_ALBUMARTIST        heimdall_meta_TypeToLocalizedString( heimdall_meta_AlbumArtist )
#define HEIMDALL_META_DISCNUMBER         heimdall_meta_TypeToLocalizedString( heimdall_meta_DiscNumber )
#define HEIMDALL_META_COMPILATION        heimdall_meta_TypeToLocalizedString( heimdall_meta_Compilation )

#define HEIMDALL_META_EXTRA_MB_ALBUMID   "MB_ALBUMID"

#endif
