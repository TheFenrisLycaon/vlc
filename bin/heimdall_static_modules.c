/*****************************************************************************
 * Copyright (C) 2021-2026 Alexandre Janniaux <ajanni@videolabs.io>
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
# include <config.h>
#endif

#include <heimdall_modules_manifest.h>

typedef int (*heimdall_set_cb)(void*, void*, int, ...);
typedef int (*heimdall_plugin_cb)(heimdall_set_cb, void*);

#define DECLARE_PLUGIN(NAME) int heimdall_entry__ ## NAME(heimdall_set_cb, void*);
#define LIST_PLUGIN(NAME) heimdall_entry__ ## NAME,

HEIMDALL_MODULE_LIST(DECLARE_PLUGIN)

__attribute__((visibility("default")))
heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_MODULE_LIST(LIST_PLUGIN)
    NULL,
};
