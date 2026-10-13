/*****************************************************************************
 * idummy.c: dummy input plugin, to manage "heimdall://" special options
 *****************************************************************************
 * Copyright (C) 2001, 2002 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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
#include <heimdall_interface.h>
#include <heimdall_demux.h>
#include <heimdall_charset.h>

static int OpenDemux( heimdall_object_t * );

heimdall_module_begin ()
    set_shortname( N_("Dummy") )
    set_description( N_("Dummy input") )
    set_capability( "access", 0 )
    set_callback( OpenDemux )
    add_shortcut( "dummy", "heimdall" )
heimdall_module_end ()

static int DemuxControl( demux_t *, int, va_list );

static int DemuxNoOp( demux_t *demux )
{
    (void) demux;
    return 0;
}

static int DemuxHold( demux_t *demux )
{
    (void) demux;
    heimdall_tick_sleep( HEIMDALL_HARD_MIN_SLEEP ); /* FIXME!!! */
    return 1;
}

typedef struct
{
    heimdall_tick_t end;
    heimdall_tick_t length;
} demux_sys_t;

static int DemuxPause( demux_t *demux )
{
    demux_sys_t *p_sys = demux->p_sys;
    heimdall_tick_t now = heimdall_tick_now();

    if( now >= p_sys->end )
        return 0;

    heimdall_tick_sleep( HEIMDALL_HARD_MIN_SLEEP ); /* FIXME!!! */
    return 1;
}

static int ControlPause( demux_t *demux, int query, va_list args )
{
    demux_sys_t *p_sys = demux->p_sys;

    switch( query )
    {
        case DEMUX_GET_POSITION:
        {
            double *ppos = va_arg( args, double * );
            double pos;
            heimdall_tick_t now = heimdall_tick_now();

            pos = 1. + ((double)(now - p_sys->end) / (double)p_sys->length);
            *ppos = (pos <= 1.) ? pos : 1.;
            break;
        }

        case DEMUX_SET_POSITION:
        {
            double pos = va_arg( args, double );
            heimdall_tick_t now = heimdall_tick_now();

            p_sys->end = now + (p_sys->length * (1. - pos));
            break;
        }

        case DEMUX_GET_LENGTH:
        {
            *va_arg( args, heimdall_tick_t * ) = p_sys->length;
            break;
        }

        case DEMUX_GET_TIME:
        {
            heimdall_tick_t *ppos = va_arg( args, heimdall_tick_t * );
            *ppos = heimdall_tick_now() + p_sys->length - p_sys->end;
            break;
        }

        case DEMUX_SET_TIME:
        {
            heimdall_tick_t pos = va_arg( args, heimdall_tick_t );
            p_sys->end = heimdall_tick_now() + p_sys->length - pos;
            break;
        }

        case DEMUX_CAN_SEEK:
            *va_arg( args, bool * ) = true;
            break;

        default:
            return DemuxControl( demux, query, args );
    }
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * OpenDemux: initialize the target, ie. parse the command
 *****************************************************************************/
static int OpenDemux( heimdall_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    const char *psz_name = p_demux->psz_location;

    p_demux->p_sys = NULL;

    /* Check for a "heimdall://nop" command */
    if( !strcasecmp( psz_name, "nop" ) )
    {
nop:
        msg_Info( p_demux, "command `nop'" );
        p_demux->pf_demux = DemuxNoOp;
        p_demux->pf_control = DemuxControl;
        return HEIMDALL_SUCCESS;
    }

    /* Check for a "heimdall://quit" command */
    if( !strcasecmp( psz_name, "quit" ) )
    {
        msg_Info( p_demux, "command `quit'" );
        p_demux->pf_demux = DemuxNoOp;
        p_demux->pf_control = DemuxControl;
        libheimdall_Quit( heimdall_object_instance(p_demux) );
        return HEIMDALL_SUCCESS;
    }

    if( !strcasecmp( psz_name, "pause" ) )
    {
        msg_Info( p_demux, "command `pause'" );

        p_demux->pf_demux = DemuxHold;
        p_demux->pf_control = DemuxControl;
        return HEIMDALL_SUCCESS;
    }

    /* Check for a "heimdall://pause:***" command */
    if( !strncasecmp( psz_name, "pause:", 6 ) )
    {
        double f = heimdall_atof_c( psz_name + 6 );
        heimdall_tick_t length = heimdall_tick_from_sec( f );

        msg_Info( p_demux, "command `pause %f'", f );
        if( length == 0 )
            goto nop; /* avoid division by zero */

        demux_sys_t *p_sys = heimdall_obj_malloc( p_this, sizeof( *p_sys ) );
        if( p_sys == NULL )
            return HEIMDALL_ENOMEM;

        p_sys->end = heimdall_tick_now() + length;
        p_sys->length = length;

        p_demux->p_sys = p_sys;
        p_demux->pf_demux = DemuxPause;
        p_demux->pf_control = ControlPause;
        return HEIMDALL_SUCCESS;
    }

    msg_Err( p_demux, "unknown command `%s'", psz_name );
    return HEIMDALL_EGENERIC;
}

static int DemuxControl( demux_t *p_demux, int i_query, va_list args )
{
    (void)p_demux; (void)i_query; (void)args;
    switch( i_query )
    {
    case DEMUX_GET_PTS_DELAY:
    {
        *va_arg( args, heimdall_tick_t * ) = DEFAULT_PTS_DELAY;
        return HEIMDALL_SUCCESS;
    }
    default:
        return HEIMDALL_EGENERIC;
    }
}
