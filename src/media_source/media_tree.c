/*****************************************************************************
 * media_tree.c
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

#include <heimdall_media_source.h>

#include <assert.h>
#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_atomic.h>
#include <heimdall_input_item.h>
#include <heimdall_threads.h>
#include <heimdall_preparser.h>
#include "../libheimdall.h"

struct heimdall_media_tree_listener_id
{
    const struct heimdall_media_tree_callbacks *cbs;
    void *userdata;
    struct heimdall_list node; /**< node of media_tree_private_t.listeners */
};

typedef struct
{
    heimdall_media_tree_t public_data;

    struct heimdall_list listeners; /**< list of heimdall_media_tree_listener_id.node */
    heimdall_mutex_t lock;
    heimdall_atomic_rc_t rc;
} media_tree_private_t;

#define mt_priv(mt) container_of(mt, media_tree_private_t, public_data)

heimdall_media_tree_t *
heimdall_media_tree_New(void)
{
    media_tree_private_t *priv = malloc(sizeof(*priv));
    if (unlikely(!priv))
        return NULL;

    heimdall_mutex_init(&priv->lock);
    heimdall_atomic_rc_init(&priv->rc);
    heimdall_list_init(&priv->listeners);

    heimdall_media_tree_t *tree = &priv->public_data;
    input_item_node_t *root = &tree->root;
    root->p_item = NULL;
    TAB_INIT(root->i_children, root->pp_children);

    return tree;
}

static inline void
heimdall_media_tree_AssertLocked(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    heimdall_mutex_assert(&priv->lock);
}

#define heimdall_media_tree_listener_foreach(listener, tree) \
    heimdall_list_foreach(listener, &mt_priv(tree)->listeners, node)

#define heimdall_media_tree_NotifyListener(tree, listener, event, ...) \
do { \
    if (listener->cbs->event) \
        listener->cbs->event(tree, ##__VA_ARGS__, listener->userdata); \
} while(0)

#define heimdall_media_tree_Notify(tree, event, ...) \
do { \
    heimdall_media_tree_AssertLocked(tree); \
    heimdall_media_tree_listener_id *listener; \
    heimdall_media_tree_listener_foreach(listener, tree) \
        heimdall_media_tree_NotifyListener(tree, listener, event, ##__VA_ARGS__); \
} while (0)

static bool
heimdall_media_tree_FindNodeByMedia(input_item_node_t *parent,
                               const input_item_t *media,
                               input_item_node_t **result,
                               input_item_node_t **result_parent)
{
    for (int i = 0; i < parent->i_children; ++i)
    {
        input_item_node_t *child = parent->pp_children[i];
        if (child->p_item == media)
        {
            *result = child;
            if (result_parent)
                *result_parent = parent;
            return true;
        }

        if (heimdall_media_tree_FindNodeByMedia(child, media, result, result_parent))
            return true;
    }

    return false;
}

static void
heimdall_media_tree_ClearChildren(input_item_node_t *root)
{
    for (int i = 0; i < root->i_children; ++i)
        input_item_node_Delete(root->pp_children[i]);

    free(root->pp_children);
    root->pp_children = NULL;
    root->i_children = 0;
}

static void
media_subtree_changed(heimdall_preparser_req *req, input_item_node_t *node,
                      void *userdata)
{
    input_item_t *media = heimdall_preparser_req_GetItem(req);
    heimdall_media_tree_t *tree = userdata;

    heimdall_media_tree_Lock(tree);
    input_item_node_t *subtree_root;
    /* TODO retrieve the node without traversing the tree */
    bool found = heimdall_media_tree_FindNodeByMedia(&tree->root, media,
                                                &subtree_root, NULL);
    if (!found) {
        /* the node probably failed to be allocated */
        heimdall_media_tree_Unlock(tree);
        return;
    }

    heimdall_media_tree_ClearChildren(subtree_root);
    /* The nodes can be directly used, as the subtree callback is given
     * ownership of the input item node. */
    subtree_root->pp_children = node->pp_children;
    subtree_root->i_children = node->i_children;
    node->pp_children = NULL;
    node->i_children = 0;
    input_item_node_Delete(node);
    heimdall_media_tree_Notify(tree, on_children_reset, subtree_root);
    heimdall_media_tree_Unlock(tree);
}

static void
media_subtree_preparse_ended(heimdall_preparser_req *req, int status, void *user_data)
{
    input_item_t *media = heimdall_preparser_req_GetItem(req);
    heimdall_media_tree_t *tree = user_data;

    heimdall_media_tree_Lock(tree);
    input_item_node_t *subtree_root;
    /* TODO retrieve the node without traversing the tree */
    bool found = heimdall_media_tree_FindNodeByMedia(&tree->root, media,
                                                &subtree_root, NULL);
    if (!found) {
        /* the node probably failed to be allocated */
        heimdall_media_tree_Unlock(tree);
        return;
    }
    heimdall_media_tree_Notify(tree, on_preparse_end, subtree_root, status);
    heimdall_media_tree_Unlock(tree);
}

static inline void
heimdall_media_tree_DestroyRootNode(heimdall_media_tree_t *tree)
{
    heimdall_media_tree_ClearChildren(&tree->root);
}

static void
heimdall_media_tree_Delete(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    heimdall_media_tree_listener_id *listener;
    heimdall_list_foreach(listener, &priv->listeners, node)
        free(listener);
    heimdall_list_init(&priv->listeners); /* reset */
    heimdall_media_tree_DestroyRootNode(tree);
    free(tree);
}

void
heimdall_media_tree_Hold(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    heimdall_atomic_rc_inc(&priv->rc);
}

void
heimdall_media_tree_Release(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    if (heimdall_atomic_rc_dec(&priv->rc))
        heimdall_media_tree_Delete(tree);
}

void
heimdall_media_tree_Lock(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    heimdall_mutex_lock(&priv->lock);
}

void
heimdall_media_tree_Unlock(heimdall_media_tree_t *tree)
{
    media_tree_private_t *priv = mt_priv(tree);
    heimdall_mutex_unlock(&priv->lock);
}

static input_item_node_t *
heimdall_media_tree_AddChild(input_item_node_t *parent, input_item_t *media)
{
    input_item_node_t *node = input_item_node_Create(media);
    if (unlikely(!node))
        return NULL;

    input_item_node_AppendNode(parent, node);

    return node;
}

static void
heimdall_media_tree_NotifyCurrentState(heimdall_media_tree_t *tree,
                                  heimdall_media_tree_listener_id *listener)
{
    heimdall_media_tree_NotifyListener(tree, listener, on_children_reset,
                                  &tree->root);
}

heimdall_media_tree_listener_id *
heimdall_media_tree_AddListener(heimdall_media_tree_t *tree,
                           const struct heimdall_media_tree_callbacks *cbs,
                           void *userdata, bool notify_current_state)
{
    heimdall_media_tree_listener_id *listener = malloc(sizeof(*listener));
    if (unlikely(!listener))
        return NULL;
    listener->cbs = cbs;
    listener->userdata = userdata;

    media_tree_private_t *priv = mt_priv(tree);
    heimdall_media_tree_Lock(tree);

    heimdall_list_append(&listener->node, &priv->listeners);

    if (notify_current_state)
        heimdall_media_tree_NotifyCurrentState(tree, listener);

    heimdall_media_tree_Unlock(tree);
    return listener;
}

void
heimdall_media_tree_RemoveListener(heimdall_media_tree_t *tree,
                              heimdall_media_tree_listener_id *listener)
{
    heimdall_media_tree_Lock(tree);
    heimdall_list_remove(&listener->node);
    heimdall_media_tree_Unlock(tree);

    free(listener);
}

input_item_node_t *
heimdall_media_tree_Add(heimdall_media_tree_t *tree, input_item_node_t *parent,
                   input_item_t *media)
{
    heimdall_media_tree_AssertLocked(tree);

    input_item_node_t *node = heimdall_media_tree_AddChild(parent, media);
    if (unlikely(!node))
        return NULL;

    heimdall_media_tree_Notify(tree, on_children_added, parent, &node, 1);

    return node;
}

bool
heimdall_media_tree_Find(heimdall_media_tree_t *tree, const input_item_t *media,
                    input_item_node_t **result,
                    input_item_node_t **result_parent)
{
    heimdall_media_tree_AssertLocked(tree);

    /* quick & dirty depth-first O(n) implementation, with n the number of nodes
     * in the tree */
    return heimdall_media_tree_FindNodeByMedia(&tree->root, media, result,
                                          result_parent);
}

bool
heimdall_media_tree_Remove(heimdall_media_tree_t *tree, input_item_t *media)
{
    heimdall_media_tree_AssertLocked(tree);

    input_item_node_t *node;
    input_item_node_t *parent;
    if (!heimdall_media_tree_FindNodeByMedia(&tree->root, media, &node, &parent))
        return false;

    input_item_node_RemoveNode(parent, node);
    heimdall_media_tree_Notify(tree, on_children_removed, parent, &node, 1);
    input_item_node_Delete(node);
    return true;
}

static const struct heimdall_preparser_cbs preparser_callbacks = {
    .on_ended = media_subtree_preparse_ended,
    .on_subtree_added = media_subtree_changed,
};

heimdall_preparser_req *
heimdall_media_tree_Preparse(heimdall_media_tree_t *tree, heimdall_preparser_t *parser,
                        input_item_t *media)
{
#ifdef TEST_MEDIA_SOURCE
    HEIMDALL_UNUSED(tree);
    HEIMDALL_UNUSED(parser);
    HEIMDALL_UNUSED(media);
    HEIMDALL_UNUSED(preparser_callbacks);
    return NULL;
#else
    heimdall_preparser_req *req =
        heimdall_preparser_req_NewParse(parser, media, HEIMDALL_PREPARSER_TYPE_PARSE |
                                   HEIMDALL_PREPARSER_OPTION_INTERACT |
                                   HEIMDALL_PREPARSER_OPTION_SUBITEMS,
                                   &preparser_callbacks, tree);
    if (req == NULL)
        return NULL;

    if (heimdall_preparser_Submit(parser, req) != HEIMDALL_SUCCESS)
    {
        heimdall_preparser_req_Release(req);
        return NULL;
    }

    return req;
#endif
}
