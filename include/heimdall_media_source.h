/*****************************************************************************
 * heimdall_media_source.h
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

#ifndef HEIMDALL_MEDIA_SOURCE_H
#define HEIMDALL_MEDIA_SOURCE_H

#include <heimdall_common.h>
#include <heimdall_input_item.h>
#include <heimdall_services_discovery.h>
#include <heimdall_preparser.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup media_source Media source
 * \ingroup input
 * @{
 */

/**
 * Media source API aims to manage "services discovery" easily from UI clients.
 *
 * A "media source provider", associated to the libheimdall instance, allows to
 * retrieve media sources (each associated to a services discovery module).
 *
 * Requesting a services discovery that is not open will automatically open it.
 * If several "clients" request the same media source (i.e. by requesting the
 * same name), they will receive the same (refcounted) media source instance.
 * As soon as a media source is released by all its clients, the associated
 * services discovery is closed.
 *
 * Each media source holds a media tree, used to store both the media
 * detected by the services discovery and the media detected by preparsing.
 * Clients may listen to the tree to be notified of changes.
 *
 * To preparse a media belonging to a media tree, use heimdall_media_tree_Preparse().
 * If subitems are detected during the preparsing, the media tree is updated
 * accordingly.
 */

/**
 * Media tree.
 *
 * Nodes must be traversed with locked held (heimdall_media_tree_Lock()).
 */
typedef struct heimdall_media_tree {
    input_item_node_t root;
} heimdall_media_tree_t;

/**
 * Callbacks to receive media tree events.
 */
struct heimdall_media_tree_callbacks
{
    /**
     * Called when the whole content of a subtree has changed.
     *
     * \param playlist the playlist
     * \param node     the node having its children reset (may be root)
     * \param userdata userdata provided to AddListener()
     */
    void
    (*on_children_reset)(heimdall_media_tree_t *tree, input_item_node_t *node,
                         void *userdata);

    /**
     * Called when children has been added to a node.
     *
     * The children may themselves contain children, which will not be notified
     * separately.
     *
     * \param playlist the playlist
     * \param node     the node having children added
     * \param children the children added
     * \param count    the number of children added
     * \param userdata userdata provided to AddListener()
     */
    void
    (*on_children_added)(heimdall_media_tree_t *tree, input_item_node_t *node,
                         input_item_node_t *const children[], size_t count,
                         void *userdata);

    /**
     * Called when children has been removed from a node.
     *
     * \param playlist the playlist
     * \param node     the node having children removed
     * \param children the children removed
     * \param count    the number of children removed
     * \param userdata userdata provided to AddListener()
     */
    void
    (*on_children_removed)(heimdall_media_tree_t *tree, input_item_node_t *node,
                           input_item_node_t *const children[], size_t count,
                           void *userdata);

    /**
     * Called when the preparsing of a node is complete
     *
     * \param tree     the media tree
     * \param node     the node being parsed
     * \param status   the reason for the preparsing termination
     * \param userdata userdata provided to AddListener()
     */
    void
    (*on_preparse_end)(heimdall_media_tree_t *tree, input_item_node_t * node,
                       int status, void *userdata);
};

/**
 * Create an empty media tree.
 */
HEIMDALL_API heimdall_media_tree_t *
heimdall_media_tree_New(void);

/**
 * Listener for media tree events.
 */
typedef struct heimdall_media_tree_listener_id heimdall_media_tree_listener_id;

/**
 * Add a listener. The lock must NOT be held.
 *
 * \param tree                 the media tree, unlocked
 * \param cbs                  the callbacks (must be valid until the listener
 *                             is removed)
 * \param userdata             userdata provided as a parameter in callbacks
 * \param notify_current_state true to notify the current state immediately via
 *                             callbacks
 */
HEIMDALL_API heimdall_media_tree_listener_id *
heimdall_media_tree_AddListener(heimdall_media_tree_t *tree,
                           const struct heimdall_media_tree_callbacks *cbs,
                           void *userdata, bool notify_current_state);

/**
 * Remove a listener. The lock must NOT be held.
 *
 * \param tree     the media tree, unlocked
 * \param listener the listener identifier returned by
 *                 heimdall_media_tree_AddListener()
 */
HEIMDALL_API void
heimdall_media_tree_RemoveListener(heimdall_media_tree_t *tree,
                              heimdall_media_tree_listener_id *listener);

/**
 * Increase the media tree reference count.
 *
 * \param tree the media tree, unlocked
 */
HEIMDALL_API void
heimdall_media_tree_Hold(heimdall_media_tree_t *tree);

/**
 * Decrease the media tree reference count.
 *
 * Destroy the media tree if it reaches 0.
 *
 * \param tree the media tree, unlocked
 */
HEIMDALL_API void
heimdall_media_tree_Release(heimdall_media_tree_t *tree);

/**
 * Lock the media tree (non-recursive).
 */
HEIMDALL_API void
heimdall_media_tree_Lock(heimdall_media_tree_t *);

/**
 * Unlock the media tree.
 */
HEIMDALL_API void
heimdall_media_tree_Unlock(heimdall_media_tree_t *);

/**
 * Add an item to the media tree.
 *
 * \param tree the media tree, locked
 * \param parent the parent node, belonging to the media tree
 * \param media the media to add as a child of `parent`
 */
HEIMDALL_API input_item_node_t *
heimdall_media_tree_Add(heimdall_media_tree_t *tree, input_item_node_t *parent,
                   input_item_t *media);

/**
 * Remove an item from the media tree.
 *
 * \param tree the media tree, locked
 * \param media the media to remove
 */
HEIMDALL_API bool
heimdall_media_tree_Remove(heimdall_media_tree_t *tree, input_item_t *media);

/**
 * Find the node containing the requested input item (and its parent).
 *
 * \param tree the media tree, locked
 * \param media the media to look for in the tree
 * \param result point to the matching node if the function returns true [OUT]
 * \param result_parent if not NULL, point to the matching node parent
 *                      if the function returns true [OUT]
 *
 * \retval true if item was found
 * \retval false if item was not found
 */
HEIMDALL_API bool
heimdall_media_tree_Find(heimdall_media_tree_t *tree, const input_item_t *media,
                    input_item_node_t **result,
                    input_item_node_t **result_parent);

/**
 * Preparse a media, and expand it in the media tree on subitems added.
 *
 * \param tree   the media tree (not necessarily locked)
 * \param parser a valid preparser
 * \param media  the media to preparse
 * \returns NULL in case of error, or a valid request handle if the
 * item was scheduled for preparsing. Cancel it with heimdall_preparser_Cancel().
 *
 * \note Ownership of the returned handle is transferred to the caller. It must
 * be released with heimdall_preparser_req_Release().
 */
HEIMDALL_API heimdall_preparser_req *
heimdall_media_tree_Preparse(heimdall_media_tree_t *tree, heimdall_preparser_t *parser,
                        input_item_t *media);

/**
 * Media source.
 *
 * A media source is associated to a "service discovery". It stores the
 * detected media in a media tree.
 */
typedef struct heimdall_media_source_t
{
    heimdall_media_tree_t *tree;
    const char *description;
} heimdall_media_source_t;

/**
 * Increase the media source reference count.
 */
HEIMDALL_API void
heimdall_media_source_Hold(heimdall_media_source_t *);

/**
 * Decrease the media source reference count.
 *
 * Destroy the media source and close the associated "service discovery" if it
 * reaches 0.
 */
HEIMDALL_API void
heimdall_media_source_Release(heimdall_media_source_t *);

/**
 * Media source provider (opaque pointer), used to get media sources.
 */
typedef struct heimdall_media_source_provider_t heimdall_media_source_provider_t;

/**
 * Return the media source provider associated to the libheimdall instance.
 */
HEIMDALL_API heimdall_media_source_provider_t *
heimdall_media_source_provider_Get(libheimdall_int_t *);

/**
 * Return the media source identified by psz_name.
 *
 * The resulting media source must be released by heimdall_media_source_Release().
 */
HEIMDALL_API heimdall_media_source_t *
heimdall_media_source_provider_GetMediaSource(heimdall_media_source_provider_t *,
                                         const char *name);

/**
 * Structure containing the description of a media source.
 */
struct heimdall_media_source_meta
{
    char *name;
    char *longname;
    enum services_discovery_category_e category;
};

/** List of media source metadata (opaque). */
typedef struct heimdall_media_source_meta_list heimdall_media_source_meta_list_t;

/**
 * Return the list of metadata of available media sources.
 *
 * If category is not 0, then only media sources for the requested category are
 * listed.
 *
 * The result must be deleted by heimdall_media_source_meta_list_Delete() (if not
 * null).
 *
 * Return NULL either on error or on empty list (this is due to the behavior
 * of the underlying heimdall_sd_GetNames()).
 *
 * \param provider the media source provider
 * \param category the category to list (0 for all)
 */
HEIMDALL_API heimdall_media_source_meta_list_t *
heimdall_media_source_provider_List(heimdall_media_source_provider_t *,
                               enum services_discovery_category_e category);

/**
 * Return the number of items in the list.
 */
HEIMDALL_API size_t
heimdall_media_source_meta_list_Count(heimdall_media_source_meta_list_t *);

/**
 * Return the item at index.
 */
HEIMDALL_API struct heimdall_media_source_meta *
heimdall_media_source_meta_list_Get(heimdall_media_source_meta_list_t *, size_t index);

/**
 * Delete the list.
 *
 * Any struct heimdall_media_source_meta retrieved from this list become invalid
 * after this call.
 */
HEIMDALL_API void
heimdall_media_source_meta_list_Delete(heimdall_media_source_meta_list_t *);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
