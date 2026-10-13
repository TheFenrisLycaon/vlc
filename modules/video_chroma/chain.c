/*****************************************************************************
 * chain.c : chain multiple video filter modules as a last resort solution
 *****************************************************************************
 * Copyright (C) 2007-2017 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria dot org>
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

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_filter.h>
#include <heimdall_mouse.h>
#include <heimdall_picture.h>
#include <heimdall_chroma_probe.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int       ActivateConverter  ( filter_t * );
static int       ActivateFilter     ( filter_t * );
static void      Destroy            ( filter_t * );

heimdall_module_begin ()
    set_description( N_("Video filtering using a chain of video filter modules") )
    set_callback_video_converter( ActivateConverter, 1 )
    add_submodule ()
        set_callback_video_filter( ActivateFilter )
heimdall_module_end ()

/*****************************************************************************
 * Local prototypes.
 *****************************************************************************/
static picture_t *Chain         ( filter_t *, picture_t * );
static void Flush               ( filter_t * );
static int ChainMouse           ( filter_t *p_filter, heimdall_mouse_t *p_mouse, const heimdall_mouse_t *p_old );

static int BuildTransformChain( filter_t *p_filter );
static int BuildChromaChain( filter_t *p_filter );
static int BuildFilterChain( filter_t *p_filter );

static int CreateChain( filter_t *p_filter, const es_format_t *p_fmt_mid );
static void EsFormatMergeSize( es_format_t *p_dst,
                               const es_format_t *p_base,
                               const es_format_t *p_size );

typedef struct
{
    filter_chain_t *p_chain;
    filter_t *p_video_filter;
} filter_sys_t;

/* Restart filter callback */
static int RestartFilterCallback( heimdall_object_t *obj, char const *psz_name,
                                  heimdall_value_t oldval, heimdall_value_t newval,
                                  void *p_data )
{ HEIMDALL_UNUSED(obj); HEIMDALL_UNUSED(psz_name); HEIMDALL_UNUSED(oldval);
    HEIMDALL_UNUSED(newval);

    var_TriggerCallback( (heimdall_object_t *)p_data, "video-filter" );
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Buffer management
 *****************************************************************************/
static picture_t *BufferChainNew( filter_t *p_filter )
{
    filter_t *p_chain_parent = p_filter->owner.sys;
    // the last filter of the internal chain gets its pictures from the original
    // filter source
    return filter_NewPicture( p_chain_parent );
}

#define CHAIN_LEVEL_MAX 2

static heimdall_decoder_device * HoldChainDecoderDevice(heimdall_object_t *o, void *sys)
{
    HEIMDALL_UNUSED(o);
    filter_t *p_chain_parent = sys;
    return filter_HoldDecoderDevice( p_chain_parent );
}

static const struct filter_video_callbacks filter_video_chain_cbs =
{
    BufferChainNew, HoldChainDecoderDevice,
};

static const struct heimdall_filter_operations filter_ops = {
    .filter_video = Chain, .flush = Flush, .video_mouse=ChainMouse, .close = Destroy,
};

/*****************************************************************************
 * Activate: allocate a chroma function
 *****************************************************************************
 * This function allocates and initializes a chroma function
 *****************************************************************************/
static int Activate( filter_t *p_filter, int (*pf_build)(filter_t *) )
{
    filter_sys_t *p_sys;
    int i_ret = HEIMDALL_EGENERIC;

    p_sys = p_filter->p_sys = calloc( 1, sizeof( *p_sys ) );
    if( !p_sys )
        return HEIMDALL_ENOMEM;

    filter_owner_t owner = {
        .video = &filter_video_chain_cbs,
        .sys = p_filter,
    };

    p_sys->p_chain = filter_chain_NewVideo( p_filter, p_filter->b_allow_fmt_out_change, &owner );
    if( !p_sys->p_chain )
    {
        free( p_sys );
        return HEIMDALL_EGENERIC;
    }

    i_ret = pf_build( p_filter );

    if( i_ret )
    {
        /* Hum ... looks like this really isn't going to work. Too bad. */
        if (p_sys->p_video_filter)
            filter_DelProxyCallbacks( p_filter, p_sys->p_video_filter,
                                      RestartFilterCallback );
        filter_chain_Delete( p_sys->p_chain );
        free( p_sys );
        return HEIMDALL_EGENERIC;
    }
    if( p_filter->b_allow_fmt_out_change )
    {
        es_format_Clean( &p_filter->fmt_out );
        es_format_Copy( &p_filter->fmt_out,
                        filter_chain_GetFmtOut( p_sys->p_chain ) );
        p_filter->vctx_out = filter_chain_GetVideoCtxOut( p_sys->p_chain );
    }
    assert(p_filter->vctx_out == filter_chain_GetVideoCtxOut( p_sys->p_chain ));
    /* */
    p_filter->ops = &filter_ops;
    return HEIMDALL_SUCCESS;
}

static int ActivateConverter( filter_t *p_filter )
{
    const bool b_chroma = !video_format_IsSameChroma( &p_filter->fmt_in.video, &p_filter->fmt_out.video);
    const bool b_transform = p_filter->fmt_in.video.orientation != p_filter->fmt_out.video.orientation;

    if( !b_chroma && !b_transform )
        return HEIMDALL_EGENERIC;

    if( var_Type( heimdall_object_parent(p_filter), "chain-level" ) != 0 )
        return HEIMDALL_EGENERIC;
    var_Create( p_filter, "chain-level", HEIMDALL_VAR_INTEGER );

    int ret = Activate( p_filter, b_transform ? BuildTransformChain :
                        BuildChromaChain );

    var_Destroy( p_filter, "chain-level" );
    return ret;
}

static int ActivateFilter( filter_t *p_filter )
{
    if( !p_filter->b_allow_fmt_out_change || p_filter->psz_name == NULL )
        return HEIMDALL_EGENERIC;

    if( var_Type( heimdall_object_parent(p_filter), "chain-level" ) != 0 )
        return HEIMDALL_EGENERIC;
    var_Create( p_filter, "chain-level", HEIMDALL_VAR_INTEGER );

    int ret = Activate( p_filter, BuildFilterChain );

    var_Destroy( p_filter, "chain-level" );
    return ret;
}

static void Destroy( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    if (p_sys->p_video_filter)
        filter_DelProxyCallbacks( p_filter, p_sys->p_video_filter,
                                  RestartFilterCallback );
    filter_chain_Delete( p_sys->p_chain );
    free( p_sys );
}

/*****************************************************************************
 * Chain
 *****************************************************************************/
static picture_t *Chain( filter_t *p_filter, picture_t *p_pic )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    return filter_chain_VideoFilter( p_sys->p_chain, p_pic );
}

static void Flush( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    filter_chain_VideoFlush( p_sys->p_chain );
}

/*****************************************************************************
 * Builders
 *****************************************************************************/

static int BuildTransformChain( filter_t *p_filter )
{

    es_format_t fmt_mid;
    int i_ret;

    /* Lets try transform first, then (potentially) resize+chroma */
    msg_Dbg( p_filter, "Trying to build transform, then chroma+resize" );
    es_format_Copy( &fmt_mid, &p_filter->fmt_in );
    video_format_TransformTo(&fmt_mid.video, p_filter->fmt_out.video.orientation);
    i_ret = CreateChain( p_filter, &fmt_mid );
    es_format_Clean( &fmt_mid );
    if( i_ret == HEIMDALL_SUCCESS )
        return HEIMDALL_SUCCESS;

    /* Lets try resize+chroma first, then transform */
    msg_Dbg( p_filter, "Trying to build chroma+resize" );
    EsFormatMergeSize( &fmt_mid, &p_filter->fmt_out, &p_filter->fmt_in );
    i_ret = CreateChain( p_filter, &fmt_mid );
    es_format_Clean( &fmt_mid );
    return i_ret;
}

static int AppendChromaChain( filter_t *p_filter, const heimdall_fourcc_t *chromas,
                              size_t chroma_count, const es_format_t *last_fmt )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    es_format_t fmt_mid;

    for( size_t i = 0; i < chroma_count; ++i )
    {

        if( i == chroma_count - 1 && last_fmt != NULL )
        {
            assert( last_fmt->i_codec == chromas[i] );
            es_format_Copy( &fmt_mid, last_fmt );
        }
        else
        {
            es_format_Copy( &fmt_mid, &p_filter->fmt_in );
            fmt_mid.i_codec = fmt_mid.video.i_chroma = chromas[i];
        }

        int i_ret = filter_chain_AppendConverter( p_sys->p_chain, &fmt_mid );
        es_format_Clean( &fmt_mid );
        if ( i_ret != HEIMDALL_SUCCESS )
            return i_ret;
    }

    return HEIMDALL_SUCCESS;
}

static int BuildChromaChain( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    int i_ret = HEIMDALL_EGENERIC;

    const bool resize = p_filter->fmt_in.video.i_width  != p_filter->fmt_out.video.i_width ||
                        p_filter->fmt_in.video.i_height != p_filter->fmt_out.video.i_height;

    size_t res_count;
    struct heimdall_chroma_conv_result *results =
        heimdall_chroma_conv_Probe( p_filter->fmt_in.video.i_chroma,
                               p_filter->fmt_out.video.i_chroma,
                               p_filter->fmt_in.video.i_width,
                               p_filter->fmt_in.video.i_height, 1,
                               0, &res_count );
    if( results == NULL )
        return i_ret;

    /* Now try chroma format list */
    for( size_t i = 0; i < res_count; ++i )
    {
        const struct heimdall_chroma_conv_result *res = &results[i];
        char *res_str = heimdall_chroma_conv_result_ToString( res );
        if( res_str == NULL )
        {
            i_ret = HEIMDALL_ENOMEM;
            break;
        }
        msg_Info( p_filter, "Trying to use chroma_chain: %s...", res_str);
        free(res_str);

        filter_chain_Reset( p_sys->p_chain, &p_filter->fmt_in, p_filter->vctx_in,
                            &p_filter->fmt_out );

        /* Try the one-pass chain: the last converter handles chroma+resize. */
        i_ret = AppendChromaChain( p_filter, &res->chain[1], res->chain_count - 1,
                                   &p_filter->fmt_out );
        if( i_ret != HEIMDALL_SUCCESS && resize )
        {
            msg_Warn( p_filter, "one-pass chain failed, trying chroma chain, "
                      "then resize");
            filter_chain_Reset( p_sys->p_chain, &p_filter->fmt_in,
                                p_filter->vctx_in, &p_filter->fmt_out );

            i_ret = AppendChromaChain( p_filter, &res->chain[1],
                                       res->chain_count - 1, NULL );
            if( i_ret == HEIMDALL_SUCCESS )
                i_ret = filter_chain_AppendConverter( p_sys->p_chain,
                                                      &p_filter->fmt_out );
        }

        if( i_ret == HEIMDALL_SUCCESS )
        {
            p_filter->vctx_out = filter_chain_GetVideoCtxOut( p_sys->p_chain );
            msg_Info( p_filter, "success");
            break;
        }
    }
    free( results );

    return i_ret;
}

static int ChainMouse( filter_t *p_filter, heimdall_mouse_t *p_mouse,
                       const heimdall_mouse_t *p_old )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    return filter_chain_MouseFilter( p_sys->p_chain, p_mouse, p_old );
}

static bool
CheckFilterChroma( filter_t *p_filter, heimdall_fourcc_t chroma, const char *name )
{
    filter_t *test = heimdall_object_create( p_filter, sizeof(filter_t) );
    if (test == NULL)
        return false;

    es_format_t fmt = p_filter->fmt_out;
    fmt.i_codec = fmt.video.i_chroma = chroma;
    test->fmt_in = fmt;
    test->fmt_out = fmt;
    test->psz_name = name;

    test->p_module = heimdall_filter_LoadModule( test, "video filter", name, true );
    bool success = test->p_module != NULL;
    heimdall_filter_Delete( test );
    return success;
}

static int BuildFilterChain( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    int i_ret = HEIMDALL_EGENERIC;

    assert( p_filter->b_allow_fmt_out_change );

    /* Search for in -> x conversion */
    size_t res_count;
    struct heimdall_chroma_conv_result *results =
        heimdall_chroma_conv_Probe( p_filter->fmt_in.video.i_chroma, 0,
                               p_filter->fmt_in.video.i_width,
                               p_filter->fmt_in.video.i_height, 1,
                               0, &res_count );
    if( results == NULL )
        return i_ret;

    for( size_t i = 0; i < res_count; ++i )
    {
        const struct heimdall_chroma_conv_result *res = &results[i];
        assert(res->chain_count >= 2);

        /* Check first if the filter could accept the new output format. This
         * might be faster to fail now than failing after creating the whole
         * chroma chain */
        if( !CheckFilterChroma( p_filter, res->chain[res->chain_count - 1],
                                p_filter->psz_name ) )
            continue;

        char *res_str = heimdall_chroma_conv_result_ToString( res );
        if( res_str == NULL )
        {
            i_ret = HEIMDALL_ENOMEM;
            break;
        }

        msg_Info( p_filter, "Trying to use chain: %s -> %s",
                  res_str, p_filter->psz_name );

        free( res_str );

        filter_chain_Reset( p_sys->p_chain, &p_filter->fmt_in, p_filter->vctx_in,
                            &p_filter->fmt_out );

        i_ret = AppendChromaChain( p_filter, &res->chain[1],
                                   res->chain_count - 1, NULL );
        if( i_ret != HEIMDALL_SUCCESS )
            continue;

        p_sys->p_video_filter =
            filter_chain_AppendFilter( p_sys->p_chain,
                                       p_filter->psz_name, p_filter->p_cfg,
                                       filter_chain_GetFmtOut( p_sys->p_chain ) );
        if( p_sys->p_video_filter == NULL)
        {
            i_ret = HEIMDALL_EGENERIC;
            continue;
        }

        filter_AddProxyCallbacks( p_filter,
                                  p_sys->p_video_filter,
                                  RestartFilterCallback );

        p_filter->vctx_out = filter_chain_GetVideoCtxOut( p_sys->p_chain );
        break;
    }
    free( results );

    if( i_ret != HEIMDALL_SUCCESS )
        filter_chain_Reset( p_sys->p_chain, &p_filter->fmt_in, p_filter->vctx_in, &p_filter->fmt_out );

    return i_ret;
}

/*****************************************************************************
 *
 *****************************************************************************/
static int CreateChain( filter_t *p_filter, const es_format_t *p_fmt_mid )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    filter_chain_Reset( p_sys->p_chain, &p_filter->fmt_in, p_filter->vctx_in, &p_filter->fmt_out );

    int i_ret = filter_chain_AppendConverter( p_sys->p_chain, p_fmt_mid );
    if ( i_ret != HEIMDALL_SUCCESS )
        return i_ret;

    i_ret = filter_chain_AppendConverter( p_sys->p_chain, &p_filter->fmt_out );
    if ( i_ret != HEIMDALL_SUCCESS )
        goto error;

    p_filter->vctx_out = filter_chain_GetVideoCtxOut( p_sys->p_chain );
    return HEIMDALL_SUCCESS;
error:
    //Clean up.
    filter_chain_Clear( p_sys->p_chain );
    return HEIMDALL_EGENERIC;
}

static void EsFormatMergeSize( es_format_t *p_dst,
                               const es_format_t *p_base,
                               const es_format_t *p_size )
{
    es_format_Copy( p_dst, p_base );

    p_dst->video.i_width  = p_size->video.i_width;
    p_dst->video.i_height = p_size->video.i_height;

    p_dst->video.i_visible_width  = p_size->video.i_visible_width;
    p_dst->video.i_visible_height = p_size->video.i_visible_height;

    p_dst->video.i_x_offset = p_size->video.i_x_offset;
    p_dst->video.i_y_offset = p_size->video.i_y_offset;

    p_dst->video.orientation = p_size->video.orientation;
    p_dst->video.i_sar_num = p_size->video.i_sar_num;
    p_dst->video.i_sar_den = p_size->video.i_sar_den;
}
