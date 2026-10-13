/*****************************************************************************
 * heimdall_stream.h: Stream (between access and demux) descriptor and methods
 *****************************************************************************
 * Copyright (C) 1999-2004 VLC authors and VideoLAN
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

#ifndef HEIMDALL_STREAM_H
#define HEIMDALL_STREAM_H 1

#include <heimdall_tick.h>
#include <heimdall_block.h>
#include <heimdall_input.h>

# ifdef __cplusplus
extern "C" {
# endif

/**
 * \defgroup stream Stream
 * \ingroup input
 * Buffered input byte streams
 * @{
 * \file
 * Byte streams and byte stream filter modules interface
 */

struct heimdall_stream_operations {
    /* Cannot fail */
    bool (*can_seek)(stream_t *);
    bool (*can_pause)(stream_t *);
    bool (*can_control_pace)(stream_t *);

    int (*get_signal)(stream_t *, double *, double *);
    int (*get_meta)(stream_t *, heimdall_meta_t *);
    int (*get_type)(stream_t *, int *);

    int (*set_pause_state)(stream_t *, bool);
    int (*set_seek_point)(stream_t *, int);
    int (*set_title)(stream_t *, int);

    void (*close)(stream_t *);

    union {
        struct {
            bool (*can_fastseek)(stream_t *);

            ssize_t (*read)(stream_t *, void *buf, size_t len);
            block_t *(*block)(stream_t *, bool *restrict eof);
            int (*readdir)(stream_t *, input_item_node_t *);
            int (*seek)(stream_t *, uint64_t);

            int (*get_title)(stream_t *, unsigned *);
            int (*get_seekpoint)(stream_t *, unsigned *);
            int (*get_size)(stream_t *, uint64_t *);
            int (*get_mtime)(stream_t *, uint64_t *);
            int (*get_title_info)(stream_t *, input_title_t ***, int *);
            int (*get_content_type)(stream_t *, char **);
            int (*get_tags)(stream_t *, const block_t **);
            int (*get_private_id_state)(stream_t *, int, bool *);
            heimdall_tick_t (*get_pts_delay)(stream_t *);

            int (*set_record_state)(stream_t *, bool, const char *, const char *);
            int (*set_private_id_state)(stream_t *, int, bool);
            int (*set_private_id_ca)(stream_t *, void *);
        } stream;
        struct {
            bool (*can_record)(demux_t *);
            bool (*can_control_rate)(demux_t *);

            int (*demux)(demux_t *);
            int (*readdir)(demux_t *, input_item_node_t *);

            bool (*has_unsupported_meta)(demux_t *);

            int (*get_title)(demux_t *, int *);
            int (*get_seekpoint)(demux_t *, int *);
            double (*get_position)(demux_t *);
            heimdall_tick_t (*get_length)(demux_t *);
            heimdall_tick_t (*get_time)(demux_t *);
            int (*get_normal_time)(demux_t *, heimdall_tick_t *);
            int (*get_title_info)(demux_t *, input_title_t ***, int *, int *, int *);
            int (*get_fps)(demux_t *, double *);
            int (*get_attachments)(demux_t *, input_attachment_t ***);
            int (*get_pts_delay)(stream_t *, heimdall_tick_t *);

            int (*set_position)(demux_t *, double, bool);
            int (*set_time)(demux_t *, heimdall_tick_t, bool);
            int (*set_next_demux_time)(demux_t *, heimdall_tick_t);
            int (*set_record_state)(demux_t *, bool, const char *);
            int (*set_rate)(demux_t *, float *);
            int (*set_group_default)(demux_t *);
            int (*set_group_all)(demux_t *);
            int (*set_group_list)(demux_t *, size_t, const int *);
            int (*set_es)(demux_t *, int);
            int (*set_es_list)(demux_t *, size_t, const int *);

            int (*nav_activate)(demux_t *);
            int (*nav_up)(demux_t *);
            int (*nav_down)(demux_t *);
            int (*nav_left)(demux_t *);
            int (*nav_right)(demux_t *);
            int (*nav_popup)(demux_t *);
            int (*nav_menu)(demux_t *);

            int (*filter_enable)(demux_t *);
            int (*filter_disable)(demux_t *);

            int (*test_and_clear_flags)(demux_t *, unsigned *);
        } demux;
    };
};

/**
 * stream_t definition
 */

struct stream_t
{
    struct heimdall_object_t obj;

    char        *psz_name;
    char        *psz_url; /**< Full URL or MRL (can be NULL) */
    const char  *psz_location; /**< Location (URL with the scheme stripped) */
    char        *psz_filepath; /**< Local file path (if applicable) */
    bool         b_preparsing; /**< True if this access is used to preparse */
    input_item_t *p_input_item;/**< Input item (can be NULL) */

    /**
     * Input stream
     *
     * Depending on the module capability:
     * - "stream filter" or "demux": input byte stream (not NULL)
     * - "access": a NULL pointer
     * - "demux_filter": upstream demuxer or demux filter
     */
    stream_t *s;

    /* es output */
    es_out_t    *out;   /* our p_es_out */

    /**
     * Read data.
     *
     * Callback to read data from the stream into a caller-supplied buffer.
     *
     * This is the legacy implementer, using \ref heimdall_stream_operations
     * should be preferred.
     *
     * This may be NULL if the stream is actually a directory rather than a
     * byte stream, or if \ref stream_t.pf_block is non-NULL.
     *
     * \param buf buffer to read data into
     * \param len buffer length (in bytes)
     *
     * \retval -1 no data available yet
     * \retval 0 end of stream (incl. fatal error)
     * \retval positive number of bytes read (no more than len)
     */
    ssize_t     (*pf_read)(stream_t *, void *buf, size_t len);

    /**
     * Read data block.
     *
     * Callback to read a block of data. The data is read into a block of
     * memory allocated by the stream. For some streams, data can be read more
     * efficiently in block of a certain size, and/or using a custom allocator
     * for buffers. In such case, this callback should be provided instead of
     * \ref stream_t.pf_read; otherwise, this should be NULL.
     *
     * This is the legacy implementer, using \ref heimdall_stream_operations
     * should be preferred.
     *
     * \param eof storage space for end-of-stream flag [OUT]
     * (*eof is always false when invoking pf_block(); pf_block() should set
     *  *eof to true if it detects the end of the stream)
     *
     * \return a data block,
     * NULL if no data available yet, on error and at end-of-stream
     */
    block_t    *(*pf_block)(stream_t *, bool *restrict eof);

    /**
     * Read directory.
     *
     * Callback to fill an item node from a directory
     * (see doc/browsing.txt for details).
     *
     * This is the legacy implementer, using \ref heimdall_stream_operations
     * should be preferred.
     *
     * NULL if the stream is not a directory.
     */
    int         (*pf_readdir)(stream_t *, input_item_node_t *);

    int         (*pf_demux)(stream_t *);

    /**
     * Seek.
     *
     * Callback to set the stream pointer (in bytes from start).
     *
     * This is the legacy implementer, using \ref heimdall_stream_operations
     * should be preferred.
     *
     * May be NULL if seeking is not supported.
     */
    int         (*pf_seek)(stream_t *, uint64_t);

    /**
     * Stream control.
     *
     * Legacy way of implementing callbacks.
     * \ref heimdall_stream_operations should be preferred.
     *
     * \see stream_query_e
     */
    int         (*pf_control)(stream_t *, int i_query, va_list);

    /**
     * Implementation of the Stream/Demux API.
     *
     * If NULL all operations will be redirected to \ref stream_t.pf_control.
     */
    const struct heimdall_stream_operations *ops;

    /**
     * Private data pointer
     */
    void *p_sys;
};

/**
 * Possible commands to send to heimdall_stream_Control() and heimdall_stream_vaControl()
 */
enum stream_query_e
{
    /* capabilities */
    STREAM_CAN_SEEK,                        /**< arg1=(bool *) res=cannot fail */
    STREAM_CAN_FASTSEEK,                    /**< arg1=(bool *) res=cannot fail */
    STREAM_CAN_PAUSE,                       /**< arg1=(bool *) res=cannot fail */
    STREAM_CAN_CONTROL_PACE,                /**< arg1=(bool *) res=cannot fail */
    STREAM_CAN_DOWNLOAD,                    /**< arg1=(bool *) res=can fail */
    /* */
    STREAM_GET_SIZE=6,                      /**< arg1=(uint64_t *) res=can fail */
    STREAM_GET_MTIME,                       /**< arg1=(uint64_t *) res=can fail
                                                 Returns the last modified time in seconds since epoch. */

    /* */
    STREAM_GET_PTS_DELAY = 0x101,           /**< arg1=(heimdall_tick_t *) res=cannot fail */
    STREAM_GET_TITLE_INFO,                  /**< arg1=(input_title_t***) arg2=(int*) res=can fail */
    STREAM_GET_TITLE,                       /**< arg1=(unsigned *) res=can fail */
    STREAM_GET_SEEKPOINT,                   /**< arg1=(unsigned *) res=can fail */
    STREAM_GET_META,                        /**< arg1=(heimdall_meta_t *) res=can fail */
    STREAM_GET_CONTENT_TYPE,                /**< arg1=(char **) res=can fail */
    STREAM_GET_SIGNAL,                      /**< arg1=(double *pf_quality), arg2=(double *pf_strength) res=can fail */
    STREAM_GET_TAGS,                        /**< arg1=(const block_t **) res=can fail */
    STREAM_GET_TYPE,                        /**< arg1=(int*) res=can fail */

    STREAM_SET_PAUSE_STATE = 0x200,         /**< arg1=(bool) res=can fail */
    STREAM_SET_TITLE,                       /**< arg1=(int) res=can fail */
    STREAM_SET_SEEKPOINT,                   /**< arg1=(int) res=can fail */

    /* XXX only data read through heimdall_stream_Read/Block will be recorded */
    STREAM_SET_RECORD_STATE,                /**< arg1=bool, arg2=const char *dir_path (if arg1 is true),
                                                 arg3=const char *psz_ext (if arg1 is true) res=can fail */

    STREAM_SET_PRIVATE_ID_STATE = 0x1000,   /**< arg1=(int i_private_data) arg2=(bool b_selected) res=can fail */
    STREAM_SET_PRIVATE_ID_CA,               /**< arg1=(void *) */
    STREAM_GET_PRIVATE_ID_STATE,            /**< arg1=(int i_private_data) arg2=(bool *) res=can fail */
};

/**
 * Reads data from a byte stream.
 *
 * This function always waits for the requested number of bytes, unless a fatal
 * error is encountered or the end-of-stream is reached first.
 *
 * If the buffer is NULL, data is skipped instead of read. This is effectively
 * a relative forward seek, but it works even on non-seekable streams.
 *
 * \param s the stream object to read from
 * \param buf start of buffer to read data into [OUT]
 * \param len number of bytes to read
 * \return the number of bytes read or a negative value on error.
 */
HEIMDALL_API ssize_t heimdall_stream_Read(stream_t *s, void *buf, size_t len) HEIMDALL_USED;

/**
 * Reads partial data from a byte stream.
 *
 * This function waits until some data is available for reading from the
 * stream, a fatal error is encountered or the end-of-stream is reached.
 *
 * Unlike heimdall_stream_Read(), this function does not wait for the full requested
 * bytes count. It can return a short count even before the end of the stream
 * and in the absence of any error.
 *
 * \param s the stream object to read from
 * \param buf start of buffer to read data into [OUT]
 * \param len buffer size (maximum number of bytes to read)
 * \return the number of bytes read, 0 on end of stream or -1 if no data available
 */
HEIMDALL_API ssize_t heimdall_stream_ReadPartial(stream_t *s, void *buf, size_t len)
HEIMDALL_USED;

/**
 * Peeks at data from a byte stream.
 *
 * This function buffers for the requested number of bytes, waiting if
 * necessary. Then it stores a pointer to the buffer. Unlike heimdall_stream_Read()
 * or heimdall_stream_Block(), this function does not modify the stream read offset.
 *
 * \note
 * The buffer remains valid until the next read/peek or seek operation on the
 * same stream. In case of error, the buffer address is undefined.
 *
 * \param s the stream object to peek from
 * \param bufp storage space for the buffer address [OUT]
 * \param len number of bytes to peek
 * \return the number of bytes actually available (shorter than requested if
 * the end-of-stream is reached), or a negative value on error.
 */
HEIMDALL_API ssize_t heimdall_stream_Peek(stream_t *s, const uint8_t **bufp, size_t len)
HEIMDALL_USED;

/**
 * Reads a data block from a byte stream.
 *
 * This function dequeues the next block of data from the byte stream. The
 * byte stream back-end decides on the size of the block; the caller cannot
 * make any assumption about it.
 *
 * The function might also return NULL spuriously - this does not necessarily
 * imply that the stream is ended nor that it has encountered a nonrecoverable
 * error.
 *
 * This function should be used instead of heimdall_stream_Read() or
 * heimdall_stream_Peek() when the caller can handle reads of any size.
 *
 * \return either a data block or NULL
 */
HEIMDALL_API block_t *heimdall_stream_ReadBlock(stream_t *) HEIMDALL_USED;

/**
 * Tells the current stream position.
 *
 * This function tells the current read offset (in bytes) from the start of
 * the start of the stream.
 * @note The read offset may be larger than the stream size, either because of
 * a seek past the end, or because the stream shrank asynchronously.
 *
 * @return the byte offset from the beginning of the stream (cannot fail)
 */
HEIMDALL_API uint64_t heimdall_stream_Tell(const stream_t *) HEIMDALL_USED;

/**
 * Checks for end of stream.
 *
 * Checks if the last attempt to reads data from the stream encountered the
 * end of stream before the attempt could be fully satisfied.
 * The value is initially false, and is reset to false by heimdall_stream_Seek().
 *
 * \note The function can return false even though the current stream position
 * is equal to the stream size. It will return true after the following attempt
 * to read more than zero bytes.
 *
 * \note It might be possible to read after the end of the stream.
 * It implies the size of the stream increased asynchronously in the mean time.
 * Streams of most types cannot trigger such a case,
 * but regular local files notably can.
 *
 * \note In principles, the stream size should match the stream offset when
 * the end-of-stream is reached. But that rule is not enforced; it is entirely
 * dependent on the underlying implementation of the stream.
 */
HEIMDALL_API bool heimdall_stream_Eof(const stream_t *) HEIMDALL_USED;

/**
 * Sets the current stream position.
 *
 * This function changes the read offset within a stream, if the stream
 * supports seeking. In case of error, the read offset is not changed.
 *
 * @note It is possible (but not useful) to seek past the end of a stream.
 *
 * \param s the stream object to seek from
 * @param offset byte offset from the beginning of the stream
 * @return zero on success, a negative value on error
 */
HEIMDALL_API int heimdall_stream_Seek(stream_t *s, uint64_t offset) HEIMDALL_USED;

HEIMDALL_API int heimdall_stream_vaControl(stream_t *s, int query, va_list args);

static inline int heimdall_stream_Control(stream_t *s, int query, ...)
{
    va_list ap;
    int ret;

    va_start(ap, query);
    ret = heimdall_stream_vaControl(s, query, ap);
    va_end(ap);
    return ret;
}

HEIMDALL_API block_t *heimdall_stream_Block(stream_t *s, size_t);
HEIMDALL_API char *heimdall_stream_ReadLine(stream_t *);

/**
 * Reads a directory.
 *
 * This function fills an input item node with any and all the items within
 * a directory. The behaviour is undefined if the stream is not a directory.
 *
 * \param s directory object to read from
 * \param node node to store the items into
 * \return HEIMDALL_SUCCESS on success
 */
HEIMDALL_API int heimdall_stream_ReadDir(stream_t *s, input_item_node_t *node);

/**
 * Closes a byte stream.
 * \param s byte stream to close
 */
HEIMDALL_API void heimdall_stream_Delete(stream_t *s);

HEIMDALL_API stream_t *heimdall_stream_CommonNew(heimdall_object_t *, void (*)(stream_t *));

HEIMDALL_USED static inline bool heimdall_stream_CanSeek(stream_t *s)
{
    bool can_seek = false;
    heimdall_stream_Control(s, STREAM_CAN_SEEK, &can_seek);
    return can_seek;
}

HEIMDALL_USED static inline bool heimdall_stream_CanFastSeek(stream_t *s)
{
    bool can_fast_seek = false;
    heimdall_stream_Control(s, STREAM_CAN_FASTSEEK, &can_fast_seek);
    return can_fast_seek;
}

HEIMDALL_USED static inline bool heimdall_stream_CanPause(stream_t *s)
{
    bool can_pause = false;
    heimdall_stream_Control(s, STREAM_CAN_PAUSE, &can_pause);
    return can_pause;
}

HEIMDALL_USED static inline bool heimdall_stream_CanPace(stream_t *s)
{
    bool can_control_pace = false;
    heimdall_stream_Control(s, STREAM_CAN_CONTROL_PACE, &can_control_pace);
    return can_control_pace;
}

HEIMDALL_USED static inline bool heimdall_stream_CanDownload(stream_t *s)
{
    bool can_download = false;
    heimdall_stream_Control(s, STREAM_CAN_DOWNLOAD, &can_download);
    return can_download;
}

HEIMDALL_USED static inline heimdall_tick_t heimdall_stream_GetPtsDelay(stream_t *s)
{
    heimdall_tick_t pts_delay;
    heimdall_stream_Control(s, STREAM_GET_PTS_DELAY, &pts_delay);
    return pts_delay;
}

HEIMDALL_USED static inline int heimdall_stream_GetSeekpoint(stream_t *s, unsigned *seekpoint)
{
    return heimdall_stream_Control(s, STREAM_GET_SEEKPOINT, seekpoint);
}

HEIMDALL_USED static inline int heimdall_stream_GetSignal(stream_t *s, double *quality, double *strength)
{
    return heimdall_stream_Control(s, STREAM_GET_SIGNAL, quality, strength);
}

HEIMDALL_USED static inline int heimdall_stream_GetTitle(stream_t *s, unsigned *title)
{
    return heimdall_stream_Control(s, STREAM_GET_TITLE, title);
}

HEIMDALL_USED static inline int heimdall_stream_GetMeta(stream_t *s, heimdall_meta_t *meta)
{
    return heimdall_stream_Control(s, STREAM_GET_META, meta);
}

HEIMDALL_USED static inline int heimdall_stream_GetType(stream_t *s, int *type)
{
    return heimdall_stream_Control(s, STREAM_GET_TYPE, type);
}

/**
 * Get the size of the stream.
 */
HEIMDALL_USED static inline int heimdall_stream_GetSize(stream_t *s, uint64_t *size)
{
    return heimdall_stream_Control(s, STREAM_GET_SIZE, size);
}

HEIMDALL_USED static inline int heimdall_stream_GetMTime(stream_t *s, uint64_t *mtime)
{
    return heimdall_stream_Control(s, STREAM_GET_MTIME, mtime);
}

HEIMDALL_USED static inline int heimdall_stream_GetTitleInfo(stream_t *s, input_title_t ***title_info, int *size)
{
    return heimdall_stream_Control(s, STREAM_GET_TITLE_INFO, title_info, size);
}

HEIMDALL_USED static inline int heimdall_stream_GetContentType(stream_t *s, char **content_type)
{
    return heimdall_stream_Control(s, STREAM_GET_CONTENT_TYPE, content_type);
}

HEIMDALL_USED static inline int heimdall_stream_GetTags(stream_t *s, const block_t **tags)
{
    return heimdall_stream_Control(s, STREAM_GET_TAGS, tags);
}

HEIMDALL_USED static inline int heimdall_stream_GetPrivateIdState(stream_t *s, int priv_id, bool *state)
{
    return heimdall_stream_Control(s, STREAM_GET_PRIVATE_ID_STATE, priv_id, state);
}

HEIMDALL_USED static inline int heimdall_stream_SetPauseState(stream_t *s, bool pause_state)
{
    return heimdall_stream_Control(s, STREAM_SET_PAUSE_STATE, pause_state);
}

HEIMDALL_USED static inline int heimdall_stream_SetSeekPoint(stream_t *s, int seekpoint)
{
    return heimdall_stream_Control(s, STREAM_SET_SEEKPOINT, seekpoint);
}

HEIMDALL_USED static inline int heimdall_stream_SetTitle(stream_t *s, int title)
{
    return heimdall_stream_Control(s, STREAM_SET_TITLE, title);
}

HEIMDALL_USED static inline int heimdall_stream_SetRecordState(stream_t *s, bool record_state, const char *dir_path, const char *ext)
{
    return heimdall_stream_Control(s, STREAM_SET_RECORD_STATE, record_state, dir_path, ext);
}

HEIMDALL_USED static inline int heimdall_stream_SetPrivateIdState(stream_t *s, int priv_id, bool state)
{
    return heimdall_stream_Control(s, STREAM_SET_PRIVATE_ID_STATE, priv_id, state);
}

/**
 * Set the private ID ca.
 *
 * The ca arg is of type `en50221_capmt_info_t`.
 */
HEIMDALL_USED static inline int heimdall_stream_SetPrivateIdCa(stream_t *s, void *ca)
{
    return heimdall_stream_Control(s, STREAM_SET_PRIVATE_ID_CA, ca);
}

static inline int64_t stream_Size( stream_t *s )
{
    uint64_t i_pos;

    if( heimdall_stream_GetSize( s, &i_pos ) )
        return 0;
    if( i_pos >> 62 )
        return (int64_t)1 << 62;
    return i_pos;
}

HEIMDALL_USED
static inline bool stream_HasExtension( stream_t *s, const char *extension )
{
    const char *name = (s->psz_filepath != NULL) ? s->psz_filepath
                                                 : s->psz_url;
    const char *ext = strrchr( name, '.' );
    return ext != NULL && !strcasecmp( ext, extension );
}

/**
 * Get the Content-Type of a stream, or NULL if unknown.
 * Result must be free()'d.
 */
static inline char *stream_ContentType( stream_t *s )
{
    char *res;
    if (heimdall_stream_GetContentType(s, &res))
        return NULL;
    return res;
}

/**
 * Get the mime-type of a stream
 *
 * \warning the returned resource is to be freed by the caller
 * \return the mime-type, or `NULL` if unknown
 **/
HEIMDALL_USED
static inline char *stream_MimeType( stream_t *s )
{
    char* mime_type = stream_ContentType( s );

    if( mime_type ) /* strip parameters */
        mime_type[strcspn( mime_type, " ;" )] = '\0';

    return mime_type;
}

/**
 * Checks for a MIME type.
 *
 * Checks if the stream has a specific MIME type.
 */
HEIMDALL_USED
static inline bool stream_IsMimeType(stream_t *s, const char *type)
{
    char *mime = stream_MimeType(s);
    if (mime == NULL)
        return false;

    bool ok = !strcasecmp(mime, type);
    free(mime);
    return ok;
}

/**
 * Create a stream from a memory buffer.
 *
 * \param obj parent Heimdall object
 * \param base start address of the memory buffer to read from
 * \param size size in bytes of the memory buffer
 * \param preserve if false, free(base) will be called when the stream is
 *                 destroyed; if true, the memory buffer is preserved
 */
HEIMDALL_API stream_t *heimdall_stream_MemoryNew(heimdall_object_t *obj, uint8_t *base,
                                       size_t size, bool preserve) HEIMDALL_USED;
#define heimdall_stream_MemoryNew(a, b, c, d) \
        heimdall_stream_MemoryNew(HEIMDALL_OBJECT(a), b, c, d)

/**
 * Create a stream_t reading from a URL.
 * You must delete it using heimdall_stream_Delete.
 */
HEIMDALL_API stream_t * heimdall_stream_NewURL(heimdall_object_t *obj, const char *url)
HEIMDALL_USED;
#define heimdall_stream_NewURL(a, b) heimdall_stream_NewURL(HEIMDALL_OBJECT(a), b)

/**
 * \defgroup stream_fifo FIFO stream
 * In-memory anonymous pipe
  @{
 */

typedef struct heimdall_stream_fifo heimdall_stream_fifo_t;

/**
 * Creates a FIFO stream.
 *
 * Creates a non-seekable byte stream object whose byte stream is generated
 * by another thread in the process. This is the LibHeimdall equivalent of an
 * anonymous pipe/FIFO.
 *
 * On the reader side, the normal stream functions are used,
 * e.g. heimdall_stream_Read() and heimdall_stream_Delete().
 *
 * The created stream object is automatically destroyed when both the reader
 * and the writer sides have been closed, with heimdall_stream_Delete() and
 * heimdall_stream_fifo_Close() respectively.
 *
 * \param parent parent Heimdall object for the stream
 * \param reader location to store read side stream pointer [OUT]
 * \return a FIFO stream object or NULL on memory error.
 */
HEIMDALL_API heimdall_stream_fifo_t *heimdall_stream_fifo_New(heimdall_object_t *parent,
                                               stream_t **reader);

/**
 * Writes a block to a FIFO stream.
 *
 * \param s FIFO stream created by heimdall_stream_fifo_New()
 * \param block data block to write to the stream
 * \return 0 on success. -1 if the reader end has already been closed
 * (errno is then set to EPIPE, and the block is deleted).
 *
 * \bug No congestion control is performed. If the reader end is not keeping
 * up with the writer end, buffers will accumulate in memory.
 */
HEIMDALL_API int heimdall_stream_fifo_Queue(heimdall_stream_fifo_t *s, block_t *block);

/**
 * Writes data to a FIFO stream.
 *
 * This is a convenience helper for heimdall_stream_fifo_Queue().
 * \param s FIFO stream created by heimdall_stream_fifo_New()
 * \param buf start address of data to write
 * \param len length of data to write in bytes
 * \return len on success, or -1 on error (errno is set accordingly)
 */
HEIMDALL_API ssize_t heimdall_stream_fifo_Write(heimdall_stream_fifo_t *s, const void *buf,
                                      size_t len);

/**
 * Terminates a FIFO stream.
 *
 * Marks the end of the FIFO stream and releases any underlying resources.
 * \param s FIFO stream created by heimdall_stream_fifo_New()
 */
HEIMDALL_API void heimdall_stream_fifo_Close(heimdall_stream_fifo_t *s);

/**
 * @}
 */

/**
 * Try to add a stream filter to an open stream.
 * @return New stream to use, or NULL if the filter could not be added.
 **/
HEIMDALL_API stream_t* heimdall_stream_FilterNew( stream_t *p_source, const char *psz_stream_filter );

/**
 * @}
 */

# ifdef __cplusplus
}
# endif

#endif
