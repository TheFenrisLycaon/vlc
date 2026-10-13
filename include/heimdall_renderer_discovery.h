/*****************************************************************************
 * heimdall_renderer_discovery.h : Renderer Discovery functions
 *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
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

#ifndef HEIMDALL_RENDERER_DISCOVERY_H
#define HEIMDALL_RENDERER_DISCOVERY_H 1

#include <heimdall_input.h>
#include <heimdall_probe.h>
#include <heimdall_url.h>

/**
 * @defgroup heimdall_renderer Heimdall renderer discovery
 * @ingroup interface
 * @{
 *
 * @file
 * This file declares Heimdall renderer discvoery structures and functions
 *
 * @defgroup heimdall_renderer_item Heimdall renderer items returned by the discovery
 * @{
 */

#define HEIMDALL_RENDERER_CAN_AUDIO 0x0001
#define HEIMDALL_RENDERER_CAN_VIDEO 0x0002

/**
 * Create a new renderer item
 *
 * @param psz_type type of the item
 * @param psz_name name of the item
 * @param psz_uri uri of the renderer item, must contains a valid protocol and
 * a valid host
 * @param psz_extra_sout extra sout options
 * @param psz_demux_filter demux filter to use with the renderer
 * @param psz_icon_uri icon uri of the renderer item
 * @param i_flags flags for the item
 * @return a renderer item or NULL in case of error
 */
HEIMDALL_API heimdall_renderer_item_t *
heimdall_renderer_item_new(const char *psz_type, const char *psz_name,
                      const char *psz_uri, const char *psz_extra_sout,
                      const char *psz_demux_filter, const char *psz_icon_uri,
                      int i_flags) HEIMDALL_USED;

/**
 * Hold a renderer item, i.e. creates a new reference
 */
HEIMDALL_API heimdall_renderer_item_t *
heimdall_renderer_item_hold(heimdall_renderer_item_t *p_item);

/**
 * Releases a renderer item, i.e. decrements its reference counter
 */
HEIMDALL_API void
heimdall_renderer_item_release(heimdall_renderer_item_t *p_item);

/**
 * Get the human readable name of a renderer item
 */
HEIMDALL_API const char *
heimdall_renderer_item_name(const heimdall_renderer_item_t *p_item);

/**
 * Get the type (not translated) of a renderer item. For now, the type can only
 * be "chromecast" ("upnp", "airplay" may come later).
 */
HEIMDALL_API const char *
heimdall_renderer_item_type(const heimdall_renderer_item_t *p_item);

/**
 * Get the demux filter to use with a renderer item
 */
HEIMDALL_API const char *
heimdall_renderer_item_demux_filter(const heimdall_renderer_item_t *p_item);

/**
 * Get the sout command of a renderer item
 */
HEIMDALL_API const char *
heimdall_renderer_item_sout(const heimdall_renderer_item_t *p_item);

/**
 * Get the icon uri of a renderer item
 */
HEIMDALL_API const char *
heimdall_renderer_item_icon_uri(const heimdall_renderer_item_t *p_item);

/**
 * Get the flags of a renderer item
 */
HEIMDALL_API int
heimdall_renderer_item_flags(const heimdall_renderer_item_t *p_item);

/**
 * @}
 * @defgroup heimdall_renderer_discovery Heimdall renderer discovery interface
 * @{
 */

struct heimdall_renderer_discovery_owner;

/**
 * Return a list of renderer discovery modules
 *
 * @param p_obj any Heimdall object to get a libheimdall instance from TODO
 * @param pppsz_names a pointer to a list of module name, NULL terminated
 * @param pppsz_longnames a pointer to a list of module longname, NULL
 * terminated
 *
 * @return HEIMDALL_SUCCESS on success, or HEIMDALL_EGENERIC on error
 */
HEIMDALL_API int
heimdall_rd_get_names(heimdall_object_t *p_obj, char ***pppsz_names,
                 char ***pppsz_longnames) HEIMDALL_USED;
#define heimdall_rd_get_names(a, b, c) \
        heimdall_rd_get_names(HEIMDALL_OBJECT(a), b, c)

/**
 * Create a new renderer discovery module
 *
 * @param p_obj the parent Heimdall object the variables will be inherited from
 * @param psz_name name of the module to load, see heimdall_rd_get_names() to get
 * the list of names
 * @param owner owner object with callback that the renderer discovery will
 * notify when new renderers are found or removed
 *
 * @return a valid heimdall_renderer_discovery, need to be released with
 * heimdall_rd_release()
 */
HEIMDALL_API heimdall_renderer_discovery_t *
heimdall_rd_new(heimdall_object_t *p_obj, const char *psz_name,
           const struct heimdall_renderer_discovery_owner *owner) HEIMDALL_USED;

HEIMDALL_API void heimdall_rd_release(heimdall_renderer_discovery_t *p_rd);

/**
 * @}
 * @defgroup heimdall_renderer_discovery_module Heimdall renderer module
 * @{
 */

struct heimdall_renderer_discovery_owner
{
    void *sys;
    void (*item_added)(struct heimdall_renderer_discovery_t *,
                       struct heimdall_renderer_item_t *);
    void (*item_removed)(struct heimdall_renderer_discovery_t *,
                         struct heimdall_renderer_item_t *);
};

struct heimdall_renderer_discovery_t
{
    struct heimdall_object_t obj;
    module_t *          p_module;

    struct heimdall_renderer_discovery_owner owner;

    char *              psz_name;
    config_chain_t *    p_cfg;

    void *p_sys;
};

/**
 * Add a new renderer item
 *
 * This will send the heimdall_RendererDiscoveryItemAdded event
 */
static inline void heimdall_rd_add_item(heimdall_renderer_discovery_t * p_rd,
                                   heimdall_renderer_item_t * p_item)
{
    p_rd->owner.item_added(p_rd, p_item);
}

/**
 * Add a new renderer item
 *
 * This will send the heimdall_RendererDiscoveryItemRemoved event
 */
static inline void heimdall_rd_remove_item(heimdall_renderer_discovery_t * p_rd,
                                      heimdall_renderer_item_t * p_item)
{
    p_rd->owner.item_removed(p_rd, p_item);
}

/**
 * Renderer Discovery probe helpers
 */
HEIMDALL_API int
heimdall_rd_probe_add(heimdall_probe_t *p_probe, const char *psz_name,
                 const char *psz_longname);

#define HEIMDALL_RD_PROBE_HELPER(name, longname) \
static int heimdall_rd_probe_open(heimdall_object_t *obj) \
{ \
    return heimdall_rd_probe_add((struct heimdall_probe_t *)obj, name, longname); \
}

#define HEIMDALL_RD_PROBE_SUBMODULE \
    add_submodule() \
        set_capability("renderer probe", 100) \
        set_callback(heimdall_rd_probe_open)

/** @} */
/** @} */

#endif
