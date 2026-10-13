/*****************************************************************************
 * heimdall_media_library.h: SQL-based media library
 *****************************************************************************
 * Copyright (C) 2008-2010 the VideoLAN Team and AUTHORS
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Lejeune <phytos@videolan.org>
 *          Jean-Philippe André <jpeg@videolan.org>
 *          Rémi Duraffort <ivoire@videolan.org>
 *          Adrien Maglo <magsoft@videolan.org>
 *          Srikanth Raju <srikiraju at gmail dot com>
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

#ifndef HEIMDALL_MEDIA_LIBRARY_H
# define HEIMDALL_MEDIA_LIBRARY_H

#include <assert.h>
#include <heimdall_common.h>

#include <time.h>

# ifdef __cplusplus
extern "C" {
# endif

typedef enum heimdall_ml_media_type_t
{
    HEIMDALL_ML_MEDIA_TYPE_UNKNOWN,
    HEIMDALL_ML_MEDIA_TYPE_VIDEO,
    HEIMDALL_ML_MEDIA_TYPE_AUDIO,
} heimdall_ml_media_type_t;

typedef enum heimdall_ml_media_subtype_t
{
    HEIMDALL_ML_MEDIA_SUBTYPE_UNKNOWN,
    HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE,
    HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE,
    HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK,
} heimdall_ml_media_subtype_t;

typedef enum heimdall_ml_file_type_t
{
    HEIMDALL_ML_FILE_TYPE_UNKNOWN,
    HEIMDALL_ML_FILE_TYPE_MAIN,
    HEIMDALL_ML_FILE_TYPE_PART,
    HEIMDALL_ML_FILE_TYPE_SOUNDTRACK,
    HEIMDALL_ML_FILE_TYPE_SUBTITLE,
    HEIMDALL_ML_FILE_TYPE_PLAYLIST,
} heimdall_ml_file_type_t;

typedef enum heimdall_ml_track_type_t
{
    HEIMDALL_ML_TRACK_TYPE_UNKNOWN,
    HEIMDALL_ML_TRACK_TYPE_VIDEO,
    HEIMDALL_ML_TRACK_TYPE_AUDIO,
    HEIMDALL_ML_TRACK_TYPE_SUBTITLE,
} heimdall_ml_track_type_t;

typedef enum heimdall_ml_thumbnail_size_t
{
    HEIMDALL_ML_THUMBNAIL_SMALL,
    HEIMDALL_ML_THUMBNAIL_BANNER,

    HEIMDALL_ML_THUMBNAIL_SIZE_COUNT
} heimdall_ml_thumbnail_size_t;

typedef enum heimdall_ml_thumbnail_status_t
{
    HEIMDALL_ML_THUMBNAIL_STATUS_MISSING,
    HEIMDALL_ML_THUMBNAIL_STATUS_AVAILABLE,
    HEIMDALL_ML_THUMBNAIL_STATUS_FAILURE,
    HEIMDALL_ML_THUMBNAIL_STATUS_PERSISTENT_FAILURE,
    HEIMDALL_ML_THUMBNAIL_STATUS_CRASH,
} heimdall_ml_thumbnail_status_t;

typedef enum heimdall_ml_history_type_t
{
    HEIMDALL_ML_HISTORY_TYPE_GLOBAL,
    HEIMDALL_ML_HISTORY_TYPE_LOCAL,
    HEIMDALL_ML_HISTORY_TYPE_NETWORK,
} heimdall_ml_history_type_t;

typedef enum heimdall_ml_playlist_type_t
{
    HEIMDALL_ML_PLAYLIST_TYPE_ALL,        /**< Playlist containing any kind of tracks */
    HEIMDALL_ML_PLAYLIST_TYPE_AUDIO,      /**< Playlist containing at least one audio track */
    HEIMDALL_ML_PLAYLIST_TYPE_VIDEO,      /**< Playlist containing at least one video track */
    HEIMDALL_ML_PLAYLIST_TYPE_AUDIO_ONLY, /**< Playlist containing audio tracks only */
    HEIMDALL_ML_PLAYLIST_TYPE_VIDEO_ONLY, /**< Playlist containing video tracks only */
} heimdall_ml_playlist_type_t;

typedef struct heimdall_ml_thumbnail_t
{
    char* psz_mrl;
    /**
     * True if a thumbnail is available, or if thumbnail generation was
     * attempted but failed
     */
    heimdall_ml_thumbnail_status_t i_status;
} heimdall_ml_thumbnail_t;

typedef struct heimdall_ml_movie_t
{
    char* psz_summary;
    char* psz_imdb_id;
} heimdall_ml_movie_t;

typedef struct heimdall_ml_show_episode_t
{
    char* psz_summary;
    char* psz_tvdb_id;
    uint32_t i_episode_nb;
    uint32_t i_season_number;
} heimdall_ml_show_episode_t;

typedef struct heimdall_ml_show_t
{
    int64_t i_id;
    char* psz_name;
    char* psz_summary;
    char* psz_artwork_mrl;
    char* psz_tvdb_id;
    unsigned int i_release_year;
    uint32_t i_nb_episodes;
    uint32_t i_nb_seasons;
} heimdall_ml_show_t;

typedef struct heimdall_ml_label_t
{
    int64_t i_id;
    char* psz_name;
} heimdall_ml_label_t;

typedef struct heimdall_ml_label_list_t
{
    size_t i_nb_items;
    heimdall_ml_label_t p_items[];
} heimdall_ml_label_list_t;

typedef struct heimdall_ml_file_t
{
    char* psz_mrl;
    int64_t i_size;
    time_t i_last_modification_date;
    heimdall_ml_file_type_t i_type;
    bool b_external;
    bool b_removable;
    bool b_present;
} heimdall_ml_file_t;

typedef struct heimdall_ml_file_list_t
{
    size_t i_nb_items;
    heimdall_ml_file_t p_items[];
} heimdall_ml_file_list_t;

typedef struct heimdall_ml_media_track_t
{
    char* psz_codec;
    char* psz_language;
    char* psz_description;
    heimdall_ml_track_type_t i_type;
    uint32_t i_bitrate;
    union
    {
        struct
        {
            // Audio
            uint32_t i_nbChannels;
            uint32_t i_sampleRate;
        } a;
        struct
        {
            // Video
            uint32_t i_height;
            uint32_t i_width;
            uint32_t i_sarNum;
            uint32_t i_sarDen;
            uint32_t i_fpsNum;
            uint32_t i_fpsDen;
        } v;
        struct
        {
            // Subtitle
            char* psz_encoding;
        } s;
    };
} heimdall_ml_media_track_t;

typedef struct heimdall_ml_media_track_list_t
{
    size_t i_nb_items;
    heimdall_ml_media_track_t p_items[];
} heimdall_ml_media_track_list_t;

typedef struct heimdall_ml_media_t
{
    int64_t i_id;

    heimdall_ml_media_type_t i_type;
    heimdall_ml_media_subtype_t i_subtype;

    heimdall_ml_file_list_t* p_files;
    heimdall_ml_media_track_list_t* p_tracks;

    int32_t i_year;
    /* Duration in milliseconds */
    int64_t i_duration;
    uint32_t i_playcount;
    double f_progress;
    time_t i_last_played_date;
    char* psz_title;
    char* psz_filename;

    heimdall_ml_thumbnail_t thumbnails[HEIMDALL_ML_THUMBNAIL_SIZE_COUNT];

    bool b_is_favorite;
    bool b_is_public;

    union
    {
        heimdall_ml_show_episode_t show_episode;
        heimdall_ml_movie_t movie;
        struct
        {
            int64_t i_artist_id;
            int64_t i_album_id;
            int64_t i_genre_id;

            int i_track_nb;
            int i_disc_nb;
        } album_track;
    };
} heimdall_ml_media_t;

typedef struct heimdall_ml_group_t
{
    int64_t i_id;

    char* psz_name;

    uint32_t i_nb_total_media;
    uint32_t i_nb_video;
    uint32_t i_nb_audio;
    uint32_t i_nb_unknown;
    uint32_t i_nb_present_media;
    uint32_t i_nb_present_video;
    uint32_t i_nb_present_audio;
    uint32_t i_nb_present_unknown;

    uint32_t i_nb_seen;
    uint32_t i_nb_present_seen;

    int64_t i_duration;

    time_t i_creation_date;
    time_t i_last_modification_date;
} heimdall_ml_group_t;

typedef struct heimdall_ml_playlist_t
{
    int64_t i_id;

    char* psz_name;

    char* psz_mrl;

    char* psz_artwork_mrl;

    unsigned int i_nb_media;
    uint32_t i_nb_video;
    uint32_t i_nb_audio;
    uint32_t i_nb_unknown;

    unsigned int i_nb_present_media;
    uint32_t i_nb_present_video;
    uint32_t i_nb_present_audio;
    uint32_t i_nb_present_unknown;

    uint32_t i_creation_date;

    int64_t i_duration; /* In ms */
    uint32_t i_nb_duration_unknown;

    bool b_is_read_only;
    bool b_is_favorite;
} heimdall_ml_playlist_t;

typedef struct heimdall_ml_artist_t
{
    int64_t i_id;
    char* psz_name;
    char* psz_shortbio;
    heimdall_ml_thumbnail_t thumbnails[HEIMDALL_ML_THUMBNAIL_SIZE_COUNT];
    char* psz_mb_id;

    unsigned int i_nb_album;
    unsigned int i_nb_tracks;
    uint32_t i_nb_present_tracks;
    bool b_is_favorite;
} heimdall_ml_artist_t;

typedef struct heimdall_ml_artist_list_t
{
    size_t i_nb_items;
    heimdall_ml_artist_t p_items[];
} heimdall_ml_artist_list_t;

typedef struct heimdall_ml_album_t {
    int64_t i_id;
    char* psz_title;
    char* psz_summary;
    heimdall_ml_thumbnail_t thumbnails[HEIMDALL_ML_THUMBNAIL_SIZE_COUNT];
    char* psz_artist;
    int64_t i_artist_id;

    uint32_t i_nb_tracks;
    uint32_t i_nb_present_tracks;
    uint32_t i_nb_discs;
    int64_t i_duration; /* in ms */
    unsigned int i_year;
    bool b_is_favorite;
} heimdall_ml_album_t;

typedef struct heimdall_ml_genre_t
{
    int64_t i_id;
    char* psz_name;
    size_t i_nb_tracks;
    heimdall_ml_thumbnail_t thumbnails[HEIMDALL_ML_THUMBNAIL_SIZE_COUNT];
    bool b_is_favorite;
} heimdall_ml_genre_t;

typedef struct heimdall_ml_media_list_t
{
    size_t i_nb_items;
    heimdall_ml_media_t p_items[];
} heimdall_ml_media_list_t;

typedef struct heimdall_ml_album_list_t
{
    size_t i_nb_items;
    heimdall_ml_album_t p_items[];
} heimdall_ml_album_list_t;

typedef struct heimdall_ml_show_list_t
{
    size_t i_nb_items;
    heimdall_ml_show_t p_items[];
} heimdall_ml_show_list_t;

typedef struct heimdall_ml_genre_list_t
{
    size_t i_nb_items;
    heimdall_ml_genre_t p_items[];
} heimdall_ml_genre_list_t;

typedef struct heimdall_ml_group_list_t
{
    size_t i_nb_items;
    heimdall_ml_group_t p_items[];
} heimdall_ml_group_list_t;

typedef struct heimdall_ml_playlist_list_t
{
    size_t i_nb_items;
    heimdall_ml_playlist_t p_items[];
} heimdall_ml_playlist_list_t;

typedef struct heimdall_ml_folder_t
{
    int64_t i_id; /**< The folder's MRL. Will be NULL if b_present is false */
    char* psz_name; /**< The folder's name */
    char* psz_mrl; /**< The folder's MRL. Will be NULL if b_present is false */
    unsigned int i_nb_media; /**< The media count */
    unsigned int i_nb_video; /**< The number of video for this folder */
    unsigned int i_nb_audio; /**< The number of audio for this volder */
    int64_t i_duration; /**< The sum of all the member durations of the folder in ms. */
    bool b_is_favorite; /**< The folder's favorite state */
    bool b_present; /**< The folder's presence state */
    bool b_banned; /**< Will be true if the user required this folder to be excluded */
} heimdall_ml_folder_t;

typedef struct heimdall_ml_folder_list_t
{
    size_t i_nb_items;
    heimdall_ml_folder_t p_items[];
} heimdall_ml_folder_list_t;

typedef struct heimdall_ml_bookmark_t
{
    int64_t i_media_id; /**< The associated media ID */
    int64_t i_time; /**< The bookmark time. The unit is arbitrary */
    char* psz_name; /**< The bookmark name */
    char* psz_description; /**< The bookmark description */
} heimdall_ml_bookmark_t;

typedef struct heimdall_ml_boomkmark_list_t
{
    size_t i_nb_items;
    heimdall_ml_bookmark_t p_items[];
} heimdall_ml_bookmark_list_t;

/* Opaque medialibrary pointer, to be used by any non-medialibrary module */
typedef struct heimdall_medialibrary_t heimdall_medialibrary_t;
/* "Private" medialibrary pointer, to be used by the core & medialibrary modules */
typedef struct heimdall_medialibrary_module_t heimdall_medialibrary_module_t;
/* Opaque event callback type */
typedef struct heimdall_ml_event_callback_t heimdall_ml_event_callback_t;

typedef enum heimdall_ml_sorting_criteria_t
{
    /*
     * Default depends on the entity type:
     * - By track number (and disc number) for album tracks
     * - Alphabetical order for others
     */
    HEIMDALL_ML_SORTING_DEFAULT,
    HEIMDALL_ML_SORTING_ALPHA,
    HEIMDALL_ML_SORTING_DURATION,
    HEIMDALL_ML_SORTING_INSERTIONDATE,
    HEIMDALL_ML_SORTING_LASTMODIFICATIONDATE,
    HEIMDALL_ML_SORTING_RELEASEDATE,
    HEIMDALL_ML_SORTING_FILESIZE,
    HEIMDALL_ML_SORTING_ARTIST,
    HEIMDALL_ML_SORTING_PLAYCOUNT,
    HEIMDALL_ML_SORTING_ALBUM,
    HEIMDALL_ML_SORTING_FILENAME,
    HEIMDALL_ML_SORTING_TRACKNUMBER,
} heimdall_ml_sorting_criteria_t;

/**
 * Generic parameter set for medialibrary queries.
 *
 * \warning Should only be default instanciated with ::heimdall_ml_query_params_create().
 */
typedef struct heimdall_ml_query_params_t
{
    const char* psz_pattern;
    uint32_t i_nbResults;
    uint32_t i_offset;
    heimdall_ml_sorting_criteria_t i_sort;
    bool b_desc;
    bool b_favorite_only;
    bool b_public_only;
} heimdall_ml_query_params_t;

enum heimdall_ml_get_queries
{
    HEIMDALL_ML_GET_MEDIA,             /**< arg1: Media    ID; ret: heimdall_ml_media_t*    */
    HEIMDALL_ML_GET_MEDIA_BY_MRL,      /**< arg1: Media   MRL; ret: heimdall_ml_media_t*    */
    HEIMDALL_ML_GET_INPUT_ITEM,        /**< arg1: Media    ID; ret: input_item_t*      */
    HEIMDALL_ML_GET_INPUT_ITEM_BY_MRL, /**< arg1: Media  MRL; ret: input_item_t*       */
    HEIMDALL_ML_GET_ALBUM,             /**< arg1: Album    ID; ret: heimdall_ml_album_t*    */
    HEIMDALL_ML_GET_ARTIST,            /**< arg1: Artist   ID; ret: heimdall_ml_artist_t*   */
    HEIMDALL_ML_GET_GENRE,             /**< arg1: Genre    ID; ret: heimdall_ml_genre_t*    */
    HEIMDALL_ML_GET_SHOW,              /**< arg1: Show     ID; ret: heimdall_ml_show_t*     */
    HEIMDALL_ML_GET_PLAYLIST,          /**< arg1: Playlist ID; ret: heimdall_ml_playlist_t* */
    HEIMDALL_ML_GET_GROUP,             /**< arg1: Group    ID; ret: heimdall_ml_group_t*    */
    HEIMDALL_ML_GET_FOLDER,            /**< arg1: folder ID; ret: heimdall_ml_folder_t*     */
};

enum heimdall_ml_list_queries
{
    /* General listing: */

    HEIMDALL_ML_LIST_MEDIA,            /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_MEDIA,           /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_VIDEOS,           /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_VIDEOS,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_MOVIES,           /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_MOVIES,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_AUDIOS,           /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_AUDIOS,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_ALBUMS,           /**< arg1 (out): heimdall_ml_album_list_t**                                                */
    HEIMDALL_ML_COUNT_ALBUMS,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_GENRES,           /**< arg1 (out): heimdall_ml_genre_list_t**                                                */
    HEIMDALL_ML_COUNT_GENRES,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_ARTISTS,          /**< arg1 bool: includeAll; arg2 (out): heimdall_ml_genre_list_t**                         */
    HEIMDALL_ML_COUNT_ARTISTS,         /**< arg1 bool: includeAll; arg2 (out): size_t*                                       */
    HEIMDALL_ML_LIST_SHOWS,            /**< arg1 (out): heimdall_ml_show_list_t**                                                 */
    HEIMDALL_ML_COUNT_SHOWS,           /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_GROUPS,           /**< arg1 (out): heimdall_ml_group_list_t**                                                */
    HEIMDALL_ML_COUNT_GROUPS,          /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_PLAYLISTS,        /**< arg1 (out): heimdall_ml_playlist_list_t**                                             */
    HEIMDALL_ML_COUNT_PLAYLISTS,       /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_HISTORY,          /**< arg1 heimdall_ml_history_type_t; arg2 (out): heimdall_ml_media_list_t**                    */
    HEIMDALL_ML_COUNT_HISTORY,         /**< arg1 heimdall_ml_history_type_t; arg2 (out): size_t*                                  */
    HEIMDALL_ML_LIST_VIDEO_HISTORY,    /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_VIDEO_HISTORY,   /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_LIST_AUDIO_HISTORY,    /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_AUDIO_HISTORY,   /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_LIST_ENTRY_POINTS,     /**< arg1 bool: list_banned; arg2 (out): heimdall_ml_folder_list_t**                       */
    HEIMDALL_ML_COUNT_ENTRY_POINTS,    /**< arg1 bool: list_banned; arg2 (out): size_t*                                      */
    HEIMDALL_ML_LIST_FOLDERS,          /**< arg1 (out): heimdall_ml_folder_list_t**                                               */
    HEIMDALL_ML_COUNT_FOLDERS,         /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FOLDERS_BY_TYPE,  /**< arg1 heimdall_ml_media_type_t: the media type. arg2 (out): heimdall_ml_media_list_t**      */
    HEIMDALL_ML_COUNT_FOLDERS_BY_TYPE, /**< arg1 heimdall_ml_media_type_t: the media type. arg2 (out): heimdall_ml_media_list_t**      */

    /* Album specific listings */
    HEIMDALL_ML_LIST_ALBUM_TRACKS,     /**< arg1: The album id. arg2 (out): heimdall_ml_media_list_t**                            */
    HEIMDALL_ML_COUNT_ALBUM_TRACKS,    /**< arg1: The album id. arg2 (out): size_t*                                          */
    HEIMDALL_ML_LIST_ALBUM_ARTISTS,    /**< arg1: The album id. arg2 (out): heimdall_ml_album_list_t**                            */
    HEIMDALL_ML_COUNT_ALBUM_ARTISTS,   /**< arg1: The album id. arg2 (out): size_t*                                          */

    /* Artist specific listings */
    HEIMDALL_ML_LIST_ARTIST_ALBUMS,    /**< arg1: The artist id. arg2(out): heimdall_ml_album_list_t**                            */
    HEIMDALL_ML_COUNT_ARTIST_ALBUMS,   /**< arg1: The artist id. arg2(out): size_t*                                          */
    HEIMDALL_ML_LIST_ARTIST_TRACKS,    /**< arg1: The artist id. arg2(out): heimdall_ml_media_list_t**                            */
    HEIMDALL_ML_COUNT_ARTIST_TRACKS,   /**< arg1: The artist id. arg2(out): size_t*                                          */

    /* Genre specific listings */
    HEIMDALL_ML_LIST_GENRE_ARTISTS,    /**< arg1: genre id;  arg2 (out): heimdall_ml_artist_list_t**                              */
    HEIMDALL_ML_COUNT_GENRE_ARTISTS,   /**< arg1: genre id;  arg2 (out): size_t*                                             */
    HEIMDALL_ML_LIST_GENRE_TRACKS,     /**< arg1: genre id;  arg2 (out): heimdall_ml_media_list_t**                               */
    HEIMDALL_ML_COUNT_GENRE_TRACKS,    /**< arg1: genre id;  arg2 (out): size_t*                                             */
    HEIMDALL_ML_LIST_GENRE_ALBUMS,     /**< arg1: genre id;  arg2 (out): heimdall_ml_album_list_t**                               */
    HEIMDALL_ML_COUNT_GENRE_ALBUMS,    /**< arg1: genre id;  arg2 (out): size_t*                                             */

    /* Show specific listings */
    HEIMDALL_ML_LIST_SHOW_EPISODES,    /**< arg1: show id; arg2(out): heimdall_ml_media_list_t**                                  */
    HEIMDALL_ML_COUNT_SHOW_EPISODES,   /**< arg1: show id; arg2(out): size_t*                                                */

    /* Media specific listings */
    HEIMDALL_ML_LIST_MEDIA_LABELS,     /**< arg1: media id;  arg2 (out): heimdall_ml_label_list_t**                               */
    HEIMDALL_ML_COUNT_MEDIA_LABELS,    /**< arg1: media id;  arg2 (out): size_t*                                             */
    HEIMDALL_ML_LIST_MEDIA_BOOKMARKS,  /**< arg1: media id;  arg2 (out): heimdall_ml_bookmark_list_t**                            */

    /* Groups specific listings */
    HEIMDALL_ML_LIST_GROUP_MEDIA,      /**< arg1: playlist id; arg2 (out): heimdall_ml_media_list_t**                             */
    HEIMDALL_ML_COUNT_GROUP_MEDIA,     /**< arg1: playlist id; arg2 (out): size_t*                                           */

    /* Playlist specific listings */
    HEIMDALL_ML_LIST_PLAYLIST_MEDIA,   /**< arg1: playlist id; arg2 (out): heimdall_ml_media_list_t**                             */
    HEIMDALL_ML_COUNT_PLAYLIST_MEDIA,  /**< arg1: playlist id; arg2 (out): size_t*                                           */

    /* Folder specific listings */
    HEIMDALL_ML_LIST_SUBFOLDERS,       /**< arg1: parent id; arg2 (out): heimdall_ml_folder_list_t**                              */
    HEIMDALL_ML_COUNT_SUBFOLDERS,      /**< arg1: parent id; arg2 (out): size_t*                                             */
    HEIMDALL_ML_LIST_FOLDER_MEDIA,     /**< arg1: folder id; arg2 (out): heimdall_ml_media_list_t**                               */
    HEIMDALL_ML_COUNT_FOLDER_MEDIA,    /**< arg1: folder id; arg2 (out): size_t*                                             */

    /* Favorites listings */
    HEIMDALL_ML_LIST_FAVORITE_MEDIA,   /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_FAVORITE_MEDIA,  /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_VIDEOS,  /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_FAVORITE_VIDEOS, /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_AUDIOS,  /**< arg1 (out): heimdall_ml_media_list_t**                                                */
    HEIMDALL_ML_COUNT_FAVORITE_AUDIOS, /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_ALBUMS,  /**< arg1 (out): heimdall_ml_album_list_t**                                                */
    HEIMDALL_ML_COUNT_FAVORITE_ALBUMS, /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_ARTISTS, /**< arg1 (out): heimdall_ml_artist_list_t**                                               */
    HEIMDALL_ML_COUNT_FAVORITE_ARTISTS,/**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_GENRES,  /**< arg1 (out): heimdall_ml_genre_list_t**                                                */
    HEIMDALL_ML_COUNT_FAVORITE_GENRES, /**< arg1 (out): size_t*                                                              */
    HEIMDALL_ML_LIST_FAVORITE_PLAYLISTS,/**< arg1 (out): heimdall_ml_playlist_list_t**                                            */
    HEIMDALL_ML_COUNT_FAVORITE_PLAYLISTS,/**< arg1 (out): size_t*                                                            */
    HEIMDALL_ML_LIST_FAVORITE_FOLDERS, /**< arg1 (out): heimdall_ml_folder_list_t**                                               */
    HEIMDALL_ML_COUNT_FAVORITE_FOLDERS,/**< arg1 (out): size_t*                                                              */

    /* Children entities listing */
    HEIMDALL_ML_LIST_MEDIA_OF,         /**< arg1: parent entity type; arg2: parent entity id; arg3(out): ml_media_list_t**   */
    HEIMDALL_ML_COUNT_MEDIA_OF,        /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_LIST_VIDEO_OF,         /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_COUNT_VIDEO_OF,        /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_LIST_AUDIO_OF,         /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_COUNT_AUDIO_OF,        /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_LIST_ARTISTS_OF,       /**< arg1: parent entity type; arg2: parent entity id; arg3(out): ml_artist_list_t**  */
    HEIMDALL_ML_COUNT_ARTISTS_OF,      /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
    HEIMDALL_ML_LIST_ALBUMS_OF,        /**< arg1: parent entity type; arg2: parent entity id; arg3(out): ml_album_list_t**   */
    HEIMDALL_ML_COUNT_ALBUMS_OF,       /**< arg1: parent entity type; arg2: parent entity id; arg3(out): size_t*             */
};

enum heimdall_ml_parent_type
{
    HEIMDALL_ML_PARENT_UNKNOWN,
    HEIMDALL_ML_PARENT_ALBUM,
    HEIMDALL_ML_PARENT_ARTIST,
    HEIMDALL_ML_PARENT_SHOW,
    HEIMDALL_ML_PARENT_GENRE,
    HEIMDALL_ML_PARENT_GROUP,
    HEIMDALL_ML_PARENT_FOLDER,
    HEIMDALL_ML_PARENT_PLAYLIST,
};

enum heimdall_ml_control
{
    /* Adds a folder to discover through the medialibrary */
    HEIMDALL_ML_ADD_FOLDER,              /**< arg1: mrl (const char*)  res: can't fail */
    HEIMDALL_ML_REMOVE_FOLDER,           /**< arg1: mrl (const char*)  res: can't fail */
    HEIMDALL_ML_BAN_FOLDER,              /**< arg1: mrl (const char*)  res: can't fail */
    HEIMDALL_ML_UNBAN_FOLDER,            /**< arg1: mrl (const char*)  res: can't fail */
    HEIMDALL_ML_IS_INDEXED,              /**< arg1: mrl (const char*) arg2 (out): bool*;       res: can fail */
    /**
     * Reload a specific folder, or all.
     * arg1: mrl (const char*), NULL to reload all folders
     * res: can't fail
     */
    HEIMDALL_ML_RELOAD_FOLDER,

    HEIMDALL_ML_SET_FOLDER_PUBLIC,       /**< arg1: mrl (const char *); res: can't fail */
    HEIMDALL_ML_SET_FOLDER_PRIVATE,      /**< arg1: mrl (const char *); res: can't fail */

    /* Pause/resume background operations, such as media discovery & media analysis */
    HEIMDALL_ML_PAUSE_BACKGROUND,        /**< no args; can't fail */
    HEIMDALL_ML_RESUME_BACKGROUND,       /**< no args; can't fail */

    /* Misc operations */
    HEIMDALL_ML_CLEAR_HISTORY,           /**< arg1: heimdall_ml_history_type_t; can't fail */

    /* Create media */
    HEIMDALL_ML_NEW_EXTERNAL_MEDIA,      /**< arg1: const char*; arg2(out): heimdall_ml_media_t** */
    HEIMDALL_ML_NEW_STREAM,              /**< arg1: const char*; arg2(out): heimdall_ml_media_t** */

    HEIMDALL_ML_REMOVE_STREAM,

    /* Media management */
    HEIMDALL_ML_MEDIA_UPDATE_PROGRESS,           /**< arg1: media id; arg2: playback position; can fail */
    HEIMDALL_ML_MEDIA_GET_MEDIA_PLAYBACK_STATE,  /**< arg1: media id; arg2: heimdall_ml_playback_state; arg3: char**; */
    HEIMDALL_ML_MEDIA_SET_MEDIA_PLAYBACK_STATE,  /**< arg1: media id; arg2: heimdall_ml_playback_state; arg3: const char*; */
    HEIMDALL_ML_MEDIA_GET_ALL_MEDIA_PLAYBACK_STATES, /**< arg1: media id; arg2(out): heimdall_ml_playback_states_all* */
    HEIMDALL_ML_MEDIA_SET_ALL_MEDIA_PLAYBACK_STATES, /**< arg1: media id; arg2: const heimdall_ml_playback_states_all* */
    HEIMDALL_ML_MEDIA_SET_THUMBNAIL,             /**< arg1: media id; arg2: const char*; arg3: heimdall_ml_thumbnail_size_t */
    HEIMDALL_ML_MEDIA_SET_GENRE_THUMBNAIL,       /**< arg1: media id; arg2: const char*; arg3: heimdall_ml_thumbnail_size_t */
    HEIMDALL_ML_MEDIA_GENERATE_THUMBNAIL,        /**< arg1: media id; arg2: heimdall_ml_thumbnail_size_t; arg3: width; arg4: height; arg5: position */
    HEIMDALL_ML_MEDIA_ADD_EXTERNAL_MRL,          /**< arg1: media id; arg2: const char*; arg3: type(heimdall_ml_file_type_t) */
    HEIMDALL_ML_MEDIA_SET_TYPE,                  /**< arg1: media id; arg2: heimdall_ml_media_type_t */
    HEIMDALL_ML_MEDIA_SET_PLAYED,                /**< arg1: media id; arg2: bool */
    HEIMDALL_ML_MEDIA_SET_FAVORITE,              /**< arg1: media id; arg2: bool */
    HEIMDALL_ML_MEDIA_ADD_LABEL,                 /**< arg1: media id; arg2: label name */
    HEIMDALL_ML_MEDIA_REMOVE_LABEL,              /**< arg1: media id; arg2: label name */
    HEIMDALL_ML_MEDIA_ADD_BOOKMARK,              /**< arg1: media id; arg2: int64_t */
    HEIMDALL_ML_MEDIA_REMOVE_BOOKMARK,           /**< arg1: media id; arg2: int64_t */
    HEIMDALL_ML_MEDIA_REMOVE_ALL_BOOKMARKS,      /**< arg1: media id */
    HEIMDALL_ML_MEDIA_UPDATE_BOOKMARK,           /**< arg1: media id; arg2: int64_t; arg3: const char*; arg4: const char* */

    /* Playlist management */
    HEIMDALL_ML_PLAYLIST_CREATE, /**< arg1: const char*; arg2(out): heimdall_ml_playlist_t**; can fail */
    HEIMDALL_ML_PLAYLIST_DELETE, /**< arg1: playlist id; can fail */
    HEIMDALL_ML_PLAYLIST_APPEND, /**< arg1: playlist id; arg2: pointer on media ids; arg3: media ids count; can fail */
    HEIMDALL_ML_PLAYLIST_INSERT, /**< arg1: playlist id; arg2: pointer on media ids; arg3: media ids count; arg4: position; can fail */
    HEIMDALL_ML_PLAYLIST_MOVE,   /**< arg1: playlist id; arg2: from; arg3: to; arg4: count; can fail */
    HEIMDALL_ML_PLAYLIST_REMOVE, /**< arg1: playlist id; arg2: position; arg3: count; can fail */
    HEIMDALL_ML_PLAYLIST_RENAME,  /**< arg1: playlist id; arg2: const char*; can fail */

    /* Set Favorites  */
    HEIMDALL_ML_FOLDER_SET_FAVORITE, /**< arg1: mrl (const char*); arg2: bool; res: can fail */
    HEIMDALL_ML_ARTIST_SET_FAVORITE, /**< arg1: artist id; arg2: bool; can fail */
    HEIMDALL_ML_ALBUM_SET_FAVORITE, /**< arg1: album id; arg2: bool; can fail */
    HEIMDALL_ML_GENRE_SET_FAVORITE, /**< arg1: genre id; arg2: bool; can fail */
    HEIMDALL_ML_PLAYLIST_SET_FAVORITE, /**< arg1: playlist id; arg2: bool; can fail */
};

/**
 * User playback settings.
 * All values/units are up to the caller and are not interpreted by the media
 * library.
 * All values are stored and returned as strings.
 * When calling heimdall_medialibrary_t::pf_control with heimdall_ml_MEDIA_GET_MEDIA_PLAYBACK_STATE,
 * the value will be returned stored in the provided char**. If the state was
 * not set yet, NULL will be returned.
 * When setting a state, NULL can be provided as a value to unset it.
 */
enum heimdall_ml_playback_state
{
    HEIMDALL_ML_PLAYBACK_STATE_RATING,
    HEIMDALL_ML_PLAYBACK_STATE_SPEED,
    HEIMDALL_ML_PLAYBACK_STATE_TITLE,
    HEIMDALL_ML_PLAYBACK_STATE_CHAPTER,
    HEIMDALL_ML_PLAYBACK_STATE_PROGRAM,
    HEIMDALL_ML_PLAYBACK_STATE_VIDEO_TRACK,
    HEIMDALL_ML_PLAYBACK_STATE_ASPECT_RATIO,
    HEIMDALL_ML_PLAYBACK_STATE_ZOOM,
    HEIMDALL_ML_PLAYBACK_STATE_CROP,
    HEIMDALL_ML_PLAYBACK_STATE_DEINTERLACE,
    HEIMDALL_ML_PLAYBACK_STATE_VIDEO_FILTER,
    HEIMDALL_ML_PLAYBACK_STATE_AUDIO_TRACK,
    HEIMDALL_ML_PLAYBACK_STATE_GAIN,
    HEIMDALL_ML_PLAYBACK_STATE_AUDIO_DELAY,
    HEIMDALL_ML_PLAYBACK_STATE_SUBTITLE_TRACK,
    HEIMDALL_ML_PLAYBACK_STATE_SUBTITLE_DELAY,
    HEIMDALL_ML_PLAYBACK_STATE_APP_SPECIFIC,
};

typedef struct heimdall_ml_playback_states_all
{
    float rate;
    float zoom;
    int current_title;
    char* current_video_track;
    char* current_audio_track;
    char *current_subtitle_track;
    char* aspect_ratio;
    char* crop;
    char* deinterlace;
    char* video_filter;
} heimdall_ml_playback_states_all;

enum heimdall_ml_event_type
{
    /**
     * Entity modification callbacks. The affected entity will be passed:
     * - As a `heimdall_ml_<type>_t`, depending on the type of the modified/inserted
     * entity, in `heimdall_ml_event_t::modification::p_<type>`
     * for ADDED and UPDATED variants.
     * - as an id, in ::heimdall_ml_event_t::deletion::i_entity_id
     * When _DELETED callbacks get invoked, the entity will already have been
     * deleted from the database, and cannot be retrieved anymore
     */
    HEIMDALL_ML_EVENT_MEDIA_ADDED,
    HEIMDALL_ML_EVENT_MEDIA_UPDATED,
    HEIMDALL_ML_EVENT_MEDIA_DELETED,
    HEIMDALL_ML_EVENT_ARTIST_ADDED,
    HEIMDALL_ML_EVENT_ARTIST_UPDATED,
    HEIMDALL_ML_EVENT_ARTIST_DELETED,
    HEIMDALL_ML_EVENT_ALBUM_ADDED,
    HEIMDALL_ML_EVENT_ALBUM_UPDATED,
    HEIMDALL_ML_EVENT_ALBUM_DELETED,
    HEIMDALL_ML_EVENT_GROUP_ADDED,
    HEIMDALL_ML_EVENT_GROUP_UPDATED,
    HEIMDALL_ML_EVENT_GROUP_DELETED,
    HEIMDALL_ML_EVENT_PLAYLIST_ADDED,
    HEIMDALL_ML_EVENT_PLAYLIST_UPDATED,
    HEIMDALL_ML_EVENT_PLAYLIST_DELETED,
    HEIMDALL_ML_EVENT_GENRE_ADDED,
    HEIMDALL_ML_EVENT_GENRE_UPDATED,
    HEIMDALL_ML_EVENT_GENRE_DELETED,
    HEIMDALL_ML_EVENT_BOOKMARKS_ADDED,
    HEIMDALL_ML_EVENT_BOOKMARKS_UPDATED,
    HEIMDALL_ML_EVENT_BOOKMARKS_DELETED,
    HEIMDALL_ML_EVENT_FOLDER_ADDED,
    HEIMDALL_ML_EVENT_FOLDER_UPDATED,
    HEIMDALL_ML_EVENT_FOLDER_DELETED,
    /**
     * A discovery started.
     *
     * This event will be emitted when the media library starts discovering a
     * scheduled entry point.
     * If more than a single entry point are queued, this event won't be fired
     * again until all operations are completed and a new operation is scheduled.
     * Once all currently queued operations are done
     * HEIMDALL_ML_EVENT_DISCOVERY_COMPLETED will be emitted.
     */
    HEIMDALL_ML_EVENT_DISCOVERY_STARTED,
    /**
     * Sent when a discovery or reload operation starts analyzing a new folder.
     * The discovered entry point is stored in
     * heimdall_ml_event_t::discovery_progress::psz_entry_point.
     */
    HEIMDALL_ML_EVENT_DISCOVERY_PROGRESS,
    /**
     * Sent when all queued discovery operations are done being processed.
     */
    HEIMDALL_ML_EVENT_DISCOVERY_COMPLETED,
    /**
     * This event is sent when a discovery failed. The entry point that failed to
     * be discovered is stored in
     * heimdall_ml_event_t::discovery_failed::psz_entry_point
     */
    HEIMDALL_ML_EVENT_DISCOVERY_FAILED,
    /**
     * Sent when a new entry point gets added to the database.
     * The entry point that was added is stored in
     * heimdall::ml_event_t::entry_point_added::psz_entry_point, and the success or failure
     * state is stored in heimdall_ml_event_t::entry_point_added::b_success
     * If successful, this event won't be emitted again for this entry point.
     * In case of failure, this event will be fired again if the same entry point
     * is queued for discovery again.
     */
    HEIMDALL_ML_EVENT_ENTRY_POINT_ADDED,
    /**
     * Sent when an entry point removal request has been processed.
     * The removed entry point is stored in
     * heimdall_ml_event_t::entry_point_removed::psz_entry_point and the success or failure
     * state is stored in heimdall_ml_event_t::entry_point_removed::b_success
     */
    HEIMDALL_ML_EVENT_ENTRY_POINT_REMOVED,
    /**
     * Sent when an entry point ban request has been processed.
     * The banned entry point is stored in
     * heimdall_ml_event_t::entry_point_banned::psz_entry_point and the operation success
     * state is stored in heimdall_ml_event_t::entry_point_banned::b_success
     */
    HEIMDALL_ML_EVENT_ENTRY_POINT_BANNED,
    /**
     * Sent when an entry point unban request has been processed.
     * The unbanned entry point is stored in
     * heimdall_ml_event_t::entry_point_unbanned::psz_entry_point and the operation success
     * state is stored in heimdall_ml_event_t::entry_point_unbanned::b_success
     */
    HEIMDALL_ML_EVENT_ENTRY_POINT_UNBANNED,
    /**
     * Sent when a discoverer or parser threads changes its idle state.
     * The idle state is stored in heimdall_ml_event_t::background_idle_changed.b_idle.
     * False means at least one background thread is in running, true means
     * both discoverer & parser threads are paused.
     */
    HEIMDALL_ML_EVENT_BACKGROUND_IDLE_CHANGED,
    /**
     * Sent when the parsing progress percentage gets updated.
     * The percentage is stored as a [0;100] integer, in
     * heimdall_ml_event_t::parsing_progress::i_percent
     * This value might decrease as more media get discovered, but it will only
     * increase once all discovery operations are completed.
     */
    HEIMDALL_ML_EVENT_PARSING_PROGRESS_UPDATED,
    /**
     * Sent after a media thumbnail was generated, or if it's generation failed.
     * The media is stored in heimdall_ml_event_t::media_thumbnail_generated::p_media
     * and the success state is stored in
     * heimdall_ml_event_t::media_thumbnail_generated::b_success
     */
    HEIMDALL_ML_EVENT_MEDIA_THUMBNAIL_GENERATED,
    /**
     * Sent after the history gets changed. It can be either cleaned, or simply
     * modified because a media was recently played/removed from the history.
     * The history type (global/local/network) is stored in
     * heimdall_ml_event_t::history_changed::history_type
     */
    HEIMDALL_ML_EVENT_HISTORY_CHANGED,
    /**
     * Sent when an application requested rescan starts being processed.
     */
    HEIMDALL_ML_EVENT_RESCAN_STARTED,
    /**
     * Sent when favorites list changes. This includes when entities are
     * marked/unmarked as favorites.
     * The entity type and id are stored in heimdall_ml_event_t::favorites_changed
     */
    HEIMDALL_ML_EVENT_FAVORITES_CHANGED,
};

typedef struct heimdall_ml_event_t
{
    int i_type;
    union
    {
        struct
        {
            const char* psz_entry_point;
        } discovery_progress;
        struct
        {
            const char* psz_entry_point;
        } discovery_failed;
        struct
        {
            const char* psz_entry_point;
            bool b_success;
        } entry_point_added;
        struct
        {
            const char* psz_entry_point;
            bool b_success;
        } entry_point_removed;
        struct
        {
            const char* psz_entry_point;
            bool b_success;
        } entry_point_banned;
        struct
        {
            const char* psz_entry_point;
            bool b_success;
        } entry_point_unbanned;
        struct
        {
            uint8_t i_percent;
        } parsing_progress;
        union
        {
            const heimdall_ml_media_t* p_media;
            const heimdall_ml_artist_t* p_artist;
            const heimdall_ml_album_t* p_album;
            const heimdall_ml_group_t* p_group;
            const heimdall_ml_playlist_t* p_playlist;
            const heimdall_ml_genre_t* p_genre;
            const heimdall_ml_bookmark_t* p_bookmark;
            const heimdall_ml_folder_t* p_folder;
        } creation;
        struct
        {
            int64_t i_entity_id;
        } modification;
        struct
        {
            int64_t i_entity_id;
        } deletion;
        struct
        {
            bool b_idle;
        } background_idle_changed;
        struct
        {
            const heimdall_ml_media_t* p_media;
            heimdall_ml_thumbnail_size_t i_size;
            bool b_success;
        } media_thumbnail_generated;
        struct
        {
            heimdall_ml_history_type_t history_type;
        } history_changed;
        struct
        {
            int64_t i_entity_id;
            int i_entity_type; /**< One of HEIMDALL_ML_PARENT_* values */
            bool b_favorite; /**< true if marked as favorite, false if unmarked */
        } favorites_changed;
    };
} heimdall_ml_event_t;

typedef void (*heimdall_ml_callback_t)( void* p_data, const heimdall_ml_event_t* p_event );

typedef struct heimdall_medialibrary_callbacks_t
{
    void (*pf_send_event)( heimdall_medialibrary_module_t* p_ml, const heimdall_ml_event_t* p_event );
} heimdall_medialibrary_callbacks_t;

struct heimdall_medialibrary_module_t
{
    struct heimdall_object_t obj;

    module_t *p_module;

    void* p_sys;

    int (*pf_control)( struct heimdall_medialibrary_module_t* p_ml, int i_query, va_list args );
    /**
     * List some entities from the medialibrary.
     *
     * \param p_ml The medialibrary module instance.
     * \param i_query The type search to be performed. \see heimdall_ml_list enumeration
     * \param p_params A pointer to a heimdall_ml_query_params_t structure, or NULL for
     * the default parameters (alphabetical ascending sort, no pagination)
     *
     * \return HEIMDALL_SUCCESS or an error code
     *
     * Refer to the individual list of heimdall_ml_list requests for the additional
     * per-query input/output parameters values & types
     */
    int (*pf_list)( struct heimdall_medialibrary_module_t* p_ml, int i_query,
                    const heimdall_ml_query_params_t* p_params, va_list args );

    /**
     * Get a specific entity by its id or another unique value
     *
     * \return The required entity, or a NULL pointer if couldn't be found.
     *
     * Refer to the list of queries for the specific return type
     */
    void* (*pf_get)( struct heimdall_medialibrary_module_t* p_ml, int i_query, va_list args );

    const heimdall_medialibrary_callbacks_t* cbs;
};

heimdall_medialibrary_t* libheimdall_MlCreate( libheimdall_int_t* p_libheimdall );
void libheimdall_MlRelease( heimdall_medialibrary_t* p_ml );

HEIMDALL_API heimdall_medialibrary_t* heimdall_ml_instance_get( heimdall_object_t* p_obj ) HEIMDALL_USED;
#define heimdall_ml_instance_get(x) heimdall_ml_instance_get( HEIMDALL_OBJECT(x) )

HEIMDALL_API void* heimdall_ml_get( heimdall_medialibrary_t* p_ml, int i_query, ... ) HEIMDALL_USED;
HEIMDALL_API int heimdall_ml_control( heimdall_medialibrary_t* p_ml, int i_query, ... ) HEIMDALL_USED;
HEIMDALL_API int heimdall_ml_list( heimdall_medialibrary_t* p_ml, int i_query,
                             const heimdall_ml_query_params_t* p_params, ... );

/**
 * \brief Registers a medialibrary callback.
 * \returns A handle to the callback, to be passed to heimdall_ml_event_unregister_callback
 */
HEIMDALL_API heimdall_ml_event_callback_t*
heimdall_ml_event_register_callback( heimdall_medialibrary_t* p_ml, heimdall_ml_callback_t cb, void* p_data );

/**
 * \brief Unregisters a medialibrary callback
 * \param p_ml an initialized medialibrary instance
 * \param p_callback The callback handle returned by heimdall_ml_register_callback
 */
HEIMDALL_API void heimdall_ml_event_unregister_callback( heimdall_medialibrary_t* p_ml,
                                               heimdall_ml_event_callback_t* p_callback );
/**
 * \brief Unregisters a medialibrary callback from the said callback.
 * \param p_ml an initialized medialibrary instance
 * \param p_callback The handle returned by heimdall_ml_register_callback
 *
 * This must only be called synchronously from the callback function provided to
 * heimdall_ml_event_register_callback
 * The p_callback handle must be considered invalid when this function returns
 */
HEIMDALL_API void heimdall_ml_event_unregister_from_callback( heimdall_medialibrary_t* p_ml,
                                                    heimdall_ml_event_callback_t* p_callback );


HEIMDALL_API void heimdall_ml_show_release( heimdall_ml_show_t* p_show );
HEIMDALL_API void heimdall_ml_artist_release( heimdall_ml_artist_t* p_artist );
HEIMDALL_API void heimdall_ml_genre_release( heimdall_ml_genre_t* p_genre );
HEIMDALL_API void heimdall_ml_media_release( heimdall_ml_media_t* p_media );
HEIMDALL_API void heimdall_ml_album_release( heimdall_ml_album_t* p_album );
HEIMDALL_API void heimdall_ml_group_release( heimdall_ml_group_t* p_group );
HEIMDALL_API void heimdall_ml_playlist_release( heimdall_ml_playlist_t* p_playlist );
HEIMDALL_API void heimdall_ml_folder_release( heimdall_ml_folder_t* p_folder );

HEIMDALL_API void heimdall_ml_label_list_release( heimdall_ml_label_list_t* p_list );
HEIMDALL_API void heimdall_ml_file_list_release( heimdall_ml_file_list_t* p_list );
HEIMDALL_API void heimdall_ml_artist_list_release( heimdall_ml_artist_list_t* p_list );
HEIMDALL_API void heimdall_ml_media_list_release( heimdall_ml_media_list_t* p_list );
HEIMDALL_API void heimdall_ml_album_list_release( heimdall_ml_album_list_t* p_list );
HEIMDALL_API void heimdall_ml_show_list_release( heimdall_ml_show_list_t* p_list );
HEIMDALL_API void heimdall_ml_genre_list_release( heimdall_ml_genre_list_t* p_list );
HEIMDALL_API void heimdall_ml_group_list_release( heimdall_ml_group_list_t* p_list );
HEIMDALL_API void heimdall_ml_playlist_list_release( heimdall_ml_playlist_list_t* p_list );
HEIMDALL_API void heimdall_ml_folder_list_release( heimdall_ml_folder_list_t* p_list );
HEIMDALL_API void heimdall_ml_playback_states_all_release( heimdall_ml_playback_states_all* prefs );
HEIMDALL_API void heimdall_ml_bookmark_release( heimdall_ml_bookmark_t* p_bookmark );
HEIMDALL_API void heimdall_ml_bookmark_list_release( heimdall_ml_bookmark_list_t* p_list );

static inline heimdall_ml_query_params_t heimdall_ml_query_params_create(void)
{
#ifdef __cplusplus
    heimdall_ml_query_params_t ret = { };
    ret.i_sort = HEIMDALL_ML_SORTING_DEFAULT;
#else
    heimdall_ml_query_params_t ret = { .i_sort = HEIMDALL_ML_SORTING_DEFAULT };
#endif
    return ret;
}

static inline int heimdall_ml_add_folder( heimdall_medialibrary_t* p_ml, const char* psz_folder )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_ADD_FOLDER, psz_folder );
}

static inline int heimdall_ml_remove_folder( heimdall_medialibrary_t* p_ml, const char* psz_folder )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_REMOVE_FOLDER, psz_folder );
}

static inline int heimdall_ml_ban_folder( heimdall_medialibrary_t* p_ml, const char* psz_folder )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_BAN_FOLDER, psz_folder );
}

static inline int heimdall_ml_unban_folder( heimdall_medialibrary_t* p_ml, const char* psz_folder )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_UNBAN_FOLDER, psz_folder );
}

static inline int heimdall_ml_is_indexed( heimdall_medialibrary_t* p_ml,
                                     const char* psz_mrl, bool* p_res )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_IS_INDEXED, psz_mrl, p_res );
}

static inline int heimdall_ml_reload_folder( heimdall_medialibrary_t* p_ml, const char* psz_mrl )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_RELOAD_FOLDER, psz_mrl );
}

static inline int heimdall_ml_set_folder_public( heimdall_medialibrary_t* p_ml, const char* psz_mrl )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_SET_FOLDER_PUBLIC, psz_mrl );
}

static inline int heimdall_ml_set_folder_private( heimdall_medialibrary_t* p_ml, const char* psz_mrl )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_SET_FOLDER_PRIVATE, psz_mrl );
}

static inline int heimdall_ml_pause_background( heimdall_medialibrary_t* p_ml )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_PAUSE_BACKGROUND );
}

static inline int heimdall_ml_resume_background( heimdall_medialibrary_t* p_ml )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_RESUME_BACKGROUND );
}

static inline int heimdall_ml_clear_history( heimdall_medialibrary_t* p_ml, heimdall_ml_history_type_t type )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_CLEAR_HISTORY, type );
}

static inline heimdall_ml_media_t* heimdall_ml_new_external_media( heimdall_medialibrary_t* p_ml, const char* psz_mrl )
{
    heimdall_ml_media_t* res;
    if ( heimdall_ml_control( p_ml, HEIMDALL_ML_NEW_EXTERNAL_MEDIA, psz_mrl, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline heimdall_ml_media_t* heimdall_ml_new_stream( heimdall_medialibrary_t* p_ml, const char* psz_mrl )
{
    heimdall_ml_media_t* res;
    if ( heimdall_ml_control( p_ml, HEIMDALL_ML_NEW_STREAM, psz_mrl, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline int heimdall_ml_remove_stream( heimdall_medialibrary_t* p_ml, int64_t i_media_id )
{
    return heimdall_ml_control(p_ml, HEIMDALL_ML_REMOVE_STREAM, i_media_id);
}

static inline int heimdall_ml_media_add_label( heimdall_medialibrary_t* p_ml,
                                          int64_t i_media_id,
                                          const char* psz_label )
{
    return heimdall_ml_control(p_ml, HEIMDALL_ML_MEDIA_ADD_LABEL, i_media_id, psz_label);
}

static inline int heimdall_ml_media_remove_label( heimdall_medialibrary_t* p_ml,
                                             int64_t i_media_id,
                                             const char* psz_label )
{
    return heimdall_ml_control(p_ml, HEIMDALL_ML_MEDIA_REMOVE_LABEL, i_media_id, psz_label);
}

static inline int heimdall_ml_media_update_progress( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                                double progress )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_UPDATE_PROGRESS, i_media_id, progress );
}

static inline int heimdall_ml_media_get_playback_state( heimdall_medialibrary_t* p_ml, int64_t i_media_id, int i_state, char** ppsz_result )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_GET_MEDIA_PLAYBACK_STATE, i_media_id, i_state, ppsz_result );
}

static inline int heimdall_ml_media_set_playback_state( heimdall_medialibrary_t* p_ml, int64_t i_media_id, int i_state, const char* psz_value )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_MEDIA_PLAYBACK_STATE, i_media_id, i_state, psz_value );
}

static inline int heimdall_ml_media_get_all_playback_pref( heimdall_medialibrary_t* p_ml,
                                                      int64_t i_media_id,
                                                      heimdall_ml_playback_states_all* prefs )
{
    return heimdall_ml_control( p_ml,HEIMDALL_ML_MEDIA_GET_ALL_MEDIA_PLAYBACK_STATES, i_media_id, prefs );
}

static inline int heimdall_ml_media_set_all_playback_states( heimdall_medialibrary_t* p_ml,
                                                        int64_t i_media_id,
                                                        const heimdall_ml_playback_states_all* prefs )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_ALL_MEDIA_PLAYBACK_STATES, i_media_id, prefs );
}

static inline int heimdall_ml_media_set_thumbnail( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                              const char* psz_mrl, heimdall_ml_thumbnail_size_t sizeType )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_THUMBNAIL, i_media_id, psz_mrl, sizeType );
}

static inline int heimdall_ml_media_set_genre_thumbnail( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                              const char* psz_mrl, heimdall_ml_thumbnail_size_t sizeType )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_GENRE_THUMBNAIL, i_media_id, psz_mrl, sizeType );
}


static inline int heimdall_ml_media_generate_thumbnail( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                                   heimdall_ml_thumbnail_size_t size_type,
                                                   uint32_t i_desired_width,
                                                   uint32_t i_desired_height,
                                                   double position )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_GENERATE_THUMBNAIL, i_media_id,
                           size_type, i_desired_width, i_desired_height, position );
}

static inline int heimdall_ml_media_add_external_mrl( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                                 const char* psz_mrl, int i_type )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_ADD_EXTERNAL_MRL, i_media_id, psz_mrl, i_type );
}

static inline int heimdall_ml_media_set_type( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                         heimdall_ml_media_type_t i_type )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_TYPE, i_media_id, (int)i_type );
}

static inline int heimdall_ml_media_set_played( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                           bool b_played )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_PLAYED, i_media_id, (int) b_played );
}

static inline int heimdall_ml_media_set_favorite( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                                             bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_SET_FAVORITE, i_media_id, (int) b_favorite );
}

static inline heimdall_ml_bookmark_list_t*
heimdall_ml_list_media_bookmarks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params,
                             int64_t i_media_id )
{
    assert( p_ml != NULL );
    heimdall_ml_bookmark_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_MEDIA_BOOKMARKS, params, i_media_id,
                      &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline int
heimdall_ml_media_add_bookmark( heimdall_medialibrary_t* p_ml, int64_t i_media_id, int64_t i_time )
{
    assert( p_ml != NULL );
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_ADD_BOOKMARK, i_media_id, i_time );
}

static inline int
heimdall_ml_media_remove_bookmark( heimdall_medialibrary_t* p_ml, int64_t i_media_id, int64_t i_time )
{
    assert( p_ml != NULL );
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_REMOVE_BOOKMARK, i_media_id, i_time );
}

static inline int
heimdall_ml_media_update_bookmark( heimdall_medialibrary_t* p_ml, int64_t i_media_id,
                              int64_t i_time, const char* psz_name,
                              const char* psz_desc )
{
    assert( p_ml != NULL );
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_UPDATE_BOOKMARK, i_media_id,
                           i_time, psz_name, psz_desc );
}

static inline int
heimdall_ml_media_remove_all_bookmarks( heimdall_medialibrary_t* p_ml, int64_t i_media_id )
{
    assert( p_ml != NULL );
    return heimdall_ml_control( p_ml, HEIMDALL_ML_MEDIA_REMOVE_ALL_BOOKMARKS, i_media_id );
}

static inline heimdall_ml_playlist_t*
heimdall_ml_playlist_create( heimdall_medialibrary_t * p_ml, const char * name)
{
    assert( p_ml != NULL );

    heimdall_ml_playlist_t* result;

    if (heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_CREATE, name, &result ) != HEIMDALL_SUCCESS )
        return NULL;

    return result;
}

static inline int
heimdall_ml_playlist_delete( heimdall_medialibrary_t * p_ml, int64_t i_playlist_id )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_DELETE, i_playlist_id );
}

static inline int
heimdall_ml_playlist_append( heimdall_medialibrary_t * p_ml, int64_t i_playlist_id, const int64_t *i_media_ids, size_t i_media_id_count )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_APPEND, i_playlist_id, i_media_ids, i_media_id_count );
}

static inline int
heimdall_ml_playlist_insert( heimdall_medialibrary_t * p_ml, int64_t i_playlist_id, const int64_t *i_media_ids,
                        size_t i_media_id_count, uint32_t i_position )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_INSERT, i_playlist_id, i_media_ids, i_media_id_count,  i_position );
}

static inline int
heimdall_ml_playlist_move( heimdall_medialibrary_t * p_ml,
                      int64_t i_playlist_id, uint32_t i_from, uint32_t i_to, uint32_t i_count )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_MOVE, i_playlist_id, i_from, i_to, i_count );
}

static inline int
heimdall_ml_playlist_remove( heimdall_medialibrary_t * p_ml, int64_t i_playlist_id, uint32_t i_position, uint32_t i_count )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_REMOVE, i_playlist_id, i_position, i_count );
}

static inline int
heimdall_ml_playlist_rename( heimdall_medialibrary_t * p_ml, int64_t i_playlist_id, const char* name )
{
    assert( p_ml != NULL );

    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_RENAME, i_playlist_id, name );
}

static inline heimdall_ml_media_t* heimdall_ml_get_media( heimdall_medialibrary_t* p_ml, int64_t i_media_id )
{
    return (heimdall_ml_media_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_MEDIA, i_media_id );
}

static inline heimdall_ml_media_t* heimdall_ml_get_media_by_mrl( heimdall_medialibrary_t* p_ml,
                                                       const char* psz_mrl )
{
    return (heimdall_ml_media_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_MEDIA_BY_MRL, psz_mrl );
}

static inline input_item_t* heimdall_ml_get_input_item( heimdall_medialibrary_t* p_ml, int64_t i_media_id )
{
    return (input_item_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_INPUT_ITEM, i_media_id );
}

static inline input_item_t* heimdall_ml_get_input_item_by_mrl( heimdall_medialibrary_t* p_ml,
                                                          const char* psz_mrl )
{
    return (input_item_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_INPUT_ITEM_BY_MRL, psz_mrl );
}

static inline heimdall_ml_album_t* heimdall_ml_get_album( heimdall_medialibrary_t* p_ml, int64_t i_album_id )
{
    return (heimdall_ml_album_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_ALBUM, i_album_id );
}

static inline heimdall_ml_artist_t* heimdall_ml_get_artist( heimdall_medialibrary_t* p_ml, int64_t i_artist_id )
{
    return (heimdall_ml_artist_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_ARTIST, i_artist_id );
}

static inline heimdall_ml_genre_t* heimdall_ml_get_genre( heimdall_medialibrary_t* p_ml, int64_t i_genre_id )
{
    return (heimdall_ml_genre_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_GENRE, i_genre_id );
}

static inline heimdall_ml_show_t* heimdall_ml_get_show( heimdall_medialibrary_t* p_ml, int64_t i_show_id )
{
    return (heimdall_ml_show_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_SHOW, i_show_id );
}

static inline heimdall_ml_group_t* heimdall_ml_get_group( heimdall_medialibrary_t* p_ml, int64_t i_group_id )
{
    return (heimdall_ml_group_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_GROUP, i_group_id );
}

static inline heimdall_ml_folder_t* heimdall_ml_get_folder( heimdall_medialibrary_t* p_ml, int64_t i_folder_id )
{
    return (heimdall_ml_folder_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_FOLDER, i_folder_id);
}

static inline heimdall_ml_playlist_t* heimdall_ml_get_playlist( heimdall_medialibrary_t* p_ml, int64_t i_playlist_id )
{
    return (heimdall_ml_playlist_t*)heimdall_ml_get( p_ml, HEIMDALL_ML_GET_PLAYLIST, i_playlist_id );
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_media_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_MEDIA_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_media_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    size_t res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_MEDIA_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return 0;
    return res;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_video_of( heimdall_medialibrary_t* p_ml,
                                                         const heimdall_ml_query_params_t* params,
                                                         int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );

    heimdall_ml_media_list_t* res;

    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_VIDEO_OF,
                      params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;

    return res;
}

static inline size_t heimdall_ml_count_video_of( heimdall_medialibrary_t* p_ml,
                                            const heimdall_ml_query_params_t* params,
                                            int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );

    size_t res;

    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_VIDEO_OF,
                      params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return 0;

    return res;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_audio_of( heimdall_medialibrary_t* p_ml,
                                                         const heimdall_ml_query_params_t* params,
                                                         int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );

    heimdall_ml_media_list_t* res;

    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_AUDIO_OF,
                      params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;

    return res;
}

static inline size_t heimdall_ml_count_audio_of( heimdall_medialibrary_t* p_ml,
                                            const heimdall_ml_query_params_t* params,
                                            int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );

    size_t res;

    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_AUDIO_OF,
                      params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return 0;

    return res;
}

static inline heimdall_ml_artist_list_t* heimdall_ml_list_artist_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_artist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ARTISTS_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_artists_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    size_t res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ARTISTS_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return 0;
    return res;
}

static inline heimdall_ml_album_list_t* heimdall_ml_list_albums_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_album_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ALBUMS_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_albums_of( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int i_parent_type, int64_t i_parent_id )
{
    heimdall_assert( p_ml != NULL );
    size_t res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ALBUMS_OF, params, i_parent_type, i_parent_id, &res ) != HEIMDALL_SUCCESS )
        return 0;
    return res;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_album_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_album_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ALBUM_TRACKS, params, i_album_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_album_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_album_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ALBUM_TRACKS, params, i_album_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_album_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_album_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ALBUM_ARTISTS, params, i_album_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_album_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_album_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ALBUM_ARTISTS, params, i_album_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_album_list_t* heimdall_ml_list_artist_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_artist_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_album_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ARTIST_ALBUMS, params, i_artist_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_artist_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_artist_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ARTIST_ALBUMS, params, i_artist_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_artist_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_artist_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ARTIST_TRACKS, params, i_artist_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_artist_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_artist_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ARTIST_TRACKS, params, i_artist_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_MEDIA, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_MEDIA, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_video_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_VIDEOS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_video_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_VIDEOS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_audio_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_AUDIOS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_audio_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_AUDIOS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_album_list_t* heimdall_ml_list_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_album_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ALBUMS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ALBUMS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_genre_list_t* heimdall_ml_list_genres( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_genre_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GENRES, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_genres( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GENRES, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

/**
 * @brief heimdall_ml_list_artists
 * @param p_ml an initialized medialibrary instance
 * @param params Query parameters, or NULL for the default
 * @param b_include_all True if you wish to fetch artists without at least one album.
 * @return
 */
static inline heimdall_ml_artist_list_t* heimdall_ml_list_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, bool b_include_all )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_artist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ARTISTS, params, (int)b_include_all, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, bool includeAll )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ARTISTS, params, includeAll, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_show_list_t* heimdall_ml_list_shows( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_show_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_SHOWS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_movies(heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params)
{
    heimdall_assert(p_ml != NULL);
    heimdall_ml_media_list_t* res;
    if (heimdall_ml_list(p_ml, HEIMDALL_ML_LIST_MOVIES, params, &res) != HEIMDALL_SUCCESS)
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_movies(heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params)
{
    heimdall_assert(p_ml != NULL);
    size_t count;
    if (heimdall_ml_list(p_ml, HEIMDALL_ML_COUNT_MOVIES, params, &count) != HEIMDALL_SUCCESS)
        return 0;
    return count;
}

static inline size_t heimdall_ml_count_shows( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_SHOWS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_artist_list_t* heimdall_ml_list_genre_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_artist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GENRE_ARTISTS, params, i_genre_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_genre_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GENRE_ARTISTS, params, i_genre_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_genre_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GENRE_TRACKS, params, i_genre_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_genre_tracks( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GENRE_TRACKS, params, i_genre_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_album_list_t* heimdall_ml_list_genre_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_album_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GENRE_ALBUMS, params, i_genre_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_genre_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_genre_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GENRE_ALBUMS, params, i_genre_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_show_episodes( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_show_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_SHOW_EPISODES, params, i_show_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_show_episodes( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_show_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GENRE_ALBUMS, params, i_show_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_label_list_t* heimdall_ml_list_media_labels( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_media_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_label_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_MEDIA_LABELS, params, i_media_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_media_labels( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_media_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_MEDIA_LABELS, params, i_media_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, heimdall_ml_history_type_t type )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_HISTORY, params, (int)type, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, heimdall_ml_history_type_t type )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_HISTORY, params, (int)type, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_video_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_VIDEO_HISTORY, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_video_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_VIDEO_HISTORY, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_audio_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_AUDIO_HISTORY, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_audio_history( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_AUDIO_HISTORY, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

//-------------------------------------------------------------------------------------------------
// Groups

static inline heimdall_ml_group_list_t* heimdall_ml_list_groups( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_group_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GROUPS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_groups( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GROUPS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_group_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_group_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_GROUP_MEDIA, params, i_group_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_group_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_group_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_GROUP_MEDIA, params, i_group_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

//-------------------------------------------------------------------------------------------------

static inline heimdall_ml_playlist_list_t* heimdall_ml_list_playlists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, heimdall_ml_playlist_type_t playlist_type )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_playlist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_PLAYLISTS, params, (int) playlist_type, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_playlists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, heimdall_ml_playlist_type_t playlist_type)
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_PLAYLISTS, params, (int) playlist_type, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_playlist_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_playlist_id )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_PLAYLIST_MEDIA, params, i_playlist_id, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_playlist_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params, int64_t i_playlist_id )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_PLAYLIST_MEDIA, params, i_playlist_id, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_folder_list_t* heimdall_ml_list_entry_points( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_folder_list_t *res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ENTRY_POINTS, params, (int)false, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_entry_points( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ENTRY_POINTS, params, (int)false, &res ) != HEIMDALL_SUCCESS )
        return 0;
    return res;
}

static inline heimdall_ml_folder_list_t*
heimdall_ml_list_banned_entry_points( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_folder_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_ENTRY_POINTS, params, (int)true, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_banned_entry_points( heimdall_medialibrary_t* p_ml,
                                                       const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_ENTRY_POINTS, params, (int)true, &res ) != HEIMDALL_SUCCESS )
        return 0;
    return res;
}

// Folders

static inline heimdall_ml_folder_list_t * heimdall_ml_list_folders(heimdall_medialibrary_t * p_ml,
                                                         const heimdall_ml_query_params_t * params)
{
    heimdall_assert(p_ml != NULL);

    heimdall_ml_folder_list_t * res;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_LIST_FOLDERS, params, &res) != HEIMDALL_SUCCESS)
        return NULL;

    return res;
}

static inline size_t heimdall_ml_count_folders(heimdall_medialibrary_t * p_ml,
                                          const heimdall_ml_query_params_t * params)
{
    heimdall_assert(p_ml != NULL);

    size_t count;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_COUNT_FOLDERS, params, &count) != HEIMDALL_SUCCESS)
        return 0;

    return count;
}

static inline
heimdall_ml_folder_list_t * heimdall_ml_list_folders_by_type(heimdall_medialibrary_t * p_ml,
                                                   const heimdall_ml_query_params_t * params,
                                                   heimdall_ml_media_type_t type)
{
    heimdall_assert(p_ml != NULL);

    heimdall_ml_folder_list_t * res;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_LIST_FOLDERS_BY_TYPE, params, (int) type, &res) != HEIMDALL_SUCCESS)
        return NULL;

    return res;
}

static inline size_t heimdall_ml_count_folders_by_type(heimdall_medialibrary_t * p_ml,
                                                  const heimdall_ml_query_params_t * params,
                                                  heimdall_ml_media_type_t type)
{
    heimdall_assert(p_ml != NULL);

    size_t count;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_COUNT_FOLDERS_BY_TYPE, params, (int) type, &count) != HEIMDALL_SUCCESS)
        return 0;

    return count;
}

// Folder Media

static inline heimdall_ml_media_list_t * heimdall_ml_list_folder_media(heimdall_medialibrary_t * p_ml,
                                                             const heimdall_ml_query_params_t * params,
                                                             int64_t i_folder_id)
{
    heimdall_assert(p_ml != NULL);

    heimdall_ml_media_list_t * res;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_LIST_FOLDER_MEDIA, params, i_folder_id, &res) != HEIMDALL_SUCCESS)
        return NULL;

    return res;
}

static inline size_t heimdall_ml_count_folder_media(heimdall_medialibrary_t * p_ml,
                                               const heimdall_ml_query_params_t * params,
                                               int64_t i_folder_id)
{
    heimdall_assert(p_ml != NULL);

    size_t count;

    if (heimdall_ml_list(p_ml, HEIMDALL_ML_COUNT_FOLDER_MEDIA, params, i_folder_id, &count) != HEIMDALL_SUCCESS)
        return 0;

    return count;
}

static inline int heimdall_ml_folder_set_favorite( heimdall_medialibrary_t* p_ml, const char* psz_mrl, bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_FOLDER_SET_FAVORITE, psz_mrl, (int)b_favorite );
}

static inline int heimdall_ml_artist_set_favorite( heimdall_medialibrary_t* p_ml, int64_t i_artist_id, bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_ARTIST_SET_FAVORITE, i_artist_id, (int)b_favorite );
}

static inline int heimdall_ml_album_set_favorite( heimdall_medialibrary_t* p_ml, int64_t i_album_id, bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_ALBUM_SET_FAVORITE, i_album_id, (int)b_favorite );
}

static inline int heimdall_ml_genre_set_favorite( heimdall_medialibrary_t* p_ml, int64_t i_genre_id, bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_GENRE_SET_FAVORITE, i_genre_id, (int)b_favorite );
}

static inline int heimdall_ml_playlist_set_favorite( heimdall_medialibrary_t *p_ml, int64_t i_playlist_id, bool b_favorite )
{
    return heimdall_ml_control( p_ml, HEIMDALL_ML_PLAYLIST_SET_FAVORITE, i_playlist_id, (int)b_favorite );
}

//-------------------------------------------------------------------------------------------------
// Favorites

static inline heimdall_ml_media_list_t* heimdall_ml_list_favorite_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_MEDIA, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_media( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_MEDIA, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_favorite_videos( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_VIDEOS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_videos( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_VIDEOS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_media_list_t* heimdall_ml_list_favorite_audios( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_media_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_AUDIOS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_audios( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_AUDIOS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_album_list_t* heimdall_ml_list_favorite_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_album_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_ALBUMS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_albums( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_ALBUMS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_artist_list_t* heimdall_ml_list_favorite_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_artist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_ARTISTS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_artists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_ARTISTS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_genre_list_t* heimdall_ml_list_favorite_genres( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_genre_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_GENRES, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_genres( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_GENRES, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_playlist_list_t* heimdall_ml_list_favorite_playlists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_playlist_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_PLAYLISTS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_playlists( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_PLAYLISTS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

static inline heimdall_ml_folder_list_t* heimdall_ml_list_favorite_folders( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    heimdall_ml_folder_list_t* res;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_LIST_FAVORITE_FOLDERS, params, &res ) != HEIMDALL_SUCCESS )
        return NULL;
    return res;
}

static inline size_t heimdall_ml_count_favorite_folders( heimdall_medialibrary_t* p_ml, const heimdall_ml_query_params_t* params )
{
    heimdall_assert( p_ml != NULL );
    size_t count;
    if ( heimdall_ml_list( p_ml, HEIMDALL_ML_COUNT_FAVORITE_FOLDERS, params, &count ) != HEIMDALL_SUCCESS )
        return 0;
    return count;
}

#ifdef __cplusplus
}
#endif /* C++ */

#ifndef __cplusplus
# define heimdall_ml_release( OBJ ) _Generic( ( OBJ ), \
    heimdall_ml_show_t*: heimdall_ml_show_release, \
    heimdall_ml_artist_t*: heimdall_ml_artist_release, \
    heimdall_ml_album_t*: heimdall_ml_album_release, \
    heimdall_ml_genre_t*: heimdall_ml_genre_release, \
    heimdall_ml_media_t*: heimdall_ml_media_release, \
    heimdall_ml_group_t*: heimdall_ml_group_release, \
    heimdall_ml_folder_t*: heimdall_ml_folder_release, \
    heimdall_ml_playlist_t*: heimdall_ml_playlist_release, \
    heimdall_ml_label_list_t*: heimdall_ml_label_list_release, \
    heimdall_ml_file_list_t*: heimdall_ml_file_list_release, \
    heimdall_ml_artist_list_t*: heimdall_ml_artist_list_release, \
    heimdall_ml_media_list_t*: heimdall_ml_media_list_release, \
    heimdall_ml_album_list_t*: heimdall_ml_album_list_release, \
    heimdall_ml_show_list_t*: heimdall_ml_show_list_release, \
    heimdall_ml_genre_list_t*: heimdall_ml_genre_list_release, \
    heimdall_ml_group_list_t*: heimdall_ml_group_list_release, \
    heimdall_ml_playlist_list_t*: heimdall_ml_playlist_list_release, \
    heimdall_ml_folder_list_t*: heimdall_ml_folder_list_release, \
    heimdall_ml_playback_states_all*: heimdall_ml_playback_states_all_release, \
    heimdall_ml_bookmark_t*: heimdall_ml_bookmark_release, \
    heimdall_ml_bookmark_list_t*: heimdall_ml_bookmark_list_release \
    )( OBJ )
#else
static inline void heimdall_ml_release( heimdall_ml_show_t* show ) { heimdall_ml_show_release( show ); }
static inline void heimdall_ml_release( heimdall_ml_artist_t* artist ) { heimdall_ml_artist_release( artist ); }
static inline void heimdall_ml_release( heimdall_ml_album_t* album ) { heimdall_ml_album_release( album ); }
static inline void heimdall_ml_release( heimdall_ml_genre_t* genre ) { heimdall_ml_genre_release( genre ); }
static inline void heimdall_ml_release( heimdall_ml_media_t* media ) { heimdall_ml_media_release( media ); }
static inline void heimdall_ml_release( heimdall_ml_group_t* group ) { heimdall_ml_group_release( group ); }
static inline void heimdall_ml_release( heimdall_ml_folder_t* folder ) { heimdall_ml_folder_release( folder ); }
static inline void heimdall_ml_release( heimdall_ml_playlist_t* playlist ) { heimdall_ml_playlist_release( playlist ); }
static inline void heimdall_ml_release( heimdall_ml_label_list_t* list ) { heimdall_ml_label_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_file_list_t* list ) { heimdall_ml_file_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_artist_list_t* list ) { heimdall_ml_artist_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_media_list_t* list ) { heimdall_ml_media_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_album_list_t* list ) { heimdall_ml_album_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_show_list_t* list ) { heimdall_ml_show_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_genre_list_t* list ) { heimdall_ml_genre_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_group_list_t* list ) { heimdall_ml_group_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_playlist_list_t* list ) { heimdall_ml_playlist_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_folder_list_t* list ) { heimdall_ml_folder_list_release( list ); }
static inline void heimdall_ml_release( heimdall_ml_playback_states_all* prefs ) { heimdall_ml_playback_states_all_release( prefs ); }
static inline void heimdall_ml_release( heimdall_ml_bookmark_t* bookmark ) { heimdall_ml_bookmark_release( bookmark ); }
static inline void heimdall_ml_release( heimdall_ml_bookmark_list_t* list ) { heimdall_ml_bookmark_list_release( list ); }
#endif

#endif /* HEIMDALL_MEDIA_LIBRARY_H */
