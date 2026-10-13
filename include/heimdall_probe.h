/*****************************************************************************
 * heimdall_probe.h: service probing interface
 *****************************************************************************
 * Copyright (C) 2009 Rémi Denis-Courmont
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

#ifndef HEIMDALL_PROBE_H
# define HEIMDALL_PROBE_H 1

# include <stdlib.h>

/**
 * \file
 * This file defines functions and structures to run-time probe Heimdall extensions
 */

# ifdef __cplusplus
extern "C" {
# endif

void *heimdall_probe (heimdall_object_t *, const char *, size_t *);
#define heimdall_probe(obj, cap, pcount) \
        heimdall_probe(HEIMDALL_OBJECT(obj), cap, pcount)

struct heimdall_probe_t
{
    struct heimdall_object_t obj;

    void  *list;
    size_t count;
};

typedef struct heimdall_probe_t heimdall_probe_t;

static inline int heimdall_probe_add(heimdall_probe_t *obj, const void *data,
                                size_t len)
{
    char *tab = (char *)realloc (obj->list, (obj->count + 1) * len);

    if (unlikely(tab == NULL))
        return HEIMDALL_ENOMEM;
    memcpy(tab + (obj->count * len), data, len);
    obj->list = tab;
    obj->count++;
    return HEIMDALL_SUCCESS;
}

# define HEIMDALL_PROBE_CONTINUE HEIMDALL_EGENERIC
# define HEIMDALL_PROBE_STOP     HEIMDALL_SUCCESS

# ifdef __cplusplus
}
# endif

#endif
