/*****************************************************************************
 * rdp.c: libfreeRDP based Remote Desktop access
 *****************************************************************************
 * Copyright (C) 2013 VideoLAN and VLC Authors
 * Copyright (C) 2026 TheFenrisLycaon
 *****************************************************************************
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdckdint.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_poll.h>
#include <heimdall_plugin.h>
#include <heimdall_demux.h>
#include <heimdall_url.h>
#include <heimdall_meta.h>

#define boolean bool

/* see MS-RDPBCGR http://msdn.microsoft.com/en-us/library/cc240445.aspx */

#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#include <freerdp/channels/channels.h>
#include <freerdp/gdi/gdi.h>

#if !defined(FREERDP_INTERFACE_VERSION)
# include <freerdp/version.h>
#endif

#include <errno.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#define USER_TEXT N_("Username")
#define USER_LONGTEXT N_("Username that will be used for the connection, " \
        "if no username is set in the URL.")
#define PASS_TEXT N_("Password")
#define PASS_LONGTEXT N_("Password that will be used for the connection, " \
        "if no username or password are set in URL.")

#define RDP_ENCRYPT N_("Encrypted connection")
#define RDP_FPS N_("Frame rate")
#define RDP_FPS_LONGTEXT N_("Acquisition rate (in fps)")

#define CFG_PREFIX "rdp-"

#define FREERDP_PIXEL_BPP(_format) (_format >> 24)

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( heimdall_object_t * );
static void Close( heimdall_object_t * );

heimdall_module_begin()
    set_shortname( N_("RDP") )
    add_shortcut( "rdp" )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    set_description( N_("RDP Remote Desktop") )
    set_capability( "access", 0 )

    add_string( CFG_PREFIX "user", NULL, USER_TEXT, USER_LONGTEXT )
        change_safe()
    add_password(CFG_PREFIX "password", NULL, PASS_TEXT, PASS_LONGTEXT)
        change_safe()
    add_float( CFG_PREFIX "fps", 5, RDP_FPS, RDP_FPS_LONGTEXT )

    add_bool( CFG_PREFIX "encrypt", false, RDP_ENCRYPT, NULL )
        change_safe()

    set_callbacks( Open, Close )
heimdall_module_end()

#define RDP_MAX_FD 32

typedef struct
{
    heimdall_thread_t thread;
    freerdp *p_instance;
    block_t *p_block;
    unsigned i_framebuffersize;

    float f_fps;
    int i_frame_interval;
    heimdall_tick_t i_starttime;

    es_out_id_t *es;

    /* pre-connect params */
    char *psz_hostname;
    int i_port;
} demux_sys_t;

/* context */

struct heimdallrdp_context_t
{
    rdpContext rdp_context; /* Extending API's struct */
    demux_t *p_demux;
    rdpSettings* p_settings;
};
typedef struct heimdallrdp_context_t heimdallrdp_context_t;

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

/* updates handlers */

static BOOL desktopResizeHandler( rdpContext *p_context )
{
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_context;
    demux_sys_t *p_sys = p_heimdallcontext->p_demux->p_sys;
    rdpGdi *p_gdi = p_context->gdi;
    int i_colordepth = FREERDP_PIXEL_BPP( p_gdi->dstFormat );

    if ( p_sys->es )
    {
        es_out_Del( p_heimdallcontext->p_demux->out, p_sys->es );
        p_sys->es = NULL;
    }

    heimdall_fourcc_t i_chroma;
    /* Now init and fill es format */
    switch ( p_gdi->dstFormat )
    {
        default:
            msg_Dbg( p_heimdallcontext->p_demux, "invalid color depth %d", i_colordepth);
            /* fallthrough */
        case PIXEL_FORMAT_RGB16:
            i_chroma = HEIMDALL_CODEC_RGB565LE;
            break;
        case PIXEL_FORMAT_BGR16:
            i_chroma = HEIMDALL_CODEC_BGR565LE;
            break;
        case PIXEL_FORMAT_RGB15:
            i_chroma = HEIMDALL_CODEC_RGB555LE;
            break;
        case PIXEL_FORMAT_BGR15:
            i_chroma = HEIMDALL_CODEC_BGR555LE;
            break;
        case PIXEL_FORMAT_RGB24:
            i_chroma = HEIMDALL_CODEC_RGB24;
            break;
        case PIXEL_FORMAT_BGR24:
            i_chroma = HEIMDALL_CODEC_BGR24;
            break;
        case PIXEL_FORMAT_ARGB32:
            i_chroma = HEIMDALL_CODEC_ARGB;
            break;
        case PIXEL_FORMAT_XRGB32:
            i_chroma = HEIMDALL_CODEC_XRGB;
            break;
        case PIXEL_FORMAT_ABGR32:
            i_chroma = HEIMDALL_CODEC_ABGR;
            break;
        case PIXEL_FORMAT_XBGR32:
            i_chroma = HEIMDALL_CODEC_XBGR;
            break;
        case PIXEL_FORMAT_BGRA32:
            i_chroma = HEIMDALL_CODEC_BGRA;
            break;
        case PIXEL_FORMAT_BGRX32:
            i_chroma = HEIMDALL_CODEC_BGRX;
            break;
        case PIXEL_FORMAT_RGBA32:
            i_chroma = HEIMDALL_CODEC_RGBA;
            break;
        case PIXEL_FORMAT_RGBX32:
            i_chroma = HEIMDALL_CODEC_RGBX;
            break;
    }

    es_format_t fmt;
    es_format_Init( &fmt, VIDEO_ES, i_chroma );
    video_format_Setup( &fmt.video, i_chroma, p_gdi->width, p_gdi->height,
                        p_gdi->width, p_gdi->height, 1, 1);

    fmt.video.i_frame_rate_base = 1000;
    fmt.video.i_frame_rate = 1000 * p_sys->f_fps;
    if (ckd_mul(&p_sys->i_framebuffersize, p_gdi->width, p_gdi->height) &&
        ckd_mul(&p_sys->i_framebuffersize, p_sys->i_framebuffersize, i_colordepth >> 3) )
    {
        msg_Err( p_heimdallcontext->p_demux, "framebuffer size overflow");
        return FALSE;
    }

    if ( p_sys->p_block )
        p_sys->p_block = block_Realloc( p_sys->p_block, 0, p_sys->i_framebuffersize );
    else
        p_sys->p_block = block_Alloc( p_sys->i_framebuffersize );

    p_sys->es = es_out_Add( p_heimdallcontext->p_demux->out, &fmt );
    return TRUE;
}

static BOOL beginPaintHandler( rdpContext *p_context )
{
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_context;
    demux_sys_t *p_sys = p_heimdallcontext->p_demux->p_sys;
    rdpGdi *p_gdi = p_context->gdi;
    p_gdi->primary->hdc->hwnd->invalid->null = TRUE;
    if ( ! p_sys->p_block && p_sys->i_framebuffersize )
        p_sys->p_block = block_Alloc( p_sys->i_framebuffersize );
    return TRUE;
}

static BOOL endPaintHandler( rdpContext *p_context )
{
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_context;
    demux_sys_t *p_sys = p_heimdallcontext->p_demux->p_sys;
    rdpGdi *p_gdi = p_context->gdi;

    if ( p_sys->p_block && p_gdi->primary_buffer )
    {
        p_sys->p_block->i_buffer = p_sys->i_framebuffersize;
        memcpy( p_sys->p_block->p_buffer, p_gdi->primary_buffer, p_sys->p_block->i_buffer );
    }
    return TRUE;
}

/* instance handlers */

static BOOL preConnectHandler( freerdp *p_instance )
{
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_instance->context;
    demux_sys_t *p_sys = p_heimdallcontext->p_demux->p_sys;

    /* Configure connection */
    p_instance->settings->SoftwareGdi = true; /* render in buffer */
    p_instance->settings->Fullscreen = true;
    p_instance->settings->ServerHostname = strdup( p_sys->psz_hostname );
    p_instance->settings->Username =
            var_InheritString( p_heimdallcontext->p_demux, CFG_PREFIX "user" );
    p_instance->settings->Password =
            var_InheritString( p_heimdallcontext->p_demux, CFG_PREFIX "password" );
    p_instance->settings->ServerPort = p_sys->i_port;
    p_instance->settings->EncryptionMethods =
            var_InheritBool( p_heimdallcontext->p_demux, CFG_PREFIX "encrypt" );

    return TRUE;
}

static BOOL postConnectHandler( freerdp *p_instance )
{
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_instance->context;

    msg_Dbg( p_heimdallcontext->p_demux, "connected to desktop %dx%d (%d bpp)",
             p_instance->settings->DesktopWidth,
             p_instance->settings->DesktopHeight,
             p_instance->settings->ColorDepth
             );

    p_instance->update->DesktopResize = desktopResizeHandler;
    p_instance->update->BeginPaint = beginPaintHandler;
    p_instance->update->EndPaint = endPaintHandler;
    UINT32 format;
    switch ( p_instance->settings->ColorDepth )
    {
        default:
            msg_Dbg( p_heimdallcontext->p_demux, "no valid pixel format found for color depth %d bpp", p_instance->settings->ColorDepth);
            /* fallthrough */
        case 16:
            format = PIXEL_FORMAT_RGB16;
            break;
        case 24:
            format = PIXEL_FORMAT_RGB24;
            break;
        case 32:
            format = PIXEL_FORMAT_ARGB32;
            break;
    }

    if ( gdi_init( p_instance, format ) != TRUE )
        return FALSE;

    desktopResizeHandler( p_instance->context );
    return TRUE;
}

static BOOL authenticateHandler( freerdp *p_instance, char** ppsz_username,
                                 char** ppsz_password, char** ppsz_domain )
{
    HEIMDALL_UNUSED(ppsz_domain);
    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_instance->context;
    *ppsz_username = var_InheritString( p_heimdallcontext->p_demux, CFG_PREFIX "user" );
    *ppsz_password = var_InheritString( p_heimdallcontext->p_demux, CFG_PREFIX "password" );
    return TRUE;
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    bool *pb;
    double *p_dbl;
    heimdall_meta_t *p_meta;

    switch( i_query )
    {
        case DEMUX_CAN_PAUSE:
        case DEMUX_CAN_SEEK:
        case DEMUX_CAN_CONTROL_PACE:
        case DEMUX_CAN_CONTROL_RATE:
        case DEMUX_HAS_UNSUPPORTED_META:
            pb = va_arg( args, bool * );
            *pb = false;
            return HEIMDALL_SUCCESS;

        case DEMUX_CAN_RECORD:
            pb = va_arg( args, bool * );
            *pb = true;
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_PTS_DELAY:
            *va_arg( args, heimdall_tick_t * ) =
                HEIMDALL_TICK_FROM_MS(var_InheritInteger( p_demux, "live-caching" ));
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_TIME:
            *va_arg( args, heimdall_tick_t * ) = heimdall_tick_now() - p_sys->i_starttime;
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_LENGTH:
            *va_arg( args, heimdall_tick_t * ) = 0;
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_FPS:
            p_dbl = va_arg( args, double * );
            *p_dbl = p_sys->f_fps;
            return HEIMDALL_SUCCESS;

        case DEMUX_GET_META:
            p_meta = va_arg( args, heimdall_meta_t * );
            heimdall_meta_Set( p_meta, heimdall_meta_Title, p_demux->psz_location );
            return HEIMDALL_SUCCESS;

        default:
            return HEIMDALL_EGENERIC;
    }
}

/*****************************************************************************
 * Demux:
 *****************************************************************************/
static void *DemuxThread( void *p_data )
{
    demux_t *p_demux = (demux_t *) p_data;
    demux_sys_t *p_sys = p_demux->p_sys;
    p_sys->i_starttime = heimdall_tick_now();
    heimdall_tick_t i_next_frame_date = heimdall_tick_now() + p_sys->i_frame_interval;
    int i_ret;

    heimdall_thread_set_name("heimdall-rdp");

    for(;;)
    {
        i_ret = 0;
        int cancel_state = heimdall_savecancel();
        if ( freerdp_shall_disconnect( p_sys->p_instance ) )
        {
            heimdall_restorecancel( cancel_state );
            msg_Warn( p_demux, "RDP server closed session" );
            es_out_Del( p_demux->out, p_sys->es );
            p_sys->es = NULL;
            return NULL;
        }

        struct
        {
            void* pp_rfds[RDP_MAX_FD]; /* Declared by rdp */
            void* pp_wfds[RDP_MAX_FD];
            int i_nbr;
            int i_nbw;
            struct pollfd ufds[RDP_MAX_FD];
        } fds;

        fds.i_nbr = fds.i_nbw = 0;

        if ( freerdp_get_fds( p_sys->p_instance, fds.pp_rfds, &fds.i_nbr,
                              fds.pp_wfds, &fds.i_nbw ) != true )
        {
            heimdall_restorecancel( cancel_state );
            msg_Err( p_demux, "cannot get FDS" );
        }
        else
        if ( (fds.i_nbr + fds.i_nbw) > 0 && p_sys->es )
        {
            heimdall_restorecancel( cancel_state );
            int i_count = 0;

            for( int i = 0; i < fds.i_nbr; i++ )
            {
                fds.ufds[ i_count ].fd = (long) fds.pp_rfds[ i ];
                fds.ufds[ i_count ].events = POLLIN ;
                fds.ufds[ i_count++ ].revents = 0;
            }
            for( int i = 0; i < fds.i_nbw && i_count < RDP_MAX_FD; i++ )
            {   /* may be useless */
                fds.ufds[ i_count ].fd = (long) fds.pp_wfds[ i ];
                fds.ufds[ i_count ].events = POLLOUT;
                fds.ufds[ i_count++ ].revents = 0;
            }
            i_ret = poll( fds.ufds, i_count, p_sys->i_frame_interval * 1000/2 );
        } else {
            heimdall_restorecancel( cancel_state );
        }

        heimdall_tick_wait( i_next_frame_date );
        i_next_frame_date += p_sys->i_frame_interval;

        if ( i_ret >= 0 )
        {
            /* Do the rendering */
            cancel_state = heimdall_savecancel();
            freerdp_check_fds( p_sys->p_instance );
            heimdall_restorecancel( cancel_state );
            block_t *p_block = block_Duplicate( p_sys->p_block );
            if (likely( p_block && p_sys->p_block ))
            {
                p_sys->p_block->i_dts = p_sys->p_block->i_pts = heimdall_tick_now() - p_sys->i_starttime;
                es_out_SetPCR( p_demux->out, p_sys->p_block->i_pts );
                es_out_Send( p_demux->out, p_sys->es, p_sys->p_block );
                p_sys->p_block = p_block;
            }
        }
    }
    return NULL;
}

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    demux_t      *p_demux = (demux_t*)p_this;
    demux_sys_t  *p_sys;

    if (p_demux->out == NULL)
        return HEIMDALL_EGENERIC;

    p_sys = heimdall_obj_calloc( p_this, 1, sizeof(demux_sys_t) );
    if( !p_sys ) return HEIMDALL_ENOMEM;

    p_sys->f_fps = var_InheritFloat( p_demux, CFG_PREFIX "fps" );
    if ( p_sys->f_fps <= 0 ) p_sys->f_fps = 1.0;
    p_sys->i_frame_interval = CLOCK_FREQ / p_sys->f_fps;

    p_sys->p_instance = freerdp_new();
    if ( !p_sys->p_instance )
    {
        msg_Err( p_demux, "rdp instantiation error" );
        return HEIMDALL_EGENERIC;
    }

    p_demux->p_sys = p_sys;
    p_sys->p_instance->PreConnect = preConnectHandler;
    p_sys->p_instance->PostConnect = postConnectHandler;
    p_sys->p_instance->Authenticate = authenticateHandler;

    /* Set up context handlers and let it be allocated */
    p_sys->p_instance->ContextSize = sizeof( heimdallrdp_context_t );
    freerdp_context_new( p_sys->p_instance );

    heimdallrdp_context_t * p_heimdallcontext = (heimdallrdp_context_t *) p_sys->p_instance->context;
    p_heimdallcontext->p_demux = p_demux;

    /* Parse uri params for pre-connect */
    heimdall_url_t url;
    heimdall_UrlParse( &url, p_demux->psz_url );

    if ( !EMPTY_STR(url.psz_host) )
        p_sys->psz_hostname = strdup( url.psz_host );
    else
        p_sys->psz_hostname = strdup( "localhost" );

    p_sys->i_port = ( url.i_port > 0 ) ? url.i_port : 3389;

    heimdall_UrlClean( &url );

    if ( ! freerdp_connect( p_sys->p_instance ) )
    {
        msg_Err( p_demux, "can't connect to rdp server" );
        goto error;
    }

    if ( heimdall_clone( &p_sys->thread, DemuxThread, p_demux ) != HEIMDALL_SUCCESS )
    {
        msg_Err( p_demux, "can't spawn thread" );
        freerdp_disconnect( p_sys->p_instance );
        goto error;
    }

    p_demux->pf_demux = NULL;
    p_demux->pf_control = Control;

    return HEIMDALL_SUCCESS;

error:
    freerdp_free( p_sys->p_instance );
    free( p_sys->psz_hostname );
    return HEIMDALL_EGENERIC;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( heimdall_object_t *p_this )
{
    demux_t     *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;

    heimdall_cancel( p_sys->thread );
    heimdall_join( p_sys->thread, NULL );

    if ( p_sys->es )
        es_out_Del( p_demux->out, p_sys->es );

    freerdp_disconnect( p_sys->p_instance );
    freerdp_free( p_sys->p_instance );

    if ( p_sys->p_block )
        block_Release( p_sys->p_block );

    free( p_sys->psz_hostname );
}
