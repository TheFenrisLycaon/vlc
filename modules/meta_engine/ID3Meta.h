/*****************************************************************************
 * ID3Meta.h : ID3v2 Meta Helper
 *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
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
#ifndef ID3META_H
#define ID3META_H

#include <heimdall_meta.h>
#include "ID3Text.h"

#define heimdall_meta_extra heimdall_meta_Title
static struct
{
    uint32_t i_tag;
    heimdall_meta_type_t type;
    const char *psz;
} const ID3_tag_to_metatype[] = {
    { HEIMDALL_FOURCC('T', 'A', 'L', 'B'), heimdall_meta_Album,       NULL },
    { HEIMDALL_FOURCC('T', 'C', 'M', 'P'), heimdall_meta_Compilation, NULL },
    { HEIMDALL_FOURCC('T', 'D', 'R', 'C'), heimdall_meta_Date,        NULL },
    { HEIMDALL_FOURCC('T', 'E', 'N', 'C'), heimdall_meta_extra,       "Encoder" },
    { HEIMDALL_FOURCC('T', 'I', 'T', '2'), heimdall_meta_Title,       NULL },
    { HEIMDALL_FOURCC('T', 'O', 'P', 'E'), heimdall_meta_extra,       "Original Artist" },
    { HEIMDALL_FOURCC('T', 'O', 'R', 'Y'), heimdall_meta_extra,       "Original Release Year" },
    { HEIMDALL_FOURCC('T', 'P', 'E', '1'), heimdall_meta_Artist,      NULL },
    { HEIMDALL_FOURCC('T', 'P', 'E', '2'), heimdall_meta_AlbumArtist, NULL },
    { HEIMDALL_FOURCC('T', 'R', 'S', 'N'), heimdall_meta_Publisher,   NULL },
    { HEIMDALL_FOURCC('T', 'R', 'S', 'O'), heimdall_meta_extra,       "Radio Station Owner" },
};
#undef heimdall_meta_extra

static bool ID3TextTagHandler( const uint8_t *p_buf, size_t i_buf,
                               heimdall_meta_type_t type, const char *psz_extra,
                               heimdall_meta_t *p_meta, bool *pb_updated )
{
    if( p_meta == NULL )
        return false;

    char *p_alloc;
    const char *psz = ID3TextConvert( p_buf, i_buf, &p_alloc );
    if( psz && *psz )
    {
        const char *psz_old = ( psz_extra ) ? heimdall_meta_GetExtra( p_meta, psz_extra ):
                                              heimdall_meta_Get( p_meta, type );
        if( !psz_old || strcmp( psz_old, psz ) )
        {
            if( pb_updated )
                *pb_updated = true;
            if( psz_extra )
                heimdall_meta_SetExtra( p_meta, psz_extra, psz );
            else
                heimdall_meta_Set( p_meta, type, psz );
        }
    }
    free( p_alloc );

    return (psz != NULL);
}

static bool ID3LinkFrameTagHandler( const uint8_t *p_buf, size_t i_buf,
                                    heimdall_meta_t *p_meta, bool *pb_updated )
{
    if( i_buf > 13 && p_meta )
    {
        const char *psz = (const char *)&p_buf[1];
        size_t i_len = i_buf - 1;
        size_t i_desclen = strnlen(psz, i_len);
        if( i_desclen < i_len - 1 && i_desclen > 11 &&
            !strncmp( "artworkURL_", psz, 11 ) )
        {
            const char *psz_old = heimdall_meta_Get( p_meta, heimdall_meta_ArtworkURL );
            if( !psz_old || strncmp( psz_old, &psz[i_desclen], i_len - i_desclen ) )
            {
                char *p_alloc = strndup(&psz[i_desclen + 1], i_len - i_desclen - 1);
                heimdall_meta_Set( p_meta, heimdall_meta_ArtworkURL, p_alloc );
                free( p_alloc );
                *pb_updated = true;
            }
        }
        return true;
    }
    return false;
}

static bool ID3HandleTag( const uint8_t *p_buf, size_t i_buf,
                          uint32_t i_tag,
                          heimdall_meta_t *p_meta, bool *pb_updated )
{
    if( i_tag == HEIMDALL_FOURCC('W', 'X', 'X', 'X') )
    {
        return ID3LinkFrameTagHandler( p_buf, i_buf, p_meta, pb_updated );
    }
    else if( i_tag == HEIMDALL_FOURCC('T', 'X', 'X', 'X') )
    {
        char *psz_key_alloc;
        const char *psz_key = ID3TextConvert( p_buf, i_buf, &psz_key_alloc );
        if( psz_key )
        {
            /* The value follows the encoding byte and the key, whose size
             * depends on the encoding, not on its UTF-8 conversion */
            const ssize_t i_keylen = ID3TextLength( &p_buf[1], i_buf - 1, p_buf[0] );
            const size_t i_len = i_keylen < 0 ? i_buf
                               : 1 + (size_t) i_keylen + ID3TextTerminatorSize( p_buf[0] );
            if( i_len < i_buf )
            {
                /* Only set those which are known as non binary */
                if( !strncasecmp( psz_key, "REPLAYGAIN_", 11 ) )
                {
                    char *psz_val_alloc;
                    const char *psz_val = ID3TextConv( &p_buf[i_len], i_buf - i_len,
                                                       p_buf[0], &psz_val_alloc );
                    if( psz_val )
                    {
                        heimdall_meta_SetExtra( p_meta, psz_key, psz_val );
                        free( psz_val_alloc );
                    }
                }
            }
            free( psz_key_alloc );
            return (heimdall_meta_GetExtraCount( p_meta ) > 0);
        }
    }
    else if ( ((const char *) &i_tag)[0] == 'T' )
    {
        for( size_t i=0; i<ARRAY_SIZE(ID3_tag_to_metatype); i++ )
        {
            if( ID3_tag_to_metatype[i].i_tag == i_tag )
                return ID3TextTagHandler( p_buf, i_buf,
                                          ID3_tag_to_metatype[i].type,
                                          ID3_tag_to_metatype[i].psz,
                                          p_meta, pb_updated );
        }
    }

    return false;
}

#endif
