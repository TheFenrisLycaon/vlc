/*****************************************************************************
 * heimdall_picture_fifo.h: picture fifo definitions
 *****************************************************************************
 * Copyright (C) 2009 VLC authors and VideoLAN
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

#ifndef HEIMDALL_PICTURE_FIFO_H
#define HEIMDALL_PICTURE_FIFO_H 1

/**
 * \file
 * This file defines picture fifo structures and functions in heimdall
 */

#include <heimdall_picture.h>

/**
 * Picture fifo handle
 *
 * It is thread safe (push/pop).
 */
typedef struct picture_fifo_t picture_fifo_t;

/**
 * It creates an empty picture_fifo_t.
 */
HEIMDALL_API picture_fifo_t * picture_fifo_New( void ) HEIMDALL_USED;

/**
 * It destroys a fifo created by picture_fifo_New.
 *
 * All pictures inside the fifo will be released by picture_Release.
 */
HEIMDALL_API void picture_fifo_Delete( picture_fifo_t * );

/**
 * Lock the picture_fifo
 *
 * All picture_fifo_t functions, except picture_fifo_Delete(), need to be
 * called with the lock held.
 */
HEIMDALL_API void picture_fifo_Lock( picture_fifo_t * );

/**
 * Unlock the picture_fifo
 */
HEIMDALL_API void picture_fifo_Unlock( picture_fifo_t * );

/**
 * It retrieves a picture_t from the fifo.
 *
 * If the fifo is empty, it return NULL without waiting.
 */
HEIMDALL_API picture_t * picture_fifo_Pop( picture_fifo_t * ) HEIMDALL_USED;

/**
 * It returns whether the fifo is empty or not.
 */
HEIMDALL_API bool picture_fifo_IsEmpty( picture_fifo_t * );

/**
 * It returns the number of pictures queued
 */
HEIMDALL_API size_t picture_fifo_GetCount( picture_fifo_t * );

/**
 * It saves a picture_t into the fifo.
 */
HEIMDALL_API void picture_fifo_Push( picture_fifo_t *, picture_t * );

/**
 * It release all picture inside the fifo that have a lower or equal date
 * if flush_before or higher or equal to if not flush_before than the given one.
 * Passing HEIMDALL_TICK_INVALID on the date releases all the pictures.
 *
 * All pictures inside the fifo will be released by picture_Release.
 */
HEIMDALL_API void picture_fifo_Flush( picture_fifo_t *, heimdall_tick_t date, bool flush_before );

#endif /* HEIMDALL_PICTURE_FIFO_H */

