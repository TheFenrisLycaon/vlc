/*****************************************************************************
 * t140.c : trivial T.140 text encoder
 *****************************************************************************
 * Copyright © 2007 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
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

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_sout.h>

static int  Open ( heimdall_object_t * );

heimdall_module_begin ()
    set_description( N_("T.140 text encoder") )
    set_capability( "spu encoder", 100 )
    set_callback( Open )
heimdall_module_end ()


static block_t *Encode ( encoder_t *, subpicture_t * );


static int Open( heimdall_object_t *p_this )
{
    encoder_t *p_enc = (encoder_t *)p_this;

    switch( p_enc->fmt_out.i_codec )
    {
        case HEIMDALL_CODEC_SUBT:
            if( ( p_enc->fmt_out.subs.psz_encoding != NULL )
             && strcasecmp( p_enc->fmt_out.subs.psz_encoding, "utf8" )
             && strcasecmp( p_enc->fmt_out.subs.psz_encoding, "UTF-8" ) )
            {
                msg_Err( p_this, "Only UTF-8 encoding supported" );
                return HEIMDALL_EGENERIC;
            }
        case HEIMDALL_CODEC_ITU_T140:
            break;

        default:
            if( !p_enc->obj.force )
                return HEIMDALL_EGENERIC;

            p_enc->fmt_out.i_codec = HEIMDALL_CODEC_ITU_T140;
    }

    static const struct heimdall_encoder_operations ops =
        { .encode_sub = Encode };
    p_enc->ops = &ops;

    return HEIMDALL_SUCCESS;
}


static block_t *Encode( encoder_t *p_enc, subpicture_t *p_spu )
{
    HEIMDALL_UNUSED( p_enc );

    subpicture_region_t *p_region;
    block_t *p_block;
    size_t len = 0;

    if( p_spu == NULL )
        return NULL;

    p_region = heimdall_spu_regions_first_or_null(&p_spu->regions);
    if( ( p_region == NULL )
     || (!subpicture_region_IsText( p_region ))
     || ( p_region->p_text == NULL )
     || ( p_region->p_text->psz_text == NULL) )
        return NULL;

    /* This should already be UTF-8 encoded, so not much effort... */
    for( const text_segment_t *p_segment = p_region->p_text;
                               p_segment; p_segment = p_segment->p_next )
    {
        if( p_segment->psz_text == NULL )
            continue;
        len += strlen( p_segment->psz_text );
    }

    p_block = block_Alloc( len + 1 );
    if( !p_block )
        return NULL;

    p_block->i_buffer = 0;
    for( const text_segment_t *p_segment = p_region->p_text;
                               p_segment; p_segment = p_segment->p_next )
    {
        if( p_segment->psz_text == NULL )
            continue;
        len = strlen( p_segment->psz_text );
        memcpy( &p_block->p_buffer[p_block->i_buffer],
                p_segment->psz_text, len );
        p_block->i_buffer += len;
    }
    p_block->p_buffer[p_block->i_buffer] = 0;

    p_block->i_pts = p_block->i_dts = p_spu->i_start;
    if( !p_spu->b_ephemer &&
        ( p_spu->i_stop != HEIMDALL_TICK_INVALID && p_spu->i_stop > p_spu->i_start ) )
        p_block->i_length = p_spu->i_stop - p_spu->i_start;

    return p_block;
}
