/*****************************************************************************
 * log.c: libheimdall new API log functions
 *****************************************************************************
 * Copyright (C) 2005 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 *
 * Authors: Damien Fouilleul <damienf@videolan.org>
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
#include <heimdall/heimdall.h>
#include "libheimdall_internal.h"
#include <heimdall_common.h>
#include <heimdall_interface.h>

/*** Logging core dispatcher ***/

void libheimdall_log_get_context(const libheimdall_log_t *ctx,
                            const char **restrict module,
                            const char **restrict file,
                            unsigned *restrict line)
{
    if (module != NULL)
        *module = ctx->psz_module;
    if (file != NULL)
        *file = ctx->file;
    if (line != NULL)
        *line = ctx->line;
}

void libheimdall_log_get_object(const libheimdall_log_t *ctx,
                           const char **restrict name,
                           const char **restrict header,
                           uintptr_t *restrict id)
{
    if (name != NULL)
        *name = (ctx->psz_object_type != NULL)
                ? ctx->psz_object_type : "generic";
    if (header != NULL)
        *header = ctx->psz_header;
    if (id != NULL)
        *id = ctx->i_object_id;
}

static void libheimdall_logf (void *data, int level, const heimdall_log_t *item,
                         const char *fmt, va_list ap)
{
    libheimdall_instance_t *inst = data;

    switch (level)
    {
        case HEIMDALL_MSG_INFO: level = LIBHEIMDALL_NOTICE;  break;
        case HEIMDALL_MSG_ERR:  level = LIBHEIMDALL_ERROR;   break;
        case HEIMDALL_MSG_WARN: level = LIBHEIMDALL_WARNING; break;
        case HEIMDALL_MSG_DBG:  level = LIBHEIMDALL_DEBUG;   break;
    }

    inst->log.cb (inst->log.data, level, item, fmt, ap);
}

static const struct heimdall_logger_operations libheimdall_log_ops = {
    libheimdall_logf, NULL
};

void libheimdall_log_unset (libheimdall_instance_t *inst)
{
    heimdall_LogSet (inst->p_libheimdall_int, NULL, NULL);
}

void libheimdall_log_set (libheimdall_instance_t *inst, libheimdall_log_cb cb, void *data)
{
    libheimdall_log_unset (inst); /* <- Barrier before modifying the callback */
    inst->log.cb = cb;
    inst->log.data = data;
    heimdall_LogSet(inst->p_libheimdall_int, &libheimdall_log_ops, inst);
}

/*** Helpers for logging to files ***/
static void libheimdall_log_file (void *data, int level, const libheimdall_log_t *log,
                             const char *fmt, va_list ap)
{
    FILE *stream = data;

    flockfile (stream);
    vfprintf (stream, fmt, ap);
    fputc ('\n', stream);
    funlockfile (stream);
    (void) level; (void) log;
}

void libheimdall_log_set_file (libheimdall_instance_t *inst, FILE *stream)
{
    libheimdall_log_set (inst, libheimdall_log_file, stream);
}
