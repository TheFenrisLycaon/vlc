/*****************************************************************************
 * nsv.c: NullSoft Video demuxer.
 *****************************************************************************
 * Copyright (C) 2004-2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

#include <limits.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_demux.h>

/* TODO:
 *  - implement NSVf parsing (to get meta data)
 *  - implement missing Control (and in the right way)
 *  - ...
 */

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open    ( heimdall_object_t * );
static void Close  ( heimdall_object_t * );

heimdall_module_begin ()
    set_description( N_("NullSoft demuxer" ) )
    set_capability( "demux", 10 )
    set_subcategory( SUBCAT_INPUT_DEMUX )
    set_callbacks( Open, Close )
    add_shortcut( "nsv" )
    add_file_extension("nsv")
heimdall_module_end ()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

typedef struct
{
    es_format_t  fmt_audio;
    es_out_id_t *p_audio;

    es_format_t  fmt_video;
    es_out_id_t *p_video;

    es_format_t  fmt_sub;
    es_out_id_t  *p_sub;

    heimdall_tick_t  i_pcr;
    heimdall_tick_t  i_time;
    heimdall_tick_t  i_pcr_inc;

    bool b_start_record;
    char *record_dir_path;
} demux_sys_t;

static int Demux  ( demux_t *p_demux );
static int Control( demux_t *p_demux, int i_query, va_list args );

static int ReSynch( demux_t *p_demux );

static int ReadNSVf( demux_t *p_demux );
static int ReadNSVs( demux_t *p_demux );

/*****************************************************************************
 * Open
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    demux_t     *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys;

    const uint8_t *p_peek;

    if( heimdall_stream_Peek( p_demux->s, &p_peek, 8 ) < 8 )
        return HEIMDALL_EGENERIC;

    if( memcmp( p_peek, "NSVf", 4 ) && memcmp( p_peek, "NSVs", 4 ) )
    {
       /* In case we had force this demuxer we try to resynch */
        if( !p_demux->obj.force || ReSynch( p_demux ) )
            return HEIMDALL_EGENERIC;
    }

    p_sys = malloc( sizeof( demux_sys_t ) );
    if( unlikely(p_sys == NULL) )
        return HEIMDALL_ENOMEM;

    /* Fill p_demux field */
    p_demux->p_sys = p_sys;
    p_demux->pf_demux = Demux;
    p_demux->pf_control = Control;

    es_format_Init( &p_sys->fmt_audio, AUDIO_ES, 0 );
    p_sys->p_audio = NULL;

    es_format_Init( &p_sys->fmt_video, VIDEO_ES, 0 );
    p_sys->p_video = NULL;

    es_format_Init( &p_sys->fmt_sub, SPU_ES, 0 );
    p_sys->p_sub = NULL;

    p_sys->i_pcr   = 0;
    p_sys->i_time  = 0;
    p_sys->i_pcr_inc = 0;

    p_sys->b_start_record = false;
    p_sys->record_dir_path = NULL;

    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Close
 *****************************************************************************/
static void Close( heimdall_object_t *p_this )
{
    demux_t     *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;

    free( p_sys->record_dir_path );
    free( p_sys );
}


/*****************************************************************************
 * Demux:
 *****************************************************************************/
static int Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    uint8_t     header[5];
    const uint8_t *p_peek;

    int         i_size;
    block_t     *p_frame;

    for( ;; )
    {
        if( heimdall_stream_Peek( p_demux->s, &p_peek, 8 ) < 8 )
            return HEIMDALL_DEMUXER_EOF;

        if( !memcmp( p_peek, "NSVf", 4 ) )
        {
            if( ReadNSVf( p_demux ) )
                return HEIMDALL_DEMUXER_EGENERIC;
        }
        else if( !memcmp( p_peek, "NSVs", 4 ) )
        {
            if( p_sys->b_start_record )
            {
                /* Enable recording once synchronized */
                heimdall_stream_Control( p_demux->s, STREAM_SET_RECORD_STATE, true,
                                    p_sys->record_dir_path, "nsv" );
                p_sys->b_start_record = false;
            }

            if( ReadNSVs( p_demux ) )
                return HEIMDALL_DEMUXER_EGENERIC;
            break;
        }
        else if( GetWLE( p_peek ) == 0xbeef )
        {
            /* Next frame of the current NSVs chunk */
            if( heimdall_stream_Read( p_demux->s, NULL, 2 ) != 2 )
            {
                msg_Warn( p_demux, "cannot read" );
                return HEIMDALL_DEMUXER_EOF;
            }
            break;
        }
        else
        {
            msg_Err( p_demux, "invalid signature 0x%x (%4.4s)", GetDWLE( p_peek ), (const char*)p_peek );
            if( ReSynch( p_demux ) )
                return HEIMDALL_DEMUXER_EGENERIC;
        }
    }

    if( heimdall_stream_Read( p_demux->s, header, 5 ) < 5 )
    {
        msg_Warn( p_demux, "cannot read" );
        return HEIMDALL_DEMUXER_EOF;
    }

    /* Set PCR */
    es_out_SetPCR( p_demux->out, HEIMDALL_TICK_0 + p_sys->i_pcr );

    /* Read video */
    i_size = ( header[0] >> 4 ) | ( header[1] << 4 ) | ( header[2] << 12 );
    if( i_size > 0 )
    {
        /* extra data ? */
        if( (header[0]&0x0f) != 0x0 )
        {
            uint8_t      aux[6];
            int          i_aux;
            heimdall_fourcc_t fcc;
            if( heimdall_stream_Read( p_demux->s, aux, 6 ) < 6 )
            {
                msg_Warn( p_demux, "cannot read" );
                return HEIMDALL_DEMUXER_EOF;
            }
            i_aux = GetWLE( aux );
            fcc   = HEIMDALL_FOURCC( aux[2], aux[3], aux[4], aux[5] );

            msg_Dbg( p_demux, "Belekas: %d - size=%d fcc=%4.4s",
                     header[0]&0xf, i_aux, (char*)&fcc );

            if( fcc == HEIMDALL_FOURCC( 'S', 'U', 'B', 'T' ) && i_aux > 2 )
            {
                if( p_sys->p_sub == NULL )
                {
                    p_sys->fmt_sub.i_codec = HEIMDALL_FOURCC( 's', 'u', 'b', 't' );
                    p_sys->p_sub = es_out_Add( p_demux->out, &p_sys->fmt_sub );
                    if( p_sys->p_sub )
                        es_out_Control( p_demux->out, ES_OUT_SET_ES, p_sys->p_sub );
                }
                if( heimdall_stream_Read( p_demux->s, NULL, 2 ) != 2 )
                    return HEIMDALL_DEMUXER_EOF;

                if( ( p_frame = heimdall_stream_Block( p_demux->s, i_aux - 2 ) ) )
                {
                    uint8_t *p = p_frame->p_buffer;

                    while( p < &p_frame->p_buffer[p_frame->i_buffer] && *p != 0 )
                    {
                        p++;
                    }
                    if( p + 1 < &p_frame->p_buffer[p_frame->i_buffer] && *p == 0 )
                    {
                        p_frame->i_buffer -= p + 1 - p_frame->p_buffer;
                        p_frame->p_buffer = p + 1;
                    }

                    /* Skip the first part (it is the language name) */
                    p_frame->i_pts = HEIMDALL_TICK_0 + p_sys->i_pcr;
                    p_frame->i_dts = HEIMDALL_TICK_0 + p_sys->i_pcr + HEIMDALL_TICK_FROM_SEC(4);

                    if( p_sys->p_sub )
                        es_out_Send( p_demux->out, p_sys->p_sub, p_frame );
                    else
                        block_Release( p_frame );
                }
            }
            else
            {
                /* We skip this extra data */
                if( heimdall_stream_Read( p_demux->s, NULL, i_aux ) != i_aux )
                {
                    msg_Warn( p_demux, "cannot read" );
                    return HEIMDALL_DEMUXER_EOF;
                }
            }
            i_size -= 6 + i_aux;
        }

        /* msg_Dbg( p_demux, "frame video size=%d", i_size ); */
        if( i_size > 0 && ( p_frame = heimdall_stream_Block( p_demux->s, i_size ) ) )
        {
            p_frame->i_dts = HEIMDALL_TICK_0 + p_sys->i_pcr;

            if( p_sys->p_video )
                es_out_Send( p_demux->out, p_sys->p_video, p_frame );
            else
            {
                block_Release( p_frame );
                msg_Dbg( p_demux, "ignoring unsupported video frame (size=%d)",
                         i_size );
            }
        }
    }

    /* Read audio */
    i_size = header[3] | ( header[4] << 8 );
    if( i_size > 0 )
    {
        /* msg_Dbg( p_demux, "frame audio size=%d", i_size ); */
        if( p_sys->fmt_audio.i_codec == HEIMDALL_FOURCC( 'a', 'r', 'a', 'w' ) )
        {
            uint8_t h[4];
            if( heimdall_stream_Read( p_demux->s, h, 4 ) < 4 )
                return HEIMDALL_DEMUXER_EOF;

            p_sys->fmt_audio.audio.i_channels = h[1];
            p_sys->fmt_audio.audio.i_rate = GetWLE( &h[2] );

            i_size -= 4;
        }
        if( p_sys->p_audio == NULL )
        {
            p_sys->p_audio = es_out_Add( p_demux->out, &p_sys->fmt_audio );
        }

        if( ( p_frame = heimdall_stream_Block( p_demux->s, i_size ) ) )
        {
            p_frame->i_dts =
            p_frame->i_pts = HEIMDALL_TICK_0 + p_sys->i_pcr;

            if( p_sys->p_audio )
                es_out_Send( p_demux->out, p_sys->p_audio, p_frame );
            else
            {
                block_Release( p_frame );
                msg_Dbg( p_demux, "ignoring unsupported audio frame (size=%d)",
                         i_size );
            }
        }
    }

    p_sys->i_pcr += p_sys->i_pcr_inc;
    if( p_sys->i_time >= 0 )
    {
        p_sys->i_time += p_sys->i_pcr_inc;
    }

    return HEIMDALL_DEMUXER_SUCCESS;
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    double f, *pf;
    bool b_bool, *pb_bool;
    int64_t i64;

    switch( i_query )
    {
        case DEMUX_CAN_SEEK:
            return heimdall_stream_vaControl( p_demux->s, i_query, args );

        case DEMUX_GET_POSITION:
            pf = va_arg( args, double * );
            i64 = stream_Size( p_demux->s );
            if( i64 > 0 )
            {
                double current = heimdall_stream_Tell( p_demux->s );
                *pf = current / (double)i64;
            }
            else
            {
                *pf = 0.0;
            }
            return HEIMDALL_SUCCESS;

        case DEMUX_SET_POSITION:
            f = va_arg( args, double );
            i64 = stream_Size( p_demux->s );

            if( heimdall_stream_Seek( p_demux->s, (int64_t)(i64 * f) ) || ReSynch( p_demux ) )
                return HEIMDALL_EGENERIC;

            p_sys->i_time = -1; /* Invalidate time display */
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_TIME:
            if( p_sys->i_time < 0 )
            {
                *va_arg( args, heimdall_tick_t * ) = 0;
                return HEIMDALL_EGENERIC;
            }
            *va_arg( args, heimdall_tick_t * ) = p_sys->i_time;
            return HEIMDALL_SUCCESS;

#if 0
        case DEMUX_GET_LENGTH:
            if( p_sys->i_mux_rate > 0 )
            {
                *va_arg( args, heimdall_tick_t * ) = heimdall_tick_from_samples( stream_Size( p_demux->s ) / 50, p_sys->i_mux_rate);
                return HEIMDALL_SUCCESS;
            }
            *va_arg( args, heimdall_tick_t * ) = 0;
            return HEIMDALL_EGENERIC;

#endif
        case DEMUX_GET_FPS:
            pf = va_arg( args, double * );
            *pf = (double)CLOCK_FREQ / (double)p_sys->i_pcr_inc;
            return HEIMDALL_SUCCESS;

        case DEMUX_CAN_RECORD:
            pb_bool = va_arg( args, bool * );
            *pb_bool = true;
            return HEIMDALL_SUCCESS;

        case DEMUX_SET_RECORD_STATE:
        {
            b_bool = (bool)va_arg( args, int );
            const char *dir_path = va_arg( args, const char * );

            free( p_sys->record_dir_path );
            p_sys->record_dir_path = NULL;

            if( !b_bool )
                heimdall_stream_Control( p_demux->s, STREAM_SET_RECORD_STATE, false );
            else if( dir_path != NULL )
            {
                p_sys->record_dir_path = strdup(dir_path);
                if( p_sys->record_dir_path == NULL )
                    return HEIMDALL_ENOMEM;
            }
            p_sys->b_start_record = b_bool;
            return HEIMDALL_SUCCESS;
        }

        case DEMUX_SET_TIME:
            return HEIMDALL_EGENERIC;

        case DEMUX_CAN_PAUSE:
        case DEMUX_SET_PAUSE_STATE:
        case DEMUX_CAN_CONTROL_PACE:
        case DEMUX_GET_PTS_DELAY:
            return demux_vaControlHelper( p_demux->s, 0, -1, 0, 1, i_query, args );

        default:
            return HEIMDALL_EGENERIC;
    }
}

/*****************************************************************************
 * ReSynch:
 *****************************************************************************/
static int ReSynch( demux_t *p_demux )
{
    for( ;; )
    {
        const uint8_t *p_peek;
        int i_peek = heimdall_stream_Peek( p_demux->s, &p_peek, 1024 );
        if( i_peek < 8 )
            break;

        int i_skip = 0;

        while( i_skip < i_peek - 4 )
        {
            if( !memcmp( p_peek, "NSVf", 4 )
             || !memcmp( p_peek, "NSVs", 4 ) )
            {
                if( i_skip > 0 && heimdall_stream_Read( p_demux->s, NULL, i_skip ) != i_skip )
                    return HEIMDALL_EGENERIC;
                return HEIMDALL_SUCCESS;
            }
            p_peek++;
            i_skip++;
        }

        if( heimdall_stream_Read( p_demux->s, NULL, i_skip ) != i_skip )
            break;
    }
    return HEIMDALL_EGENERIC;
}

/*****************************************************************************
 * ReadNSVf:
 *****************************************************************************/
static int ReadNSVf( demux_t *p_demux )
{
    /* demux_sys_t *p_sys = p_demux->p_sys; */
    const uint8_t     *p;

    msg_Dbg( p_demux, "new NSVf chunk" );
    if( heimdall_stream_Peek( p_demux->s, &p, 8 ) < 8 )
    {
        return HEIMDALL_EGENERIC;
    }

    uint32_t i_header_size = GetDWLE( &p[4] );
    msg_Dbg( p_demux, "    - size=%" PRIu32, i_header_size );

    if( i_header_size == 0 || i_header_size == UINT32_MAX )
        return HEIMDALL_EGENERIC;
#if (UINT32_MAX > SSIZE_MAX)
    if( i_header_size > SSIZE_MAX )
        return HEIMDALL_EGENERIC;
#endif
    if ( heimdall_stream_Read( p_demux->s, NULL, i_header_size ) != i_header_size )
         return HEIMDALL_EGENERIC;
    return HEIMDALL_SUCCESS;
}
/*****************************************************************************
 * ReadNSVs:
 *****************************************************************************/
static int ReadNSVs( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    uint8_t      header[19];
    heimdall_fourcc_t fcc;

    if( heimdall_stream_Read( p_demux->s, header, 19 ) < 19 )
    {
        msg_Warn( p_demux, "cannot read" );
        return HEIMDALL_EGENERIC;
    }

    /* Video */
    switch( ( fcc = HEIMDALL_FOURCC( header[4], header[5], header[6], header[7] ) ) )
    {
        case HEIMDALL_FOURCC( 'V', 'P', '3', ' ' ):
        case HEIMDALL_FOURCC( 'V', 'P', '3', '0' ):
            fcc = HEIMDALL_FOURCC( 'V', 'P', '3', '0' );
            break;

        case HEIMDALL_FOURCC( 'V', 'P', '3', '1' ):
            fcc = HEIMDALL_FOURCC( 'V', 'P', '3', '1' );
            break;

        case HEIMDALL_FOURCC( 'V', 'P', '5', ' ' ):
        case HEIMDALL_FOURCC( 'V', 'P', '5', '0' ):
            fcc = HEIMDALL_FOURCC( 'V', 'P', '5', '0' );
            break;
        case HEIMDALL_FOURCC( 'V', 'P', '6', '0' ):
        case HEIMDALL_FOURCC( 'V', 'P', '6', '1' ):
        case HEIMDALL_FOURCC( 'V', 'P', '6', '2' ):
        case HEIMDALL_FOURCC( 'V', 'P', '8', '0' ):
        case HEIMDALL_FOURCC( 'H', '2', '6', '4' ):
        case HEIMDALL_FOURCC( 'N', 'O', 'N', 'E' ):
            break;
        default:
            msg_Err( p_demux, "unsupported video codec %4.4s", (char *)&fcc );
            break;
    }
    if( fcc != HEIMDALL_FOURCC( 'N', 'O', 'N', 'E' ) && fcc != p_sys->fmt_video.i_codec  )
    {
        es_format_Init( &p_sys->fmt_video, VIDEO_ES, fcc );
        p_sys->fmt_video.video.i_width = GetWLE( &header[12] );
        p_sys->fmt_video.video.i_height = GetWLE( &header[14] );
        p_sys->fmt_video.video.i_visible_width = p_sys->fmt_video.video.i_width;
        p_sys->fmt_video.video.i_visible_height = p_sys->fmt_video.video.i_height;
        if( p_sys->p_video )
        {
            es_out_Del( p_demux->out, p_sys->p_video );
        }
        p_sys->p_video = es_out_Add( p_demux->out, &p_sys->fmt_video );

        msg_Dbg( p_demux, "    - video `%4.4s' %dx%d",
                 (char*)&fcc,
                 p_sys->fmt_video.video.i_width,
                 p_sys->fmt_video.video.i_height );
    }

    /* Audio */
    switch( ( fcc = HEIMDALL_FOURCC( header[8], header[9], header[10], header[11] ) ) )
    {
        case HEIMDALL_FOURCC( 'M', 'P', '3', ' ' ):
            fcc = HEIMDALL_FOURCC( 'm', 'p', 'g', 'a' );
            break;
        case HEIMDALL_FOURCC( 'P', 'C', 'M', ' ' ):
            fcc = HEIMDALL_FOURCC( 'a', 'r', 'a', 'w' );
            break;
        case HEIMDALL_FOURCC( 'A', 'A', 'C', ' ' ):
        case HEIMDALL_FOURCC( 'A', 'A', 'C', 'P' ):
            fcc = HEIMDALL_FOURCC( 'm', 'p', '4', 'a' );
            break;
        case HEIMDALL_FOURCC( 'S', 'P', 'X', ' ' ):
            fcc = HEIMDALL_FOURCC( 's', 'p', 'x', ' ' );
            break;
        case HEIMDALL_FOURCC( 'N', 'O', 'N', 'E' ):
            break;
        default:
            msg_Err( p_demux, "unsupported audio codec %4.4s", (char *)&fcc );
            break;
    }

    if( fcc != HEIMDALL_FOURCC( 'N', 'O', 'N', 'E' ) && fcc != p_sys->fmt_audio.i_codec )
    {
        msg_Dbg( p_demux, "    - audio `%4.4s'", (char*)&fcc );

        if( p_sys->p_audio )
        {
            es_out_Del( p_demux->out, p_sys->p_audio );
            p_sys->p_audio = NULL;
        }
        es_format_Init( &p_sys->fmt_audio, AUDIO_ES, fcc );
    }

    if( header[16]&0x80 )
    {
        /* Fractional frame rate */
        switch( header[16]&0x03 )
        {
            case 0: /* 30 fps */
                p_sys->i_pcr_inc = 33333; /* 300000/9 */
                break;
            case 1: /* 29.97 fps */
                p_sys->i_pcr_inc = 33367; /* 300300/9 */
                break;
            case 2: /* 25 fps */
                p_sys->i_pcr_inc = 40000; /* 360000/9 */
                break;
            case 3: /* 23.98 fps */
                p_sys->i_pcr_inc = 41700; /* 375300/9 */
                break;
        }

        if( header[16] < 0xc0 )
            p_sys->i_pcr_inc = p_sys->i_pcr_inc * (((header[16] ^ 0x80) >> 2 ) +1 );
        else
            p_sys->i_pcr_inc = p_sys->i_pcr_inc / (((header[16] ^ 0xc0) >> 2 ) +1 );
    }
    else if( header[16] != 0 )
    {
        /* Integer frame rate */
        p_sys->i_pcr_inc = heimdall_tick_from_samples(1, header[16]);
    }
    else
    {
        msg_Dbg( p_demux, "invalid fps (0x00)" );
        p_sys->i_pcr_inc = HEIMDALL_TICK_FROM_MS(40);
    }
    //msg_Dbg( p_demux, "    - fps=%.3f", 1000000.0 / (double)p_sys->i_pcr_inc );

    if( p_sys->p_audio == NULL && p_sys->p_video == NULL )
    {
        msg_Err( p_demux, "unable to play neither audio nor video, aborting." );
        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}
