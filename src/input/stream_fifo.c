/*****************************************************************************
 * stream_fifo.c
 *****************************************************************************
 * Copyright (C) 2016 Rémi Denis-Courmont
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_block.h>
#include <heimdall_queue.h>
#include <heimdall_stream.h>

#include "stream.h"

struct heimdall_stream_fifo {
    heimdall_queue_t queue;
    bool eof;
};

struct heimdall_stream_fifo_private {
    heimdall_stream_fifo_t *writer;
};

static heimdall_stream_fifo_t *heimdall_stream_fifo_Writer(stream_t *s)
{
    struct heimdall_stream_fifo_private *sys = heimdall_stream_Private(s);

    return sys->writer;
}

static void heimdall_stream_fifo_Destroy(stream_t *s)
{
    struct heimdall_stream_fifo *writer = heimdall_stream_fifo_Writer(s);
    block_t *block;
    bool closed;

    heimdall_queue_Lock(&writer->queue);
    block = heimdall_queue_DequeueAllUnlocked(&writer->queue);
    closed = writer->eof;
    writer->eof = true;
    heimdall_queue_Unlock(&writer->queue);

    block_ChainRelease(block);

    if (closed)
        /* Destroy shared state if write end is already closed */
        free(writer);
}

static block_t *heimdall_stream_fifo_Block(stream_t *s, bool *restrict eof)
{
    struct heimdall_stream_fifo *sys = heimdall_stream_fifo_Writer(s);
    block_t *block = heimdall_queue_DequeueKillable(&sys->queue, &sys->eof);

    if (block == NULL)
        *eof = true;

    return block;
}

static int heimdall_stream_fifo_Control(stream_t *s, int query, va_list ap)
{
    (void) s;

    switch (query)
    {
        case STREAM_CAN_SEEK:
        case STREAM_CAN_FASTSEEK:
        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
            *va_arg(ap, bool *) = false;
            break;

        case STREAM_GET_PTS_DELAY:
            *va_arg(ap, heimdall_tick_t *) = DEFAULT_PTS_DELAY;
            break;

        default:
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

heimdall_stream_fifo_t *heimdall_stream_fifo_New(heimdall_object_t *parent, stream_t **reader)
{
    struct heimdall_stream_fifo *writer = malloc(sizeof (*writer));
    if (unlikely(writer == NULL))
        return NULL;

    heimdall_queue_Init(&writer->queue, offsetof (block_t, p_next));
    writer->eof = false;

    struct heimdall_stream_fifo_private *sys;
    stream_t *s = heimdall_stream_CustomNew(parent, heimdall_stream_fifo_Destroy,
                                       sizeof (*sys), "stream");
    if (unlikely(s == NULL)) {
        free(writer);
        return NULL;
    }

    sys = heimdall_stream_Private(s);
    sys->writer = writer;
    s->pf_block = heimdall_stream_fifo_Block;
    s->pf_seek = NULL;
    s->pf_control = heimdall_stream_fifo_Control;
    *reader = s;
    return writer;
}

int heimdall_stream_fifo_Queue(heimdall_stream_fifo_t *writer, block_t *block)
{
    heimdall_queue_Lock(&writer->queue);
    if (likely(!writer->eof))
    {
        heimdall_queue_EnqueueUnlocked(&writer->queue, block);
        block = NULL;
    }
    heimdall_queue_Unlock(&writer->queue);

    if (unlikely(block != NULL))
    {
        block_Release(block);
        errno = EPIPE;
        return -1;
    }
    return 0;
}

ssize_t heimdall_stream_fifo_Write(heimdall_stream_fifo_t *writer,
                              const void *buf, size_t len)
{
    block_t *block = block_Alloc(len);
    if (unlikely(block == NULL))
        return -1;

    memcpy(block->p_buffer, buf, len);
    return heimdall_stream_fifo_Queue(writer, block) ? -1 : (ssize_t)len;
}

void heimdall_stream_fifo_Close(heimdall_stream_fifo_t *writer)
{
    bool closed;

    heimdall_queue_Lock(&writer->queue);
    closed = writer->eof;
    writer->eof = true;
    heimdall_queue_Signal(&writer->queue);
    heimdall_queue_Unlock(&writer->queue);

    if (closed)
        /* Destroy shared state if read end is already closed */
        free(writer);
}
