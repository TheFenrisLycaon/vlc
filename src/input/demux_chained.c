/*****************************************************************************
 * demux_chained.c
 *****************************************************************************
 * Copyright (C) 1999-2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
 *          Rémi Denis-Courmont
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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <heimdall_common.h>
#include <heimdall_demux.h>
#include "demux.h"

struct heimdall_demux_chained_t
{
    heimdall_stream_fifo_t *writer;
    stream_t *reader;

    heimdall_thread_t thread;
    heimdall_mutex_t  lock;

    struct
    {
        double  position;
        heimdall_tick_t length;
        heimdall_tick_t time;
    } stats;

    es_out_t *out;
    char name[];
};

static void *heimdall_demux_chained_Thread(void *data)
{
    heimdall_thread_set_name("heimdall-demux-chain");

    heimdall_demux_chained_t *dc = data;
    demux_t *demux = demux_New(HEIMDALL_OBJECT(dc->reader), dc->name, "heimdall://nop",
                               dc->reader, dc->out);
    if (demux == NULL)
    {
        heimdall_stream_Delete(dc->reader);
        return NULL;
    }

    /* Stream FIFO cannot apply DVB filters.
     * Get all programs and let the E/S output sort them out. */
    demux_Control(demux, DEMUX_SET_GROUP_ALL);

    /* Main loop */
    heimdall_tick_t next_update = 0;

    do
        if (demux_TestAndClearFlags(demux, UINT_MAX) || heimdall_tick_now() >= next_update)
        {
            double newpos;
            heimdall_tick_t newlen;
            heimdall_tick_t newtime;

            if (demux_Control(demux, DEMUX_GET_POSITION, &newpos))
                newpos = 0.;
            if (demux_Control(demux, DEMUX_GET_LENGTH, &newlen))
                newlen = 0;
            if (demux_Control(demux, DEMUX_GET_TIME, &newtime))
                newtime = 0;

            heimdall_mutex_lock(&dc->lock);
            dc->stats.position = newpos;
            dc->stats.length = newlen;
            dc->stats.time = newtime;
            heimdall_mutex_unlock(&dc->lock);

            next_update = heimdall_tick_now() + HEIMDALL_TICK_FROM_MS(250);
        }
    while (demux_Demux(demux) > 0);

    demux_Delete(demux);
    return NULL;
}

heimdall_demux_chained_t *heimdall_demux_chained_New(heimdall_object_t *parent,
                                           const char *name, es_out_t *out)
{
    heimdall_demux_chained_t *dc = malloc(sizeof (*dc) + strlen(name) + 1);
    if (unlikely(dc == NULL))
        return NULL;

    dc->writer = heimdall_stream_fifo_New(parent, &dc->reader);
    if (dc->writer == NULL)
    {
        free(dc);
        return NULL;
    }

    dc->stats.position = 0.;
    dc->stats.length = 0;
    dc->stats.time = 0;
    dc->out = out;
    strcpy(dc->name, name);

    heimdall_mutex_init(&dc->lock);

    if (heimdall_clone(&dc->thread, heimdall_demux_chained_Thread, dc))
    {
        heimdall_stream_Delete(dc->reader);
        heimdall_stream_fifo_Close(dc->writer);
        free(dc);
        dc = NULL;
    }
    return dc;
}

void heimdall_demux_chained_Send(heimdall_demux_chained_t *dc, block_t *block)
{
    heimdall_stream_fifo_Queue(dc->writer, block);
}

int heimdall_demux_chained_ControlVa(heimdall_demux_chained_t *dc, int query, va_list ap)
{
    switch (query)
    {
        case DEMUX_GET_POSITION:
            heimdall_mutex_lock(&dc->lock);
            *va_arg(ap, double *) = dc->stats.position;
            heimdall_mutex_unlock(&dc->lock);
            break;
        case DEMUX_GET_LENGTH:
            heimdall_mutex_lock(&dc->lock);
            *va_arg(ap, heimdall_tick_t *) = dc->stats.length;
            heimdall_mutex_unlock(&dc->lock);
            break;
        case DEMUX_GET_TIME:
            heimdall_mutex_lock(&dc->lock);
            *va_arg(ap, heimdall_tick_t *) = dc->stats.time;
            heimdall_mutex_unlock(&dc->lock);
            break;
        default:
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

void heimdall_demux_chained_Delete(heimdall_demux_chained_t *dc)
{
    heimdall_stream_fifo_Close(dc->writer);
    heimdall_join(dc->thread, NULL);
    free(dc);
}
