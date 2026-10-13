/*****************************************************************************
 * journal.c: SystemD journal logger plugin
 *****************************************************************************
 * Copyright © 2015 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>
#include <inttypes.h>
#include <syslog.h>
#include <systemd/sd-journal.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>

static const int priorities[4] = {
    [HEIMDALL_MSG_INFO] = LOG_INFO,
    [HEIMDALL_MSG_ERR]  = LOG_ERR,
    [HEIMDALL_MSG_WARN] = LOG_WARNING,
    [HEIMDALL_MSG_DBG]  = LOG_DEBUG,
};

static void Log(void *opaque, int type, const heimdall_log_t *meta,
                const char *format, va_list ap)
{
    static const char default_msg[] = "message lost";
    char *msg;

    if (vasprintf(&msg, format, ap) == -1)
        msg = (char *)default_msg;

    sd_journal_send("MESSAGE=%s", msg,
        "PRIORITY=%d", priorities[type],
        "CODE_FILE=%s", (meta->file != NULL) ? meta->file : "",
        "CODE_LINE=%u", meta->line,
        "CODE_FUNC=%s", (meta->func != NULL) ? meta->func : "",
        //"ERRNO=%d"
        "HEIMDALL_TID=%lu" /* change to OBJECT_TID if standardized */, meta->tid,
        "HEIMDALL_OBJECT_ID=%"PRIxPTR, meta->i_object_id,
        "HEIMDALL_OBJECT_TYPE=%s", meta->psz_object_type,
        "HEIMDALL_MODULE=%s", meta->psz_module,
        "HEIMDALL_HEADER=%s", (meta->psz_header != NULL) ? meta->psz_header : "",
        NULL);

    if (msg != default_msg)
        free(msg);
    (void) opaque;
}

static const struct heimdall_logger_operations ops = { Log, NULL };

static const struct heimdall_logger_operations *Open(heimdall_object_t *obj, void **sysp)
{
    if (!var_InheritBool(obj, "syslog"))
        return NULL;

    (void) sysp;
    return &ops;
}

heimdall_module_begin()
    set_shortname(N_("Journal"))
    set_description(N_("SystemD journal logger"))
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("logger", 30)
    set_callback(Open)
    add_shortcut("journal")
heimdall_module_end()
