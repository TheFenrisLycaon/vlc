/*****************************************************************************
 * mrl_helpers.h
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

#ifndef INPUT_MRL_HELPERS_H
#define INPUT_MRL_HELPERS_H

#include <string.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_memstream.h>
#include <heimdall_arrays.h>
#include <heimdall_url.h>

/**
 * \defgroup mrl_helpers MRL helpers
 * \ingroup mrl
 *
 * Helper functions related to parsing, as well as generating, data
 * related to the \link mrl MRL-specification\endlink.
 *
 * @{
 * \file
 **/

/**
 * Escape a fragment identifier for use within an MRL
 *
 * The function will generate a string that follows the \link mrl
 * MRL-specification\endlink regarding \em fragment-identifiers.
 *
 * See the \link mrl MRL-specification\endlink for a detailed
 * explanation of how `payload` will be escaped.
 *
 * \param[in] payload the data to escape.
 * \return he created string on success, and NULL on error.
 **/
HEIMDALL_MALLOC static inline char *
mrl_EscapeFragmentIdentifier( char const* payload )
{
    struct heimdall_memstream mstream;

#define RFC3986_SUBDELIMS  "!" "$" "&" "'" "(" ")" \
                           "*" "+" "," ";" "="
#define RFC3986_ALPHA      "abcdefghijklmnopqrstuvwxyz" \
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define RFC3986_DIGIT      "0123456789"
#define RFC3986_UNRESERVED RFC3986_ALPHA RFC3986_DIGIT "-" "." "_" "~"
#define RFC3986_PCHAR      RFC3986_UNRESERVED RFC3986_SUBDELIMS ":" "@"
#define RFC3986_FRAGMENT   RFC3986_PCHAR "/" "?"

    if( heimdall_memstream_open( &mstream ) )
        return NULL;

    for( char const* p = payload; *p; ++p )
    {
        heimdall_memstream_printf( &mstream,
            ( strchr( "!?", *p ) == NULL &&
              strchr( RFC3986_FRAGMENT, *p ) ? "%c" : "%%%02hhx"), *p );
    }

#undef RFC3986_FRAGMENT
#undef RFC3986_PCHAR
#undef RFC3986_UNRESERVEd
#undef RFC3986_DIGIT
#undef RFC3986_ALPHA
#undef RFC3986_SUBDELIMS

    if( heimdall_memstream_close( &mstream ) )
        return NULL;

    return mstream.ptr;
}

static inline char *
mrl_AppendAnchorFragment( const char *anchor, char const *payload )
{
    struct heimdall_memstream mstream;
    if( heimdall_memstream_open( &mstream ) )
        return NULL;

    if( anchor )
        heimdall_memstream_puts( &mstream, anchor );
    else
        heimdall_memstream_putc( &mstream, '#' );

    char *escaped = mrl_EscapeFragmentIdentifier( payload );
    if( escaped == NULL )
    {
        if( !heimdall_memstream_close( &mstream ) )
            free( mstream.ptr );
        return NULL;
    }

    heimdall_memstream_puts( &mstream, "!+" );
    heimdall_memstream_puts( &mstream, escaped );
    free( escaped );

    if( heimdall_memstream_close( &mstream ) )
        return NULL;

    return mstream.ptr;
}

struct mrl_info
{
    heimdall_array_t identifiers;
    heimdall_array_t volumes;
    char const *extra;
};

static inline void
mrl_info_Clean( struct mrl_info *mrli )
{
    for( size_t i = 0; i < heimdall_array_count( &mrli->identifiers ); ++i )
        free( heimdall_array_item_at_index( &mrli->identifiers, i ) );
    heimdall_array_clear( &mrli->identifiers );
    for( size_t i = 0; i < heimdall_array_count( &mrli->volumes ); ++i )
        free( heimdall_array_item_at_index( &mrli->volumes, i ) );
    heimdall_array_clear( &mrli->volumes );
}

static inline void
mrl_info_Init( struct mrl_info *mrli )
{
    heimdall_array_init( &mrli->identifiers );
    heimdall_array_init( &mrli->volumes );
    mrli->extra = NULL;
}

/**
 * Split an \link mrl_technical_fragment MRL-fragment\endlink into identifiers
 *
 * Function used to split the fragment-data (also referred to as
 * anchor-data) into an array containing each of the files specified.
 *
 * See the \link mrl MRL-specification\endlink for detailed
 * information regarding how `payload` will be interpreted.
 *
 * \warning On success, the caller has ownership of the contents of *out_items
 *          which means that it is responsible for freeing the individual
 *          elements, as well as cleaning the array itself.
 *
 * \param[out] out_items storage for a heimdall_array_t that will contain the
 *                       parsed identifiers on success.
 * \param[out] out_extra `*out_extra` will point to any remaining data (if any)
 * \param[in] payload the data to parse
 * \return HEIMDALL_SUCCESS on success, an error-code on failure
 **/
static inline int
mrl_FragmentSplit( struct mrl_info *mrli,
                   char const* payload )
{
    while( strncmp( payload, "!/", 2 ) == 0 )
    {
        payload += 2;

        int len = strcspn( payload, "!?" );
        char* decoded = strndup( payload, len );

        if( unlikely( !decoded ) || !heimdall_uri_decode( decoded ) )
            goto error;

        if( heimdall_array_append( &mrli->identifiers, decoded ) )
        {
            free( decoded );
            goto error;
        }
        payload += len;
    }

    while( strncmp( payload, "!+", 2 ) == 0 )
    {
        payload += 2;

        int len = strcspn( payload, "!?" );
        char* decoded = strndup( payload, len );

        if( unlikely( !decoded ) || !heimdall_uri_decode( decoded ) )
            goto error;

        if( heimdall_array_append( &mrli->volumes, decoded ) )
        {
            free( decoded );
            goto error;
        }
        payload += len;
    }

    if( *payload )
    {
        if( *payload == '!' )
            goto error;

        if( *payload == '?' && heimdall_array_count( &mrli->identifiers ) )
            ++payload;

        mrli->extra = payload;
    }

    return HEIMDALL_SUCCESS;

error:
    return HEIMDALL_EGENERIC;
}

/** @} */

#endif /* include-guard */
