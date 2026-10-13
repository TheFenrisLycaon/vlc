/*****************************************************************************
 * cache.h: access cache helper
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
#include <heimdall_list.h>

struct heimdall_access_cache_entry
{
    void *context;

    char *url;
    char *username;

    heimdall_tick_t timeout;
    void (*free_cb)(void *context);

    struct heimdall_list node;
};

#ifdef __has_attribute
  #if __has_attribute(destructor)
    #define HEIMDALL_ACCESS_CACHE_CAN_REGISTER
  #endif
#endif

struct heimdall_access_cache
{
    heimdall_mutex_t lock;
#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
    heimdall_once_t once;
    heimdall_cond_t cond;
    heimdall_thread_t thread;
    bool running;
#endif

    struct heimdall_list entries;
};

#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
#define HEIMDALL_ACCESS_CACHE_INITIALIZER(name) { \
    .once = HEIMDALL_STATIC_ONCE, \
    .lock = HEIMDALL_STATIC_MUTEX, \
    .cond = HEIMDALL_STATIC_COND, \
    .running = false, \
    .entries = HEIMDALL_LIST_INITIALIZER(&name.entries), \
}
#else
#define HEIMDALL_ACCESS_CACHE_INITIALIZER(name) { \
    .lock = HEIMDALL_STATIC_MUTEX, \
    .entries = HEIMDALL_LIST_INITIALIZER(&name.entries), \
}
#endif

static inline char *
heimdall_access_cache_entry_CreateSmbUrl(const char *server, const char *share)
{
    char *url;
    if (asprintf(&url, "smb://%s/%s", server, share) == -1)
        return NULL;
    return url;
}

struct heimdall_access_cache_entry *
heimdall_access_cache_entry_New(void *context, const char *url, const char *username,
                           void (*free_cb)(void *context));

static inline struct heimdall_access_cache_entry *
heimdall_access_cache_entry_NewSmb(void *context, const char *server,
                              const char *share, const char *username,
                              void (*free_cb)(void *context))
{
    char *url = heimdall_access_cache_entry_CreateSmbUrl(server, share);
    if (url == NULL)
        return NULL;

    struct heimdall_access_cache_entry *entry =
        heimdall_access_cache_entry_New(context, url, username, free_cb);
    free(url);
    return entry;
}

/* Delete the cache entry without firing the free_cb */
void
heimdall_access_cache_entry_Delete(struct heimdall_access_cache_entry *entry);

void
heimdall_access_cache_AddEntry(struct heimdall_access_cache *cache,
                          struct heimdall_access_cache_entry *entry);

struct heimdall_access_cache_entry *
heimdall_access_cache_GetEntry(struct heimdall_access_cache *cache,
                          const char *url, const char *username);

static inline struct heimdall_access_cache_entry *
heimdall_access_cache_GetSmbEntry(struct heimdall_access_cache *cache,
                             const char *server, const char *share,
                             const char *username)
{
    char *url = heimdall_access_cache_entry_CreateSmbUrl(server, share);
    if (url == NULL)
        return NULL;

    struct heimdall_access_cache_entry *entry =
        heimdall_access_cache_GetEntry(cache, url, username);
    free(url);

    return entry;
}

#ifdef HEIMDALL_ACCESS_CACHE_CAN_REGISTER
void
heimdall_access_cache_Destroy(struct heimdall_access_cache *cache);

#define HEIMDALL_ACCESS_CACHE_REGISTER(name) \
static struct heimdall_access_cache name = HEIMDALL_ACCESS_CACHE_INITIALIZER(name); \
__attribute__((destructor)) static void heimdall_access_cache_destructor_##name(void) \
{ \
    heimdall_access_cache_Destroy(&name); \
}
#else
#define HEIMDALL_ACCESS_CACHE_REGISTER(name) \
static struct heimdall_access_cache name = HEIMDALL_ACCESS_CACHE_INITIALIZER(name);
#warning "can't register access cache"
#endif
