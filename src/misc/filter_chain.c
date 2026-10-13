/*****************************************************************************
 * filter_chain.c : Handle chains of filter_t objects.
 *****************************************************************************
 * Copyright (C) 2008 VLC authors and VideoLAN
 * Copyright (C) 2008-2014 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Antoine Cellerier <dionoea at lycaria dot org>
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

#include <heimdall_filter.h>
#include <heimdall_configuration.h>
#include <heimdall_modules.h>
#include <heimdall_mouse.h>
#include <heimdall_spu.h>
#include "../libheimdall.h"
#include <assert.h>

module_t *heimdall_filter_LoadModule(filter_t *p_filter, const char *capability,
                                const char *name, bool strict)
{
    if (name == NULL || name[0] == '\0')
        name = "any";

    /* Find matching modules */
    module_t **mods;
    ssize_t total = heimdall_module_match(capability, name, strict,
                                     &mods, NULL);

    if (unlikely(total < 0))
        return NULL;

    struct heimdall_logger *log = p_filter->obj.logger;

    heimdall_debug(log, "looking for %s module matching \"%s\": %zd candidates",
              capability, name, total);

    p_filter->p_module = NULL;
    for (size_t i = 0; i < (size_t)total; i++) {
        module_t *cand = mods[i];
        int ret = HEIMDALL_EGENERIC;
        heimdall_filter_open cb = heimdall_module_map(log, cand);

        if (cb == NULL)
            continue;

        p_filter->p_module = cand;
        ret = cb(p_filter);
        if (ret == HEIMDALL_SUCCESS)
        {
            heimdall_debug(log, "using %s module \"%s\"", capability,
                        module_get_object(cand));
            assert( p_filter->ops != NULL );
            break;
        }

        heimdall_objres_clear(&p_filter->obj);
        p_filter->p_module = NULL;

        if (ret == HEIMDALL_ETIMEOUT)
            break;
        if (ret == HEIMDALL_ENOMEM)
        {
            free(mods);
            return NULL;
        }
    }

    if (p_filter->p_module == NULL)
        heimdall_debug(log, "no %s modules matched with name %s", capability, name);

    free(mods);
    if (p_filter->p_module != NULL) {
        var_Create(p_filter, "module-name", HEIMDALL_VAR_STRING);
        var_SetString(p_filter, "module-name", module_get_object(p_filter->p_module));
    }

    return p_filter->p_module;
}

void heimdall_filter_UnloadModule(filter_t *p_filter)
{
    if (likely(p_filter->p_module))
    {
        if ( p_filter->ops->close )
            p_filter->ops->close( p_filter );

        msg_Dbg(p_filter, "removing \"%s\" module \"%s\"", module_get_capability(p_filter->p_module),
                module_get_object(p_filter->p_module));
        var_Destroy(p_filter, "module-name");

        p_filter->p_module = NULL;
    }

    heimdall_objres_clear(&p_filter->obj);
}

typedef struct chained_filter_t
{
    /* Public part of the filter structure */
    filter_t filter;
    struct heimdall_list node;
    heimdall_mouse_t mouse;
    heimdall_picture_chain_t pending;
} chained_filter_t;

/* */
struct filter_chain_t
{
    heimdall_object_t *obj;
    filter_owner_t parent_video_owner; /**< Owner (downstream) callbacks */

    struct heimdall_list filter_list; /* chained_filter_t */

    es_format_t fmt_in; /**< Chain input format (constant) */
    heimdall_video_context *vctx_in; /**< Chain input video context (set on Reset) */
    es_format_t fmt_out; /**< Chain output format (constant) */
    bool b_allow_fmt_out_change; /**< Each filter can change the output */
    const char *filter_cap; /**< Filter modules capability */
    const char *conv_cap; /**< Converter modules capability */
};

/**
 * Local prototypes
 */
static void FilterDeletePictures( heimdall_picture_chain_t * );

static filter_chain_t *filter_chain_NewInner( heimdall_object_t *obj,
    const char *cap, const char *conv_cap, bool fmt_out_change,
    enum es_format_category_e cat )
{
    assert( obj != NULL );
    assert( cap != NULL );

    filter_chain_t *chain = malloc( sizeof (*chain) );
    if( unlikely(chain == NULL) )
        return NULL;

    chain->obj = obj;
    heimdall_list_init( &chain->filter_list );
    es_format_Init( &chain->fmt_in, cat, 0 );
    chain->vctx_in = NULL;
    es_format_Init( &chain->fmt_out, cat, 0 );
    chain->b_allow_fmt_out_change = fmt_out_change;
    chain->filter_cap = cap;
    chain->conv_cap = conv_cap;
    return chain;
}

#undef filter_chain_NewSPU
filter_chain_t *filter_chain_NewSPU( heimdall_object_t *obj, const char *cap )
{
    return filter_chain_NewInner( obj, cap, NULL, false, SPU_ES );
}

/** Chained filter picture allocator function */
static picture_t *filter_chain_VideoBufferNew( filter_t *filter )
{
    picture_t *pic;
    chained_filter_t *chained = container_of(filter, chained_filter_t, filter);
    filter_chain_t *chain = filter->owner.sys;
    if( !heimdall_list_is_last( &chained->node, &chain->filter_list ) )
    {
        // HACK as intermediate filters may not have the same video format as
        // the last one handled by the owner
        filter_owner_t saved_owner = filter->owner;
        filter->owner = (filter_owner_t) {0};
        pic = filter_NewPicture( filter );
        filter->owner = saved_owner;
        if( pic == NULL )
            msg_Err( filter, "Failed to allocate picture" );
    }
    else
    {
        // the owner of the chain requires pictures from the last filter to be grabbed from its callback
        /* XXX ugly */
        filter_owner_t saved_owner = filter->owner;
        filter->owner = chain->parent_video_owner;
        pic = filter_NewPicture( filter );
        filter->owner = saved_owner;
    }
    return pic;
}

static heimdall_decoder_device * filter_chain_HoldDecoderDevice(heimdall_object_t *o, void *sys)
{
    filter_chain_t *chain = sys;

    if (chain->parent_video_owner.video == NULL ||
        chain->parent_video_owner.video->hold_device == NULL)
        return NULL;

    return chain->parent_video_owner.video->hold_device(o, chain->parent_video_owner.sys);
}

static const struct filter_video_callbacks filter_chain_video_cbs =
{
    filter_chain_VideoBufferNew, filter_chain_HoldDecoderDevice,
};

#undef filter_chain_NewVideo
filter_chain_t *filter_chain_NewVideo( heimdall_object_t *obj, bool allow_change,
                                       const filter_owner_t *restrict owner )
{
    filter_chain_t *chain =
        filter_chain_NewInner( obj, "video filter",
                                  "video converter", allow_change, VIDEO_ES );
    if (unlikely(chain == NULL))
        return NULL;

    if( owner != NULL && owner->video != NULL )
    {
        // keep this to get pictures for the last filter in the chain
        chain->parent_video_owner = *owner;
    }
    else
        chain->parent_video_owner = (filter_owner_t){0};
    return chain;
}

void filter_chain_Clear( filter_chain_t *p_chain )
{
    chained_filter_t *chained;
    heimdall_list_foreach( chained, &p_chain->filter_list, node )
        filter_chain_DeleteFilter( p_chain, &chained->filter );
}

void filter_chain_Delete( filter_chain_t *p_chain )
{
    filter_chain_Clear( p_chain );

    es_format_Clean( &p_chain->fmt_in );
    if ( p_chain->vctx_in )
        heimdall_video_context_Release( p_chain->vctx_in );
    es_format_Clean( &p_chain->fmt_out );

    free( p_chain );
}
/**
 * Filter chain reinitialisation
 */
void filter_chain_Reset( filter_chain_t *p_chain,
                         const es_format_t *p_fmt_in, heimdall_video_context *vctx_in,
                         const es_format_t *p_fmt_out )
{
    filter_chain_Clear( p_chain );

    assert(p_fmt_in != NULL);
    es_format_Clean( &p_chain->fmt_in );
    es_format_Copy( &p_chain->fmt_in, p_fmt_in );
    if ( p_chain->vctx_in )
        heimdall_video_context_Release( p_chain->vctx_in );
    p_chain->vctx_in = vctx_in ? heimdall_video_context_Hold(vctx_in) : NULL;

    assert(p_fmt_out != NULL);
    es_format_Clean( &p_chain->fmt_out );
    es_format_Copy( &p_chain->fmt_out, p_fmt_out );
}

static filter_t *filter_chain_AppendInner( filter_chain_t *chain,
    const char *name, const char *capability, const config_chain_t *cfg,
    const es_format_t *fmt_out )
{
    chained_filter_t *chained =
        heimdall_custom_create( chain->obj, sizeof(*chained), "filter" );
    if( unlikely(chained == NULL) )
        return NULL;

    filter_t *filter = &chained->filter;

    const es_format_t *fmt_in;
    heimdall_video_context *vctx_in;
    chained_filter_t *last =
        heimdall_list_last_entry_or_null( &chain->filter_list, chained_filter_t, node );
    if( last != NULL )
    {
        fmt_in = &last->filter.fmt_out;
        vctx_in = last->filter.vctx_out;
    }
    else
    {
        fmt_in = &chain->fmt_in;
        vctx_in = chain->vctx_in;
    }

    if( fmt_out == NULL )
        fmt_out = &chain->fmt_out;

    es_format_Copy( &filter->fmt_in, fmt_in );
    filter->vctx_in = vctx_in;
    es_format_Copy( &filter->fmt_out, fmt_out );
    filter->b_allow_fmt_out_change = chain->b_allow_fmt_out_change;
    filter->p_cfg = cfg;
    filter->psz_name = name;

    if (fmt_in->i_cat == VIDEO_ES)
    {
        filter->owner.video = &filter_chain_video_cbs;
        filter->owner.sys = chain;
    }
    else
        filter->owner.sub = NULL;

    char *name_chained = NULL;
    const char *module_name = name;
    assert( capability != NULL );
    if (name != NULL && name[0] != '\0' && chain->b_allow_fmt_out_change )
    {
        /* Append the "chain" video filter to the current list.
         * This filter will be used if the requested filter fails to load.
         * It will then try to add a video converter before. */
        if (asprintf(&name_chained, "%s,chain", name) == -1)
            goto error;
        module_name = name_chained;
    }

    filter->p_module =
        heimdall_filter_LoadModule(filter, capability, module_name, name != NULL);

   if (filter->p_module == NULL)
      goto error;

    heimdall_list_append( &chained->node, &chain->filter_list );

    heimdall_mouse_Init( &chained->mouse );
    heimdall_picture_chain_Init( &chained->pending );

    msg_Dbg( chain->obj, "Filter '%s' (%p) appended to chain (%p)",
             (name != NULL) ? name : module_GetShortName(filter->p_module),
             (void *)filter, (void *)chain );
    free(name_chained);
    return filter;

error:
    if( name != NULL )
        msg_Err( chain->obj, "Failed to create %s '%s'", capability, name );
    else
        msg_Err( chain->obj, "Failed to create %s", capability );
    free(name_chained);
    heimdall_objres_clear(&filter->obj);
    es_format_Clean( &filter->fmt_out );
    es_format_Clean( &filter->fmt_in );
    heimdall_object_delete(filter);
    return NULL;
}

filter_t *filter_chain_AppendFilter( filter_chain_t *chain,
    const char *name, const config_chain_t *cfg,
    const es_format_t *fmt_out )
{
    return filter_chain_AppendInner( chain, name, chain->filter_cap, cfg,
                                     fmt_out );
}

int filter_chain_AppendConverter( filter_chain_t *chain,
    const es_format_t *fmt_out )
{
    return filter_chain_AppendInner( chain, NULL, chain->conv_cap, NULL,
                                     fmt_out ) != NULL ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

void filter_chain_DeleteFilter( filter_chain_t *chain, filter_t *filter )
{
    chained_filter_t *chained = container_of(filter, chained_filter_t, filter);

    /* Remove it from the chain */
    heimdall_list_remove( &chained->node );

    heimdall_filter_UnloadModule( filter );

    msg_Dbg( chain->obj, "Filter %p removed from chain", (void *)filter );
    FilterDeletePictures( &chained->pending );

    es_format_Clean( &filter->fmt_out );
    es_format_Clean( &filter->fmt_in );

    heimdall_object_delete(filter);
    /* FIXME: check fmt_in/fmt_out consistency */
}


int filter_chain_AppendFromString( filter_chain_t *chain, const char *str )
{
    char *buf = NULL;
    int ret = 0;

    while( str != NULL && str[0] != '\0' )
    {
        config_chain_t *cfg;
        char *name;

        char *next = config_ChainCreate( &name, &cfg, str );

        str = next;
        free( buf );
        buf = next;

        filter_t *filter = filter_chain_AppendFilter( chain, name, cfg, NULL );
        if( cfg )
            config_ChainDestroy( cfg );

        if( filter == NULL )
        {
            msg_Err( chain->obj, "Failed to append '%s' to chain", name );
            free( name );
            goto error;
        }

        free( name );
        ret++;
    }

    free( buf );
    return ret;

error:
    while( ret > 0 ) /* Unwind */
    {
        chained_filter_t *last =
            heimdall_list_last_entry_or_null( &chain->filter_list, chained_filter_t, node );
        assert( last != NULL );
        filter_chain_DeleteFilter( chain, &last->filter );
        ret--;
    }
    free( buf );
    return HEIMDALL_EGENERIC;
}

int filter_chain_ForEach( filter_chain_t *chain,
                          int (*cb)( filter_t *, void * ), void *opaque )
{
    chained_filter_t *f;
    heimdall_list_foreach( f, &chain->filter_list, node )
    {
        int ret = cb( &f->filter, opaque );
        if( ret )
            return ret;
    }
    return HEIMDALL_SUCCESS;
}

bool filter_chain_IsEmpty(const filter_chain_t *chain)
{
    return heimdall_list_is_empty( &chain->filter_list );
}

const es_format_t *filter_chain_GetFmtOut( const filter_chain_t *p_chain )
{
    chained_filter_t *last =
        heimdall_list_last_entry_or_null( &p_chain->filter_list, chained_filter_t, node );
    if( last != NULL )
        return &last->filter.fmt_out;

    /* Unless filter_chain_Reset has been called we are doomed */
    return &p_chain->fmt_out;
}

heimdall_video_context *filter_chain_GetVideoCtxOut(const filter_chain_t *p_chain)
{
    chained_filter_t *last =
        heimdall_list_last_entry_or_null( &p_chain->filter_list, chained_filter_t, node );
    if( last != NULL )
        return last->filter.vctx_out;

    /* No filter was added, the filter chain has no effect, make sure the chromas are compatible */
    assert( video_format_IsSameChroma( &p_chain->fmt_in.video, &p_chain->fmt_out.video ) );
    return p_chain->vctx_in;
}

static picture_t *FilterSingleChainedFilter( chained_filter_t *f, picture_t *p_pic )
{
    filter_t *p_filter = &f->filter;
    p_pic = p_filter->ops->filter_video( p_filter, p_pic );
    if( !p_pic )
        return NULL;

    if( !heimdall_picture_chain_IsEmpty( &f->pending ) )
    {
        msg_Warn( p_filter, "dropping pictures" );
        FilterDeletePictures( &f->pending );
    }
    f->pending = picture_GetAndResetChain( p_pic );
    return p_pic;
}

picture_t *filter_chain_VideoFilter( filter_chain_t *p_chain, picture_t *p_pic )
{
    if( p_pic )
    {
        chained_filter_t *f;
        heimdall_list_foreach( f, &p_chain->filter_list, node )
        {
            p_pic = FilterSingleChainedFilter( f, p_pic );
            if( !p_pic )
                break;
        }
        if( p_pic )
            return p_pic;
    }

    // look backward in filters for a pending picture
    chained_filter_t *b;
    heimdall_list_reverse_foreach( b, &p_chain->filter_list, node )
    {
        p_pic = heimdall_picture_chain_PopFront( &b->pending );
        if (p_pic == NULL)
            continue;

        // iterate forward through the next filters
        struct heimdall_list_it f_it = heimdall_list_it_b;
        heimdall_list_it_next(&f_it);
        for ( ; heimdall_list_it_continue(&f_it); heimdall_list_it_next(&f_it) )
        {
            chained_filter_t *f =
                container_of(f_it.current, chained_filter_t, node);
            p_pic = FilterSingleChainedFilter( f, p_pic );
            if( !p_pic )
                break;
        }
        if( p_pic )
            return p_pic;
    }
    return NULL;
}

void filter_chain_VideoFlush( filter_chain_t *p_chain )
{
    chained_filter_t *f;
    heimdall_list_foreach( f, &p_chain->filter_list, node )
    {
        filter_t *p_filter = &f->filter;

        FilterDeletePictures( &f->pending );

        filter_Flush( p_filter );
    }
}

int filter_chain_MouseFilter( filter_chain_t *p_chain, heimdall_mouse_t *p_dst, const heimdall_mouse_t *p_src )
{
    heimdall_mouse_t current = *p_src;
    chained_filter_t *f;

    heimdall_list_reverse_foreach( f, &p_chain->filter_list, node )
    {
        filter_t *p_filter = &f->filter;

        if( p_filter->ops->video_mouse )
        {
            heimdall_mouse_t old = f->mouse;
            heimdall_mouse_t filtered = current;

            f->mouse = current;
            if( p_filter->ops->video_mouse( p_filter, &filtered, &old) )
                return HEIMDALL_EGENERIC;
            current = filtered;
        }
    }

    *p_dst = current;
    return HEIMDALL_SUCCESS;
}

/* Helpers */
static void FilterDeletePictures( heimdall_picture_chain_t *pictures )
{
    while( !heimdall_picture_chain_IsEmpty( pictures ) )
    {
        picture_t *next = heimdall_picture_chain_PopFront( pictures );
        picture_Release( next );
    }
}
