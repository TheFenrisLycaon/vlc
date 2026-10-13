/*****************************************************************************
 * stats.c : stats plugin for heimdall
 *****************************************************************************
 * Copyright (C) 2000-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Example usage:
 *  $ heimdall movie.avi --sout="#transcode{aenc=dummy,venc=stats}:\
 *                          std{access=http,mux=dummy,dst=0.0.0.0:8081}"
 *  $ heimdall -vv http://127.0.0.1:8081 --demux=stats --vout=stats --codec=stats
 */

#define kBufferSize 0x500

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_demux.h>

/*** Decoder ***/
static int DecodeBlock( decoder_t *p_dec, block_t *p_block )
{
    picture_t * p_pic = NULL;

    if( p_block == NULL ) /* No Drain */
        return HEIMDALLDEC_SUCCESS;

    if( !decoder_UpdateVideoFormat( p_dec ) )
        p_pic = decoder_NewPicture( p_dec );
    if( !p_pic )
        goto error;

    heimdall_tick_t tick;
    if( p_block->i_buffer == kBufferSize )
    {
        memcpy(&tick, p_block->p_buffer, sizeof(heimdall_tick_t));
        msg_Dbg( p_dec, "got %"PRIu64" ms",
                 MS_FROM_HEIMDALL_TICK(tick) );
        msg_Dbg( p_dec, "got %"PRIu64" ms offset",
                 MS_FROM_HEIMDALL_TICK(heimdall_tick_now() - tick) );
    }
    else
    {
        msg_Dbg( p_dec, "got a packet not from stats demuxer" );
        tick = heimdall_tick_now();
    }
    memcpy(p_pic->p->p_pixels, &tick, sizeof(heimdall_tick_t));

    p_pic->date = p_block->i_pts != HEIMDALL_TICK_INVALID ?
            p_block->i_pts : p_block->i_dts;

error:
    block_Release( p_block );
    if ( p_pic )
        decoder_QueueVideo( p_dec, p_pic );
    return HEIMDALLDEC_SUCCESS;
}

static int OpenDecoder ( heimdall_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t*)p_this;

    msg_Dbg( p_this, "opening stats decoder" );

    /* Set callbacks */
    p_dec->pf_decode = DecodeBlock;

    /* */
    p_dec->fmt_out.i_codec = HEIMDALL_CODEC_I420;
    p_dec->fmt_out.video.i_width = 100;
    p_dec->fmt_out.video.i_height = 100;
    p_dec->fmt_out.video.i_sar_num = 1;
    p_dec->fmt_out.video.i_sar_den = 1;

    return HEIMDALL_SUCCESS;
}

/*** Encoder ***/
#ifdef ENABLE_SOUT
static block_t *EncodeVideo( encoder_t *p_enc, picture_t *p_pict )
{
    (void)p_pict;
    block_t * p_block = block_Alloc( kBufferSize );
    if (unlikely(!p_block))
      return NULL;
    heimdall_tick_t now = heimdall_tick_now();
    memcpy(p_block->p_buffer, &now, sizeof(heimdall_tick_t));
    p_block->i_buffer = kBufferSize;
    p_block->i_length = kBufferSize;
    p_block->i_dts = p_pict->date;

    msg_Dbg( p_enc, "putting %"PRIu64"ms",
             MS_FROM_HEIMDALL_TICK(now) );
    return p_block;
}

static block_t *EncodeAudio( encoder_t *p_enc, block_t *p_abuff )
{
    (void)p_abuff;
    (void)p_enc;
    return NULL;
}

static int OpenVideoEncoder ( heimdall_object_t *p_this )
{
    encoder_t *p_enc = (encoder_t *)p_this;

    msg_Dbg( p_this, "opening stats encoder" );

    static const struct heimdall_encoder_operations video_ops =
        { .encode_video = EncodeVideo };

    p_enc->ops = &video_ops;
    return HEIMDALL_SUCCESS;
}

static int OpenAudioEncoder ( heimdall_object_t *p_this )
{
    encoder_t *p_enc = (encoder_t *)p_this;

    msg_Dbg( p_this, "opening stats encoder" );

    static const struct heimdall_encoder_operations audio_ops =
        { .encode_audio = EncodeAudio };

    p_enc->ops = &audio_ops;
    return HEIMDALL_SUCCESS;
}
#endif

/*** Demuxer ***/
typedef struct
{
    es_format_t     fmt;
    es_out_id_t     *p_es;

    date_t          pts;
} demux_sys_t;

static int Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    block_t * p_block = heimdall_stream_Block( p_demux->s, kBufferSize );

    if( !p_block ) return 1;

    p_block->i_dts = p_block->i_pts = date_Increment( &p_sys->pts, kBufferSize );

    heimdall_tick_t tick;
    memcpy(&tick, p_block->p_buffer, sizeof(heimdall_tick_t));
    msg_Dbg( p_demux, "demux got %"PRId64" ms offset",
             MS_FROM_HEIMDALL_TICK(heimdall_tick_now() - tick) );

    //es_out_SetPCR( p_demux->out, p_block->i_pts );

    es_out_Send( p_demux->out, p_sys->p_es, p_block );

    return 1;
}

static int DemuxControl( demux_t *p_demux, int i_query, va_list args )
{
    return demux_vaControlHelper( p_demux->s,
                                   0, 0, 0, 1,
                                   i_query, args );
}

static int OpenDemux ( heimdall_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys;

    p_demux->p_sys = NULL;

    /* Only when selected */
    if( *p_demux->psz_name == '\0' )
        return HEIMDALL_EGENERIC;

    msg_Dbg( p_demux, "Init Stat demux" );

    p_demux->pf_demux   = Demux;
    p_demux->pf_control = DemuxControl;

    p_demux->p_sys = p_sys = malloc( sizeof( demux_sys_t ) );
    if( !p_demux->p_sys )
        return HEIMDALL_ENOMEM;

    date_Init( &p_sys->pts, 1, 1 );
    date_Set( &p_sys->pts, HEIMDALL_TICK_0 );

    es_format_Init( &p_sys->fmt, VIDEO_ES, HEIMDALL_FOURCC('s','t','a','t') );
    p_sys->fmt.video.i_width = 720;
    p_sys->fmt.video.i_height= 480;

    p_sys->p_es = es_out_Add( p_demux->out, &p_sys->fmt );

    return HEIMDALL_SUCCESS;
}

static void CloseDemux ( heimdall_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;

    msg_Dbg( p_demux, "Closing Stat demux" );

    free( p_demux->p_sys );
}

heimdall_module_begin ()
    set_shortname( N_("Stats"))
#ifdef ENABLE_SOUT
    set_description( N_("Stats encoder function") )
    set_capability( "video encoder", 0 )
    add_shortcut( "stats" )
    set_callback( OpenVideoEncoder )

    add_submodule ()
    set_description( N_("Stats encoder function") )
    set_capability( "audio encoder", 0 )
    add_shortcut( "stats" )
    set_callback( OpenAudioEncoder )

    add_submodule ()
#endif
        set_section( N_( "Stats decoder" ), NULL )
        set_description( N_("Stats decoder function") )
        set_capability( "video decoder", 0 )
        add_shortcut( "stats" )
        set_callback( OpenDecoder )
    add_submodule()
        set_section( N_( "Stats decoder" ), NULL )
        set_description( N_("Stats decoder function") )
        set_capability( "audio decoder", 0 )
        add_shortcut( "stats" )
        set_callback( OpenDecoder )
    add_submodule()
        set_section( N_( "Stats decoder" ), NULL )
        set_description( N_("Stats decoder function") )
        set_capability( "spu decoder", 0 )
        add_shortcut( "stats" )
        set_callback( OpenDecoder )
    add_submodule ()
        set_section( N_( "Stats demux" ), NULL )
        set_description( N_("Stats demux function") )
        set_capability( "demux", 0 )
        add_shortcut( "stats" )
        set_callbacks( OpenDemux, CloseDemux )
heimdall_module_end ()
