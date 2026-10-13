/*****************************************************************************
 * heimdall_decoder.h: decoder API
 *****************************************************************************
 * Copyright (C) 1999-2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef HEIMDALL_DECODER_H
#define HEIMDALL_DECODER_H 1

/**
 * \ingroup decoder
 * @{
 */

#include <heimdall_tick.h>

typedef struct heimdall_input_decoder_t heimdall_input_decoder_t;
typedef struct heimdall_spu_highlight_t heimdall_spu_highlight_t;

/**
 * This defines an opaque input resource handler.
 */
typedef struct input_resource_t input_resource_t;

/* */
struct heimdall_clock_t;
struct heimdall_frame_t;


/**
 * Spawn a decoder thread outside of the input thread.
 */
HEIMDALL_API heimdall_input_decoder_t *
heimdall_input_decoder_Create( heimdall_object_t *, const es_format_t *, const char *es_id,
                          struct heimdall_clock_t *, input_resource_t * ) HEIMDALL_USED;

/**
 * Delete an existing heimdall_input_decoder_t instance.
 *
 * Close the decoder implementation and delete the heimdall_input_decoder_t
 * instance.
 * The instance must have been drained using heimdall_input_decoder_Drain() or
 * flushed using heimdall_input_decoder_Flush() after any previous call to
 * heimdall_input_decoder_Decode() before calling the destructor.
 *
 * @param decoder The heimdall_input_decoder_t to delete, created from
 *        heimdall_input_decoder_Create().
 */
HEIMDALL_API void heimdall_input_decoder_Delete( heimdall_input_decoder_t * decoder);

/**
 * Put a heimdall_frame_t in the decoder's fifo.
 * Thread-safe w.r.t. the decoder. May be a cancellation point.
 *
 * @param p_dec the decoder object
 * @param frame the data frame
 * @param do_pace whether we wait for some decoding to happen or not
 */
HEIMDALL_API void heimdall_input_decoder_Decode( heimdall_input_decoder_t *p_dec, struct heimdall_frame_t *frame, bool do_pace );

/**
 * Signals that there are no further frames to decode, and requests that the
 * decoder drain all pending buffers. This is used to ensure that all
 * intermediate buffers empty and no samples get lost at the end of the stream.
 *
 * @note The function does not actually wait for draining. It just signals that
 * draining should be performed once the decoder has emptied FIFO.
 */
HEIMDALL_API void heimdall_input_decoder_Drain( heimdall_input_decoder_t * );

/**
 * Returns the drained state
 *
 * @warning This function need to be polled (every few ms) to know when the
 * decoder is drained
 * @return true if drained (after a call to heimdall_input_decoder_Drain())
 */
 HEIMDALL_API bool heimdall_input_decoder_IsDrained( heimdall_input_decoder_t * );

/**
 * Requests that the decoder immediately discard all pending buffers.
 * This is useful when seeking or when deselecting a stream.
 */
HEIMDALL_API void heimdall_input_decoder_Flush( heimdall_input_decoder_t * );
HEIMDALL_API int  heimdall_input_decoder_SetSpuHighlight( heimdall_input_decoder_t *, const heimdall_spu_highlight_t * );
HEIMDALL_API void heimdall_input_decoder_ChangeDelay( heimdall_input_decoder_t *, heimdall_tick_t i_delay );

/**
 * It creates an empty input resource handler.
 *
 * The given object MUST stay alive as long as the input_resource_t is
 * not deleted.
 */
HEIMDALL_API input_resource_t * input_resource_New( heimdall_object_t * ) HEIMDALL_USED;

/**
 * It releases an input resource.
 */
HEIMDALL_API void input_resource_Release( input_resource_t * );

/** @} */
#endif
