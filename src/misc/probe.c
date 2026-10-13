/*****************************************************************************
 * probe.c : run-time service listing
 *****************************************************************************
 * Copyright © 2009 Rémi Denis-Courmont
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
#include <heimdall_probe.h>
#include <heimdall_modules.h>
#include "../libheimdall.h"

#undef heimdall_probe
void *heimdall_probe (heimdall_object_t *obj,
                 const char *capability, size_t *restrict pcount)
{
    heimdall_probe_t *probe = heimdall_custom_create (obj, sizeof(*probe), "probe");
    if (unlikely(probe == NULL))
    {
        *pcount = 0;
        return NULL;
    }
    probe->list = NULL;
    probe->count = 0;

    module_t *mod = module_need (probe, capability, NULL, false);
    if (mod != NULL)
    {
        msg_Warn (probe, "probing halted");
        module_unneed (probe, mod);
    }

    void *ret = probe->list;
    *pcount = probe->count;
    heimdall_object_delete(probe);
    return ret;
}
