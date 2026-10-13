/*****************************************************************************
 * memory.c: Memory keystore
 *****************************************************************************
 * Copyright © 2015-2016 VLC authors, VideoLAN and VideoLabs
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

#include <stdio.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_plugin.h>
#include <heimdall_keystore.h>
#include <heimdall_strings.h>

#include <assert.h>

#include "list_util.h"

static int Open(heimdall_object_t *);
static void Close(heimdall_object_t *);

heimdall_module_begin()
    set_shortname(N_("Memory keystore"))
    set_description(N_("Secrets are stored in memory"))
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("keystore", 0)
    set_callbacks(Open, Close)
    add_shortcut("memory")
heimdall_module_end ()

struct heimdall_keystore_sys
{
    struct ks_list  list;
    heimdall_mutex_t     lock;
};

static int
Store(heimdall_keystore *p_keystore, const char *const ppsz_values[KEY_MAX],
      const uint8_t *p_secret, size_t i_secret_len, const char *psz_label)
{
    (void) psz_label;
    heimdall_keystore_sys *p_sys = p_keystore->p_sys;
    struct ks_list *p_list = &p_sys->list;
    heimdall_keystore_entry *p_entry;
    int i_ret = HEIMDALL_EGENERIC;

    heimdall_mutex_lock(&p_sys->lock);
    p_entry = ks_list_find_entry(p_list, ppsz_values, NULL);
    if (p_entry)
        heimdall_keystore_release_entry(p_entry);
    else
    {
        p_entry = ks_list_new_entry(p_list);
        if (!p_entry)
            goto end;
    }
    if (ks_values_copy((const char **)p_entry->ppsz_values, ppsz_values))
        goto end;

    if (heimdall_keystore_entry_set_secret(p_entry, p_secret, i_secret_len))
        goto end;

    i_ret = HEIMDALL_SUCCESS;
end:
    heimdall_mutex_unlock(&p_sys->lock);
    return i_ret;
}

static unsigned int
Find(heimdall_keystore *p_keystore, const char *const ppsz_values[KEY_MAX],
     heimdall_keystore_entry **pp_entries)
{
    heimdall_keystore_sys *p_sys = p_keystore->p_sys;
    struct ks_list *p_list = &p_sys->list;
    struct ks_list out_list = { 0 };
    heimdall_keystore_entry *p_entry;
    unsigned i_index = 0;

    heimdall_mutex_lock(&p_sys->lock);
    while ((p_entry = ks_list_find_entry(p_list, ppsz_values, &i_index)))
    {
        heimdall_keystore_entry *p_out_entry = ks_list_new_entry(&out_list);

        if (!p_out_entry
         || ks_values_copy((const char **)p_out_entry->ppsz_values,
                           (const char *const*)p_entry->ppsz_values))
        {
            ks_list_free(&out_list);
            break;
        }

        if (heimdall_keystore_entry_set_secret(p_out_entry, p_entry->p_secret,
                                          p_entry->i_secret_len))
        {
            ks_list_free(&out_list);
            break;
        }
    }
    heimdall_mutex_unlock(&p_sys->lock);

    *pp_entries = out_list.p_entries;

    return out_list.i_count;
}

static unsigned int
Remove(heimdall_keystore *p_keystore, const char *const ppsz_values[KEY_MAX])
{
    heimdall_keystore_sys *p_sys = p_keystore->p_sys;
    struct ks_list *p_list = &p_sys->list;
    heimdall_keystore_entry *p_entry;
    unsigned i_index = 0, i_count = 0;

    heimdall_mutex_lock(&p_sys->lock);
    while ((p_entry = ks_list_find_entry(p_list, ppsz_values, &i_index)))
    {
        heimdall_keystore_release_entry(p_entry);
        i_count++;
    }
    heimdall_mutex_unlock(&p_sys->lock);

    return i_count;
}

static void
Close(heimdall_object_t *p_this)
{
    heimdall_keystore *p_keystore = (heimdall_keystore *)p_this;
    heimdall_keystore_sys *p_sys = p_keystore->p_sys;

    ks_list_free(&p_sys->list);
    free(p_sys);
}

static int
Open(heimdall_object_t *p_this)
{
    heimdall_keystore *p_keystore = (heimdall_keystore *)p_this;
    p_keystore->p_sys = calloc(1, sizeof(heimdall_keystore_sys));
    if (!p_keystore->p_sys)
        return HEIMDALL_EGENERIC;

    heimdall_mutex_init(&p_keystore->p_sys->lock);
    p_keystore->pf_store = Store;
    p_keystore->pf_find = Find;
    p_keystore->pf_remove = Remove;

    return HEIMDALL_SUCCESS;
}
