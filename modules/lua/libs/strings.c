/*****************************************************************************
 * strings.c
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
 *          Pierre d'Herbemont <pdherbemont # videolan.org>
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

/// String transformation functions
// @module heimdall.strings

/*
 * Preamble
 */
#ifndef  _GNU_SOURCE
#   define  _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_meta.h>
#include <heimdall_charset.h>

#include "../heimdall.h"
#include "../libs.h"

/*
 * String transformations
 */

/// Decode a list of URIs.
//
// This function returns as many variables as it had arguments.
// @function    decode_uri
// @tparam      string uri The URI to decode
// @treturn     string Decoded URI
static int heimdalllua_decode_uri( lua_State *L )
{
    int i_top = lua_gettop( L );
    int i;
    for( i = 1; i <= i_top; i++ )
    {
        const char *psz_cstring = luaL_checkstring( L, 1 );
        char *psz_string = heimdall_uri_decode_duplicate( psz_cstring );
        lua_remove( L, 1 ); /* remove elements to prevent being limited by
                             * the stack's size (this function will work with
                             * up to (stack size - 1) arguments */
        lua_pushstring( L, psz_string );
        free( psz_string );
    }
    return i_top;
}

/// Encode a list of URI components.
//
// This function returns as many variables as it had arguments.
// @function    encode_uri_component
// @tparam      string uri_comp An URI component to encode
// @treturn     string Encoded URI
static int heimdalllua_encode_uri_component( lua_State *L )
{
    int i_top = lua_gettop( L );
    int i;
    for( i = 1; i <= i_top; i++ )
    {
        const char *psz_cstring = luaL_checkstring( L, 1 );
        char *psz_string = heimdall_uri_encode( psz_cstring );
        lua_remove( L,1 );
        lua_pushstring( L, psz_string );
        free( psz_string );
    }
    return i_top;
}

/// Convert a file path to a URI.
// @function    make_uri
// @tparam      string path The file path to convert
// @tparam[opt] string scheme Scheme to use for the URI (default auto: `file`, `fd` or `smb`)
// @treturn     string The URI for the given path
static int heimdalllua_make_uri( lua_State *L )
{
    const char *psz_input = luaL_checkstring( L, 1 );
    const char *psz_scheme = luaL_optstring( L, 2, NULL );
    if( strstr( psz_input, "://" ) == NULL )
    {
        char *psz_uri = heimdall_path2uri( psz_input, psz_scheme );
        lua_pushstring( L, psz_uri );
        free( psz_uri );
    }
    else
        lua_pushstring( L, psz_input );
    return 1;
}

/// Convert a URI to a local path.
// @function    make_path
// @tparam      string uri The URI to convert
// @treturn     string The file path for the given URI
static int heimdalllua_make_path( lua_State *L )
{
    const char *psz_input = luaL_checkstring( L, 1 );
    char *psz_path = heimdall_uri2path( psz_input);
    lua_pushstring( L, psz_path );
    free( psz_path );
    return 1;
}

/// URL components.
//
// A table returned by the @{url_parse} function.
// @field[type=string] protocol
// @field[type=string] username
// @field[type=string] password
// @field[type=string] host
// @field[type=int] port
// @field[type=string] path
// @field[type=string] option
// @table url_components

/// Parse a URL.
// @function    url_parse
// @tparam      string url The URL to parse
// @return      @{url_components}
int heimdalllua_url_parse( lua_State *L )
{
    const char *psz_url = luaL_checkstring( L, 1 );
    heimdall_url_t url;

    heimdall_UrlParse( &url, psz_url );

    lua_newtable( L );
    lua_pushstring( L, url.psz_protocol );
    lua_setfield( L, -2, "protocol" );
    lua_pushstring( L, url.psz_username );
    lua_setfield( L, -2, "username" );
    lua_pushstring( L, url.psz_password );
    lua_setfield( L, -2, "password" );
    lua_pushstring( L, url.psz_host );
    lua_setfield( L, -2, "host" );
    lua_pushinteger( L, url.i_port );
    lua_setfield( L, -2, "port" );
    lua_pushstring( L, url.psz_path );
    lua_setfield( L, -2, "path" );
    lua_pushstring( L, url.psz_option );
    lua_setfield( L, -2, "option" );

    heimdall_UrlClean( &url );

    return 1;
}

/// Resolve XML special characters.
//
// Decode the XML special characters in a list of strings.
// This function returns as many variables as it had arguments.
// @function    resolve_xml_special_chars
// @tparam      string input Input string
// @treturn     string String with the XML special characters decoded
static int heimdalllua_resolve_xml_special_chars( lua_State *L )
{
    int i_top = lua_gettop( L );
    int i;
    for( i = 1; i <= i_top; i++ )
    {
        const char *psz_cstring = luaL_checkstring( L, 1 );
        char *psz_string = strdup( psz_cstring );
        lua_remove( L, 1 ); /* remove elements to prevent being limited by
                             * the stack's size (this function will work with
                             * up to (stack size - 1) arguments */
        heimdall_xml_decode( psz_string );
        lua_pushstring( L, psz_string );
        free( psz_string );
    }
    return i_top;
}

/// Encode XML special characters.
//
// Encode the XML special characters in a list of strings.
// This function returns as many variables as it had arguments.
// @function    convert_xml_special_chars
// @tparam      string input Input string
// @treturn     string String with the XML special characters encoded
static int heimdalllua_convert_xml_special_chars( lua_State *L )
{
    int i_top = lua_gettop( L );
    int i;
    for( i = 1; i <= i_top; i++ )
    {
        char *psz_string = heimdall_xml_encode( luaL_checkstring(L,1) );
        lua_remove( L, 1 );
        lua_pushstring( L, psz_string );
        free( psz_string );
    }
    return i_top;
}

/// Convert string from given charset to UTF-8.
//
// Convert a string from the specified input character encoding into UTF-8.
// @function    from_charset
// @tparam      string charset The charset of the input string
// @tparam      string input   Input string
// @treturn[1]  string String converted to UTF-8
// @treturn[2]  string Empty string on error
static int heimdalllua_from_charset( lua_State *L )
{
    if( lua_gettop( L ) < 2 ) return heimdalllua_error( L );

    size_t i_in_bytes;
    const char *psz_input = luaL_checklstring( L, 2, &i_in_bytes );
    if( i_in_bytes == 0 ) return heimdalllua_error( L );

    const char *psz_charset = luaL_checkstring( L, 1 );
    char *psz_output = FromCharset( psz_charset, psz_input, i_in_bytes );
    lua_pushstring( L, psz_output ? psz_output : "" );
    free( psz_output );
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_strings_reg[] = {
    { "decode_uri", heimdalllua_decode_uri },
    { "encode_uri_component", heimdalllua_encode_uri_component },
    { "make_uri", heimdalllua_make_uri },
    { "make_path", heimdalllua_make_path },
    { "url_parse", heimdalllua_url_parse },
    { "resolve_xml_special_chars", heimdalllua_resolve_xml_special_chars },
    { "convert_xml_special_chars", heimdalllua_convert_xml_special_chars },
    { "from_charset", heimdalllua_from_charset },
    { NULL, NULL }
};

void luaopen_strings( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_strings_reg );
    lua_setfield( L, -2, "strings" );
}
