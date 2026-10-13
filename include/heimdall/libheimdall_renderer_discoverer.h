/*****************************************************************************
 * libheimdall_renderer_discoverer.h:  libheimdall external API
 *****************************************************************************
 * Copyright © 2016 VLC authors and VideoLAN
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

#ifndef HEIMDALL_LIBHEIMDALL_RENDERER_DISCOVERER_H
#define HEIMDALL_LIBHEIMDALL_RENDERER_DISCOVERER_H 1

# ifdef __cplusplus
extern "C" {
# endif

/**
 * @defgroup libheimdall_renderer_discoverer LibHeimdall renderer discoverer
 * @ingroup libheimdall
 * LibHeimdall renderer discoverer finds available renderers available on the local
 * network
 * @{
 * @file
 * LibHeimdall renderer discoverer external API
 */

typedef struct libheimdall_renderer_discoverer_t libheimdall_renderer_discoverer_t;

/**
 * Renderer discoverer description
 *
 * \see libheimdall_renderer_discoverer_list_get()
 */
typedef struct libheimdall_rd_description_t
{
    char *psz_name;
    char *psz_longname;
} libheimdall_rd_description_t;

/** The renderer can render audio */
#define LIBHEIMDALL_RENDERER_CAN_AUDIO 0x0001
/** The renderer can render video */
#define LIBHEIMDALL_RENDERER_CAN_VIDEO 0x0002

/**
 * Renderer item
 *
 * This struct is passed by a @ref libheimdall_renderer_discoverer_cbs when a new
 * renderer is added or deleted.
 *
 * An item is valid until the @ref libheimdall_renderer_discoverer_cbs::on_item_removed
 * callback is called with the same pointer.
 */
typedef struct libheimdall_renderer_item_t libheimdall_renderer_item_t;

/**
 * Hold a renderer item, i.e. creates a new reference
 *
 * This functions need to called from the libheimdall_renderer_discoverer_cbs::on_item_added
 * callback if the libheimdall user wants to use this item after. (for display or
 * for passing it to the mediaplayer for example).
 *
 * \version LibHeimdall 4.0.0 or later
 *
 * \return the current item
 */
LIBHEIMDALL_API libheimdall_renderer_item_t *
libheimdall_renderer_item_retain(libheimdall_renderer_item_t *p_item);

/**
 * Releases a renderer item, i.e. decrements its reference counter
 *
 * \version LibHeimdall 3.0.0 or later
 */
LIBHEIMDALL_API void
libheimdall_renderer_item_release(libheimdall_renderer_item_t *p_item);

/**
 * Get the human readable name of a renderer item
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \return the name of the item (can't be NULL, must *not* be freed)
 */
LIBHEIMDALL_API const char *
libheimdall_renderer_item_name(const libheimdall_renderer_item_t *p_item);

/**
 * Get the type (not translated) of a renderer item. For now, the type can only
 * be "chromecast" ("upnp", "airplay" may come later).
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \return the type of the item (can't be NULL, must *not* be freed)
 */
LIBHEIMDALL_API const char *
libheimdall_renderer_item_type(const libheimdall_renderer_item_t *p_item);

/**
 * Get the icon uri of a renderer item
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \return the uri of the item's icon (can be NULL, must *not* be freed)
 */
LIBHEIMDALL_API const char *
libheimdall_renderer_item_icon_uri(const libheimdall_renderer_item_t *p_item);

/**
 * Get the flags of a renderer item
 *
 * \see LIBHEIMDALL_RENDERER_CAN_AUDIO
 * \see LIBHEIMDALL_RENDERER_CAN_VIDEO
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \return bitwise flag: capabilities of the renderer, see
 */
LIBHEIMDALL_API int
libheimdall_renderer_item_flags(const libheimdall_renderer_item_t *p_item);

/**
 * struct defining callbacks for libheimdall_renderer_discoverer_new()
 */
struct libheimdall_renderer_discoverer_cbs
{
    /**
     * Version of struct libheimdall_renderer_discoverer_cbs
     */
    uint32_t version;

    /**
     * Callback prototype that notify when the discoverer added an item
     *
     * \note Mandatory (can't be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set by libheimdall_renderer_discoverer_new()
     * \param item the new added item
     */
    void (*on_item_added)(void *opaque, libheimdall_renderer_item_t *item);

    /**
     * Callback prototype that notify when the discoverer removed an item
     *
     * \note Optional (can be NULL).
     * available since version 0
     *
     * \param opaque opaque pointer set by libheimdall_renderer_discoverer_new()
     * \param item the removed item
     */
    void (*on_item_removed)(void *opaque, libheimdall_renderer_item_t *item);
};

/**
 * Create a renderer discoverer object by name
 *
 * After this object is created, items addition and removal will be
 * notified through the provided callbacks.
 *
 * You need to call libheimdall_renderer_discoverer_start() in order to start the
 * discovery.
 *
 * \see libheimdall_renderer_discoverer_start()
 *
 * \version LibHeimdall 4.0.0 or later
 *
 * \param p_inst libheimdall instance
 * \param psz_name service name; use libheimdall_renderer_discoverer_list_get() to
 * get a list of the discoverer names available in this libHeimdall instance
 * \param cbs callback to listen to events (can't be NULL). The pointed
 * struct must be kept alive (and not modified) by the caller until the
 * returned object is destroyed with libheimdall_renderer_discoverer_destroy().
 * \param cbs_opaque opaque pointer used by the callbacks
 * \return media discover object or NULL in case of error
 */
LIBHEIMDALL_API libheimdall_renderer_discoverer_t *
libheimdall_renderer_discoverer_new( libheimdall_instance_t *p_inst,
                                const char *psz_name,
                                const struct libheimdall_renderer_discoverer_cbs *cbs,
                                void *cbs_opaque );

/**
 * Destroy a renderer discoverer object
 *
 * \version LibHeimdall 4.0.0 or later
 *
 * \param p_rd renderer discoverer object
 */
LIBHEIMDALL_API void
libheimdall_renderer_discoverer_destroy( libheimdall_renderer_discoverer_t *p_rd );

/**
 * Start renderer discovery
 *
 * To stop it, call libheimdall_renderer_discoverer_stop() or
 * libheimdall_renderer_discoverer_destroy() directly.
 *
 * \see libheimdall_renderer_discoverer_stop()
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \param p_rd renderer discoverer object
 * \return -1 in case of error, 0 otherwise
 */
LIBHEIMDALL_API int
libheimdall_renderer_discoverer_start( libheimdall_renderer_discoverer_t *p_rd );

/**
 * Stop renderer discovery.
 *
 * \see libheimdall_renderer_discoverer_start()
 *
 * \version LibHeimdall 3.0.0 or later
 *
 * \param p_rd renderer discoverer object
 */
LIBHEIMDALL_API void
libheimdall_renderer_discoverer_stop( libheimdall_renderer_discoverer_t *p_rd );

/**
 * Get media discoverer services
 *
 * \see libheimdall_renderer_list_release()
 *
 * \version LibHeimdall 3.0.0 and later
 *
 * \param p_inst libheimdall instance
 * \param ppp_services address to store an allocated array of renderer
 * discoverer services (must be freed with libheimdall_renderer_list_release() by
 * the caller) [OUT]
 *
 * \return the number of media discoverer services (0 on error)
 */
LIBHEIMDALL_API size_t
libheimdall_renderer_discoverer_list_get( libheimdall_instance_t *p_inst,
                                     libheimdall_rd_description_t ***ppp_services );

/**
 * Release an array of media discoverer services
 *
 * \see libheimdall_renderer_discoverer_list_get()
 *
 * \version LibHeimdall 3.0.0 and later
 *
 * \param pp_services array to release
 * \param i_count number of elements in the array
 */
LIBHEIMDALL_API void
libheimdall_renderer_discoverer_list_release( libheimdall_rd_description_t **pp_services,
                                         size_t i_count );

/** @} */

# ifdef __cplusplus
}
# endif

#endif
