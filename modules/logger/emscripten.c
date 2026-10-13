/*****************************************************************************
 * emscripten.c: Emscripten logger
 *****************************************************************************
 * Copyright © 2022 VLC authors, VideoLAN and Videolabs
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

#include <emscripten.h>

#include <stdarg.h>
#include <stdio.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>

static void EmscriptenPrintMsg(void *opaque, int type, const heimdall_log_t *p_item,
                               const char *format, va_list ap)
{
    int prio;
    char *message;
    int verbosity = (int)opaque;

    if (verbosity < type)
        return;

    if (vasprintf(&message, format, ap) < 0)
        return;
    switch (type) {
        case HEIMDALL_MSG_ERR:
            prio = EM_LOG_CONSOLE | EM_LOG_ERROR;
            break;
        case HEIMDALL_MSG_WARN:
            prio = EM_LOG_CONSOLE | EM_LOG_WARN;
            break;
        case HEIMDALL_MSG_INFO:
            prio = EM_LOG_CONSOLE | EM_LOG_INFO;
            break;
        case HEIMDALL_MSG_DBG:
            prio = EM_LOG_CONSOLE | EM_LOG_DEBUG;
        default:
            prio = EM_LOG_CONSOLE;
    }

    emscripten_log(prio, "[heimdall.js: 0x%"PRIxPTR"/0x%"PRIxPTR"] %s %s: %s",
                   p_item->i_object_id, p_item->tid, p_item->psz_module,
                   p_item->psz_object_type, message);
    free(message);
}

static const struct heimdall_logger_operations ops = { EmscriptenPrintMsg, NULL };

static const struct heimdall_logger_operations *Open(heimdall_object_t *obj, void **sysp)
{
    int verbosity = var_InheritInteger(obj, "verbose");

    if (verbosity < 0)
        return NULL;
    verbosity += HEIMDALL_MSG_ERR;
    *sysp = (void *)(int)verbosity;
    return &ops;
}

heimdall_module_begin()
    set_shortname(N_("emlog"))
    set_description(N_("Emscripten logger"))
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("logger", 30)
    set_callback(Open)
heimdall_module_end ()
