/*****************************************************************************
 * net.c: Network related functions
 *****************************************************************************
 * Copyright (C) 2007-2008 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Antoine Cellerier <dionoea at lycaria tod org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>       /* poll structures and defines */
#endif
#include <sys/stat.h>

#include <heimdall_common.h>
#include <heimdall_network.h>
#include <heimdall_url.h>
#include <heimdall_fs.h>
#include <heimdall_interrupt.h>

#include "../heimdall.h"
#include "../libs.h"
#include "misc.h"

static heimdalllua_dtable_t *heimdalllua_get_dtable( lua_State *L )
{
    return heimdalllua_get_object( L, heimdalllua_get_dtable );
}

heimdall_interrupt_t *heimdalllua_set_interrupt( lua_State *L )
{
    heimdalllua_dtable_t *dt = heimdalllua_get_dtable( L );
    return heimdall_interrupt_set( dt->interrupt );
}

/** Maps an OS file descriptor to a Heimdall Lua file descriptor */
static int heimdalllua_fd_map( lua_State *L, int fd )
{
    heimdalllua_dtable_t *dt = heimdalllua_get_dtable( L );

    if( (unsigned)fd < 3u )
        return -1;

#ifndef NDEBUG
    for( unsigned i = 0; i < dt->fdc; i++ )
        assert( dt->fdv[i] != fd );
#endif

    for( unsigned i = 0; i < dt->fdc; i++ )
    {
        if( dt->fdv[i] == -1 )
        {
            dt->fdv[i] = fd;
            return 3 + i;
        }
    }

    if( dt->fdc >= 64 )
        return -1;

    int *fdv = realloc( dt->fdv, (dt->fdc + 1) * sizeof (dt->fdv[0]) );
    if( unlikely(fdv == NULL) )
        return -1;

    dt->fdv = fdv;
    dt->fdv[dt->fdc] = fd;
    fd = 3 + dt->fdc;
    dt->fdc++;
    return fd;
}

static int heimdalllua_fd_map_safe( lua_State *L, int fd )
{
    int luafd = heimdalllua_fd_map( L, fd );
    if( luafd == -1 )
        net_Close( fd );
    return luafd;
}

/** Gets the OS file descriptor mapped to a Heimdall Lua file descriptor */
static int heimdalllua_fd_get( lua_State *L, unsigned idx )
{
    heimdalllua_dtable_t *dt = heimdalllua_get_dtable( L );

    if( idx < 3u )
        return idx;
    idx -= 3;
    return (idx < dt->fdc) ? dt->fdv[idx] : -1;
}

/** Gets the Heimdall Lua file descriptor mapped from an OS file descriptor */
static int heimdalllua_fd_get_lua( lua_State *L, int fd )
{
    heimdalllua_dtable_t *dt = heimdalllua_get_dtable( L );

    if( (unsigned)fd < 3u )
        return fd;
    for( unsigned i = 0; i < dt->fdc; i++ )
        if( dt->fdv[i] == fd )
            return 3 + i;
    return -1;
}

/** Unmaps an OS file descriptor from Heimdall Lua */
static void heimdalllua_fd_unmap( lua_State *L, unsigned idx )
{
    heimdalllua_dtable_t *dt = heimdalllua_get_dtable( L );
    int fd;

    if( idx < 3u )
        return; /* Never close stdin/stdout/stderr. */

    idx -= 3;
    if( idx >= dt->fdc )
        return;

    fd = dt->fdv[idx];
    dt->fdv[idx] = -1;
    while( dt->fdc > 0 && dt->fdv[dt->fdc - 1] == -1 )
        dt->fdc--;
    /* realloc() not really needed */
#ifndef NDEBUG
    for( unsigned i = 0; i < dt->fdc; i++ )
        assert( dt->fdv[i] != fd );
#else
    (void) fd;
#endif
}

static void heimdalllua_fd_unmap_safe( lua_State *L, unsigned idx )
{
    int fd = heimdalllua_fd_get( L, idx );

    heimdalllua_fd_unmap( L, idx );
    if( fd != -1 )
        net_Close( fd );
}

/*****************************************************************************
 * Net listen
 *****************************************************************************/
static int heimdalllua_net_listen_close( lua_State * );
static int heimdalllua_net_accept( lua_State * );
static int heimdalllua_net_fds( lua_State * );

static const luaL_Reg heimdalllua_net_listen_reg[] = {
    { "accept", heimdalllua_net_accept },
    { "fds", heimdalllua_net_fds },
    { NULL, NULL }
};

static int heimdalllua_net_listen_tcp( lua_State *L )
{
    heimdall_object_t *p_this = heimdalllua_get_this( L );
    const char *psz_host = luaL_checkstring( L, 1 );
    int i_port = luaL_checkinteger( L, 2 );
    int *pi_fd = net_ListenTCP( p_this, psz_host, i_port );
    if( pi_fd == NULL )
        return luaL_error( L, "Cannot listen on %s:%d", psz_host, i_port );

    for( unsigned i = 0; pi_fd[i] != -1; i++ )
        if( heimdalllua_fd_map( L, pi_fd[i] ) == -1 )
        {
            while( i > 0 )
                heimdalllua_fd_unmap( L, heimdalllua_fd_get_lua( L, pi_fd[--i] ) );

            net_ListenClose( pi_fd );
            return luaL_error( L, "Cannot listen on %s:%d", psz_host, i_port );
        }

    int **ppi_fd = lua_newuserdata( L, sizeof( int * ) );
    *ppi_fd = pi_fd;

    if( luaL_newmetatable( L, "net_listen" ) )
    {
        lua_newtable( L );
        luaL_register( L, NULL, heimdalllua_net_listen_reg );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, heimdalllua_net_listen_close );
        lua_setfield( L, -2, "__gc" );
    }

    lua_setmetatable( L, -2 );
    return 1;
}

static int heimdalllua_net_listen_close( lua_State *L )
{
    int **ppi_fd = (int**)luaL_checkudata( L, 1, "net_listen" );
    int *pi_fd = *ppi_fd;

    for( unsigned i = 0; pi_fd[i] != -1; i++ )
        heimdalllua_fd_unmap( L, heimdalllua_fd_get_lua( L, pi_fd[i] ) );

    net_ListenClose( pi_fd );
    return 0;
}

static int heimdalllua_net_fds( lua_State *L )
{
    int **ppi_fd = (int**)luaL_checkudata( L, 1, "net_listen" );
    int *pi_fd = *ppi_fd;

    int i_count = 0;
    while( pi_fd[i_count] != -1 )
        lua_pushinteger( L, heimdalllua_fd_get_lua( L, pi_fd[i_count++] ) );

    return i_count;
}

static int heimdalllua_net_accept( lua_State *L )
{
    heimdall_object_t *p_this = heimdalllua_get_this( L );
    int **ppi_fd = (int**)luaL_checkudata( L, 1, "net_listen" );
    int i_fd = net_Accept( p_this, *ppi_fd );

    lua_pushinteger( L, heimdalllua_fd_map_safe( L, i_fd ) );
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static int heimdalllua_net_connect_tcp( lua_State *L )
{
    heimdall_object_t *p_this = heimdalllua_get_this( L );
    const char *psz_host = luaL_checkstring( L, 1 );
    int i_port = luaL_checkinteger( L, 2 );
    int i_fd = net_ConnectTCP( p_this, psz_host, i_port );
    lua_pushinteger( L, heimdalllua_fd_map_safe( L, i_fd ) );
    return 1;
}

static int heimdalllua_net_close( lua_State *L )
{
    int i_fd = luaL_checkinteger( L, 1 );
    heimdalllua_fd_unmap_safe( L, i_fd );
    return 0;
}

static int heimdalllua_net_send( lua_State *L )
{
    int fd = heimdalllua_fd_get( L, luaL_checkinteger( L, 1 ) );
    size_t i_len;
    const char *psz_buffer = luaL_checklstring( L, 2, &i_len );

    i_len = (size_t)luaL_optinteger( L, 3, i_len );
    lua_pushinteger( L,
        (fd != -1) ? heimdall_send( fd, psz_buffer, i_len, 0 ) : -1 );
    return 1;
}

static int heimdalllua_net_recv( lua_State *L )
{
    int fd = heimdalllua_fd_get( L, luaL_checkinteger( L, 1 ) );
    size_t i_len = (size_t)luaL_optinteger( L, 2, 1 );
    char psz_buffer[i_len];

    ssize_t i_ret = (fd != -1) ? recv( fd, psz_buffer, i_len, 0 ) : -1;
    if( i_ret > 0 )
        lua_pushlstring( L, psz_buffer, i_ret );
    else
        lua_pushnil( L );
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
/* Takes a { fd : events } table as first arg and modifies it to { fd : revents } */
static int heimdalllua_net_poll( lua_State *L )
{
    luaL_checktype( L, 1, LUA_TTABLE );

    int i_fds = 0;
    lua_pushnil( L );
    while( lua_next( L, 1 ) )
    {
        i_fds++;
        lua_pop( L, 1 );
    }

    struct pollfd *p_fds = xmalloc( i_fds * sizeof( *p_fds ) );
    int *luafds = xmalloc( i_fds * sizeof( *luafds ) );

    lua_pushnil( L );
    for( int i = 0; lua_next( L, 1 ); i++ )
    {
        luafds[i] = luaL_checkinteger( L, -2 );
        p_fds[i].fd = heimdalllua_fd_get( L, luafds[i] );
        p_fds[i].events = luaL_checkinteger( L, -1 );
        p_fds[i].events &= POLLIN | POLLOUT | POLLPRI;
        lua_pop( L, 1 );
    }

    heimdall_interrupt_t *oint = heimdalllua_set_interrupt( L );
    int ret = 1, val = -1;

    do
    {
        if( heimdall_killed() )
            break;
        val = heimdall_poll_i11e( p_fds, i_fds, -1 );
    }
    while( val == -1 && errno == EINTR );

    heimdall_interrupt_set( oint );

    for( int i = 0; i < i_fds; i++ )
    {
        lua_pushinteger( L, luafds[i] );
        lua_pushinteger( L, (val >= 0) ? p_fds[i].revents : 0 );
        lua_settable( L, 1 );
    }
    lua_pushinteger( L, val );

    free( luafds );
    free( p_fds );

    if( val == -1 )
        return luaL_error( L, "Interrupted." );
    return ret;
}

/*****************************************************************************
 *
 *****************************************************************************/
/*
static int heimdalllua_fd_open( lua_State *L )
{
}
*/

#ifndef _WIN32
static int heimdalllua_fd_write( lua_State *L )
{
    int fd = heimdalllua_fd_get( L, luaL_checkinteger( L, 1 ) );
    size_t i_len;
    const char *psz_buffer = luaL_checklstring( L, 2, &i_len );

    i_len = (size_t)luaL_optinteger( L, 3, i_len );
    lua_pushinteger( L, (fd != -1) ? heimdall_write( fd, psz_buffer, i_len ) : -1 );
    return 1;
}

static int heimdalllua_fd_read( lua_State *L )
{
    int fd = heimdalllua_fd_get( L, luaL_checkinteger( L, 1 ) );
    size_t i_len = (size_t)luaL_optinteger( L, 2, 1 );
    char psz_buffer[i_len];

    ssize_t i_ret = (fd != -1) ? read( fd, psz_buffer, i_len ) : -1;
    if( i_ret > 0 )
        lua_pushlstring( L, psz_buffer, i_ret );
    else
        lua_pushnil( L );
    return 1;
}
#endif

/*****************************************************************************
 *
 *****************************************************************************/
static int heimdalllua_stat( lua_State *L )
{
    const char *psz_path = luaL_checkstring( L, 1 );
    struct stat s;
    if( heimdall_stat( psz_path, &s ) )
        return 0;
        //return luaL_error( L, "Couldn't stat %s.", psz_path );
    lua_newtable( L );
    if( S_ISREG( s.st_mode ) )
        lua_pushliteral( L, "file" );
    else if( S_ISDIR( s.st_mode ) )
        lua_pushliteral( L, "dir" );
#ifdef S_ISCHR
    else if( S_ISCHR( s.st_mode ) )
        lua_pushliteral( L, "character device" );
#endif
#ifdef S_ISBLK
    else if( S_ISBLK( s.st_mode ) )
        lua_pushliteral( L, "block device" );
#endif
#ifdef S_ISFIFO
    else if( S_ISFIFO( s.st_mode ) )
        lua_pushliteral( L, "fifo" );
#endif
#ifdef S_ISLNK
    else if( S_ISLNK( s.st_mode ) )
        lua_pushliteral( L, "symbolic link" );
#endif
#ifdef S_ISSOCK
    else if( S_ISSOCK( s.st_mode ) )
        lua_pushliteral( L, "socket" );
#endif
    else
        lua_pushliteral( L, "unknown" );
    lua_setfield( L, -2, "type" );
    lua_pushinteger( L, s.st_mode );
    lua_setfield( L, -2, "mode" );
    lua_pushinteger( L, s.st_uid );
    lua_setfield( L, -2, "uid" );
    lua_pushinteger( L, s.st_gid );
    lua_setfield( L, -2, "gid" );
    lua_pushinteger( L, s.st_size );
    lua_setfield( L, -2, "size" );
    lua_pushinteger( L, s.st_atime );
    lua_setfield( L, -2, "access_time" );
    lua_pushinteger( L, s.st_mtime );
    lua_setfield( L, -2, "modification_time" );
    lua_pushinteger( L, s.st_ctime );
    lua_setfield( L, -2, "creation_time" );
    return 1;
}

static int heimdalllua_opendir( lua_State *L )
{
    const char *psz_dir = luaL_checkstring( L, 1 );
    heimdall_DIR *p_dir;
    int i = 0;

    if( ( p_dir = heimdall_opendir( psz_dir ) ) == NULL )
        return luaL_error( L, "cannot open directory `%s'.", psz_dir );

    lua_newtable( L );
    for( ;; )
    {
        const char *psz_filename = heimdall_readdir( p_dir );
        if( !psz_filename ) break;
        i++;
        lua_pushstring( L, psz_filename );
        lua_rawseti( L, -2, i );
    }
    heimdall_closedir( p_dir );
    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static const luaL_Reg heimdalllua_net_intf_reg[] = {
    { "listen_tcp", heimdalllua_net_listen_tcp },
    { "connect_tcp", heimdalllua_net_connect_tcp },
    { "close", heimdalllua_net_close },
    { "send", heimdalllua_net_send },
    { "recv", heimdalllua_net_recv },
    { "poll", heimdalllua_net_poll },
#ifndef _WIN32
    { "read", heimdalllua_fd_read },
    { "write", heimdalllua_fd_write },
#endif
    /* The following functions do not depend on intf_thread_t and do not really
     * belong in net.* but are left here for backward compatibility: */
    { "url_parse", heimdalllua_url_parse /* deprecated since 3.0.0 */ },
    { "stat", heimdalllua_stat }, /* Not really "net" */
    { "opendir", heimdalllua_opendir }, /* Not really "net" */
    { NULL, NULL }
};

static void luaopen_net_intf( lua_State *L )
{
    lua_newtable( L );
    luaL_register( L, NULL, heimdalllua_net_intf_reg );
#define ADD_CONSTANT( value )    \
    lua_pushinteger( L, POLL##value ); \
    lua_setfield( L, -2, "POLL"#value );
    ADD_CONSTANT( IN )
    ADD_CONSTANT( PRI )
    ADD_CONSTANT( OUT )
    ADD_CONSTANT( ERR )
    ADD_CONSTANT( HUP )
    ADD_CONSTANT( NVAL )
    lua_setfield( L, -2, "net" );
}

int heimdalllua_fd_init( lua_State *L, heimdalllua_dtable_t *dt )
{
    dt->interrupt = heimdall_interrupt_create();
    if( unlikely(dt->interrupt == NULL) )
        return -1;
    dt->fdv = NULL;
    dt->fdc = 0;
    heimdalllua_set_object( L, heimdalllua_get_dtable, dt );
    luaopen_net_intf( L );
    return 0;
}

void heimdalllua_fd_interrupt( heimdalllua_dtable_t *dt )
{
    heimdall_interrupt_kill( dt->interrupt );
}

/** Releases all (leaked) Heimdall Lua file descriptors. */
void heimdalllua_fd_cleanup( heimdalllua_dtable_t *dt )
{
    for( unsigned i = 0; i < dt->fdc; i++ )
        if( dt->fdv[i] != -1 )
            net_Close( dt->fdv[i] );
    free( dt->fdv );
    heimdall_interrupt_destroy(dt->interrupt);
}
