/*****************************************************************************
 * ancillary.c: ancillary management functions
 *****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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

#include <heimdall_common.h>
#include <heimdall_atomic.h>

#include <heimdall_ancillary.h>

struct heimdall_ancillary
{
    heimdall_atomic_rc_t rc;

    heimdall_ancillary_id id;
    void *data;
    heimdall_ancillary_free_cb free_cb;
};

struct heimdall_ancillary *
heimdall_ancillary_CreateWithFreeCb(void *data,
                               heimdall_ancillary_id id,
                               heimdall_ancillary_free_cb free_cb)
{
    struct heimdall_ancillary *ancillary = malloc(sizeof(*ancillary));

    if (ancillary == NULL)
        return NULL;

    heimdall_atomic_rc_init(&ancillary->rc);
    ancillary->id = id;
    ancillary->data = data;
    ancillary->free_cb = free_cb;

    return ancillary;
}

void
heimdall_ancillary_Release(struct heimdall_ancillary *ancillary)
{
    if (heimdall_atomic_rc_dec(&ancillary->rc))
    {
        if (ancillary->free_cb != NULL)
            ancillary->free_cb(ancillary->data);
        free(ancillary);
    }
}

struct heimdall_ancillary *
heimdall_ancillary_Hold(struct heimdall_ancillary *ancillary)
{
    heimdall_atomic_rc_inc(&ancillary->rc);
    return ancillary;
}

void *
heimdall_ancillary_GetData(const struct heimdall_ancillary *ancillary)
{
    return ancillary->data;
}

void
heimdall_ancillary_array_Clear(heimdall_ancillary_array *array)
{
    struct heimdall_ancillary *ancillary;
    heimdall_vector_foreach(ancillary, array)
        heimdall_ancillary_Release(ancillary);
    heimdall_vector_clear(array);
}

int
heimdall_ancillary_array_Merge(heimdall_ancillary_array *dst_array,
                          const heimdall_ancillary_array *src_array)
{
    if (src_array->size == 0)
        return HEIMDALL_SUCCESS;

    int ret = HEIMDALL_SUCCESS;
    for (size_t i = 0; i < src_array->size && ret == HEIMDALL_SUCCESS; ++i)
        ret = heimdall_ancillary_array_Insert(dst_array, src_array->data[i]);

    return HEIMDALL_SUCCESS;
}

int
heimdall_ancillary_array_MergeAndClear(heimdall_ancillary_array *dst_array,
                                  heimdall_ancillary_array *src_array)
{
    if (dst_array->size == 0)
    {
        *dst_array = *src_array;
        heimdall_ancillary_array_Init(src_array);
        return HEIMDALL_SUCCESS;
    }

    int ret = heimdall_ancillary_array_Merge(dst_array, src_array);
    if (ret == HEIMDALL_SUCCESS)
        heimdall_ancillary_array_Clear(src_array);
    return ret;
}

int
heimdall_ancillary_array_Insert(heimdall_ancillary_array *array,
                           struct heimdall_ancillary *ancillary)
{
    assert(ancillary != NULL);

    for (size_t i = 0; i < array->size; ++i)
    {
        struct heimdall_ancillary *ancillary_it = array->data[i];
        if (ancillary_it->id == ancillary->id)
        {
            heimdall_ancillary_Release(ancillary_it);
            array->data[i] = heimdall_ancillary_Hold(ancillary);
            return HEIMDALL_SUCCESS;
        }
    }

    bool success = heimdall_vector_push(array, ancillary);
    if (!success)
        return HEIMDALL_ENOMEM;
    heimdall_ancillary_Hold(ancillary);

    return HEIMDALL_SUCCESS;
}

struct heimdall_ancillary *
heimdall_ancillary_array_Get(const heimdall_ancillary_array *array,
                        heimdall_ancillary_id id)
{
    struct heimdall_ancillary *ancillary_it;
    heimdall_vector_foreach(ancillary_it, array)
    {
        if (ancillary_it->id == id)
            return ancillary_it;
    }
    return NULL;
}
