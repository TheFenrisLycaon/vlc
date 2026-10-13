/*****************************************************************************
 * error.c: Error handling for libheimdall
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "libheimdall_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <heimdall/libheimdall.h>


static const char oom[] = "Out of memory";
/* TODO: use only one thread-specific key for whole libheimdall */
static heimdall_threadvar_t context;

static char *get_error (void)
{
    return heimdall_threadvar_get (context);
}

static void free_msg (void *msg)
{
    if (msg != oom)
        free (msg);
}

static void free_error (void)
{
    free_msg (get_error ());
}

static heimdall_mutex_t lock = HEIMDALL_STATIC_MUTEX;
static uintptr_t refs = 0;

void libheimdall_threads_init (void)
{
    heimdall_mutex_lock (&lock);
    if (refs++ == 0)
        heimdall_threadvar_create (&context, free_msg);
    heimdall_mutex_unlock (&lock);
}

void libheimdall_threads_deinit (void)
{
    heimdall_mutex_lock (&lock);
    assert (refs > 0);
    if (--refs == 0)
    {
        free_error ();
        heimdall_threadvar_delete (&context);
    }
    heimdall_mutex_unlock (&lock);
}

/**
 * Gets a human-readable error message for the last LibHeimdall error in the calling
 * thread. The resulting string is valid until another error occurs (at least
 * until the next LibHeimdall call).
 *
 * @return NULL if there was no error, a nul-terminated string otherwise.
 */
const char *libheimdall_errmsg (void)
{
    return get_error ();
}

/**
 * Clears the LibHeimdall error status for the current thread. This is optional.
 * By default, the error status is automatically overridden when a new error
 * occurs, and destroyed when the thread exits.
 */
void libheimdall_clearerr (void)
{
    free_error ();
    int ret = heimdall_threadvar_set (context, NULL);
    if(unlikely(ret != 0))
        abort();
}

/**
 * Sets the LibHeimdall error status and message for the current thread.
 * Any previous error is overridden.
 * \param fmt the format string
 * \param ap the arguments
 * \return a nul terminated string in any case
 */
static const char *libheimdall_vprinterr (const char *fmt, va_list ap)
{
    char *msg;

    assert (fmt != NULL);
    if (vasprintf (&msg, fmt, ap) == -1)
        msg = (char *)oom;

    free_error ();
    int ret = heimdall_threadvar_set (context, msg);
    if(unlikely(ret != 0))
        abort();
    return msg;
}

/**
 * Sets the LibHeimdall error status and message for the current thread.
 * Any previous error is overridden.
 * @return a nul terminated string (always)
 */
const char *libheimdall_printerr (const char *fmt, ...)
{
    va_list ap;
    const char *msg;

    va_start (ap, fmt);
    msg = libheimdall_vprinterr (fmt, ap);
    va_end (ap);
    return msg;
}

