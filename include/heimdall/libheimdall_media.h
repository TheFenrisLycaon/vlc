/*****************************************************************************
 * libheimdall_media.h:  libheimdall external API
 *****************************************************************************
 * Copyright (C) 1998-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
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

#ifndef HEIMDALL_LIBHEIMDALL_MEDIA_H
#define HEIMDALL_LIBHEIMDALL_MEDIA_H 1

#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media_track.h>

# ifdef __cplusplus
extern "C" {
# else
#  include <stdbool.h>
# endif
#include <stddef.h>

/** \defgroup libheimdall_media LibHeimdall media
 * \ingroup libheimdall
 * @ref libheimdall_media_t is an abstract representation of a playable media.
 * It consists of a media location and various optional meta data.
 * @{
 * \file
 * LibHeimdall media item/descriptor external API
 */

typedef struct libheimdall_media_t libheimdall_media_t;

/** Meta data types */
typedef enum libheimdall_meta_t {
    libheimdall_meta_Title,
    libheimdall_meta_Artist,
    libheimdall_meta_Genre,
    libheimdall_meta_Copyright,
    libheimdall_meta_Album,
    libheimdall_meta_TrackNumber,
    libheimdall_meta_Description,
    libheimdall_meta_Rating,
    libheimdall_meta_Date,
    libheimdall_meta_Setting,
    libheimdall_meta_URL,
    libheimdall_meta_Language,
    libheimdall_meta_NowPlaying,
    libheimdall_meta_Publisher,
    libheimdall_meta_EncodedBy,
    libheimdall_meta_ArtworkURL,
    libheimdall_meta_TrackID,
    libheimdall_meta_TrackTotal,
    libheimdall_meta_Director,
    libheimdall_meta_Season,
    libheimdall_meta_Episode,
    libheimdall_meta_ShowName,
    libheimdall_meta_Actors,
    libheimdall_meta_AlbumArtist,
    libheimdall_meta_DiscNumber,
    libheimdall_meta_DiscTotal,
    libheimdall_meta_Compilation,
    /* Add new meta types HERE */
} libheimdall_meta_t;

/**
 * libheimdall media or media_player state
 */
typedef enum libheimdall_state_t
{
    libheimdall_NothingSpecial=0,
    libheimdall_Opening,
    libheimdall_Playing,
    libheimdall_Paused,
    libheimdall_Stopped,
    libheimdall_Stopping,
    libheimdall_Error
} libheimdall_state_t;

enum
{
    libheimdall_media_option_trusted = 0x2,
    libheimdall_media_option_unique = 0x100
};

typedef struct libheimdall_media_stats_t
{
    /* Input */
    uint64_t     i_read_bytes;
    float       f_input_bitrate;

    /* Demux */
    uint64_t     i_demux_read_bytes;
    float       f_demux_bitrate;
    uint64_t     i_demux_corrupted;
    uint64_t     i_demux_discontinuity;

    /* Decoders */
    uint64_t     i_decoded_video;
    uint64_t     i_decoded_audio;

    /* Video Output */
    uint64_t     i_displayed_pictures;
    uint64_t     i_late_pictures;
    uint64_t     i_lost_pictures;

    /* Audio output */
    uint64_t     i_played_abuffers;
    uint64_t     i_lost_abuffers;
} libheimdall_media_stats_t;

/**
 * Media type
 *
 * \see libheimdall_media_get_type
 */
typedef enum libheimdall_media_type_t {
    libheimdall_media_type_unknown,
    libheimdall_media_type_file,
    libheimdall_media_type_directory,
    libheimdall_media_type_disc,
    libheimdall_media_type_stream,
    libheimdall_media_type_playlist,
} libheimdall_media_type_t;

/**
 * Type of a media slave: subtitle or audio.
 */
typedef enum libheimdall_media_slave_type_t
{
    libheimdall_media_slave_type_subtitle,
    libheimdall_media_slave_type_generic,
    libheimdall_media_slave_type_audio = libheimdall_media_slave_type_generic,
} libheimdall_media_slave_type_t;

/**
 * A slave of a libheimdall_media_t
 * \see libheimdall_media_slaves_get
 */
typedef struct libheimdall_media_slave_t
{
    char *                          psz_uri;
    libheimdall_media_slave_type_t       i_type;
    unsigned int                    i_priority;
} libheimdall_media_slave_t;

/**
 * Type of stat that can be requested from libheimdall_media_get_filestat()
 */
#define libheimdall_media_filestat_mtime 0
#define libheimdall_media_filestat_size 1

/**
 * struct defining callbacks for libheimdall_media_new_callbacks()
 */
struct libheimdall_media_open_cbs
{
    /** 
     * Version of struct libheimdall_media_open_cbs
     */
    uint32_t version;

    /**
     * Callback prototype to open a custom bitstream input media.
     *
     * The same media item can be opened multiple times. Each time, this callback
     * is invoked. It should allocate and initialize any instance-specific
     * resources, then store them in *datap. The instance resources can be freed
     * in the @ref close callback.
     *
     * \param cbs_opaque private pointer as passed to libheimdall_media_new_callbacks()
     * \param datap storage space for a private data pointer [OUT]
     * \param sizep byte length of the bitstream or UINT64_MAX if unknown [OUT]
     *
     * \note For convenience, *datap is initially NULL and *sizep is initially 0.
     *
     * \note Optional (can be NULL),
     * available since version 0
     *
     * \note If NULL, the opaque pointer will be passed to read_cb,
     * seek_cb and close_cb, and the stream size will be treated as unknown.
     *
     * \return 0 on success, non-zero on error. In case of failure, the other
     * callbacks will not be invoked and any value stored in *datap and *sizep is
     * discarded.
     */
    int (*open)(void *cbs_opaque, void **datap, uint64_t *sizep);

    /**
     * Callback prototype to read data from a custom bitstream input media.
     *
     * \param data private pointer as set by the @ref open callback
     * \param buf start address of the buffer to read data into
     * \param len bytes length of the buffer
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     *
     * \return strictly positive number of bytes read, 0 on end-of-stream,
     *         or -1 on non-recoverable error
     *
     * \note If no data is immediately available, then the callback should sleep.
     * \warning The application is responsible for avoiding deadlock situations.
     */
    ptrdiff_t (*read)(void *data, unsigned char *buf, size_t len);

    /**
     * Callback prototype to seek a custom bitstream input media.
     *
     * \note Optional (can be NULL if seeking is not supported),
     * available since version 0
     *
     * \param data private pointer as set by the @ref open callback
     * \param offset absolute byte offset to seek to
     * \return 0 on success, -1 on error.
     */
    int (*seek)(void *data, uint64_t offset);

    /**
     * Callback prototype to close a custom bitstream input media.
     *
     * \note Optional (can be NULL),
     * available since version 0
     *
     * \param data private pointer as set by the @ref open callback
     */
    void (*close)(void *data);
};

/**
 * Create a media with a certain given media resource location,
 * for instance a valid URL.
 *
 * \note To refer to a local file with this function,
 * the file://... URI syntax <b>must</b> be used (see IETF RFC3986).
 * We recommend using libheimdall_media_new_path() instead when dealing with
 * local files.
 *
 * \see libheimdall_media_release
 *
 * \param psz_mrl the media location
 * \return the newly created media or NULL on error
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_new_location(const char * psz_mrl);

/**
 * Create a media for a certain file path.
 *
 * \see libheimdall_media_release
 *
 * \param path local filesystem path
 * \return the newly created media or NULL on error
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_new_path(const char *path);

/**
 * Create a media for an already open file descriptor.
 * The file descriptor shall be open for reading (or reading and writing).
 *
 * Regular file descriptors, pipe read descriptors and character device
 * descriptors (including TTYs) are supported on all platforms.
 * Block device descriptors are supported where available.
 * Directory descriptors are supported on systems that provide fdopendir().
 * Sockets are supported on all platforms where they are file descriptors,
 * i.e. all except Windows.
 *
 * \note This library will <b>not</b> automatically close the file descriptor
 * under any circumstance. Nevertheless, a file descriptor can usually only be
 * rendered once in a media player. To render it a second time, the file
 * descriptor should probably be rewound to the beginning with lseek().
 *
 * \see libheimdall_media_release
 *
 * \version LibHeimdall 1.1.5 and later.
 *
 * \param fd open file descriptor
 * \return the newly created media or NULL on error
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_new_fd(int fd);

/**
 * Create a media with custom callbacks to read the data from.
 *
 * \param cbs callback to setup the media (can't be NULL). The pointed
 * struct must be kept alive (and not modified) by the caller until all
 * player instances that were supplied this media item are stopped.
 * \param cbs_opaque opaque pointer for the open callback
 *
 * \return the newly created media or NULL on error
 *
 * \note The callbacks may be called asynchronously (from another thread).
 * A single stream instance need not be reentrant. However the open_cb needs to
 * be reentrant if the media is used by multiple player instances.
 *
 * \warning The callbacks may be used until all or any player instances
 * that were supplied the media item are stopped.
 *
 * \warning The function prototype changed between LibHeimdall 3.0.0 and LibHeimdall 4.0.0
 *
 * \see libheimdall_media_release
 *
 * \version LibHeimdall 3.0.0 and later
 */
LIBHEIMDALL_API libheimdall_media_t *
libheimdall_media_new_callbacks(const struct libheimdall_media_open_cbs *cbs,
                           void *cbs_opaque);

/**
 * Create a media as an empty node with a given name.
 *
 * \see libheimdall_media_release
 *
 * \param psz_name the name of the node
 * \return the new empty media or NULL on error
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_new_as_node(const char * psz_name);

/**
 * Add an option to the media.
 *
 * This option will be used to determine how the media_player will
 * read the media. This allows to use Heimdall's advanced
 * reading/streaming options on a per-media basis.
 *
 * \note The options are listed in 'heimdall --longhelp' from the command line,
 * e.g. "--sout-all". Keep in mind that available options and their semantics
 * vary across LibHeimdall versions and builds.
 * \warning Not all options affects libheimdall_media_t objects:
 * Specifically, due to architectural issues most audio and video options,
 * such as text renderer options, have no effects on an individual media.
 * These options must be set through libheimdall_new() instead.
 *
 * \param p_md the media descriptor
 * \param psz_options the options (as a string)
 */
LIBHEIMDALL_API void libheimdall_media_add_option(
                                   libheimdall_media_t *p_md,
                                   const char * psz_options );

/**
 * Add an option to the media with configurable flags.
 *
 * This option will be used to determine how the media_player will
 * read the media. This allows to use Heimdall's advanced
 * reading/streaming options on a per-media basis.
 *
 * The options are detailed in heimdall --longhelp, for instance
 * "--sout-all". Note that all options are not usable on medias:
 * specifically, due to architectural issues, video-related options
 * such as text renderer options cannot be set on a single media. They
 * must be set on the whole libheimdall instance instead.
 *
 * \param p_md the media descriptor
 * \param psz_options the options (as a string)
 * \param i_flags the flags for this option
 */
LIBHEIMDALL_API void libheimdall_media_add_option_flag(
                                   libheimdall_media_t *p_md,
                                   const char * psz_options,
                                   unsigned i_flags );


/**
 * Retain a reference to a media descriptor object (libheimdall_media_t). Use
 * libheimdall_media_release() to decrement the reference count of a
 * media descriptor object.
 *
 * \param p_md the media descriptor
 * \return the same object
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_retain( libheimdall_media_t *p_md );

/**
 * Decrement the reference count of a media descriptor object. If the
 * reference count is 0, then libheimdall_media_release() will release the
 * media descriptor object. If the media descriptor object has been released it
 * should not be used again.
 *
 * \param p_md the media descriptor
 */
LIBHEIMDALL_API void libheimdall_media_release( libheimdall_media_t *p_md );


/**
 * Get the media resource locator (mrl) from a media descriptor object
 *
 * \param p_md a media descriptor object
 * \return string with mrl of media descriptor object
 */
LIBHEIMDALL_API char *libheimdall_media_get_mrl( libheimdall_media_t *p_md );

/**
 * Duplicate a media descriptor object.
 *
 * \warning the duplicated media won't share forthcoming updates from the
 * original one.
 *
 * \param p_md a media descriptor object.
 */
LIBHEIMDALL_API libheimdall_media_t *libheimdall_media_duplicate( libheimdall_media_t *p_md );

/**
 * Read the meta of the media.
 *
 * Note, you need to parse using libheimdall_parser API or play the media
 * at least once before calling this function.
 * If the media has not yet been parsed this will return NULL.
 *
 * \param p_md the media descriptor
 * \param e_meta the meta to read
 * \return the media's meta
 */
LIBHEIMDALL_API char *libheimdall_media_get_meta( libheimdall_media_t *p_md,
                                             libheimdall_meta_t e_meta );

/**
 * Set the meta of the media (this function will not save the meta, call
 * libheimdall_media_save_meta in order to save the meta)
 *
 * \param p_md the media descriptor
 * \param e_meta the meta to write
 * \param psz_value the media's meta
 */
LIBHEIMDALL_API void libheimdall_media_set_meta( libheimdall_media_t *p_md,
                                           libheimdall_meta_t e_meta,
                                           const char *psz_value );

/**
 * Read the meta extra of the media.
 *
 * If the media has not yet been parsed this will return NULL.
 *
 * \see libheimdall_parser
 *
 * \param p_md the media descriptor
 * \param psz_name the meta extra to read (nonnullable)
 * \return the media's meta extra or NULL
 */
LIBHEIMDALL_API char *libheimdall_media_get_meta_extra( libheimdall_media_t *p_md,
                                              const char *psz_name );

/**
 * Set the meta of the media (this function will not save the meta, call
 * libheimdall_media_save_meta in order to save the meta)
 *
 * \param p_md the media descriptor
 * \param psz_name the meta extra to write (nonnullable)
 * \param psz_value the media's meta extra (nullable)
 * Removed from meta extra if set to NULL
 */
LIBHEIMDALL_API void libheimdall_media_set_meta_extra( libheimdall_media_t *p_md,
                                             const char *psz_name,
                                             const char *psz_value );

/**
 * Read the meta extra names of the media.
 *
 * \param p_md the media descriptor
 * \param pppsz_names the media's meta extra name array
 * you can access the elements using the return value (count)
 * must be released with libheimdall_media_meta_extra_names_release()
 * \return the meta extra count
 */
LIBHEIMDALL_API unsigned libheimdall_media_get_meta_extra_names( libheimdall_media_t *p_md,
                                                       char ***pppsz_names );

/**
 * Release a media meta extra names
 *
 * \param ppsz_names meta extra names array to release
 * \param i_count number of elements in the array
 */
LIBHEIMDALL_API void libheimdall_media_meta_extra_names_release( char **ppsz_names,
                                                       unsigned i_count );

/**
 * Save the meta previously set
 *
 * \param inst LibHeimdall instance
 * \param p_md the media descriptor
 * \return true if the write operation was successful
 */
LIBHEIMDALL_API int libheimdall_media_save_meta( libheimdall_instance_t *inst,
                                       libheimdall_media_t *p_md );

/**
 * Get the current statistics about the media
 * \param p_md media descriptor object
 * \param p_stats structure that contain the statistics about the media
 *                 (this structure must be allocated by the caller)
 * \retval true statistics are available
 * \retval false otherwise
 */
LIBHEIMDALL_API bool libheimdall_media_get_stats(libheimdall_media_t *p_md,
                                       libheimdall_media_stats_t *p_stats);

/* The following method uses libheimdall_media_list_t, however, media_list usage is optional
 * and this is here for convenience */
#define HEIMDALL_FORWARD_DECLARE_OBJECT(a) struct a

/**
 * Get subitems of media descriptor object. This will increment
 * the reference count of supplied media descriptor object. Use
 * libheimdall_media_list_release() to decrement the reference counting.
 *
 * \param p_md media descriptor object
 * \return list of media descriptor subitems or NULL
 */
LIBHEIMDALL_API HEIMDALL_FORWARD_DECLARE_OBJECT(libheimdall_media_list_t *)
libheimdall_media_subitems( libheimdall_media_t *p_md );

/**
 * Get duration (in us) of media descriptor object item.
 *
 * The duration is known if the media was parsed using libheimdall_parser API,
 * played at least once, or created by a source that already advertises it,
 * such as a playlist carrying duration metadata.
 *
 * \param p_md media descriptor object
 * \return duration of media item or -1 if it is unknown
 */
LIBHEIMDALL_API libheimdall_time_t
   libheimdall_media_get_duration( libheimdall_media_t *p_md );

/**
 * Get a 'stat' value of media descriptor object item.
 *
 * \note 'stat' values are currently only parsed by directory accesses. This
 * mean that only sub medias of a directory media, parsed with
 * libheimdall_parser API can have valid 'stat' properties.
 * \version LibHeimdall 4.0.0 and later.
 *
 * \param p_md media descriptor object
 * \param type a valid libheimdall_media_stat_ define
 * \param out field in which the value will be stored
 * \return 1 on success, 0 if not found, -1 on error.
 */
LIBHEIMDALL_API int
   libheimdall_media_get_filestat( libheimdall_media_t *p_md, unsigned type, uint64_t *out );

/**
 * Whether the media has been parsed.
 *
 * \param p_md media descriptor object
 * \return true if the media has been parsed, false otherwise
 * \version LibHeimdall 4.0.0 or later
 */
LIBHEIMDALL_API bool libheimdall_media_is_parsed( libheimdall_media_t *p_md );

/**
 * Sets media descriptor's user_data. user_data is specialized data
 * accessed by the host application, Heimdall.framework uses it as a pointer to
 * an native object that references a libheimdall_media_t pointer
 *
 * \param p_md media descriptor object
 * \param p_new_user_data pointer to user data
 */
LIBHEIMDALL_API void
    libheimdall_media_set_user_data( libheimdall_media_t *p_md, void *p_new_user_data );

/**
 * Get media descriptor's user_data. user_data is specialized data
 * accessed by the host application, Heimdall.framework uses it as a pointer to
 * an native object that references a libheimdall_media_t pointer
 *
 * \see libheimdall_media_set_user_data
 *
 * \param p_md media descriptor object
 */
LIBHEIMDALL_API void *libheimdall_media_get_user_data( libheimdall_media_t *p_md );

/**
 * Get the track list for one type
 *
 * \version LibHeimdall 4.0.0 and later.
 *
 * \note You need to parse using libheimdall_parser API or play the media
 * at least once before calling this function.  Not doing this will result in
 * an empty list.
 *
 * \see libheimdall_media_tracklist_count
 * \see libheimdall_media_tracklist_at
 *
 * \param p_md media descriptor object
 * \param type type of the track list to request
 *
 * \return a valid libheimdall_media_tracklist_t or NULL in case of error, if there
 * is no track for a category, the returned list will have a size of 0, delete
 * with libheimdall_media_tracklist_delete()
 */
LIBHEIMDALL_API libheimdall_media_tracklist_t *
libheimdall_media_get_tracklist( libheimdall_media_t *p_md, libheimdall_track_type_t type );

/**
 * Get codec description from media elementary stream
 *
 * Note, you need to parse using libheimdall_parser API or play the media
 * at least once before calling this function.
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \see libheimdall_media_track_t
 *
 * \param i_type i_type from libheimdall_media_track_t
 * \param i_codec i_codec or i_original_fourcc from libheimdall_media_track_t
 *
 * \return codec description
 */
LIBHEIMDALL_API
const char *libheimdall_media_get_codec_description( libheimdall_track_type_t i_type,
                                                uint32_t i_codec );

/**
 * Get the media type of the media descriptor object
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \see libheimdall_media_type_t
 *
 * \param p_md media descriptor object
 *
 * \return media type
 */
LIBHEIMDALL_API
libheimdall_media_type_t libheimdall_media_get_type( libheimdall_media_t *p_md );

/**
 * Add a slave to the current media.
 *
 * A slave is an external input source that may contains an additional subtitle
 * track (like a .srt) or an additional audio track (like a .ac3).
 *
 * \note This function must be called before the media is parsed (via
 * libheimdall_parser API) or before the media is played (via
 * libheimdall_media_player_play())
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \param p_md media descriptor object
 * \param i_type subtitle or audio
 * \param i_priority from 0 (low priority) to 4 (high priority)
 * \param psz_uri Uri of the slave (should contain a valid scheme).
 *
 * \return 0 on success, -1 on error.
 */
LIBHEIMDALL_API
int libheimdall_media_slaves_add( libheimdall_media_t *p_md,
                             libheimdall_media_slave_type_t i_type,
                             unsigned int i_priority,
                             const char *psz_uri );

/**
 * Clear all slaves previously added by libheimdall_media_slaves_add() or
 * internally.
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \param p_md media descriptor object
 */
LIBHEIMDALL_API
void libheimdall_media_slaves_clear( libheimdall_media_t *p_md );

/**
 * Get a media descriptor's slave list
 *
 * The list will contain slaves parsed by Heimdall or previously added by
 * libheimdall_media_slaves_add(). The typical use case of this function is to save
 * a list of slave in a database for a later use.
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \see libheimdall_media_slaves_add
 *
 * \param p_md media descriptor object
 * \param ppp_slaves address to store an allocated array of slaves (must be
 * freed with libheimdall_media_slaves_release()) [OUT]
 *
 * \return the number of slaves (zero on error)
 */
LIBHEIMDALL_API
unsigned int libheimdall_media_slaves_get( libheimdall_media_t *p_md,
                                      libheimdall_media_slave_t ***ppp_slaves );

/**
 * Release a media descriptor's slave list
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \param pp_slaves slave array to release
 * \param i_count number of elements in the array
 */
LIBHEIMDALL_API
void libheimdall_media_slaves_release( libheimdall_media_slave_t **pp_slaves,
                                  unsigned int i_count );

/** @}*/

# ifdef __cplusplus
}
# endif

#include <heimdall/libheimdall_parser.h>

#endif /* HEIMDALL_LIBHEIMDALL_MEDIA_H */
