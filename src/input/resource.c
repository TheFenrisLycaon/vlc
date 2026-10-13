/*****************************************************************************
 * resource.c
 *****************************************************************************
 * Copyright (C) 2008 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar < fenrir _AT_ lycaria _DOT_ org >
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_atomic.h>
#include <heimdall_decoder.h>
#include <heimdall_vout.h>
#include <heimdall_spu.h>
#include <heimdall_aout.h>
#include <heimdall_sout.h>
#include "../libheimdall.h"
#include "../stream_output/stream_output.h"
#include "../audio_output/aout_internal.h"
#include "../video_output/vout_internal.h"
#include "input_interface.h"
#include "event.h"
#include "resource.h"

struct vout_resource
{
    vout_thread_t *vout;
    enum heimdall_vout_order order;
    bool started;
    char *psz_prev_title;

    struct heimdall_list node;
};

struct input_resource_t
{
    heimdall_atomic_rc_t rc;

    heimdall_object_t   *p_parent;

    /* This lock is used to serialize request and protect
     * our variables */
    heimdall_mutex_t    lock;

    /* */
    input_thread_t *p_input;

    sout_stream_t *p_sout;
    char *psz_sout;
    vout_thread_t   *p_vout_dummy;
    struct vout_resource *vout_rsc_free;

    /* This lock is used to protect vout resources access (for hold)
     * It is a special case because of embed video (possible deadlock
     * between vout window request and vout holds in some(qt) interface)
     */
    heimdall_mutex_t    lock_hold;

    /* You need lock+lock_hold to write to the following variables and
     * only lock or lock_hold to read them */
    struct heimdall_list vout_rscs;

    bool            b_aout_busy;
    audio_output_t *p_aout;
};

#define resource_GetFirstVoutRsc(resource) \
    heimdall_list_first_entry_or_null(&resource->vout_rscs, struct vout_resource, node)

static inline struct vout_resource *
resource_GetVoutRsc(input_resource_t *resource, vout_thread_t *vout)
{
    struct vout_resource *vout_rsc;

    heimdall_list_foreach(vout_rsc, &resource->vout_rscs, node)
        if (vout_rsc->vout == vout)
            return vout_rsc;

    heimdall_assert_unreachable();
}

static inline struct vout_resource *
vout_resource_Create(vout_thread_t *vout)
{
    struct vout_resource *vout_rsc = malloc(sizeof(*vout_rsc));
    if (unlikely(vout_rsc == NULL))
        return NULL;

    vout_rsc->started = false;
    vout_rsc->vout = vout;
    vout_rsc->psz_prev_title = NULL;
    return vout_rsc;
}

static inline void
vout_resource_Add(struct vout_resource *vout_rsc, input_resource_t *resource)
{
    heimdall_list_append(&vout_rsc->node, &resource->vout_rscs);
}

static inline vout_thread_t *
vout_resource_Remove(struct vout_resource *vout_rsc)
{
    heimdall_list_remove(&vout_rsc->node);
    vout_thread_t *vout = vout_rsc->vout;
    free(vout_rsc->psz_prev_title);
    free(vout_rsc);

    assert(vout);
    return vout;
}

/* */
static void DestroySout( input_resource_t *p_resource )
{
    if( p_resource->p_sout )
    {
        msg_Dbg( p_resource->p_parent, "destroying stream output" );
        sout_StreamChainDelete( p_resource->p_sout, NULL );
        free(p_resource->psz_sout);
    }
    p_resource->p_sout = NULL;
}

/* */
static void DestroyVout( input_resource_t *p_resource )
{
    assert( heimdall_list_is_empty( &p_resource->vout_rscs )
         || p_resource->vout_rsc_free == resource_GetFirstVoutRsc( p_resource ) );

    if( p_resource->vout_rsc_free )
    {
        heimdall_mutex_lock(&p_resource->lock_hold);
        vout_thread_t *vout = vout_resource_Remove( p_resource->vout_rsc_free );
        heimdall_mutex_unlock(&p_resource->lock_hold);

        vout_Close( vout );
        p_resource->vout_rsc_free = NULL;
    }
}

static void VoutSetAndDeduplicateTitle( vout_thread_t *p_vout,
                                        char **ppsz_title,
                                        char **ppsz_prev_title )
{
    if( !*ppsz_prev_title || strcmp( *ppsz_prev_title, *ppsz_title ) )
    {
        vout_DisplayTitle( p_vout, *ppsz_title );
        free( *ppsz_prev_title );
        *ppsz_prev_title = *ppsz_title;
        *ppsz_title = NULL;
    }
}

static void DisplayVoutTitle( input_resource_t *p_resource,
                              vout_thread_t *p_vout,
                              char ** ppsz_prev_title )
{
    if( p_resource->p_input == NULL )
        return;

    /* TODO display the title only one time for the same input ? */

    input_item_t *p_item = input_GetItem( p_resource->p_input );

    char *psz_nowplaying = input_item_GetNowPlayingFb( p_item );
    if( psz_nowplaying && *psz_nowplaying )
    {
        VoutSetAndDeduplicateTitle( p_vout, &psz_nowplaying, ppsz_prev_title );
    }
    else
    {
        char *psz_artist = input_item_GetArtist( p_item );
        char *psz_name = input_item_GetTitle( p_item );

        if( !psz_name || *psz_name == '\0' )
        {
            free( psz_name );
            psz_name = input_item_GetName( p_item );
        }
        if( psz_artist && *psz_artist )
        {
            char *psz_string = NULL;
            if( asprintf( &psz_string, "%s - %s", psz_name, psz_artist ) > 0 )
            {
                VoutSetAndDeduplicateTitle( p_vout, &psz_string, ppsz_prev_title );
                free( psz_string );
            }
        }
        else if( psz_name )
        {
            VoutSetAndDeduplicateTitle( p_vout, &psz_name, ppsz_prev_title );
        }
        free( psz_name );
        free( psz_artist );
    }
    free( psz_nowplaying );
}

/* Audio output */
audio_output_t *input_resource_GetAout( input_resource_t *p_resource )
{
    audio_output_t *p_aout;

    heimdall_mutex_lock( &p_resource->lock_hold );
    p_aout = p_resource->p_aout;

    if( p_aout == NULL || p_resource->b_aout_busy )
    {
        msg_Dbg( p_resource->p_parent, "creating audio output" );
        heimdall_mutex_unlock( &p_resource->lock_hold );

        p_aout = aout_New( p_resource->p_parent );
        if( p_aout == NULL )
            return NULL; /* failed */

        heimdall_mutex_lock( &p_resource->lock_hold );
        if( p_resource->p_aout == NULL )
            p_resource->p_aout = p_aout;
    }
    else
        msg_Dbg( p_resource->p_parent, "reusing audio output" );

    if( p_resource->p_aout == p_aout )
    {
        assert( !p_resource->b_aout_busy );
        p_resource->b_aout_busy = true;
    }
    heimdall_mutex_unlock( &p_resource->lock_hold );
    return p_aout;
}

void input_resource_PutAout( input_resource_t *p_resource,
                             audio_output_t *p_aout )
{
    assert( p_aout != NULL );

    heimdall_mutex_lock( &p_resource->lock_hold );
    if( p_aout == p_resource->p_aout )
    {
        assert( p_resource->b_aout_busy );
        p_resource->b_aout_busy = false;
        msg_Dbg( p_resource->p_parent, "keeping audio output" );
        p_aout = NULL;
    }
    else
        msg_Dbg( p_resource->p_parent, "destroying extra audio output" );
    heimdall_mutex_unlock( &p_resource->lock_hold );

    if( p_aout != NULL )
        aout_Release( p_aout );
}

audio_output_t *input_resource_HoldAout( input_resource_t *p_resource )
{
    audio_output_t *p_aout;

    heimdall_mutex_lock( &p_resource->lock_hold );
    p_aout = p_resource->p_aout;
    if( p_aout != NULL )
        aout_Hold(p_aout);
    heimdall_mutex_unlock( &p_resource->lock_hold );

    return p_aout;
}

void input_resource_ResetAout( input_resource_t *p_resource )
{
    audio_output_t *p_aout = NULL;

    heimdall_mutex_lock( &p_resource->lock_hold );
    if( !p_resource->b_aout_busy )
        p_aout = p_resource->p_aout;

    p_resource->p_aout = NULL;
    p_resource->b_aout_busy = false;
    heimdall_mutex_unlock( &p_resource->lock_hold );

    if( p_aout != NULL )
        aout_Release( p_aout );
}

/* Common */
input_resource_t *input_resource_New( heimdall_object_t *p_parent )
{
    input_resource_t *p_resource = calloc( 1, sizeof(*p_resource) );
    if( !p_resource )
        return NULL;

    p_resource->p_vout_dummy = vout_CreateDummy(p_parent);
    if( !p_resource->p_vout_dummy )
    {
        free( p_resource );
        return NULL;
    }

    heimdall_list_init( &p_resource->vout_rscs );

    heimdall_atomic_rc_init( &p_resource->rc );
    p_resource->p_parent = p_parent;
    heimdall_mutex_init( &p_resource->lock );
    heimdall_mutex_init( &p_resource->lock_hold );
    return p_resource;
}

void input_resource_Release( input_resource_t *p_resource )
{
    if( !heimdall_atomic_rc_dec( &p_resource->rc ) )
        return;

    DestroySout( p_resource );
    DestroyVout( p_resource );
    if( p_resource->p_aout != NULL )
        aout_Release( p_resource->p_aout );

    vout_Release( p_resource->p_vout_dummy );
    free( p_resource );
}

input_resource_t *input_resource_Hold( input_resource_t *p_resource )
{
    heimdall_atomic_rc_inc( &p_resource->rc );
    return p_resource;
}

void input_resource_SetInput( input_resource_t *p_resource, input_thread_t *p_input )
{
    heimdall_mutex_lock( &p_resource->lock );

    if( p_resource->p_input && !p_input )
        assert( heimdall_list_is_empty( &p_resource->vout_rscs )
             || p_resource->vout_rsc_free == resource_GetFirstVoutRsc( p_resource ) );

    /* */
    p_resource->p_input = p_input;

    heimdall_mutex_unlock( &p_resource->lock );
}

static void input_resource_PutVoutLocked(input_resource_t *p_resource,
                                         vout_thread_t *vout,
                                         enum input_resource_vout_state *vout_state)
{
    assert(vout != NULL);
    struct vout_resource *vout_rsc = resource_GetVoutRsc(p_resource, vout);
    assert(vout_rsc != NULL);

    if (vout_state != NULL)
        *vout_state = vout_rsc->started ? INPUT_RESOURCE_VOUT_STOPPED
                                        : INPUT_RESOURCE_VOUT_NOTCHANGED;

    if (vout_rsc->started)
    {
        vout_StopDisplay(vout_rsc->vout);
        vout_rsc->started = false;
    }

    if (vout_rsc == resource_GetFirstVoutRsc(p_resource))
    {
        assert(p_resource->vout_rsc_free == NULL || p_resource->vout_rsc_free == vout_rsc);

        msg_Dbg(p_resource->p_parent, "saving a free vout");
        p_resource->vout_rsc_free = vout_rsc;
    }
    else
    {
        msg_Dbg(p_resource->p_parent, "destroying vout (already one saved or active)");

        heimdall_mutex_lock(&p_resource->lock_hold);
        vout_resource_Remove(vout_rsc);
        heimdall_mutex_unlock(&p_resource->lock_hold);

        vout_Stop(vout);
        vout_Close(vout);
    }
}

void input_resource_PutVout(input_resource_t *p_resource,
                            vout_thread_t *vout,
                            enum input_resource_vout_state *vout_state)
{
    heimdall_mutex_lock( &p_resource->lock );
    input_resource_PutVoutLocked( p_resource, vout, vout_state );
    heimdall_mutex_unlock( &p_resource->lock );
}

static struct vout_resource *
RequestVoutRsc(input_resource_t *p_resource)
{
    struct vout_resource *vout_rsc = NULL;

    if( p_resource->vout_rsc_free != NULL )
    {
        /* The free vout is always the first one */
        msg_Dbg(p_resource->p_parent, "trying to reuse free vout");

        vout_rsc = p_resource->vout_rsc_free;
        p_resource->vout_rsc_free = NULL;
    }
    else
    {
        /* Use the dummy vout as the parent of the future main vout. This
         * will allow the future vout to inherit all parameters
         * pre-configured on this dummy vout. */
        heimdall_object_t *parent = heimdall_list_is_empty( &p_resource->vout_rscs ) ?
            HEIMDALL_OBJECT(p_resource->p_vout_dummy) : p_resource->p_parent;
        vout_thread_t *vout = vout_Create(parent);
        if (vout == NULL)
            return NULL;

        vout_rsc = vout_resource_Create(vout);
        if (vout_rsc == NULL)
        {
            vout_Close(vout);
            return NULL;
        }

        vout_rsc->order = heimdall_list_is_empty( &p_resource->vout_rscs ) ?
            HEIMDALL_VOUT_ORDER_PRIMARY : HEIMDALL_VOUT_ORDER_SECONDARY;

        heimdall_mutex_lock(&p_resource->lock_hold);
        vout_resource_Add(vout_rsc, p_resource);
        heimdall_mutex_unlock(&p_resource->lock_hold);
    }

    return vout_rsc;
}

vout_thread_t *input_resource_RequestVout(input_resource_t *p_resource,
                                          heimdall_video_context *vctx,
                                          const vout_configuration_t *cfg,
                                          enum heimdall_vout_order *order,
                                          enum input_resource_vout_state *vout_state)
{
    heimdall_mutex_lock( &p_resource->lock );
    struct vout_resource *vout_rsc = NULL;

    if (vout_state != NULL)
        *vout_state = INPUT_RESOURCE_VOUT_NOTCHANGED;

    vout_configuration_t dcfg = *cfg;
    if (dcfg.vout == NULL)
    {
        vout_rsc = RequestVoutRsc(p_resource);
        if (vout_rsc == NULL)
        {
            heimdall_mutex_unlock( &p_resource->lock );
            return NULL;
        }
        dcfg.vout = vout_rsc->vout;
    }
    else
    {
        vout_rsc = resource_GetVoutRsc(p_resource, dcfg.vout);
        assert(vout_rsc != NULL);

        /* the caller is going to reuse the free vout, it's not free anymore */
        if (p_resource->vout_rsc_free == vout_rsc)
            p_resource->vout_rsc_free = NULL;
    }

    if (order != NULL)
        *order = vout_rsc->order;

    if (dcfg.fmt == NULL)
    {
        /* A NULL fmt means that only the vout creation is requested, do not
         * start it with vout_Request(). */
        heimdall_mutex_unlock(&p_resource->lock);
        return dcfg.vout;
    }

    if (vout_Request(&dcfg, vctx, p_resource->p_input)) {
        if (vout_rsc->started && vout_state != NULL)
            *vout_state = INPUT_RESOURCE_VOUT_STOPPED;

        vout_rsc->started = false;
        input_resource_PutVoutLocked(p_resource, dcfg.vout, NULL);
        heimdall_mutex_unlock(&p_resource->lock);
        return NULL;
    }

    if (!vout_rsc->started)
    {
        vout_rsc->started = true;
        if (vout_state != NULL)
            *vout_state = INPUT_RESOURCE_VOUT_STARTED;

        DisplayVoutTitle(p_resource, cfg->vout, &vout_rsc->psz_prev_title);

        /* Send original viewpoint to the input in order to update other ESes */
        if (p_resource->p_input != NULL)
        {
            input_control_param_t param = { .viewpoint = cfg->fmt->pose };
            input_ControlPush(p_resource->p_input, INPUT_CONTROL_SET_INITIAL_VIEWPOINT,
                              &param);
        }
    }
    heimdall_mutex_unlock( &p_resource->lock );

    return dcfg.vout;
}

vout_thread_t *input_resource_HoldVout( input_resource_t *p_resource )
{
    heimdall_mutex_lock( &p_resource->lock_hold );

    struct vout_resource *vout_rsc = resource_GetFirstVoutRsc( p_resource );
    vout_thread_t *p_vout = vout_rsc != NULL ? vout_rsc->vout : NULL;
    if( p_vout )
        vout_Hold(p_vout);

    heimdall_mutex_unlock( &p_resource->lock_hold );

    return p_vout;
}

vout_thread_t *input_resource_HoldDummyVout( input_resource_t *p_resource )
{
    return vout_Hold(p_resource->p_vout_dummy);
}

void input_resource_HoldVouts( input_resource_t *p_resource, vout_thread_t ***ppp_vout,
                               size_t *pi_vout )
{
    vout_thread_t **pp_vout;

    *pi_vout = 0;
    *ppp_vout = NULL;

    heimdall_mutex_lock( &p_resource->lock_hold );

    if( heimdall_list_is_empty( &p_resource->vout_rscs ) )
        goto exit;

    size_t count = 0;
    struct vout_resource *vout_rsc;
    heimdall_list_foreach( vout_rsc, &p_resource->vout_rscs, node )
        count ++;

    pp_vout = heimdall_alloc( count, sizeof(*pp_vout) );
    if( !pp_vout )
        goto exit;

    *ppp_vout = pp_vout;
    *pi_vout = count;

    count = 0;
    heimdall_list_foreach( vout_rsc, &p_resource->vout_rscs, node )
        pp_vout[count++] = vout_Hold( vout_rsc->vout );

exit:
    heimdall_mutex_unlock( &p_resource->lock_hold );
}

void input_resource_StopFreeVout(input_resource_t *p_resource)
{
    heimdall_mutex_lock(&p_resource->lock);
    if (p_resource->vout_rsc_free != NULL)
    {
        msg_Dbg(p_resource->vout_rsc_free->vout, "stop free vout");
        vout_Stop(p_resource->vout_rsc_free->vout);
    }
    heimdall_mutex_unlock(&p_resource->lock);
}

/* */
sout_stream_t *input_resource_RequestSout( input_resource_t *p_resource, const char *psz_sout )
{
    sout_stream_t *sout;

    assert(psz_sout != NULL);
    heimdall_mutex_lock( &p_resource->lock );
    /* Check the validity of the sout */
    if (p_resource->p_sout != NULL
     && strcmp(p_resource->psz_sout, psz_sout) != 0)
    {
        msg_Dbg(p_resource->p_parent, "destroying unusable sout");
        DestroySout(p_resource);
    }

    sout = p_resource->p_sout;

    if (sout != NULL)
    {
        /* Reuse it */
        msg_Dbg(p_resource->p_parent, "reusing sout");
        msg_Dbg(p_resource->p_parent, "you probably want to use gather stream_out");
        p_resource->p_sout = NULL;
    }
    else
    {
        /* Create a new one */
        p_resource->psz_sout = strdup(psz_sout);

        if (likely(p_resource->psz_sout != NULL)) {
            sout = sout_NewInstance(p_resource->p_parent, psz_sout);
            if (sout == NULL)
                free(p_resource->psz_sout);
        }
    }
    heimdall_mutex_unlock( &p_resource->lock );
    return sout;
}

void input_resource_PutSout(input_resource_t *resource, sout_stream_t *sout)
{
    if (sout == NULL)
    {
        input_resource_TerminateSout(resource);
        return;
    }

    heimdall_mutex_lock(&resource->lock);
    assert(resource->p_sout == NULL);
    resource->p_sout = sout;
    heimdall_mutex_unlock(&resource->lock);
}

void input_resource_TerminateSout( input_resource_t *p_resource )
{
    heimdall_mutex_lock( &p_resource->lock );
    DestroySout(p_resource);
    heimdall_mutex_unlock( &p_resource->lock );
}
