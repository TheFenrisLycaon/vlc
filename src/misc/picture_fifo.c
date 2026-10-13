/*****************************************************************************
 * picture_fifo.c : picture fifo functions
 *****************************************************************************
 * Copyright (C) 2009 VLC authors and VideoLAN
 * Copyright (C) 2009 Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_picture_fifo.h>

/*****************************************************************************
 *
 *****************************************************************************/
struct picture_fifo_t {
    heimdall_mutex_t lock;
    heimdall_picture_chain_t pics;
    size_t count;
};

static void PictureFifoReset(picture_fifo_t *fifo)
{
    heimdall_picture_chain_Init( &fifo->pics );
    fifo->count = 0;
}
static void PictureFifoPush(picture_fifo_t *fifo, picture_t *picture)
{
    assert(!picture_HasChainedPics(picture));
    heimdall_picture_chain_Append( &fifo->pics, picture );
    fifo->count++;
}

picture_fifo_t *picture_fifo_New(void)
{
    picture_fifo_t *fifo = malloc(sizeof(*fifo));
    if (!fifo)
        return NULL;

    heimdall_mutex_init(&fifo->lock);
    PictureFifoReset(fifo);
    return fifo;
}

void picture_fifo_Push(picture_fifo_t *fifo, picture_t *picture)
{
    heimdall_mutex_assert(&fifo->lock);
    PictureFifoPush(fifo, picture);
}
picture_t *picture_fifo_Pop(picture_fifo_t *fifo)
{
    heimdall_mutex_assert(&fifo->lock);
    picture_t *pic = heimdall_picture_chain_PopFront( &fifo->pics );
    if (pic != NULL)
        fifo->count--;
    return pic;
}
bool picture_fifo_IsEmpty(picture_fifo_t *fifo)
{
    heimdall_mutex_assert(&fifo->lock);
    return heimdall_picture_chain_IsEmpty( &fifo->pics );
}
HEIMDALL_API size_t picture_fifo_GetCount(picture_fifo_t *fifo)
{
    heimdall_mutex_assert(&fifo->lock);
    return fifo->count;
}
void picture_fifo_Flush(picture_fifo_t *fifo, heimdall_tick_t date, bool flush_before)
{
    heimdall_mutex_assert(&fifo->lock);
    picture_t *picture;

    heimdall_picture_chain_t flush_chain;

    heimdall_picture_chain_Init(&flush_chain);
    fifo->count = 0;

    if (date == HEIMDALL_TICK_INVALID)
        heimdall_picture_chain_GetAndClear(&fifo->pics, &flush_chain);
    else {
        heimdall_picture_chain_t filter_chain;
        heimdall_picture_chain_GetAndClear(&fifo->pics, &filter_chain);

        while ( !heimdall_picture_chain_IsEmpty( &filter_chain ) ) {
            picture = heimdall_picture_chain_PopFront( &filter_chain );

            if (( flush_before && picture->date <= date) ||
                (!flush_before && picture->date >= date))
                heimdall_picture_chain_Append( &flush_chain, picture );
            else
                PictureFifoPush(fifo, picture);
        }
    }

    while ((picture = heimdall_picture_chain_PopFront(&flush_chain)) != NULL)
        picture_Release(picture);
}
void picture_fifo_Delete(picture_fifo_t *fifo)
{
    heimdall_mutex_lock(&fifo->lock);
    picture_fifo_Flush(fifo, HEIMDALL_TICK_INVALID, true);
    heimdall_mutex_unlock(&fifo->lock);
    free(fifo);
}

void picture_fifo_Lock(picture_fifo_t *fifo)
{
    heimdall_mutex_lock(&fifo->lock);
}

void picture_fifo_Unlock(picture_fifo_t *fifo)
{
    heimdall_mutex_unlock(&fifo->lock);
}