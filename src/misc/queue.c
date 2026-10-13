/*****************************************************************************
 * queue.c: generic queue (FIFO)
 *****************************************************************************
 * Copyright (C) 2020 Rémi Denis-Courmont
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
#include <stdint.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_queue.h>

/* Opaque struct type.
 *
 * ISO C uses the same representation for all pointer-to-struct types.
 * Still different pointer types are not compatible, i.e. cannot alias.
 * So use memcpy() to read/write pointer values.
 */
struct heimdall_queue_entry;

static void entry_set(struct heimdall_queue_entry **pp, struct heimdall_queue_entry *e)
{
    memcpy(pp, &e, sizeof (e));
}

static struct heimdall_queue_entry *entry_get(struct heimdall_queue_entry *const *pp)
{
    struct heimdall_queue_entry *e;

    memcpy(&e, pp, sizeof (e));
    return e;
}

static struct heimdall_queue_entry **next_p(const struct heimdall_queue_entry *e,
                                       ptrdiff_t offset)
{
    return (struct heimdall_queue_entry **)(((unsigned char *)e) + offset);
}

static void next_set(struct heimdall_queue_entry *e, struct heimdall_queue_entry *next,
                     ptrdiff_t offset)
{
    entry_set(next_p(e, offset), next);
}

static struct heimdall_queue_entry *next_get(const struct heimdall_queue_entry *e,
                                        ptrdiff_t offset)
{
    return entry_get(next_p(e, offset));
}

void heimdall_queue_Init(heimdall_queue_t *q, ptrdiff_t next_offset)
{
    q->first = NULL;
    q->lastp = &q->first;
    q->next_offset = next_offset;
    heimdall_mutex_init(&q->lock);
    heimdall_cond_init(&q->wait);
}

void heimdall_queue_EnqueueUnlocked(heimdall_queue_t *q, void *entry)
{
    struct heimdall_queue_entry **lastp;
    const ptrdiff_t offset = q->next_offset;

    heimdall_mutex_assert(&q->lock);
    assert(entry_get(q->lastp) == NULL);
    entry_set(q->lastp, entry);

    for (lastp = q->lastp; entry != NULL; entry = next_get(entry, offset))
        lastp = next_p(entry, offset);

    q->lastp = lastp;
    heimdall_queue_Signal(q);
}

void *heimdall_queue_DequeueUnlocked(heimdall_queue_t *q)
{
    heimdall_mutex_assert(&q->lock);

    void *entry = q->first;
    const ptrdiff_t offset = q->next_offset;

    if (entry != NULL) {
        struct heimdall_queue_entry *next = next_get(entry, offset);

        next_set(entry, NULL, offset);
        q->first = next;

        if (next == NULL)
            q->lastp = &q->first;
    }

    return entry;
}

void *heimdall_queue_DequeueAllUnlocked(heimdall_queue_t *q)
{
    heimdall_mutex_assert(&q->lock);

    void *entry = q->first;

    q->first = NULL;
    q->lastp = &q->first;

    return entry;
}

void heimdall_queue_Enqueue(heimdall_queue_t *q, void *entry)
{
    heimdall_queue_Lock(q);
    heimdall_queue_EnqueueUnlocked(q, entry);
    heimdall_queue_Unlock(q);
}

void *heimdall_queue_Dequeue(heimdall_queue_t *q)
{
    void *entry;

    heimdall_queue_Lock(q);
    heimdall_testcancel();

    while (heimdall_queue_IsEmpty(q))
        heimdall_queue_Wait(q);

    entry = heimdall_queue_DequeueUnlocked(q);
    heimdall_queue_Unlock(q);

    return entry;
}

void *heimdall_queue_DequeueAll(heimdall_queue_t *q)
{
    void *entry;

    heimdall_queue_Lock(q);
    entry = heimdall_queue_DequeueAllUnlocked(q);
    heimdall_queue_Unlock(q);

    return entry;
}
