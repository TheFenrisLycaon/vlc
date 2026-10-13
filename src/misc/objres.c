/*****************************************************************************
 * objres.c: heimdall_object_t resources
 *****************************************************************************
 * Copyright (C) 2017 Rémi Denis-Courmont
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
#include <stdckdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <heimdall_common.h>

#include "../libheimdall.h"
#include "variables.h"

struct heimdall_res
{
    struct heimdall_res *prev;
    void (*release)(void *);
    max_align_t payload[];
};

static struct heimdall_res **heimdall_obj_res(heimdall_object_t *obj)
{
    return &heimdall_internals(obj)->resources;
}

void *heimdall_objres_new(size_t size, void (*release)(void *))
{
    if (unlikely(ckd_add(&size, sizeof (struct heimdall_res), size)))
    {
        errno = ENOMEM;
        return NULL;
    }

    struct heimdall_res *res = malloc(size);
    if (unlikely(res == NULL))
        return NULL;

    res->release = release;
    return res->payload;
}

void heimdall_objres_push(heimdall_object_t *obj, void *data)
{
    struct heimdall_res **restrict pp = heimdall_obj_res(obj);
    struct heimdall_res *res = container_of(data, struct heimdall_res, payload);

    res->prev = *pp;
    *pp = res;
}

static void *heimdall_objres_pop(heimdall_object_t *obj)
{
    struct heimdall_res **restrict pp = heimdall_obj_res(obj);
    struct heimdall_res *res = *pp;

    if (res == NULL)
        return NULL;
    *pp = res->prev;
    return res->payload;
}

void heimdall_objres_clear(heimdall_object_t *obj)
{
    void *data;

    while ((data = heimdall_objres_pop(obj)) != NULL)
    {
        struct heimdall_res *res = container_of(data, struct heimdall_res, payload);

        res->release(res->payload);
        free(res);
    }
}

void heimdall_objres_remove(heimdall_object_t *obj, void *data,
                       bool (*match)(void *, void *))
{
    struct heimdall_res **restrict pp = heimdall_obj_res(obj);

    /* With a doubly-linked list, this function could have constant complexity.
     * But that would require one more pointer per resource.
     *
     * Any given list should contain a fairly small number of resources,
     * and in most cases, the resources are destroyed implicitly by
     * heimdall_objres_clear().
     */
    for (;;)
    {
        struct heimdall_res *res = *pp;

        assert(res != NULL); /* invalid free? */

        if (match(res->payload, data))
        {
            *pp = res->prev;
            res->release(res->payload);
            free(res);
            return;
        }

        pp = &res->prev;
    }
}

static void dummy_release(void *data)
{
    (void) data;
}

static bool ptrcmp(void *a, void *b)
{
    return a == b;
}

void *(heimdall_obj_malloc)(heimdall_object_t *obj, size_t size)
{
    void *ptr = heimdall_objres_new(size, dummy_release);
    if (likely(ptr != NULL))
        heimdall_objres_push(obj, ptr);
    return ptr;
}

void *(heimdall_obj_calloc)(heimdall_object_t *obj, size_t nmemb, size_t size)
{
    size_t tabsize;
    if (unlikely(ckd_mul(&tabsize, nmemb, size)))
    {
        errno = ENOMEM;
        return NULL;
    }

    void *ptr = heimdall_obj_malloc(obj, tabsize);
    if (likely(ptr != NULL))
        memset(ptr, 0, tabsize);
    return ptr;
}

static void *heimdall_obj_memdup(heimdall_object_t *obj, const void *base, size_t len)
{
    void *ptr = heimdall_obj_malloc(obj, len);
    if (likely(ptr != NULL))
        memcpy(ptr, base, len);
    return ptr;
}

char *(heimdall_obj_strdup)(heimdall_object_t *obj, const char *str)
{
    return heimdall_obj_memdup(obj, str, strlen(str) + 1);
}

void (heimdall_obj_free)(heimdall_object_t *obj, void *ptr)
{
    heimdall_objres_remove(obj, ptr, ptrcmp);
}
