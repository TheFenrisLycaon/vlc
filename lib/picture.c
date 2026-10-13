/*****************************************************************************
 * picture.c:  libheimdall API picture management
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include <stdckdint.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include "libheimdall_internal.h"

#include <heimdall_atomic.h>
#include <heimdall_picture.h>
#include <heimdall_block.h>
#include <heimdall_image.h>
#include <heimdall_input.h>
#include <heimdall_fs.h>

#include "picture_internal.h"

struct libheimdall_picture_t
{
    heimdall_atomic_rc_t rc;
    libheimdall_picture_type_t type;
    block_t* converted;
    video_format_t fmt;
    libheimdall_time_t time;
    input_attachment_t* attachment;
};

struct libheimdall_picture_list_t
{
    size_t count;
    libheimdall_picture_t* pictures[];
};

libheimdall_picture_t* libheimdall_picture_new( heimdall_object_t* p_obj, picture_t* input,
                                      libheimdall_picture_type_t type,
                                      unsigned int width, unsigned int height,
                                      bool crop )
{
    libheimdall_picture_t *pic = malloc( sizeof( *pic ) );
    if ( unlikely( pic == NULL ) )
        return NULL;
    heimdall_atomic_rc_init( &pic->rc );
    pic->type = type;
    pic->time = libheimdall_time_from_heimdall_tick( input->date );
    pic->attachment = NULL;

    static const heimdall_fourcc_t table[] = {
        [libheimdall_picture_Jpg] = HEIMDALL_CODEC_JPEG,
        [libheimdall_picture_Png] = HEIMDALL_CODEC_PNG,
        [libheimdall_picture_Argb] = HEIMDALL_CODEC_ARGB,
        [libheimdall_picture_WebP] = HEIMDALL_CODEC_WEBP,
        [libheimdall_picture_Rgba] = HEIMDALL_CODEC_RGBA,
    };
    assert(ARRAY_SIZE(table) > type && table[type] != 0);
    heimdall_fourcc_t format = table[type];

    /* libheimdall's API uses unsigned dimensions where both 0 means "same size as
       the media". picture_Export expresses that with negative dimensions, so
       translate the convention here. */
    int export_width = width, export_height = height;
    if ( width == 0 && height == 0 )
        export_width = export_height = -1;

    if ( picture_Export( p_obj, &pic->converted, &pic->fmt,
                         input, format, export_width, export_height, crop ) != HEIMDALL_SUCCESS )
    {
        free( pic );
        return NULL;
    }

    return pic;
}

static void libheimdall_picture_block_release( block_t* block )
{
    free( block );
}

static const struct heimdall_block_callbacks block_cbs =
{
    libheimdall_picture_block_release,
};

static bool IsSupportedByLibHeimdall(heimdall_fourcc_t fcc)
{
    switch (fcc)
    {
        case HEIMDALL_CODEC_PNG:
        case HEIMDALL_CODEC_JPEG:
        case HEIMDALL_CODEC_WEBP:
            return true;
        default:
            return false;
    }
}

static libheimdall_picture_t* libheimdall_picture_from_attachment( input_attachment_t* attachment )
{
    heimdall_fourcc_t fcc = image_Mime2Fourcc( attachment->psz_mime );
    if (!IsSupportedByLibHeimdall(fcc))
        return NULL;

    libheimdall_picture_t *pic = malloc( sizeof( *pic ) );
    if ( unlikely( pic == NULL ) )
        return NULL;
    pic->converted = block_New(&block_cbs, attachment->p_data,
                               attachment->i_data);
    if ( unlikely( pic->converted == NULL ) )
    {
        free(pic);
        return NULL;
    }
    heimdall_atomic_rc_init( &pic->rc );
    pic->attachment = heimdall_input_attachment_Hold( attachment );
    pic->time = HEIMDALL_TICK_INVALID;
    video_format_Init( &pic->fmt, 0 );
    switch ( fcc )
    {
    case HEIMDALL_CODEC_PNG:
        pic->type = libheimdall_picture_Png;
        break;
    case HEIMDALL_CODEC_JPEG:
        pic->type = libheimdall_picture_Jpg;
        break;
    case HEIMDALL_CODEC_WEBP:
        pic->type = libheimdall_picture_WebP;
        break;
    default:
        heimdall_assert_unreachable();
    }

    return pic;
}

libheimdall_picture_t *libheimdall_picture_retain( libheimdall_picture_t* pic )
{
    heimdall_atomic_rc_inc( &pic->rc );
    return pic;
}

void libheimdall_picture_release( libheimdall_picture_t* pic )
{
    if ( heimdall_atomic_rc_dec( &pic->rc ) == false )
        return;
    video_format_Clean( &pic->fmt );
    if ( pic->converted )
        block_Release( pic->converted );
    if ( pic->attachment )
        heimdall_input_attachment_Release( pic->attachment );
    free( pic );
}

int libheimdall_picture_save( const libheimdall_picture_t* pic, const char* path )
{
    FILE* file = heimdall_fopen( path, "wb" );
    if ( !file )
        return -1;
    size_t res = fwrite( pic->converted->p_buffer,
                         pic->converted->i_buffer, 1, file );
    fclose( file );
    return res == 1 ? 0 : -1;
}

const unsigned char* libheimdall_picture_get_buffer( const libheimdall_picture_t* pic,
                                                size_t *size )
{
    assert( size != NULL );
    *size = pic->converted->i_buffer;
    return pic->converted->p_buffer;
}

libheimdall_picture_type_t libheimdall_picture_type( const libheimdall_picture_t* pic )
{
    return pic->type;
}

unsigned int libheimdall_picture_get_stride( const libheimdall_picture_t *pic )
{
    assert( pic->type == libheimdall_picture_Argb || pic->type == libheimdall_picture_Rgba );
    return pic->fmt.i_width * 4;
}

unsigned int libheimdall_picture_get_width( const libheimdall_picture_t* pic )
{
    return pic->fmt.i_visible_width;
}

unsigned int libheimdall_picture_get_height( const libheimdall_picture_t* pic )
{
    return pic->fmt.i_visible_height;
}

libheimdall_time_t libheimdall_picture_get_time( const libheimdall_picture_t* pic )
{
    return pic->time;
}

libheimdall_picture_list_t* libheimdall_picture_list_from_attachments( input_attachment_t* const* attachments,
                                                             size_t nb_attachments )
{
    size_t size = 0;
    libheimdall_picture_list_t* list;
    if (ckd_mul(&size, nb_attachments, sizeof (libheimdall_picture_t *)) ||
        ckd_add(&size, sizeof (*list), size))
        return NULL;

    list = malloc( size );
    if ( !list )
        return NULL;
    list->count = 0;
    for ( size_t i = 0; i < nb_attachments; ++i )
    {
        input_attachment_t* a = attachments[i];
        libheimdall_picture_t *pic = libheimdall_picture_from_attachment( a );
        if( !pic )
            continue;
        list->pictures[list->count] = pic;
        list->count++;
    }
    return list;
}

size_t libheimdall_picture_list_count( const libheimdall_picture_list_t* list )
{
    assert( list );
    return list->count;
}

libheimdall_picture_t* libheimdall_picture_list_at( const libheimdall_picture_list_t* list,
                                          size_t index )
{
    assert( list );
    return list->pictures[index];
}

void libheimdall_picture_list_destroy( libheimdall_picture_list_t* list )
{
    if ( !list )
        return;
    for ( size_t i = 0; i < list->count; ++i )
        libheimdall_picture_release( list->pictures[i] );
    free( list );
}
