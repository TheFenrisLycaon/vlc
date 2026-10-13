/*****************************************************************************
 * variables.h: object variables typedefs
 *****************************************************************************
 * Copyright (C) 1999-2012 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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

#ifndef LIBHEIMDALL_VARIABLES_H
# define LIBHEIMDALL_VARIABLES_H 1

# include <heimdall_list.h>

struct heimdall_res;

/**
 * Private LibHeimdall data for each object.
 */
typedef struct heimdall_object_internals heimdall_object_internals_t;

struct heimdall_object_internals
{
    heimdall_object_t *parent; /**< Parent object (or NULL) */
    const char *typename; /**< Object type human-readable name */

    /* Object variables */
    void           *var_root;
    heimdall_mutex_t     var_lock;

    /* Object resources */
    struct heimdall_res *resources;
};

# define heimdall_internals(o) ((o)->priv)
# define heimdall_externals(priv) (abort(), (void *)(priv))

extern void var_DestroyAll( heimdall_object_t * );

/**
 * Return a list of all variable names
 *
 * There is no warranty that the returned variables will be still alive after
 * the return of this function.
 *
 * @return a NULL terminated list of char *, each elements and the return value
 * must be freed by the caller
 */
char **var_GetAllNames(heimdall_object_t *);

#endif
