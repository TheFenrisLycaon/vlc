/*****************************************************************************
 * heimdall_frame.h: frame management functions
 *****************************************************************************
 * Copyright (C) 2003 VLC authors and VideoLAN
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

#ifndef HEIMDALL_FRAME_H
#define HEIMDALL_FRAME_H 1

#include <heimdall_tick.h>
#include <heimdall_ancillary.h>


/**
 * \defgroup frame Frames
 * \ingroup input
 *
 * Frames of binary data.
 *
 * @ref heimdall_frame_t is a generic structure to represent a binary blob within Heimdall.
 * The primary goal of the structure is to avoid memory copying as data is
 * passed around. It is notably used between the \ref demux, the packetizer
 * (if present) and the \ref decoder, and for audio, between the \ref decoder,
 * the audio filters, and the \ref audio_output.
 *
 * @{
 * \file
 * Frames definition and functions
 */

#include <sys/types.h>  /* for ssize_t */

/****************************************************************************
 * frame:
 ****************************************************************************
 * - i_flags may not always be set (ie could be 0, even for a key frame
 *      it depends where you receive the buffer (before/after a packetizer
 *      and the demux/packetizer implementations.
 * - i_dts/i_pts could be HEIMDALL_TICK_INVALID, it means no pts/dts
 * - i_length: length in microsecond of the packet, can be null except in the
 *      sout where it is mandatory.
 *
 * - i_buffer number of valid data pointed by p_buffer
 *      you can freely decrease it but never increase it yourself
 *      (use heimdall_frame_Realloc)
 * - p_buffer: pointer over data. You should never overwrite it, you can
 *   only increment it to skip data, in others cases use heimdall_frame_Realloc
 *   (don't duplicate yourself in a bigger buffer, heimdall_frame_Realloc is
 *   optimised for preheader/postdata increase)
 ****************************************************************************/

typedef struct heimdall_frame_t heimdall_frame_t;

/** The content doesn't follow the last frame, possible some frames in between
 *  have been lost */
#define HEIMDALL_FRAME_FLAG_DISCONTINUITY 0x0001
/** Intra frame */
#define HEIMDALL_FRAME_FLAG_TYPE_I        0x0002
/** Inter frame with backward reference only */
#define HEIMDALL_FRAME_FLAG_TYPE_P        0x0004
/** Inter frame with backward and forward reference */
#define HEIMDALL_FRAME_FLAG_TYPE_B        0x0008
/** For inter frame when you don't know the real type */
#define HEIMDALL_FRAME_FLAG_TYPE_PB       0x0010
/** Warn that this frame is a header one */
#define HEIMDALL_FRAME_FLAG_HEADER        0x0020
/** This frame contains the last part of a sequence  */
#define HEIMDALL_FRAME_FLAG_END_OF_SEQUENCE 0x0040
/** This frame is scrambled */
#define HEIMDALL_FRAME_FLAG_SCRAMBLED     0x0100
/** This frame has to be decoded but not be displayed */
#define HEIMDALL_FRAME_FLAG_PREROLL       0x0200
/** This frame is corrupted and/or there is data loss  */
#define HEIMDALL_FRAME_FLAG_CORRUPTED     0x0400
/** This frame is last of its access unit */
#define HEIMDALL_FRAME_FLAG_AU_END        0x0800
/** This frame contains an interlaced picture with top field stored first */
#define HEIMDALL_FRAME_FLAG_TOP_FIELD_FIRST 0x1000
/** This frame contains an interlaced picture with bottom field stored first */
#define HEIMDALL_FRAME_FLAG_BOTTOM_FIELD_FIRST 0x2000
/** This frame contains a single field from interlaced picture. */
#define HEIMDALL_FRAME_FLAG_SINGLE_FIELD  0x4000

/** This frame contains an interlaced picture */
#define HEIMDALL_FRAME_FLAG_INTERLACED_MASK \
    (HEIMDALL_FRAME_FLAG_TOP_FIELD_FIRST|HEIMDALL_FRAME_FLAG_BOTTOM_FIELD_FIRST|HEIMDALL_FRAME_FLAG_SINGLE_FIELD)

#define HEIMDALL_FRAME_FLAG_TYPE_MASK \
    (HEIMDALL_FRAME_FLAG_TYPE_I|HEIMDALL_FRAME_FLAG_TYPE_P|HEIMDALL_FRAME_FLAG_TYPE_B|HEIMDALL_FRAME_FLAG_TYPE_PB)

/* These are for input core private usage only */
#define HEIMDALL_FRAME_FLAG_CORE_PRIVATE_MASK  0x00ff0000
#define HEIMDALL_FRAME_FLAG_CORE_PRIVATE_SHIFT 16

/* These are for module private usage only */
#define HEIMDALL_FRAME_FLAG_PRIVATE_MASK  0xff000000
#define HEIMDALL_FRAME_FLAG_PRIVATE_SHIFT 24

struct heimdall_frame_callbacks
{
    void (*free)(heimdall_frame_t *);
};

struct heimdall_frame_t
{
    heimdall_frame_t    *p_next;

    uint8_t    *p_buffer; /**< Payload start */
    size_t      i_buffer; /**< Payload length */
    uint8_t    *p_start; /**< Buffer start */
    size_t      i_size; /**< Buffer total size */

    uint32_t    i_flags;
    unsigned    i_nb_samples; /* Used for audio */

    heimdall_tick_t  i_pts;
    heimdall_tick_t  i_dts;
    heimdall_tick_t  i_length;

    heimdall_ancillary_array ancillaries;

    const struct heimdall_frame_callbacks *cbs;
};

/**
 * Initializes a custom frame.
 *
 * This function initialize a frame of timed data allocated by custom means.
 * This allows passing data without copying even if the data has been allocated
 * with unusual means or outside of LibHeimdall.
 *
 * Normally, frames are allocated and initialized by heimdall_frame_Alloc() instead.
 *
 * @param frame allocated frame structure to initialize
 * @param cbs structure of custom callbacks to handle the frame [IN]
 * @param base start address of the frame data
 * @param length byte length of the frame data
 *
 * @return @c frame (this function cannot fail)
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_Init(heimdall_frame_t *frame,
                                    const struct heimdall_frame_callbacks *cbs,
                                    void *base, size_t length);

/**
 * Creates a custom frame.
 *
 * This function initialize a frame of timed data allocated by custom means.
 * This allows passing data without copying even if the data has been allocated
 * with unusual means or outside of LibHeimdall.
 *
 * Normally, frames are allocated and initialized by heimdall_frame_Alloc() instead.
 *
 * @param cbs structure of custom callbacks to handle the frame [IN]
 * @param base start address of the frame data
 * @param length byte length of the frame data
 *
 * @return the created frame, or NULL on memory error.
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_New(const struct heimdall_frame_callbacks *cbs,
                                   void *base, size_t length);

/**
 * Allocates a frame.
 *
 * Creates a new frame with the requested size.
 * The frame must be released with heimdall_frame_Release().
 *
 * @param size size in bytes (possibly zero)
 * @return the created frame, or NULL on memory error.
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_Alloc(size_t size) HEIMDALL_USED HEIMDALL_MALLOC;

HEIMDALL_API heimdall_frame_t *heimdall_frame_TryRealloc(heimdall_frame_t *, ssize_t pre, size_t body) HEIMDALL_USED;

/**
 * Reallocates a frame.
 *
 * This function expands, shrinks or moves a data frame.
 * In many cases, this function can return without any memory allocation by
 * reusing spare buffer space. Otherwise, a new frame is created and data is
 * copied.
 *
 * @param frame the frame to realloc, which will be freed after the call to
 *        this function
 * @param pre count of bytes to prepend if positive,
 *            count of leading bytes to discard if negative
 * @param body new bytes size of the frame
 *
 * @return the reallocated frame on success, NULL on error.
 *
 * @note Skipping leading bytes can be achieved directly by subtracting from
 * heimdall_frame_t.i_buffer and adding heimdall_frame_t.p_buffer.
 * @note Discard trailing bytes can be achieved directly by subtracting from
 * heimdall_frame_t.i_buffer.
 * @note On error, the frame is discarded.
 * To avoid that, use heimdall_frame_TryRealloc() instead.
 */
HEIMDALL_API heimdall_frame_t *
heimdall_frame_Realloc(heimdall_frame_t *frame, ssize_t pre, size_t body) HEIMDALL_USED;

/**
 * Releases a frame.
 *
 * This function works for any @ref heimdall_frame_t frame, regardless of the way it was
 * allocated.
 *
 * @note
 * If the frame is in a chain, this function does <b>not</b> release any
 * subsequent frame in the chain. Use heimdall_frame_ChainRelease() for that purpose.
 *
 * @param frame frame to release (cannot be NULL)
 */
HEIMDALL_API void heimdall_frame_Release(heimdall_frame_t *frame);

/**
 * Merge two ancillary arrays
 *
 * @param frame the frame that hold the destination ancillary array
 * @param src_array pointer to an ancillary array
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
static inline int
heimdall_frame_MergeAncillaries(heimdall_frame_t *frame,
                           const heimdall_ancillary_array *src_array)
{
    return heimdall_ancillary_array_Merge(&frame->ancillaries, src_array);
}

/**
 * Merge and clear two ancillary arrays
 *
 * @param frame the frame that hold the destination ancillary array
 * @param src_array pointer to the source ancillary array, will point to empty
 * data after this call.
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
static inline int
heimdall_frame_MergeAndClearAncillaries(heimdall_frame_t *frame,
                                   heimdall_ancillary_array *src_array)
{
    return heimdall_ancillary_array_MergeAndClear(&frame->ancillaries, src_array);
}

/**
 * Attach an ancillary to the frame
 *
 * @warning the ancillary will be released only if the frame is allocated from
 * a heimdall_frame Alloc function (heimdall_frame_Alloc(), heimdall_frame_mmap_Alloc()...).
 *
 * @note Several ancillaries can be attached to a frame, but if two ancillaries
 * are identified by the same ID, only the last one take precedence.
 *
 * @param frame the frame to attach an ancillary
 * @param ancillary ancillary that will be held by the frame, can't be NULL
 * @return HEIMDALL_SUCCESS in case of success, HEIMDALL_ENOMEM in case of alloc error
 */
static inline int
heimdall_frame_AttachAncillary(heimdall_frame_t *frame, struct heimdall_ancillary *ancillary)
{
    return heimdall_ancillary_array_Insert(&frame->ancillaries, ancillary);
}

/**
 * Return the ancillary identified by an ID
 *
 * @param frame the frame to read the ancillary from
 * @param id id of ancillary to request
 * @return the ancillary or NULL if the ancillary for that particular id is
 * not present
 */
static inline struct heimdall_ancillary *
heimdall_frame_GetAncillary(heimdall_frame_t *frame, heimdall_ancillary_id id)
{
    return heimdall_ancillary_array_Get(&frame->ancillaries, id);
}

/**
 * Copy frame properties from src to dst
 *
 * Copy i_flags, i_nb_samples, i_dts, i_pts, and i_length.
 *
 * @note if src has an ancillary, the ancillary will be copied and refcounted
 * to dst.
 *
 * @param dst the frame to copy properties into
 * @param src the frame to copy properties from
 */
HEIMDALL_API void heimdall_frame_CopyProperties( heimdall_frame_t *dst, const heimdall_frame_t *src );

/**
 * Duplicates a frame.
 *
 * Creates a writeable duplicate of a frame.
 *
 * @return the duplicate on success, NULL on error.
 */
HEIMDALL_USED
static inline heimdall_frame_t *heimdall_frame_Duplicate( const heimdall_frame_t *frame )
{
    heimdall_frame_t *p_dup = heimdall_frame_Alloc( frame->i_buffer );
    if( p_dup == NULL )
        return NULL;

    heimdall_frame_CopyProperties( p_dup, frame );
    memcpy( p_dup->p_buffer, frame->p_buffer, frame->i_buffer );

    return p_dup;
}

/**
 * Wraps heap in a frame.
 *
 * Creates a @ref heimdall_frame_t out of an existing heap allocation.
 * This is provided by LibHeimdall so that manually heap-allocated frames can safely
 * be deallocated even after the origin plugin has been unloaded from memory.
 *
 * When heimdall_frame_Release() is called, Heimdall will free() the specified pointer.
 *
 * @param addr base address of the heap allocation (will be free()'d)
 * @param length bytes length of the heap allocation
 * @return NULL in case of error (ptr free()'d in that case), or a valid
 * heimdall_frame_t pointer.
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_heap_Alloc(void *addr, size_t length) HEIMDALL_USED HEIMDALL_MALLOC;

/**
 * Wraps a memory mapping in a frame
 *
 * Creates a @ref heimdall_frame_t from a virtual address memory mapping (mmap).
 * This is provided by LibHeimdall so that mmap frames can safely be deallocated
 * even after the allocating plugin has been unloaded from memory.
 *
 * @param addr base address of the mapping (as returned by mmap)
 * @param length length (bytes) of the mapping (as passed to mmap)
 * @return NULL if addr is MAP_FAILED, or an error occurred (in the later
 * case, munmap(addr, length) is invoked before returning).
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_mmap_Alloc(void *addr, size_t length) HEIMDALL_USED HEIMDALL_MALLOC;

/**
 * Wraps a System V memory segment in a frame
 *
 * Creates a @ref heimdall_frame_t from a System V shared memory segment (shmget()).
 * This is provided by LibHeimdall so that segments can safely be deallocated
 * even after the allocating plugin has been unloaded from memory.
 *
 * @param addr base address of the segment (as returned by shmat())
 * @param length length (bytes) of the segment (as passed to shmget())
 * @return NULL if an error occurred (in that case, shmdt(addr) is invoked
 * before returning NULL).
 */
HEIMDALL_API heimdall_frame_t * heimdall_frame_shm_Alloc(void *addr, size_t length) HEIMDALL_USED HEIMDALL_MALLOC;

/**
 * Maps a file handle in memory.
 *
 * Loads a file into a frame of memory through a file descriptor.
 * If possible a private file mapping is created. Otherwise, the file is read
 * normally. This function is a cancellation point.
 *
 * @note On 32-bits platforms,
 * this function will not work for very large files,
 * due to memory space constraints.
 *
 * @param fd file descriptor to load from
 * @param write If true, request a read/write private mapping.
 *              If false, request a read-only potentially shared mapping.
 *
 * @return a new frame with the file content at p_buffer, and file length at
 * i_buffer (release it with heimdall_frame_Release()), or NULL upon error (see errno).
 */
HEIMDALL_API heimdall_frame_t *heimdall_frame_File(int fd, bool write) HEIMDALL_USED HEIMDALL_MALLOC;

/**
 * Maps a file in memory.
 *
 * Loads a file into a frame of memory from a path to the file.
 * See also heimdall_frame_File().
 *
 * @param path the file path to load the memory block from
 * @param write If true, request a read/write private mapping.
 *              If false, request a read-only potentially shared mapping.
 */
HEIMDALL_API heimdall_frame_t *
heimdall_frame_FilePath(const char *path, bool write)
HEIMDALL_USED HEIMDALL_MALLOC;

static inline void heimdall_frame_Cleanup (void *frame)
{
    heimdall_frame_Release ((heimdall_frame_t *)frame);
}
#define heimdall_frame_cleanup_push( frame ) heimdall_cleanup_push (heimdall_frame_Cleanup, frame)

/**
 * \defgroup heimdall_frame_chain Frame chain
 * @{
 */

/**
 * Appends a @ref heimdall_frame_t to the chain
 *
 * The given frame is appended to the last frame of the given chain.
 *
 * @attention
 *  Using this function on long chains or repeatedly calling it
 *  to append a lot of frames can be slow, as it has to iterate the
 *  whole chain to append the frame.
 *  In these cases @ref heimdall_frame_ChainLastAppend should be used.
 *
 * @param pp_list   Pointer to the heimdall_frame_t chain
 * @param frame   The heimdall_frame_t to append (can be NULL)
 *
 * @see heimdall_frame_ChainLastAppend()
 *
 * Example:
 * @code{.c}
 * heimdall_frame_t *p_chain = NULL;
 *
 * heimdall_frame_ChainAppend(&p_chain, p_frame);
 * @endcode
 */
static inline void heimdall_frame_ChainAppend( heimdall_frame_t **pp_list, heimdall_frame_t *frame )
{
    if( *pp_list == NULL )
    {
        *pp_list = frame;
    }
    else
    {
        heimdall_frame_t *p = *pp_list;

        while( p->p_next ) p = p->p_next;
        p->p_next = frame;
    }
}

/**
 * Appends a @ref heimdall_frame_t to the last frame pointer and update it
 *
 * Uses a pointer over a pointer to p_next of the last frame of the frame chain
 * to append a frame at the end of the chain and updates the pointer to the new
 * last frame's @c p_next. If the appended frame is itself a chain, it is iterated
 * till the end to correctly update @c ppp_last.
 *
 * @param[in,out] ppp_last  Pointer to pointer to the end of the chain
 *                          (The heimdall_frame_t::p_next of the last heimdall_frame_t in the chain)
 * @param         frame   The heimdall_frame_t to append
 *
 * Example:
 * @code{.c}
 * heimdall_frame_t *p_frame = NULL;
 * heimdall_frame_t **pp_frame_last = &p_frame;
 *
 * heimdall_frame_ChainLastAppend(&pp_frame_last, p_other_frame);
 * @endcode
 */
static inline void heimdall_frame_ChainLastAppend( heimdall_frame_t ***ppp_last, heimdall_frame_t *frame )
{
    heimdall_frame_t *p_last = frame;

    **ppp_last = frame;

    while( p_last->p_next ) p_last = p_last->p_next;
    *ppp_last = &p_last->p_next;
}

/**
 * Releases a chain of blocks
 *
 * The frame pointed to by frame and all following frames in the
 * chain are released.
 *
 * @param frame   Pointer to first heimdall_frame_t of the chain to release
 *
 * @see heimdall_frame_Release()
 */
static inline void heimdall_frame_ChainRelease( heimdall_frame_t *frame )
{
    while( frame )
    {
        heimdall_frame_t *p_next = frame->p_next;
        heimdall_frame_Release( frame );
        frame = p_next;
    }
}

/**
 * Extracts data from a chain of frames
 *
 * Copies the specified amount of data from the chain into the given buffer.
 * If the data in the chain is less than the maximum amount given, the remainder
 * of the buffer is not modified.
 *
 * @param      p_list   Pointer to the first heimdall_frame_t of the chain to copy from
 * @param[out] p_data   Destination buffer to copy the data to
 * @param      i_max    Number of bytes to copy
 * @return              Number of bytes actually copied
 *
 * @see heimdall_frame_ChainGather()
 */
static size_t heimdall_frame_ChainExtract( heimdall_frame_t *p_list, void *p_data, size_t i_max )
{
    size_t  i_total = 0;
    uint8_t *p = (uint8_t*)p_data;

    while( p_list && i_max )
    {
        size_t i_copy = __MIN( i_max, p_list->i_buffer );
        memcpy( p, p_list->p_buffer, i_copy );
        i_max   -= i_copy;
        i_total += i_copy;
        p       += i_copy;

        p_list = p_list->p_next;
    }
    return i_total;
}

/**
 * Retrieves chain properties
 *
 * Can be used to retrieve count of frames, number of bytes and the duration
 * of the chain.
 *
 * @param       p_list      Pointer to the first heimdall_frame_t of the chain
 * @param[out]  pi_count    Pointer to count of frames in the chain (may be NULL)
 * @param[out]  pi_size     Pointer to number of bytes in the chain (may be NULL)
 * @param[out]  pi_length   Pointer to length (duration) of the chain (may be NULL)
 */
static inline void heimdall_frame_ChainProperties( const heimdall_frame_t *p_list, int *pi_count, size_t *pi_size, heimdall_tick_t *pi_length )
{
    size_t i_size = 0;
    heimdall_tick_t i_length = 0;
    int i_count = 0;

    while( p_list )
    {
        i_size += p_list->i_buffer;
        i_length += p_list->i_length;
        i_count++;

        p_list = p_list->p_next;
    }

    if( pi_size )
        *pi_size = i_size;
    if( pi_length )
        *pi_length = i_length;
    if( pi_count )
        *pi_count = i_count;
}

/**
 * Gathers a chain into a single heimdall_frame_t
 *
 * All frames in the chain are gathered into a single heimdall_frame_t and the
 * original chain is released.
 *
 * @param   p_list  Pointer to the first heimdall_frame_t of the chain to gather
 * @return  Returns a pointer to a new heimdall_frame_t or NULL if the frame can not
 *          be allocated, in which case the original chain is not released.
 *          If the chain pointed to by p_list is already gathered, a pointer
 *          to it is returned and no new frame will be allocated.
 *
 * @see heimdall_frame_ChainExtract()
 */
static inline heimdall_frame_t *heimdall_frame_ChainGather( heimdall_frame_t *p_list )
{
    size_t  i_total = 0;
    heimdall_tick_t i_length = 0;
    heimdall_frame_t *g;

    if( p_list->p_next == NULL )
        return p_list;  /* Already gathered */

    heimdall_frame_ChainProperties( p_list, NULL, &i_total, &i_length );

    g = heimdall_frame_Alloc( i_total );
    if( !g )
        return NULL;
    heimdall_frame_ChainExtract( p_list, g->p_buffer, g->i_buffer );

    g->i_flags = p_list->i_flags;
    g->i_pts   = p_list->i_pts;
    g->i_dts   = p_list->i_dts;
    g->i_length = i_length;

    /* free p_list */
    heimdall_frame_ChainRelease( p_list );
    return g;
}

/**
 * @}
 * \defgroup block_fifo Block FIFO
 * Thread-safe block queue functions
 * @{
 */

#include <heimdall_queue.h>

/**
 * Creates a thread-safe FIFO queue of blocks.
 *
 * See also heimdall_fifo_Put() and heimdall_fifo_Get().
 * The created queue must be deleted with heimdall_fifo_Delete().
 *
 * @return the FIFO or NULL on memory error
 */
HEIMDALL_API heimdall_fifo_t *heimdall_fifo_New(void) HEIMDALL_USED HEIMDALL_MALLOC;

/**
 * Delete a FIFO created by heimdall_fifo_New().
 *
 * @note Any queued blocks are also deleted.
 * @warning No other threads may be using the FIFO when this function is
 * called. Otherwise, undefined behaviour will occur.
 */
HEIMDALL_API void heimdall_fifo_Delete(heimdall_fifo_t *);

/**
 * Dequeue the first block from the FIFO. If necessary, wait until there is
 * one block in the queue. This function is (always) cancellation point.
 *
 * @return a valid block
 */
HEIMDALL_API heimdall_frame_t *heimdall_fifo_Get(heimdall_fifo_t *) HEIMDALL_USED;

/**
 * Peeks the first block in the FIFO.
 *
 * @warning This function leaves the block in the FIFO.
 * You need to protect against concurrent threads who could dequeue the block.
 * Preferably, there should be only one thread reading from the FIFO.
 *
 * @warning This function is undefined if the FIFO is empty.
 *
 * @return a valid block.
 */
HEIMDALL_API heimdall_frame_t *heimdall_fifo_Show(heimdall_fifo_t *);

static inline heimdall_queue_t *heimdall_fifo_queue(const heimdall_fifo_t *fifo)
{
    return (heimdall_queue_t *)fifo;
}

/**
 * Locks a block FIFO.
 *
 * No more than one thread can lock the FIFO at any given
 * time, and no other thread can modify the FIFO while it is locked.
 * heimdall_fifo_Unlock() releases the lock.
 *
 * @note If the FIFO is already locked by another thread, this function waits.
 * This function is not a cancellation point.
 *
 * @warning Recursively locking a single FIFO is undefined. Locking more than
 * one FIFO at a time may lead to lock inversion; mind the locking order.
 */
static inline void heimdall_fifo_Lock(heimdall_fifo_t *fifo)
{
    heimdall_queue_Lock(heimdall_fifo_queue(fifo));
}

/**
 * Unlocks a block FIFO.
 *
 * The calling thread must have locked the FIFO previously with
 * heimdall_fifo_Lock(). Otherwise, the behaviour is undefined.
 *
 * @note This function is not a cancellation point.
 */
static inline void heimdall_fifo_Unlock(heimdall_fifo_t *fifo)
{
    heimdall_queue_Unlock(heimdall_fifo_queue(fifo));
}

/**
 * Wakes up one thread waiting on the FIFO, if any.
 *
 * @note This function is not a cancellation point.
 *
 * @warning For race-free operations, the FIFO should be locked by the calling
 * thread. The function can be called on a unlocked FIFO however.
 */
static inline void heimdall_fifo_Signal(heimdall_fifo_t *fifo)
{
    heimdall_queue_Signal(heimdall_fifo_queue(fifo));
}

/**
 * Waits on the FIFO.
 *
 * Atomically unlocks the FIFO and waits until one thread signals the FIFO,
 * then locks the FIFO again. A signal can be sent by queueing a block to the
 * previously empty FIFO or by calling heimdall_fifo_Signal() directly.
 * This function may also return spuriously at any moment.
 *
 * @note This function is a cancellation point. In case of cancellation, the
 * the FIFO will be locked before cancellation cleanup handlers are processed.
 */
static inline void heimdall_fifo_Wait(heimdall_fifo_t *fifo)
{
    heimdall_queue_Wait(heimdall_fifo_queue(fifo));
}

static inline void heimdall_fifo_WaitCond(heimdall_fifo_t *fifo, heimdall_cond_t *condvar)
{
    heimdall_queue_t *q = heimdall_fifo_queue(fifo);

    heimdall_cond_wait(condvar, &q->lock);
}

/**
 * Queues a linked-list of blocks into a locked FIFO.
 *
 * @param fifo a fifo object locked with ::heimdall_fifo_Lock()
 * @param block the head of the list of blocks
 *              (if NULL, this function has no effects)
 *
 * @note This function is not a cancellation point.
 *
 * @warning The FIFO must be locked by the calling thread using
 * heimdall_fifo_Lock(). Otherwise behaviour is undefined.
 */
HEIMDALL_API void heimdall_fifo_QueueUnlocked(heimdall_fifo_t *fifo, heimdall_frame_t *block);

/**
 * Dequeues the first block from a locked FIFO, if any.
 *
 * @note This function is not a cancellation point.
 *
 * @warning The FIFO must be locked by the calling thread using
 * heimdall_fifo_Lock(). Otherwise behaviour is undefined.
 *
 * @return the first block in the FIFO or NULL if the FIFO is empty
 */
HEIMDALL_API heimdall_frame_t *heimdall_fifo_DequeueUnlocked(heimdall_fifo_t *) HEIMDALL_USED;

/**
 * Dequeues the all blocks from a locked FIFO.
 *
 * This is equivalent to calling heimdall_fifo_DequeueUnlocked() repeatedly until
 * the FIFO is emptied, but this function is much faster.
 *
 * @note This function is not a cancellation point.
 *
 * @warning The FIFO must be locked by the calling thread using
 * heimdall_fifo_Lock(). Otherwise behaviour is undefined.
 *
 * @return a linked-list of all blocks in the FIFO (possibly NULL)
 */
HEIMDALL_API heimdall_frame_t *heimdall_fifo_DequeueAllUnlocked(heimdall_fifo_t *) HEIMDALL_USED;

/**
 * Counts blocks in a FIFO.
 *
 * Checks how many blocks are queued in a locked FIFO.
 *
 * @note This function is not cancellation point.
 *
 * @warning The FIFO must be locked by the calling thread using
 * heimdall_fifo_Lock(). Otherwise behaviour is undefined.
 *
 * @return the number of blocks in the FIFO (zero if it is empty)
 */
HEIMDALL_API size_t heimdall_fifo_GetCount(const heimdall_fifo_t *) HEIMDALL_USED;

/**
 * Counts bytes in a FIFO.
 *
 * Checks how many bytes are queued in a locked FIFO.
 *
 * @note This function is not cancellation point.
 *
 * @warning The FIFO must be locked by the calling thread using
 * heimdall_fifo_Lock(). Otherwise behaviour is undefined.
 *
 * @return the total number of bytes
 *
 * @note Zero bytes does not necessarily mean that the FIFO is empty since
 * a block could contain zero bytes. Use heimdall_fifo_GetCount() to determine if
 * a FIFO is empty.
 */
HEIMDALL_API size_t heimdall_fifo_GetBytes(const heimdall_fifo_t *) HEIMDALL_USED;

/**
 * Checks whether the heimdall_fifo_t object is being locked.
 *
 * This function checks if the calling thread holds a given heimdall_fifo_t
 * object. It has no side effects and is essentially intended for run-time
 * debugging.
 *
 * @note This function is the heimdall_fifo_t equivalent of heimdall_mutex_held.
 *
 * @note To assert that the calling thread holds a lock, the helper macro
 * heimdall_fifo_Assert() should be used instead of this function.
 *
 * @retval false the fifo is not locked by the calling thread
 * @retval true the fifo is locked by the calling thread
 */
HEIMDALL_API bool heimdall_fifo_Held(const heimdall_fifo_t *fifo) HEIMDALL_USED;

/**
 * Asserts that a heimdall_fifo_t is locked by the calling thread.
 */
#define heimdall_fifo_Assert(fifo) assert(heimdall_fifo_Held(fifo))

HEIMDALL_USED static inline bool heimdall_fifo_IsEmpty(const heimdall_fifo_t *fifo)
{
    return heimdall_queue_IsEmpty(heimdall_fifo_queue(fifo));
}

static inline void heimdall_fifo_Cleanup(void *fifo)
{
    heimdall_fifo_Unlock((heimdall_fifo_t *)fifo);
}
#define heimdall_fifo_CleanupPush(fifo) heimdall_cleanup_push(heimdall_fifo_Cleanup, fifo)

/**
 * Clears all blocks in a FIFO.
 */
static inline void heimdall_fifo_Empty(heimdall_fifo_t *fifo)
{
    heimdall_frame_t *block;

    heimdall_fifo_Lock(fifo);
    block = heimdall_fifo_DequeueAllUnlocked(fifo);
    heimdall_fifo_Unlock(fifo);
    heimdall_frame_ChainRelease(block);
}

/**
 * Immediately queue one block at the end of a FIFO.
 *
 * @param fifo queue
 * @param block head of a block list to queue (may be NULL)
 */
static inline void heimdall_fifo_Put(heimdall_fifo_t *fifo, heimdall_frame_t *block)
{
    heimdall_fifo_Lock(fifo);
    heimdall_fifo_QueueUnlocked(fifo, block);
    heimdall_fifo_Unlock(fifo);
}

/* FIXME: not (really) thread-safe */
HEIMDALL_USED HEIMDALL_DEPRECATED
static inline size_t heimdall_fifo_Size (heimdall_fifo_t *fifo)
{
    size_t size;

    heimdall_fifo_Lock(fifo);
    size = heimdall_fifo_GetBytes(fifo);
    heimdall_fifo_Unlock(fifo);
    return size;
}

/* FIXME: not (really) thread-safe */
HEIMDALL_USED HEIMDALL_DEPRECATED
static inline size_t heimdall_fifo_Count (heimdall_fifo_t *fifo)
{
    size_t depth;

    heimdall_fifo_Lock(fifo);
    depth = heimdall_fifo_GetCount(fifo);
    heimdall_fifo_Unlock(fifo);
    return depth;
}

/** @} */

/** @} */

#endif /* HEIMDALL_FRAME_H */
