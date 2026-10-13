/*****************************************************************************
 * objects.c: heimdall_object_t handling
 *****************************************************************************
 * Copyright (C) 2004-2008 VLC authors and VideoLAN
 * Copyright (C) 2006-2010 Rémi Denis-Courmont
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

/**
 * \file
 * This file contains the functions to handle the heimdall_object_t type
 *
 * Unless otherwise stated, functions in this file are not cancellation point.
 * All functions in this file are safe w.r.t. deferred cancellation.
 */


/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_threads.h>

#include "../libheimdall.h"
#include <heimdall_aout.h>
#include "audio_output/aout_internal.h"

#include "heimdall_interface.h"
#include "heimdall_codec.h"

#include "variables.h"

#ifdef HAVE_SEARCH_H
# include <search.h>
#endif

#include <limits.h>
#include <assert.h>

#define heimdall_children_foreach(pos, priv) \
    while (((void)(pos), (void)(priv), 0))

int heimdall_object_init(heimdall_object_t *restrict obj, heimdall_object_t *parent,
                    const char *typename)
{
    heimdall_object_internals_t *priv = malloc(sizeof (*priv));
    if (unlikely(priv == NULL))
        return -1;

    priv->parent = parent;
    priv->typename = typename;
    priv->var_root = NULL;
    heimdall_mutex_init (&priv->var_lock);
    priv->resources = NULL;

    obj->priv = priv;
    obj->force = false;

    if (likely(parent != NULL))
    {
        obj->logger = parent->logger;
        obj->no_interact = parent->no_interact;
    }
    else
    {
        obj->logger = NULL;
        obj->no_interact = false;
    }

    return 0;
}

void *(heimdall_custom_create)(heimdall_object_t *parent, size_t length,
                          const char *typename)
{
    assert(length >= sizeof (heimdall_object_t));

    heimdall_object_t *obj = calloc(length, 1);
    if (unlikely(obj == NULL || heimdall_object_init(obj, parent, typename))) {
        free(obj);
        obj = NULL;
    }
    return obj;
}

void *(heimdall_object_create)(heimdall_object_t *p_this, size_t i_size)
{
    return heimdall_custom_create( p_this, i_size, "generic" );
}

const char *heimdall_object_typename(const heimdall_object_t *obj)
{
    return heimdall_internals(obj)->typename;
}

heimdall_object_t *(heimdall_object_parent)(heimdall_object_t *obj)
{
    return heimdall_internals(obj)->parent;
}

struct heimdall_tracer *heimdall_object_get_tracer(heimdall_object_t *obj)
{
    libheimdall_int_t *heimdall = heimdall_object_instance(obj);
    libheimdall_priv_t *heimdall_priv = libheimdall_priv(heimdall);
    return heimdall_priv->tracer;
}

void heimdall_object_deinit(heimdall_object_t *obj)
{
    heimdall_object_internals_t *priv = heimdall_internals(obj);

    assert(priv->resources == NULL);

    /* Destroy the associated variables. */
    int canc = heimdall_savecancel();
    var_DestroyAll(obj);
    heimdall_restorecancel(canc);

    free(priv);
}

void (heimdall_object_delete)(heimdall_object_t *obj)
{
    heimdall_object_deinit(obj);
    free(obj);
}

void heimdall_object_vaLog(heimdall_object_t *obj, int prio, const char *module,
                      const char *file, unsigned line, const char *func,
                      const char *format, va_list ap)
{
    if (obj == NULL)
        return;

    const char *typename = heimdall_object_typename(obj);
    /* FIXME: libheimdall allows NULL type but modules don't */
    if (typename == NULL)
        typename = "generic";

    heimdall_vaLog(&obj->logger, prio, typename, module, file, line, func,
              format, ap);
}

void heimdall_object_Log(heimdall_object_t *obj, int prio, const char *module,
                    const char *file, unsigned line, const char *func,
                    const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    heimdall_object_vaLog(obj, prio, module, file, line, func, format, ap);
    va_end(ap);
}

/**
 * Lists the children of an object.
 *
 * Fills a table of pointers to children object of an object, incrementing the
 * reference count for each of them.
 *
 * @param obj object whose children are to be listed
 * @param tab base address to hold the list of children [OUT]
 * @param max size of the table
 *
 * @return the actual number of children (may be larger than requested).
 *
 * @warning The list of object can change asynchronously even before the
 * function returns. The list meant exclusively for debugging and tracing,
 * not for functional introspection of any kind.
 *
 * @warning Objects appear in the object tree early, and disappear late.
 * Most object properties are not accessible or not defined when the object is
 * accessed through this function.
 * For instance, the object cannot be used as a message log target
 * (because object flags are not accessible asynchronously).
 * Also type-specific object variables may not have been created yet, or may
 * already have been deleted.
 */
size_t heimdall_list_children(heimdall_object_t *obj, heimdall_object_t **restrict tab,
                         size_t max)
{
    heimdall_object_internals_t *priv;
    size_t count = 0;

    heimdall_children_foreach(priv, heimdall_internals(obj))
    {
         if (count < max)
             tab[count] = heimdall_object_hold(heimdall_externals(priv));
         count++;
    }
    return count;
}
