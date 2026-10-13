/*****************************************************************************
 * rand.c: random number/bytes generation functions
 *****************************************************************************
 * Copyright (C) 2007-2018 the VideoLAN team
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_rand.h>

#include "../heimdall.h"
#include "../libs.h"

static int heimdalllua_rand_number( lua_State *L )
{
    long rand = heimdall_lrand48();
    lua_pushnumber( L, rand );
    return 1;
}

static int heimdalllua_rand_bytes( lua_State *L )
{
    lua_Integer i_size = luaL_checkinteger( L, 1 );
    char* p_buff = malloc( i_size * sizeof( *p_buff ) );
    if ( unlikely( p_buff == NULL ) )
        return heimdalllua_error( L );
    heimdall_rand_bytes( p_buff, i_size );
    lua_pushlstring( L, p_buff, i_size );
    free( p_buff );
    return 1;
}

static const luaL_Reg heimdalllua_rand_reg[] = {
    { "number", heimdalllua_rand_number },
    { "bytes", heimdalllua_rand_bytes },

    { NULL, NULL }
};

void luaopen_rand( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_rand_reg );
    lua_setfield( L, -2, "rand" );
}
