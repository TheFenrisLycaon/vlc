/*****************************************************************************
 * spatializer.cpp: sound reverberation
 *****************************************************************************
 * Copyright (C) 2004, 2006, 2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Google Summer of Code 2007
 *
 * Authors: Biodun Osunkunle <biodun@videolan.org>
 *
 * Mentor : Jean-Baptiste Kempf <jb@videolan.org>
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

#include <stdlib.h>                                      /* malloc(), free() */
#include <math.h>

#include <new>
using std::nothrow;

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_aout.h>
#include <heimdall_filter.h>

#include "revmodel.hpp"
#define SPAT_AMP 0.3

#include <array>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( heimdall_object_t * );

#define ROOMSIZE_TEXT N_("Room size")
#define ROOMSIZE_LONGTEXT N_("Defines the virtual surface of the room" \
                             " emulated by the filter." )

#define WIDTH_TEXT N_("Room width")
#define WIDTH_LONGTEXT N_("Width of the virtual room")

#define WET_TEXT N_("Wet")
#define WET_LONGTEXT NULL

#define DRY_TEXT N_("Dry")
#define DRY_LONGTEXT NULL

#define DAMP_TEXT N_("Damp")
#define DAMP_LONGTEXT NULL

heimdall_module_begin ()
    set_description( N_("Audio Spatializer") )
    set_shortname( N_("Spatializer" ) )
    set_capability( "audio filter", 0 )
    set_subcategory( SUBCAT_AUDIO_AFILTER )

    set_callback( Open )
    add_shortcut( "spatializer" )
    add_float_with_range( "spatializer-roomsize", 0.85, 0., 1.1,
                            ROOMSIZE_TEXT, ROOMSIZE_LONGTEXT )
    add_float_with_range( "spatializer-width", 1,     0.,  1.,
                            WIDTH_TEXT,WIDTH_LONGTEXT )
    add_float_with_range( "spatializer-wet",   0.4,   0.,  1.,
                            WET_TEXT,WET_LONGTEXT )
    add_float_with_range( "spatializer-dry",   0.5,   0.,  1.,
                            DRY_TEXT,DRY_LONGTEXT )
    add_float_with_range( "spatializer-damp",  0.5,   0.,  1.,
                            DAMP_TEXT,DAMP_LONGTEXT )
heimdall_module_end ()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

namespace {

struct filter_sys_t
{
    heimdall_mutex_t lock;
    revmodel *p_reverbm;
};

} // namespace

#define DECLARECB(fn) static int fn (heimdall_object_t *,char const *, \
                                     heimdall_value_t, heimdall_value_t, void *)
DECLARECB( RoomCallback  );
DECLARECB( WetCallback   );
DECLARECB( DryCallback   );
DECLARECB( DampCallback  );
DECLARECB( WidthCallback );

#undef  DECLARECB

namespace {

struct callback_s {
  const char *psz_name;
  int (*fp_callback)(heimdall_object_t *,const char *,
                     heimdall_value_t,heimdall_value_t,void *);
  void (revmodel::* fp_set)(float);
};

} // namespace

static const std::array<callback_s, 5> callbacks = {{
    { "spatializer-roomsize", RoomCallback,  &revmodel::setroomsize },
    { "spatializer-width",    WidthCallback, &revmodel::setwidth },
    { "spatializer-wet",      WetCallback,   &revmodel::setwet },
    { "spatializer-dry",      DryCallback,   &revmodel::setdry },
    { "spatializer-damp",     DampCallback,  &revmodel::setdamp }
}};

static block_t *DoWork( filter_t *, block_t * );

/*****************************************************************************
 * Close: close the filter
 *****************************************************************************/
static void Close( filter_t *p_filter )
{
    filter_sys_t *p_sys = reinterpret_cast<filter_sys_t *>( p_filter->p_sys );
    heimdall_object_t *p_aout = heimdall_object_parent(p_filter);

    /* Delete the callbacks */
    for (const auto& cb : callbacks)
    {
        var_DelCallback(p_aout, cb.psz_name, cb.fp_callback, p_sys);
    }

    delete p_sys->p_reverbm;
    free( p_sys );
    msg_Dbg( &p_filter->obj, "Closing filter spatializer" );
}

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    filter_t     *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys;
    heimdall_object_t *p_aout = heimdall_object_parent(p_filter);

     /* Allocate structure */
    p_filter->p_sys = p_sys = (filter_sys_t*)malloc( sizeof( *p_sys ) );
    if( !p_sys )
        return HEIMDALL_ENOMEM;

    /* Force new to return 0 on failure instead of throwing, since we don't
       want an exception to leak back to C code. Bad things would happen. */
    p_sys->p_reverbm = new (nothrow) revmodel;
    if( !p_sys->p_reverbm )
    {
        free( p_sys );
        return HEIMDALL_ENOMEM;
    }

    heimdall_mutex_init( &p_sys->lock );

    for (const auto& cb : callbacks)
    {
        /* NOTE: C++ pointer-to-member function call from table lookup. */
        float value = var_CreateGetFloatCommand(p_aout, cb.psz_name);
        (p_sys->p_reverbm->*(cb.fp_set))(value);
        var_AddCallback(p_aout, cb.psz_name, cb.fp_callback, p_sys);
    }

    p_filter->fmt_in.audio.i_format = HEIMDALL_CODEC_FL32;
    aout_FormatPrepare(&p_filter->fmt_in.audio);
    p_filter->fmt_out.audio = p_filter->fmt_in.audio;

    static const struct heimdall_filter_operations filter_ops = []{
        struct heimdall_filter_operations ops {};
        ops.filter_audio = DoWork;
        ops.close = Close;
        return ops;
    }();

    p_filter->ops = &filter_ops;
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * SpatFilter: process samples buffer
 * DoWork: call SpatFilter
 *****************************************************************************/

static void SpatFilter( filter_t *p_filter, float *out, float *in,
                        unsigned i_samples, unsigned i_channels )
{
    filter_sys_t *p_sys = reinterpret_cast<filter_sys_t *>( p_filter->p_sys );
    heimdall_mutex_locker locker( &p_sys->lock );

    for( unsigned i = 0; i < i_samples; i++ )
    {
        for( unsigned ch = 0 ; ch < 2; ch++)
        {
            in[ch] = in[ch] * SPAT_AMP;
        }
        p_sys->p_reverbm->processreplace( in, out , 1, i_channels);
        in  += i_channels;
        out += i_channels;
    }
}

static block_t *DoWork( filter_t * p_filter, block_t * p_in_buf )
{
    SpatFilter( p_filter, (float*)p_in_buf->p_buffer,
               (float*)p_in_buf->p_buffer, p_in_buf->i_nb_samples,
               aout_FormatNbChannels( &p_filter->fmt_in.audio ) );
    return p_in_buf;
}


/*****************************************************************************
 * Variables callbacks
 *****************************************************************************/

static int RoomCallback( heimdall_object_t *p_this, char const *,
                         heimdall_value_t, heimdall_value_t newval, void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    heimdall_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setroomsize(newval.f_float);
    msg_Dbg( p_this, "room size is now %3.1f", newval.f_float );
    return HEIMDALL_SUCCESS;
}

static int WidthCallback( heimdall_object_t *p_this, char const *,
                          heimdall_value_t, heimdall_value_t newval, void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    heimdall_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setwidth(newval.f_float);
    msg_Dbg( p_this, "width is now %3.1f", newval.f_float );
    return HEIMDALL_SUCCESS;
}

static int WetCallback( heimdall_object_t *p_this, char const *,
                        heimdall_value_t, heimdall_value_t newval, void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    heimdall_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setwet(newval.f_float);
    msg_Dbg( p_this, "'wet' value is now %3.1f", newval.f_float );
    return HEIMDALL_SUCCESS;
}

static int DryCallback( heimdall_object_t *p_this, char const *,
                        heimdall_value_t, heimdall_value_t newval, void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    heimdall_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setdry(newval.f_float);
    msg_Dbg( p_this, "'dry' value is now %3.1f", newval.f_float );
    return HEIMDALL_SUCCESS;
}

static int DampCallback( heimdall_object_t *p_this, char const *,
                         heimdall_value_t, heimdall_value_t newval, void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    heimdall_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setdamp(newval.f_float);
    msg_Dbg( p_this, "'damp' value is now %3.1f", newval.f_float );
    return HEIMDALL_SUCCESS;
}
