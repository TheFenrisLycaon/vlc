/*****************************************************************************
 * tracer.c: tracing interface
 * This library provides an interface to the traces to be used by other
 * modules. See heimdall_config.h for output configuration.
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

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_modules.h>
#include <heimdall_tracer.h>
#include "../libheimdall.h"

struct heimdall_tracer {
    const struct heimdall_tracer_operations *ops;
};

/**
 * Module-based message trace.
 */
struct heimdall_tracer_module {
    struct heimdall_object_t obj;
    struct heimdall_tracer tracer;
    void *opaque;
};

#undef heimdall_tracer_TraceWithTs
void heimdall_tracer_TraceWithTs(struct heimdall_tracer *tracer, heimdall_tick_t ts,
                            const struct heimdall_tracer_trace *trace)
{
    assert(tracer->ops->trace != NULL);
    struct heimdall_tracer_module *module =
            container_of(tracer, struct heimdall_tracer_module, tracer);

    /* Pass message to the callback */
    tracer->ops->trace(module->opaque, ts, trace);
}

static int heimdall_tracer_load(void *func, bool forced, va_list ap)
{
    heimdall_tracer_open_cb activate = func;
    struct heimdall_tracer_module *module = va_arg(ap, struct heimdall_tracer_module *);

    (void) forced;
    module->tracer.ops = activate(HEIMDALL_OBJECT(module), &module->opaque);
    return (module->tracer.ops != NULL) ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

struct heimdall_tracer *heimdall_tracer_Create(heimdall_object_t *parent, const char *module_name)
{
    struct heimdall_tracer_module *module;

    module = heimdall_custom_create(parent, sizeof (*module), "tracer");
    if (unlikely(module == NULL))
        return NULL;

    if (heimdall_module_load(heimdall_object_logger(module), "tracer", module_name, false,
                        heimdall_tracer_load, module) == NULL) {
        heimdall_object_delete(HEIMDALL_OBJECT(module));
        return NULL;
    }

    return &module->tracer;
}

void heimdall_tracer_Destroy(struct heimdall_tracer *tracer)
{
    struct heimdall_tracer_module *module =
        container_of(tracer, struct heimdall_tracer_module, tracer);

    if (module->tracer.ops->destroy != NULL)
        module->tracer.ops->destroy(module->opaque);

    heimdall_object_delete(HEIMDALL_OBJECT(module));
}
