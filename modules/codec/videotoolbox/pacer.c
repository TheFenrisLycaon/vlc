/*****************************************************************************
 * pacer.c: decoder picture output pacing
 *****************************************************************************
 * Copyright © 2017-2022 VideoLabs, VideoLAN and VLC authors
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
#include <heimdall_common.h>
#include <heimdall_threads.h>

//#define PIC_PACER_DEBUG

#include "pacer.h"

#include <assert.h>

#define PIC_PACER_ALLOCATABLE_MAX (1 /* callback, pre-reorder */ \
                                 + 2 /* filters */ \
                                 + 1 /* display */ \
                                 + 1 /* next/prev display */)
#define PIC_PACER_DECODE_QUEUE 4  /* max async decode before callback */

void pic_pacer_Clean(struct pic_pacer *pic_pacer)
{
    (void) pic_pacer;
}

void pic_pacer_Init(struct pic_pacer *pic_pacer)
{
    heimdall_mutex_init(&pic_pacer->lock);
    heimdall_cond_init(&pic_pacer->wait);
    pic_pacer->nb_fields_out = 0;
    pic_pacer->allocated_fields_max = 6 /* pics */ * 2;
    pic_pacer->allocated_fields_next = pic_pacer->allocated_fields_max;
    pic_pacer->queued_fields_for_decode = 0;
}

void pic_pacer_AccountAllocation(struct pic_pacer *pic_pacer, bool b_field)
{
    heimdall_mutex_lock(&pic_pacer->lock);
    pic_pacer->nb_fields_out += b_field ? 1 : 2;
    heimdall_mutex_unlock(&pic_pacer->lock);
}

void pic_pacer_AccountScheduledDecode(struct pic_pacer *pic_pacer, bool b_field)
{
    heimdall_mutex_lock(&pic_pacer->lock);
    pic_pacer->queued_fields_for_decode += b_field ? 1 : 2;
    heimdall_mutex_unlock(&pic_pacer->lock);
}

void pic_pacer_AccountFinishedDecode(struct pic_pacer *pic_pacer, bool b_field)
{
    heimdall_mutex_lock(&pic_pacer->lock);
    pic_pacer->queued_fields_for_decode -= b_field ? 1 : 2;
    heimdall_cond_signal(&pic_pacer->wait);
    heimdall_mutex_unlock(&pic_pacer->lock);
}

void pic_pacer_WaitAllocatableSlot(struct pic_pacer *pic_pacer, bool b_field)
{
    heimdall_mutex_lock(&pic_pacer->lock);
    uint8_t allocatable_fields_total = pic_pacer->allocated_fields_max + PIC_PACER_DECODE_QUEUE;

    while( pic_pacer->queued_fields_for_decode +
               pic_pacer->nb_fields_out + (1 - !!b_field) >= allocatable_fields_total )
    {
#ifdef PIC_PACER_DEBUG
        fprintf(stderr, "input pacing %d+%d >= %d\n",
                pic_pacer->queued_fields_for_decode, pic_pacer->nb_fields_out, allocatable_fields_total);
#endif
        heimdall_cond_wait(&pic_pacer->wait, &pic_pacer->lock);
        /*update*/
        allocatable_fields_total = pic_pacer->allocated_fields_max + PIC_PACER_DECODE_QUEUE;
    }
    heimdall_mutex_unlock(&pic_pacer->lock);
}

void pic_pacer_AccountDeallocation(struct pic_pacer *pic_pacer, bool b_field)
{
    heimdall_mutex_lock(&pic_pacer->lock);
    assert(pic_pacer->nb_fields_out > 0);
    pic_pacer->nb_fields_out -= (b_field ? 1 : 2);

    /* our shrink condition */
    if(pic_pacer->allocated_fields_next < pic_pacer->allocated_fields_max &&
        pic_pacer->nb_fields_out <= pic_pacer->allocated_fields_next)
        pic_pacer->allocated_fields_max = pic_pacer->allocated_fields_next;

    heimdall_cond_signal(&pic_pacer->wait);

    heimdall_mutex_unlock(&pic_pacer->lock);
}

void pic_pacer_UpdateMaxBuffering(struct pic_pacer *pic_pacer, uint8_t pic_max)
{
    heimdall_mutex_lock(&pic_pacer->lock);

    const uint8_t fields_max = (pic_max + PIC_PACER_ALLOCATABLE_MAX) * 2;
    bool b_growing  = fields_max > pic_pacer->allocated_fields_max;
#ifdef PIC_PACER_DEBUG
    fprintf(stderr, "updating pacer max %d/%d to %d\n",
            pic_pacer->nb_fields_out, pic_pacer->allocated_fields_max / 2, pic_max);
#endif
    if(b_growing)
    {
        pic_pacer->allocated_fields_max = fields_max;
        pic_pacer->allocated_fields_next = fields_max;
        heimdall_cond_signal(&pic_pacer->wait);
    }
    else
    {
        pic_pacer->allocated_fields_next = fields_max;
    }

    heimdall_mutex_unlock(&pic_pacer->lock);
}
