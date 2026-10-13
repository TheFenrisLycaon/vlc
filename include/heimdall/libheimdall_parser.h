/*****************************************************************************
 * libheimdall_parser.h:  libheimdall parser API
 *****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef HEIMDALL_LIBHEIMDALL_PARSER_H
#define HEIMDALL_LIBHEIMDALL_PARSER_H 1

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>

# ifdef __cplusplus
extern "C" {
# endif

/** \defgroup libheimdall_parser LibHeimdall parser
 * \ingroup libheimdall
 * \ref libheimdall_parser_t is an abstract representation of a parser
 * @{
 * \file
 * LibHeimdall parser API
 */
typedef struct libheimdall_parser_t libheimdall_parser_t;

typedef struct libheimdall_media_t libheimdall_media_t;
typedef struct libheimdall_picture_list_t libheimdall_picture_list_t;
typedef struct libheimdall_picture_t libheimdall_picture_t;
typedef enum libheimdall_picture_type_t libheimdall_picture_type_t;

/**
 * A parser request object
 */
typedef struct libheimdall_parser_request_t libheimdall_parser_request_t;

/**
 * A thumbnailer request object
 */
typedef struct libheimdall_thumbnailer_request_t libheimdall_thumbnailer_request_t;

/**
 * Opaque handle of a parsing/thumbnailing task.
 *
 * Identifies a task created by libheimdall_parser_task_new_parse() or
 * libheimdall_parser_task_new_thumbnail() and started with libheimdall_parser_submit().
 * It can be passed to libheimdall_parser_cancel_request() to cancel that request.
 *
 * \note Validity starts when a libheimdall_parser_task_new*() function returns a
 * non-NULL handle and ends with libheimdall_parser_task_release().
 */
typedef struct libheimdall_parser_task libheimdall_parser_task;

/**
 * Parse flags used by libheimdall_parser_request_t
 */
typedef enum libheimdall_media_parse_flag_t
{
    /**
     * Parse media
     */
    libheimdall_media_parse = 0x01,
    /**
     * Fetch meta and cover art using local resources
     */
    libheimdall_media_fetch_local = 0x02,
    /**
     * Fetch meta and cover art using network resources
     */
    libheimdall_media_fetch_network = 0x04,
    /**
     * Interact with the user (via libheimdall_dialog_cbs) when preparsing this item
     * (and not its sub items). Set this flag in order to receive a callback
     * when the input is asking for credentials.
     */
    libheimdall_media_do_interact = 0x08,
} libheimdall_media_parse_flag_t;

/**
 * Outcome of a finished parse request, reported by
 * \ref libheimdall_parser_cbs.on_parsed
 *
 */
typedef enum libheimdall_parser_status_t
{
    /** The parsing failed */
    libheimdall_parser_status_failed,
    /** The parsing timed out */
    libheimdall_parser_status_timeout,
    /** The parsing was cancelled */
    libheimdall_parser_status_cancelled,
    /** The parsing completed successfully */
    libheimdall_parser_status_done,
} libheimdall_parser_status_t;

/**
 * Thumbnailer seek type
 */
typedef enum libheimdall_thumbnailer_seek_type_t
{
    /** Don't seek (default) */
    libheimdall_thumbnailer_seek_none,
    /** Seek by time */
    libheimdall_thumbnailer_seek_time,
    /** Seek by position */
    libheimdall_thumbnailer_seek_pos,
} libheimdall_thumbnailer_seek_type_t;

/**
 * Thumbnailer seek speed
 */
typedef enum libheimdall_thumbnailer_seek_speed_t
{
    /** Precise, but potentially slow */
    libheimdall_media_thumbnail_seek_precise,
    /** Fast, but potentially imprecise */
    libheimdall_media_thumbnail_seek_fast,
} libheimdall_thumbnailer_seek_speed_t;

/**
 * Thumbnailer seek value
 *
 * The active member is selected based on the associated
 * \ref libheimdall_thumbnailer_seek_type_t.
 */
typedef union libheimdall_thumbnailer_seek_value_t
{
    /** Seek time, only use if type == libheimdall_thumbnailer_seek_time */
    libheimdall_time_t time;
    /** Seek position, only use if type == libheimdall_thumbnailer_seek_pos */
    double pos;
} libheimdall_thumbnailer_seek_value_t;

/**
 * struct defining callbacks for libheimdall_parser_task_new_parse
 */
struct libheimdall_parser_cbs
{
    /** 
     * Version of struct libheimdall_parser_cbs
     */
    uint32_t version;

    /**
     * Callback prototype that notify when a parser request finishes
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set by cbs_opaque
     * \param task opaque handle returned by libheimdall_parser_task_new_parse()
     * \param status terminal parse outcome, \ref libheimdall_parser_status_t
     */
    void (*on_parsed)(void *opaque,
                      libheimdall_parser_task *task,
                      libheimdall_parser_status_t status);

    /**
     * Callback prototype that notify when the parser add new attachments to
     * the media.
     *
     * Called before on_parsed, if there are valid attachments.
     *
     * \note Optional (can be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set by cbs_opaque
     * \param task opaque handle returned by libheimdall_parser_task_new_parse()
     * \param list list of pictures, the list is only valid from this
     * callback, each pictures can be held separately with
     * libheimdall_picture_retain().
     */
    void (*on_attachments_added)(void *opaque,
                                 libheimdall_parser_task *task,
                                 libheimdall_picture_list_t *list);
};

/**
 * struct defining a parse request
 */
struct libheimdall_parser_request_t
{
    /**
     * Version of libheimdall_parser_request_t
     */
    uint32_t version;

    /**
     * Media to parse
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     */
    libheimdall_media_t *media;

    /**
     * Parse flags, a combination of \ref libheimdall_media_parse_flag_t
     * (default to libheimdall_media_parse if 0)
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    libheimdall_media_parse_flag_t parse_flags;
};

/**
 * struct defining callbacks for libheimdall_parser_task_new_thumbnail
 */
struct libheimdall_thumbnailer_cbs
{
    /**
     * Version of struct libheimdall_thumbnailer_cbs
     */
    uint32_t version;

    /**
     * Callback prototype that notify when a thumbnailer request finishes
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set in cbs_opaque
     * \param task opaque handle returned by libheimdall_parser_task_new_thumbnail()
     * \param picture generated thumbnail, the picture is only valid from this
     * callback, it can be held separately with libheimdall_picture_retain().
     * NULL in case of an error, timeout or request was cancelled.
     */
    void (*on_ended)( void *opaque, libheimdall_parser_task *task, libheimdall_picture_t *picture );
};

/**
 * struct defining a thumbnailer request
 */
struct libheimdall_thumbnailer_request_t
{
    /**
     * Version of libheimdall_thumbnailer_request_t
     */
    uint32_t version;

    /**
     * Media source of the thumbnail.
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     */
    libheimdall_media_t *media;

    /**
     * Thumbnail width (0 by default)
     * \note The resulting thumbnail size can either be:
     *
     * - Hardcoded by providing both width & height. In which case, the image will
     *   be stretched to match the provided aspect ratio, or cropped if crop is true.
     * 
     * - Derived from the media aspect ratio if only width or height is provided and
     *   the other one is set to 0.
     *
     * - Of the same size as the media if both width and height are set to 0.
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    unsigned int width;

    /**
     * Thumbnail height (0 by default)
     * \note The resulting thumbnail size can either be:
     *
     * - Hardcoded by providing both width & height. In which case, the image will
     *   be stretched to match the provided aspect ratio, or cropped if crop is true.
     *
     * - Derived from the media aspect ratio if only width or height is provided and
     *   the other one is set to 0.
     *
     * - Of the same size as the media if both width and height are set to 0.
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    unsigned int height;

    /**
     * True to enable crop (false by default)
     *
     * \note Only meaningful when both width and height are non-zero
     * (hardcoded thumbnail size); ignored otherwise.
     *
     * \note Optional (can be false),
     * available since version 0
     */
    bool crop;

    /**
     * Picture type (libheimdall_picture_Argb by default)
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    libheimdall_picture_type_t type;

    /**
     * Seek parameters
     *
     * \note Optional (can be zero-initialized, in which case the members
     * will be set to their default values),
     * available since version 0
     */
    struct
    {
        /**
         * by time or by position (libheimdall_thumbnailer_seek_none by default)
         */
        libheimdall_thumbnailer_seek_type_t type;

        /** Seek value, the active member is selected by type
         *
         * \note `time` by default, although it won't be used if type is libheimdall_thumbnailer_seek_none
         */
        libheimdall_thumbnailer_seek_value_t value;

        /** precise or fast mode (libheimdall_media_thumbnail_seek_precise by default) */
        libheimdall_thumbnailer_seek_speed_t speed;
    } seek;

    /**
     * True to enable hardware decoder (false by default)
     *
     * \note Optional (can be false),
     * available since version 0
     */
    bool hw_dec;
};

/**
 * struct defining parser configuration
 */
struct libheimdall_parser_cfg
{
    /**
     * Version of struct libheimdall_parser_cfg
     */
    uint32_t version;

    /**
     * The maximum number of threads used by the parser, 0 for default
     * (1 thread)
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    uint32_t max_parser_threads;

    /**
     * The maximum number of threads used by the thumbnailer, 0 for default
     * (1 thread)
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    uint32_t max_thumbnailer_threads;

    /**
     * Timeout of the parser in us, 0 for no limits, or -1 to inherit the value of preparse-timeout
     *
     * \note Optional (can be 0),
     * available since version 0
     */
    libheimdall_time_t timeout;
};

/**
 * Create a parser
 *
 * \param inst LibHeimdall instance to create parser with
 * \param cfg a pointer to a valid configuration struct
 * \return a new parser object, or NULL on error.
 * It must be released by libheimdall_parser_destroy().
 * \version libheimdall 4.0.0 or later
 */
LIBHEIMDALL_API libheimdall_parser_t *
libheimdall_parser_new(libheimdall_instance_t *inst,
                  const struct libheimdall_parser_cfg *cfg);

/**
 * Destroy a parser and free resources.
 * All pending and running tasks are cancelled, and reported as cancelled
 * via callback. This API waits for all worker threads to join.
 *
 * \note It is safe to call this API while tasks are still running or pending.
 *
 * \param parser the parser
 * \version libheimdall 4.0.0 or later
 */
LIBHEIMDALL_API void libheimdall_parser_destroy(libheimdall_parser_t *parser);

/**
 * Create a media parsing task
 *
 * This prepares a task handle to fetch (local or network) art, meta data and/or
 * tracks information. Nothing runs and no callback can fire until the handle is passed
 * to libheimdall_parser_submit().
 *
 * \note A media can be parsed multiple times, for instance to refresh network
 * metadata; a previous successful, failed, timed out or cancelled parse does
 * not prevent submitting a new task created from the same libheimdall_media_t,
 * but re-submitting a task handle that was already submitted
 * successfully is undefined behavior. \see libheimdall_parser_submit().
 *
 * \param parser the parser
 * \param req a pointer to a valid request struct
 * \param cbs a pointer to a valid callbacks struct. The pointed struct
 * must be kept alive (and not modified) by the caller until libheimdall_parser_cbs.on_parsed
 * is called for the returned task handle.
 * \param cbs_opaque an opaque pointer to be passed to the callbacks
 * \return NULL in case of error, or a task handle owned by the caller. It must
 * be released with libheimdall_parser_task_release(), whether or not it is
 * submitted.
 * \version LibHeimdall 4.0.0 or later
 */
LIBHEIMDALL_API libheimdall_parser_task *
libheimdall_parser_task_new_parse(libheimdall_parser_t *parser,
                             const libheimdall_parser_request_t *req,
                             const struct libheimdall_parser_cbs *cbs, void *cbs_opaque);

/**
 * Create a thumbnail generation task
 *
 * Nothing runs and no callback can fire until the task is passed to libheimdall_parser_submit().
 *
 * \param parser the parser
 * \param req a pointer to a valid request struct
 * \param cbs a pointer to a valid callbacks struct. The pointed struct
 * must be kept alive (and not modified) by the caller until libheimdall_thumbnailer_cbs.on_ended
 * is called for the returned task handle.
 * \param cbs_opaque an opaque pointer to be passed to the callbacks
 * \return NULL in case of error, or a task handle owned by the caller
 * \version LibHeimdall 4.0.0 or later
 */
LIBHEIMDALL_API libheimdall_parser_task *
libheimdall_parser_task_new_thumbnail(libheimdall_parser_t *parser,
                                 const libheimdall_thumbnailer_request_t *req,
                                 const struct libheimdall_thumbnailer_cbs *cbs,
                                 void *cbs_opaque);

/**
 * Start a task created by libheimdall_parser_task_new_parse() or
 * libheimdall_parser_task_new_thumbnail()
 *
 * On success the task is scheduled and its completion callback is
 * guaranteed to be called exactly once, including when the task is cancelled
 * with libheimdall_parser_cancel_request(). That callback may run even before this
 * function returns.
 *
 * \note On failure no callback is invoked. The caller keeps its reference and
 * may submit the task again.
 *
 * \note A task may be submitted at most once, and may only be re-submitted if
 * the previous attempt failed. Submitting does not transfer the caller's
 * reference. It keeps owning the handle and must release it.
 *
 * \param parser the parser the task was created from
 * \param task a task that has not been submitted yet
 * \return 0 on success, -1 on error
 * \version LibHeimdall 4.0.0 or later
 */
LIBHEIMDALL_API int
libheimdall_parser_submit(libheimdall_parser_t *parser, libheimdall_parser_task *task);

/**
 * Cancel a parser request
 *
 * \param parser the parser
 * \param task A parser task returned by libheimdall_parser_task_new_*()
 * or NULL to cancel all requests.
 * \return the number of requests cancelled
 *
 * \note
 * - When a task is cancelled, for parsing, the `on_parsed` callback will be triggered
 *   with libheimdall_parser_status_cancelled status, and for thumbnailing, the `on_ended`
 *   callback will be triggered with a NULL picture.
 *
 * - If the request is already in a terminated state (finished, cancelled, error, timeout),
 *   or it was never submitted, the call is a no-op and no callback will be invoked.
 * \version libheimdall 4.0.0 or later
 */
LIBHEIMDALL_API size_t libheimdall_parser_cancel_request(libheimdall_parser_t *parser,
                                               libheimdall_parser_task *task);

/**
 * Fetch the media associated with the task handle.
 *
 * \param task A parser task returned by libheimdall_parser_task_new_*()
 * \return libheimdall_media_t associated with the task
 *
 * \note The returned media is held by the task, it must not be
 * released by the caller.
 */
LIBHEIMDALL_API libheimdall_media_t *
libheimdall_parser_task_get_media(libheimdall_parser_task *task);

/**
 * Release a parser task handle.
 *
 * \param task the parser task handle returned by libheimdall_parser_task_new_*()
 *
 * \note
 * - The libheimdall_parser_task_new*() function call transfers the ownership of the task
 *   handle to the caller. It must be released whether or not it was submitted.
 *
 * - Mandatory to call to avoid memory leaks.
 *
 * - It is safe to call this API from within the on_parsed/on_ended callback.
 *
 * - The task handle should not be used after calling this function.
 *
 * - If called on an active request, it doesn't cancel the task,
 *   use libheimdall_parser_cancel_request() for that.
 */
LIBHEIMDALL_API void libheimdall_parser_task_release(libheimdall_parser_task *task);

/** @} */

# ifdef __cplusplus
}
# endif

#endif /* HEIMDALL_LIBHEIMDALL_PARSER_H */
