/*****************************************************************************
 * stereo_pan.c : Stereo panning filter
 *****************************************************************************
 * Copyright © 2021 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vedanta Nayak <vedantnayak2@gmail.com>
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

#include <heimdall_common.h>
#include <heimdall_aout.h>
#include <heimdall_filter.h>
#include <heimdall_plugin.h>
#include <stdatomic.h>

#define PAN_CONTROL_TEXT N_("Pan control")
#define PAN_CONTROL_LONGTEXT N_(\
        "Set pan control between 0 (left) and 1 (right). Default: 0.5")

typedef struct
{
    _Atomic float f_pan;
}filter_sys_t;

static block_t *Process ( filter_t *p_filter, block_t *p_in_buf )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    float *p_in = (float *)p_in_buf->p_buffer;
    size_t i_nb_samples = p_in_buf->i_nb_samples;
    size_t i_nb_channels = aout_FormatNbChannels( &p_filter->fmt_in.audio );
    float f_pan = atomic_load( &p_sys->f_pan );

    for ( size_t i = 0 ; i < i_nb_samples ; ++i )
    {
        float f_left = p_in[ i * i_nb_channels ];
        float f_right = p_in[ i * i_nb_channels + 1 ];
        p_in[ i * i_nb_channels ] = f_left * (1 - f_pan);
        p_in[ i * i_nb_channels + 1 ] = f_right * f_pan;
    }
    return p_in_buf;
}

static int paramCallback( heimdall_object_t *p_this, char const *psz_var,
                          heimdall_value_t oldval, heimdall_value_t newval,
                          void *p_data)
{
    filter_sys_t *p_sys = p_data;

    HEIMDALL_UNUSED(oldval);
    HEIMDALL_UNUSED(p_this);
    HEIMDALL_UNUSED(psz_var);

    atomic_store( &p_sys->f_pan, newval.f_float);

    return HEIMDALL_SUCCESS;
}

static void Close (heimdall_object_t *p_in)
{
    filter_t *p_filter = (filter_t *)p_in;
    filter_sys_t *p_sys = p_filter->p_sys;
    heimdall_object_t *p_aout = heimdall_object_parent(p_filter);
    var_DelCallback( p_aout, "pan-control", paramCallback, p_sys );
    var_Destroy( p_aout, "pan-control" );
    free(p_sys);
}

static int Open (filter_t *p_filter)
{
    if (p_filter->fmt_out.audio.i_channels < 2) {
        msg_Err( p_filter, "At least 2 audio channels are required" );
        return HEIMDALL_EGENERIC;
    }
    heimdall_object_t *p_aout = heimdall_object_parent(p_filter);
    filter_sys_t *p_sys = p_filter->p_sys = malloc( sizeof(*p_sys) );
    if (unlikely(!p_sys))
        return HEIMDALL_ENOMEM;

    atomic_init( &p_sys->f_pan,
            var_CreateGetFloat( p_aout, "pan-control" ) );
    var_AddCallback( p_aout, "pan-control", paramCallback, p_sys );

    p_filter->fmt_out.audio.i_format = HEIMDALL_CODEC_FL32;
    p_filter->fmt_in.audio.i_format = HEIMDALL_CODEC_FL32;
    aout_FormatPrepare( &p_filter->fmt_in.audio );
    aout_FormatPrepare( &p_filter->fmt_out.audio );

    static const struct heimdall_filter_operations filter_ops =
    {
        .filter_audio = Process,
    };
    p_filter->ops = &filter_ops;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname ( N_("Stereo Pan") )
    set_description ( N_("Perform Stereo Panning") )
    set_subcategory ( SUBCAT_AUDIO_AFILTER )
    add_float_with_range( "pan-control", 0.5, 0, 1,
            PAN_CONTROL_TEXT, PAN_CONTROL_LONGTEXT )
    set_capability ( "audio filter", 0 )
    set_callbacks ( Open, Close )
heimdall_module_end ()
