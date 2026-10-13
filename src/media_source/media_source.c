/*****************************************************************************
 * media_source.c
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
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

#include "media_source.h"

#include <assert.h>
#include <heimdall_atomic.h>
#include <heimdall_playlist.h>
#include <heimdall_services_discovery.h>
#include <heimdall_vector.h>
#include "../libheimdall.h"

#ifdef TEST_MEDIA_SOURCE
#define heimdall_module_name "test"
#endif /* TEST_MEDIA_SOURCE */

typedef struct
{
    heimdall_media_source_t public_data;

    services_discovery_t *sd;
    size_t rc;
    heimdall_media_source_provider_t *owner;
    struct heimdall_list node;
    char name[];
} media_source_private_t;

#define ms_priv(ms) container_of(ms, media_source_private_t, public_data)

struct heimdall_media_source_provider_t
{
    struct heimdall_object_t obj;
    heimdall_mutex_t lock;
    struct heimdall_list media_sources;
};

/* A new item has been added to a certain services discovery */
static void
services_discovery_item_added(services_discovery_t *sd,
                              input_item_t *parent, input_item_t *media)
{
    heimdall_media_source_t *ms = sd->owner.sys;
    heimdall_media_tree_t *tree = ms->tree;

    msg_Dbg(sd, "adding: %s", media->psz_name ? media->psz_name : "(null)");

    heimdall_media_tree_Lock(tree);

    input_item_node_t *parent_node;
    if (parent)
        heimdall_media_tree_Find(tree, parent, &parent_node, NULL);
    else
        parent_node = &tree->root;

    bool added = heimdall_media_tree_Add(tree, parent_node, media) != NULL;
    if (unlikely(!added))
        msg_Err(sd, "could not allocate media tree node");

    heimdall_media_tree_Unlock(tree);
}

static void
services_discovery_item_removed(services_discovery_t *sd, input_item_t *media)
{
    heimdall_media_source_t *ms = sd->owner.sys;
    heimdall_media_tree_t *tree = ms->tree;

    msg_Dbg(sd, "removing: %s", media->psz_name ? media->psz_name : "(null)");

    heimdall_media_tree_Lock(tree);
    bool removed = heimdall_media_tree_Remove(tree, media);
    heimdall_media_tree_Unlock(tree);

    if (unlikely(!removed))
    {
        msg_Err(sd, "removing item not added"); /* SD plugin bug */
        return;
    }
}

static const struct services_discovery_callbacks sd_cbs = {
    .item_added = services_discovery_item_added,
    .item_removed = services_discovery_item_removed,
};

static heimdall_media_source_t *
heimdall_media_source_New(heimdall_media_source_provider_t *provider, const char *name)
{
    media_source_private_t *priv = malloc(sizeof(*priv) + strlen(name) + 1);
    if (unlikely(!priv))
        return NULL;

    priv->rc = 1;

    heimdall_media_source_t *ms = &priv->public_data;

    /* heimdall_sd_Create() may call services_discovery_item_added(), which will read
     * its tree, so it must be initialized first */
    ms->tree = heimdall_media_tree_New();
    if (unlikely(!ms->tree))
    {
        free(priv);
        return NULL;
    }

    strcpy(priv->name, name);

    struct services_discovery_owner_t owner = {
        .cbs = &sd_cbs,
        .sys = ms,
    };

    priv->sd = heimdall_sd_Create(provider, name, &owner);
    if (unlikely(!priv->sd))
    {
        heimdall_media_tree_Release(ms->tree);
        free(priv);
        return NULL;
    }

    /* sd->description is set during heimdall_sd_Create() */
    ms->description = priv->sd->description;

    priv->owner = provider;

    return ms;
}


static void
heimdall_media_source_Delete(heimdall_media_source_t *ms)
{
    media_source_private_t *priv = ms_priv(ms);

    heimdall_sd_Destroy(priv->sd);
    heimdall_media_tree_Release(ms->tree);
    free(priv);
}

void
heimdall_media_source_Hold(heimdall_media_source_t *ms)
{
    media_source_private_t *priv = ms_priv(ms);
    heimdall_media_source_provider_t *provider = priv->owner;
    heimdall_mutex_lock(&provider->lock);
    heimdall_assert(priv->rc != 0);
    priv->rc++;
    heimdall_mutex_unlock(&provider->lock);
}

void
heimdall_media_source_Release(heimdall_media_source_t *ms)
{
    media_source_private_t *priv = ms_priv(ms);
    heimdall_media_source_provider_t *provider = priv->owner;

    heimdall_mutex_lock(&provider->lock);
    heimdall_assert(priv->rc != 0);
    priv->rc--;
    if (priv->rc != 0)
    {
        heimdall_mutex_unlock(&provider->lock);
        return;
    }
    heimdall_list_remove(&priv->node);
    heimdall_mutex_unlock(&provider->lock);

    heimdall_media_source_Delete(ms);
}

static heimdall_media_source_t *
heimdall_media_source_provider_Find(heimdall_media_source_provider_t *provider,
                               const char *name)
{
    heimdall_mutex_assert(&provider->lock);
    media_source_private_t *entry;
    heimdall_list_foreach(entry, &provider->media_sources, node)
        if (!strcmp(name, entry->name))
            return &entry->public_data;
    return NULL;
}

heimdall_media_source_provider_t *
heimdall_media_source_provider_Get(libheimdall_int_t *libheimdall)
{
    return libheimdall_priv(libheimdall)->media_source_provider;
}

static void *
CreateObject(heimdall_object_t *parent, size_t length, const char *typename)
{
#ifdef TEST_MEDIA_SOURCE
    HEIMDALL_UNUSED(parent);
    HEIMDALL_UNUSED(typename);
    return malloc(length);
#else
    return heimdall_custom_create(parent, length, typename);
#endif
}

static void
ReleaseObject(void *obj)
{
#ifdef TEST_MEDIA_SOURCE
    free(obj);
#else
    heimdall_object_delete((heimdall_media_source_provider_t *)obj);
#endif
}

#undef heimdall_media_source_provider_New
heimdall_media_source_provider_t *
heimdall_media_source_provider_New(heimdall_object_t *parent)
{
    heimdall_media_source_provider_t *provider =
            CreateObject(parent, sizeof(*provider), "media-source-provider");
    if (unlikely(!provider))
        return NULL;

    heimdall_mutex_init(&provider->lock);
    heimdall_list_init(&provider->media_sources);
    return provider;
}

void
heimdall_media_source_provider_Delete(heimdall_media_source_provider_t *provider)
{
    ReleaseObject(provider);
}

static heimdall_media_source_t *
heimdall_media_source_provider_Add(heimdall_media_source_provider_t *provider,
                              const char *name)
{
    heimdall_mutex_assert(&provider->lock);

    heimdall_media_source_t *ms = heimdall_media_source_New(provider, name);
    if (unlikely(!ms))
        return NULL;

    heimdall_list_append(&ms_priv(ms)->node, &provider->media_sources);
    return ms;
}

heimdall_media_source_t *
heimdall_media_source_provider_GetMediaSource(heimdall_media_source_provider_t *provider,
                                         const char *name)
{
    heimdall_mutex_lock(&provider->lock);
    heimdall_media_source_t *ms = heimdall_media_source_provider_Find(provider, name);
    if (ms)
    {
        media_source_private_t *priv = ms_priv(ms);
        heimdall_assert(priv->rc != 0);
        priv->rc++;
    }
    else
        ms = heimdall_media_source_provider_Add(provider, name);
    heimdall_mutex_unlock(&provider->lock);

    return ms;
}

struct heimdall_media_source_meta_list
{
    struct HEIMDALL_VECTOR(struct heimdall_media_source_meta) vec;
};

struct heimdall_media_source_meta_list *
heimdall_media_source_provider_List(heimdall_media_source_provider_t *provider,
                               enum services_discovery_category_e category)
{
    char **longnames;
    int *categories;
    char **names = heimdall_sd_GetNames(provider, &longnames, &categories);
    if (!names)
        /* heimdall_sd_GetNames() returns NULL both on error or no result */
        return NULL;

    struct heimdall_media_source_meta_list *list = malloc(sizeof(*list));
    if (unlikely(!list))
        return NULL;

    heimdall_vector_init(&list->vec);
    for (size_t i = 0; names[i]; ++i)
    {
        if (category && categories[i] != (int) category)
        {
            free(names[i]);
            free(longnames[i]);
            /* only list items for the requested category */
            continue;
        }

        struct heimdall_media_source_meta meta = {
            .name = names[i],
            .longname = longnames[i],
            .category = categories[i],
        };
        bool ok = heimdall_vector_push(&list->vec, meta);
        if (unlikely(!ok)) {
            /* failure, clean up */
            for (char **p = names; *p; ++p)
                free(*p);
            for (char **p = longnames; *p; ++p)
                free(*p);
            heimdall_vector_destroy(&list->vec);
            free(list);
            list = NULL;
            break;
        }
    }

    free(names);
    free(longnames);
    free(categories);

    return list;
}

size_t
heimdall_media_source_meta_list_Count(heimdall_media_source_meta_list_t *list)
{
    return list->vec.size;
}

struct heimdall_media_source_meta *
heimdall_media_source_meta_list_Get(heimdall_media_source_meta_list_t *list, size_t index)
{
    return &list->vec.data[index];
}

void
heimdall_media_source_meta_list_Delete(heimdall_media_source_meta_list_t *list) {
    for (size_t i = 0; i < list->vec.size; ++i)
    {
        free(list->vec.data[i].name);
        free(list->vec.data[i].longname);
    }
    heimdall_vector_destroy(&list->vec);
    free(list);
}
