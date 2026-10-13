/*****************************************************************************
 * libheimdall_media_discoverer.h:  libheimdall external API
 *****************************************************************************
 * Copyright (C) 1998-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
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

#ifndef HEIMDALL_LIBHEIMDALL_MEDIA_DISCOVERER_H
#define HEIMDALL_LIBHEIMDALL_MEDIA_DISCOVERER_H 1

# ifdef __cplusplus
extern "C" {
# endif

typedef struct libheimdall_media_t libheimdall_media_t;

/**
 * Category of a media discoverer
 * \see libheimdall_media_discoverer_list_get()
 */
typedef enum libheimdall_media_discoverer_category_t {
    /** devices, like portable music player */
    libheimdall_media_discoverer_devices,
    /** LAN/WAN services, like Upnp, SMB, or SAP */
    libheimdall_media_discoverer_lan,
    /** Podcasts */
    libheimdall_media_discoverer_podcasts,
    /** Local directories, like Video, Music or Pictures directories */
    libheimdall_media_discoverer_localdirs,
} libheimdall_media_discoverer_category_t;

/**
 * Media discoverer description
 * \see libheimdall_media_discoverer_list_get()
 */
typedef struct libheimdall_media_discoverer_description_t {
    char *psz_name;
    char *psz_longname;
    libheimdall_media_discoverer_category_t i_cat;
} libheimdall_media_discoverer_description_t;

/** \defgroup libheimdall_media_discoverer LibHeimdall media discovery
 * \ingroup libheimdall
 * LibHeimdall media discovery finds available media via various means.
 * This corresponds to the service discovery functionality in Heimdall media player.
 * Different plugins find potential medias locally (e.g. user media directory),
 * from peripherals (e.g. video capture device), on the local network
 * (e.g. SAP) or on the Internet (e.g. Internet radios).
 * @{
 * \file
 * LibHeimdall media discovery external API
 */

typedef struct libheimdall_media_discoverer_t libheimdall_media_discoverer_t;

/**
 * struct defining callbacks for libheimdall_media_discoverer_new()
 */
struct libheimdall_media_discoverer_cbs {
    /** 
     * Version of struct libheimdall_media_discoverer_cbs
     */
    uint32_t version;

    /**
     * Callback prototype that notify when the discoverer added a media
     *
     * \note Optional (can be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set by libheimdall_media_discoverer_new()
     * \param parent parent of the new added media or NULL if there is no
     * parents (more likely)
     * \param media the new added media
     */
    void (*on_media_added)(void *opaque, libheimdall_media_t *parent,
                           libheimdall_media_t *media);

    /**
     * Callback prototype that notify when the discoverer removed a media
     *
     * \note Optional (can be NULL),
     * available since version 0
     *
     * \param opaque opaque pointer set by libheimdall_media_discoverer_new()
     * \param media the removed media
     */
    void (*on_media_removed)(void *opaque, libheimdall_media_t *media);
};

/**
 * Create a media discoverer object by name.
 *
 * You need to call libheimdall_media_discoverer_start() in order to start the
 * discovery.
 *
 * \see libheimdall_media_discoverer_start
 *
 * \param p_inst libheimdall instance
 * \param psz_name service name; use libheimdall_media_discoverer_list_get() to get
 * a list of the discoverer names available in this libHeimdall instance
 * \param cbs callback to listen to events (can be NULL). The pointed
 * struct must be kept alive (and not modified) by the caller until the
 * returned object is destroyed with libheimdall_media_discoverer_destroy().
 * \param cbs_opaque opaque pointer used by the callbacks
 * \return media discover object or NULL in case of error
 * \version LibHeimdall 3.0.0 or later
 */
LIBHEIMDALL_API libheimdall_media_discoverer_t *
libheimdall_media_discoverer_new( libheimdall_instance_t * p_inst,
                             const char * psz_name,
                             const struct libheimdall_media_discoverer_cbs *cbs,
                             void *cbs_opaque );

/**
 * Start media discovery.
 *
 * To stop it, call libheimdall_media_discoverer_stop() or
 * libheimdall_media_discoverer_destroy() directly.
 *
 * \see libheimdall_media_discoverer_stop
 *
 * \param p_mdis media discover object
 * \return -1 in case of error, 0 otherwise
 * \version LibHeimdall 3.0.0 or later
 */
LIBHEIMDALL_API int
libheimdall_media_discoverer_start( libheimdall_media_discoverer_t * p_mdis );

/**
 * Stop media discovery.
 *
 * \see libheimdall_media_discoverer_start
 *
 * \param p_mdis media discover object
 * \version LibHeimdall 3.0.0 or later
 */
LIBHEIMDALL_API void
libheimdall_media_discoverer_stop( libheimdall_media_discoverer_t * p_mdis );

/**
 * Destroy a media discoverer object. If the discovery is running, it will be
 * stopped first.
 *
 * \param p_mdis media service discover object
 * \version LibHeimdall 4.0.0 or later
 */
LIBHEIMDALL_API void
libheimdall_media_discoverer_destroy( libheimdall_media_discoverer_t * p_mdis );

/**
 * Query if media service discover object is running.
 *
 * \param p_mdis media service discover object
 *
 * \retval true running
 * \retval false not running
 */
LIBHEIMDALL_API bool
libheimdall_media_discoverer_is_running(libheimdall_media_discoverer_t *p_mdis);

/**
 * Get media discoverer services by category
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \param p_inst libheimdall instance
 * \param i_cat category of services to fetch
 * \param ppp_services address to store an allocated array of media discoverer
 * services (must be freed with libheimdall_media_discoverer_list_release() by
 * the caller) [OUT]
 *
 * \return the number of media discoverer services (0 on error)
 */
LIBHEIMDALL_API size_t
libheimdall_media_discoverer_list_get( libheimdall_instance_t *p_inst,
                                  libheimdall_media_discoverer_category_t i_cat,
                                  libheimdall_media_discoverer_description_t ***ppp_services );

/**
 * Release an array of media discoverer services
 *
 * \version LibHeimdall 3.0.0 and later.
 *
 * \see libheimdall_media_discoverer_list_get()
 *
 * \param pp_services array to release
 * \param i_count number of elements in the array
 */
LIBHEIMDALL_API void
libheimdall_media_discoverer_list_release( libheimdall_media_discoverer_description_t **pp_services,
                                      size_t i_count );

/**@} */

# ifdef __cplusplus
}
# endif

#endif /* <heimdall/libheimdall.h> */
