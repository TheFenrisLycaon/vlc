/*****************************************************************************
 * cache.c: access cache helper
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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
#include <heimdall_threads.h>

#include "cache.h"

#include <assert.h>

#define HEIMDALL_ACCESS_CACHE_TTL HEIMDALL_TICK_FROM_SEC(5)
#define HEIMDALL_ACCESS_CACHE_MAX_ENTRY 5

void
heimdall_access_cache_entry_Delete(struct heimdall_access_cache_entry *entry)
{
    free(entry->url);
    free(entry->username);

    free(entry);
}

struct heimdall_access_cache_entry *
heimdall_access_cache_entry_New(void *context, const char *url, const char *username,
                           void (*free_cb)(void *context))
{
    struct heimdall_access_cache_entry *entry = malloc(sizeof(*entry));
    if (unlikely(entry == NULL))
        return NULL;

    entry->url = strdup(url);
    entry->username = username ? strdup(username) : NULL;
    if (!entry->url || (entry->username == NULL) != (username == NULL))
    {
        heimdall_access_cache_entry_Delete(entry);
        return NULL;
    }

    entry->context = context;
    entry->free_cb = free_cb;

    return entry;
}

#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
static void *
heimdall_access_cache_Thread(void *data)
{
    struct heimdall_access_cache *cache = data;

    heimdall_thread_set_name("heimdall-axs-cache");

    heimdall_mutex_lock(&cache->lock);
    while (cache->running)
    {
        if (!heimdall_list_is_empty(&cache->entries))
        {
            struct heimdall_access_cache_entry *entry =
                heimdall_list_first_entry_or_null(&cache->entries,
                                             struct heimdall_access_cache_entry, node);

            if (entry->timeout == 0 ||
                heimdall_cond_timedwait(&cache->cond, &cache->lock, entry->timeout) != 0)
            {
                heimdall_list_remove(&entry->node);

                heimdall_mutex_unlock(&cache->lock);

                entry->free_cb(entry->context);
                heimdall_access_cache_entry_Delete(entry);

                heimdall_mutex_lock(&cache->lock);
            }
        }
        else
            heimdall_cond_wait(&cache->cond, &cache->lock);
    }
    heimdall_mutex_unlock(&cache->lock);

    return NULL;
}

static void
heimdall_access_cache_InitOnce(void *data)
{
    struct heimdall_access_cache *cache = data;

    heimdall_mutex_lock(&cache->lock);

    cache->running = true;
    int ret = heimdall_clone(&cache->thread, heimdall_access_cache_Thread, cache);
    if (ret != 0)
        cache->running = false;

    heimdall_mutex_unlock(&cache->lock);
}

void
heimdall_access_cache_Destroy(struct heimdall_access_cache *cache)
{
    heimdall_mutex_lock(&cache->lock);
    if (cache->running)
    {
        cache->running = false;
        heimdall_cond_signal(&cache->cond);
        heimdall_mutex_unlock(&cache->lock);
        heimdall_join(cache->thread, NULL);
    }
    else
        heimdall_mutex_unlock(&cache->lock);

    struct heimdall_access_cache_entry *entry;
    heimdall_list_foreach(entry, &cache->entries, node)
    {
        entry->free_cb(entry->context);
        heimdall_access_cache_entry_Delete(entry);
    }
}
#endif

void
heimdall_access_cache_AddEntry(struct heimdall_access_cache *cache,
                          struct heimdall_access_cache_entry *entry)
{
#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
    heimdall_once(&cache->once, heimdall_access_cache_InitOnce, cache);
#endif

    heimdall_mutex_lock(&cache->lock);

#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
    if (!cache->running)
    {
        heimdall_mutex_unlock(&cache->lock);
        entry->free_cb(entry->context);
        heimdall_access_cache_entry_Delete(entry);
        return;
    }
#endif

    struct heimdall_access_cache_entry *it;
    size_t count = 0;
    heimdall_list_foreach(it, &cache->entries, node)
        count++;

    if (count >= HEIMDALL_ACCESS_CACHE_MAX_ENTRY)
    {
        /* Too many entries, signal the thread that will delete the first one */
        it = heimdall_list_first_entry_or_null(&cache->entries,
                                          struct heimdall_access_cache_entry, node);
        it->timeout = 0;
    }

    entry->timeout = heimdall_tick_now() + HEIMDALL_ACCESS_CACHE_TTL;
    heimdall_list_append(&entry->node, &cache->entries);

#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
    heimdall_cond_signal(&cache->cond);
#endif
    heimdall_mutex_unlock(&cache->lock);
}

struct heimdall_access_cache_entry *
heimdall_access_cache_GetEntry(struct heimdall_access_cache *cache,
                          const char *url, const char *username)
{
#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
    heimdall_once(&cache->once, heimdall_access_cache_InitOnce, cache);
#endif

    heimdall_mutex_lock(&cache->lock);

    struct heimdall_access_cache_entry *it;

    heimdall_list_foreach(it, &cache->entries, node)
    {

        if (strcmp(url, it->url) == 0
         && (username == NULL) == (it->username == NULL)
         && (username != NULL ? strcmp(username, it->username) == 0 : true))
        {
            heimdall_list_remove(&it->node);
#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
            heimdall_cond_signal(&cache->cond);
#endif
            heimdall_mutex_unlock(&cache->lock);
            return it;
        }
    }

    heimdall_mutex_unlock(&cache->lock);

    return NULL;
}
