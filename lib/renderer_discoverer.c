/*****************************************************************************
 * renderer_discoverer.c: libheimdall renderer API
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_renderer_discoverer.h>

#include <heimdall_common.h>
#include <heimdall_arrays.h>

#include "libheimdall_internal.h"
#include "renderer_discoverer_internal.h"

struct libheimdall_renderer_discoverer_t
{
    heimdall_object_t *          p_object;
    heimdall_renderer_discovery_t *p_rd;

    const struct libheimdall_renderer_discoverer_cbs *cbs;
    void *cbs_opaque;

    int                     i_items;
    heimdall_renderer_item_t **  pp_items;
    char                    name[];
};

static_assert( HEIMDALL_RENDERER_CAN_AUDIO == LIBHEIMDALL_RENDERER_CAN_AUDIO &&
               HEIMDALL_RENDERER_CAN_VIDEO == LIBHEIMDALL_RENDERER_CAN_VIDEO,
              "core/libheimdall renderer flags mismatch" );

heimdall_renderer_item_t *
libheimdall_renderer_item_to_heimdall( libheimdall_renderer_item_t *p_item )
{
    return (heimdall_renderer_item_t*) p_item;
}

static void renderer_discovery_item_added( heimdall_renderer_discovery_t *rd,
                                           heimdall_renderer_item_t *p_item )
{
    libheimdall_renderer_discoverer_t *p_lrd = rd->owner.sys;

    heimdall_renderer_item_hold( p_item );

    TAB_APPEND( p_lrd->i_items, p_lrd->pp_items, p_item );

    p_lrd->cbs->on_item_added( p_lrd->cbs_opaque,
                               (libheimdall_renderer_item_t*) p_item );
}

static void renderer_discovery_item_removed( heimdall_renderer_discovery_t *rd,
                                             heimdall_renderer_item_t *p_item )
{
    libheimdall_renderer_discoverer_t *p_lrd = rd->owner.sys;

    int i_idx;
    TAB_FIND( p_lrd->i_items, p_lrd->pp_items, p_item, i_idx );
    assert( i_idx != -1 );
    TAB_ERASE( p_lrd->i_items, p_lrd->pp_items, i_idx );

    if( p_lrd->cbs->on_item_removed != NULL )
        p_lrd->cbs->on_item_removed( p_lrd->cbs_opaque,
                                     (libheimdall_renderer_item_t*) p_item );

    heimdall_renderer_item_release( p_item );
}

libheimdall_renderer_item_t *
libheimdall_renderer_item_retain(libheimdall_renderer_item_t *p_item)
{
    heimdall_renderer_item_hold( (heimdall_renderer_item_t *) p_item );
    return p_item;
}

void
libheimdall_renderer_item_release(libheimdall_renderer_item_t *p_item)
{
    heimdall_renderer_item_release( (heimdall_renderer_item_t *) p_item );
}

const char *
libheimdall_renderer_item_name( const libheimdall_renderer_item_t *p_item )
{
    return heimdall_renderer_item_name( (const heimdall_renderer_item_t *) p_item );
}

const char *
libheimdall_renderer_item_type( const libheimdall_renderer_item_t *p_item )
{
    return heimdall_renderer_item_type( (const heimdall_renderer_item_t *) p_item );
}

const char *
libheimdall_renderer_item_icon_uri( const libheimdall_renderer_item_t *p_item )
{
    return heimdall_renderer_item_icon_uri( (const heimdall_renderer_item_t *) p_item );
}

int
libheimdall_renderer_item_flags( const libheimdall_renderer_item_t *p_item )
{
    return heimdall_renderer_item_flags( (const heimdall_renderer_item_t *) p_item );
}

libheimdall_renderer_discoverer_t *
libheimdall_renderer_discoverer_new( libheimdall_instance_t *p_inst,
                                const char *psz_name,
                                const struct libheimdall_renderer_discoverer_cbs *cbs,
                                void *cbs_opaque )
{
    assert( cbs != NULL && cbs->on_item_added != NULL );

    /* No different versions to handle for now */
    assert(cbs->version <= 0);

    size_t len = strlen( psz_name ) + 1;
    libheimdall_renderer_discoverer_t *p_lrd = malloc( sizeof(*p_lrd) + len );

    if( unlikely(p_lrd == NULL) )
        return NULL;

    p_lrd->p_object = HEIMDALL_OBJECT(p_inst->p_libheimdall_int);
    memcpy( p_lrd->name, psz_name, len );
    TAB_INIT( p_lrd->i_items, p_lrd->pp_items );
    p_lrd->p_rd = NULL;
    p_lrd->cbs = cbs;
    p_lrd->cbs_opaque = cbs_opaque;

    return p_lrd;
}

void
libheimdall_renderer_discoverer_destroy( libheimdall_renderer_discoverer_t *p_lrd )
{
    libheimdall_renderer_discoverer_stop( p_lrd );
    free( p_lrd );
}

int
libheimdall_renderer_discoverer_start( libheimdall_renderer_discoverer_t *p_lrd )
{
    assert( p_lrd->p_rd == NULL );

    struct heimdall_renderer_discovery_owner owner =
    {
        p_lrd,
        renderer_discovery_item_added,
        renderer_discovery_item_removed,
    };

    p_lrd->p_rd = heimdall_rd_new( p_lrd->p_object, p_lrd->name, &owner );
    return p_lrd->p_rd != NULL ? 0 : -1;
}

void
libheimdall_renderer_discoverer_stop( libheimdall_renderer_discoverer_t *p_lrd )
{
    if( p_lrd->p_rd != NULL )
    {
        heimdall_rd_release( p_lrd->p_rd );
        p_lrd->p_rd = NULL;
    }

    for( int i = 0; i < p_lrd->i_items; ++i )
        heimdall_renderer_item_release( p_lrd->pp_items[i] );
    TAB_CLEAN( p_lrd->i_items, p_lrd->pp_items );
}

void
libheimdall_renderer_discoverer_list_release( libheimdall_rd_description_t **pp_services,
                                         size_t i_count )
{
    if( i_count > 0 )
    {
        for( size_t i = 0; i < i_count; ++i )
        {
            free( pp_services[i]->psz_name );
            free( pp_services[i]->psz_longname );
        }
        free( *pp_services );
        free( pp_services );
    }
}

size_t
libheimdall_renderer_discoverer_list_get( libheimdall_instance_t *p_inst,
                                     libheimdall_rd_description_t ***ppp_services )
{
    assert( p_inst != NULL && ppp_services != NULL );

    /* Fetch all rd names, and longnames */
    char **ppsz_names, **ppsz_longnames;
    int i_ret = heimdall_rd_get_names( p_inst->p_libheimdall_int, &ppsz_names,
                                  &ppsz_longnames );

    if( i_ret != HEIMDALL_SUCCESS )
    {
        *ppp_services = NULL;
        return 0;
    }

    /* Count the number of sd matching our category (i_cat/i_core_cat) */
    size_t i_nb_services = 0;
    char **ppsz_name = ppsz_names;
    for( ; *ppsz_name != NULL; ppsz_name++ )
        i_nb_services++;

    libheimdall_rd_description_t **pp_services = NULL,
                                              *p_services = NULL;
    if( i_nb_services > 0 )
    {
        /* Double alloc here, so that the caller iterates through pointers of
         * struct instead of structs. This allows us to modify the struct
         * without breaking the API. */

        pp_services =
            malloc( i_nb_services
                    * sizeof(libheimdall_rd_description_t *) );
        p_services =
            malloc( i_nb_services
                    * sizeof(libheimdall_rd_description_t) );
        if( pp_services == NULL || p_services == NULL )
        {
            free( pp_services );
            free( p_services );
            pp_services = NULL;
            p_services = NULL;
            i_nb_services = 0;
            /* Even if alloc fails, the next loop must be run in order to free
             * names returned by heimdall_sd_GetNames */
        }
    }

    /* Fill output pp_services or free unused name, longname */
    char **ppsz_longname = ppsz_longnames;
    unsigned int i_service_idx = 0;
    libheimdall_rd_description_t *p_service = p_services;
    for( ppsz_name = ppsz_names; *ppsz_name != NULL; ppsz_name++, ppsz_longname++ )
    {
        if( pp_services != NULL )
        {
            p_service->psz_name = *ppsz_name;
            p_service->psz_longname = *ppsz_longname;
            pp_services[i_service_idx++] = p_service++;
        }
        else
        {
            free( *ppsz_name );
            free( *ppsz_longname );
        }
    }
    free( ppsz_names );
    free( ppsz_longnames );

    *ppp_services = pp_services;
    return i_nb_services;
}
