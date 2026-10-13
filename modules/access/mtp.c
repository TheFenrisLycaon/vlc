/*****************************************************************************
 * mtp.c: mtp input (mtp: access plug-in)
 *****************************************************************************
 * Copyright (C) 2001-2006 VLC authors and VideoLAN
 * Copyright © 2006-2008 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Fabio Ritrovato <exsephiroth87@gmail.com>
 * Original file.c: Christophe Massiot <massiot@via.ecp.fr>
 *                  Rémi Denis-Courmont
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

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_access.h>
#include <heimdall_dialog.h>
#include <heimdall_fs.h>

#include "libmtp.h"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

static int  Open ( heimdall_object_t * );
static void Close( heimdall_object_t * );

heimdall_module_begin()
    set_description( N_("MTP input") )
    set_shortname( N_("MTP") )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    set_capability( "access", 0 )
    add_shortcut( "mtp" )
    set_callbacks( Open, Close )
heimdall_module_end()

/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/

static int  Seek( stream_t *, uint64_t );
static ssize_t Read( stream_t *, void *, size_t );
static int  Control( stream_t *, int, va_list );

/*****************************************************************************
 * Open: open the file
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    stream_t     *p_access = ( stream_t* )p_this;
    uint32_t i_bus;
    uint8_t i_dev;
    uint16_t i_product_id;
    int i_track_id;
    LIBMTP_raw_device_t *p_rawdevices;
    int i_numrawdevices;

    int *fdp = heimdall_obj_malloc( p_this, sizeof (*fdp) );
    if( unlikely(fdp == NULL) )
        return HEIMDALL_ENOMEM;

    if( sscanf( p_access->psz_location, "%"SCNu32":%"SCNu8":%"SCNu16":%d",
                &i_bus, &i_dev, &i_product_id, &i_track_id ) != 4 )
        return HEIMDALL_EGENERIC;

    if( LIBMTP_Detect_Raw_Devices( &p_rawdevices, &i_numrawdevices ) )
        return HEIMDALL_EGENERIC;

    int fd = -1;

    for( int i = 0; i < i_numrawdevices; i++ )
    {
        if( i_bus == p_rawdevices[i].bus_location &&
            i_dev == p_rawdevices[i].devnum &&
            i_product_id == p_rawdevices[i].device_entry.product_id )
        {
            LIBMTP_mtpdevice_t *p_device;

            p_device = LIBMTP_Open_Raw_Device( &p_rawdevices[i] );
            if( p_device == NULL )
                break;

            fd = heimdall_memfd();
            if( unlikely(fd == -1) )
                break;

            msg_Dbg( p_access, "copying to memory" );
            LIBMTP_Get_File_To_File_Descriptor( p_device, i_track_id, fd,
                                                NULL, NULL );
            LIBMTP_Release_Device( p_device );
            break;
        }
    }
    free( p_rawdevices );

    if( fd == -1 )
    {
        msg_Err( p_access, "cannot find %s", p_access->psz_location );
        return HEIMDALL_EGENERIC;
    }

    if( lseek( fd, 0, SEEK_SET ) ) /* Reset file descriptor offset */
    {
        close( fd );
        return HEIMDALL_EGENERIC;
    }

    *fdp = fd;
    p_access->p_sys = fdp;
    ACCESS_SET_CALLBACKS( Read, NULL, Control, Seek );
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Close: close the target
 *****************************************************************************/
static void Close( heimdall_object_t * p_this )
{
    stream_t *p_access = ( stream_t* )p_this;
    int *fdp = p_access->p_sys;

    heimdall_close ( *fdp );
}

/*****************************************************************************
 * Read: standard read on a file descriptor.
 *****************************************************************************/
static ssize_t Read( stream_t *p_access, void *p_buffer, size_t i_len )
{
    int *fdp = p_access->p_sys, fd = *fdp;
    ssize_t i_ret = read( fd, p_buffer, i_len );

    if( i_ret < 0 )
    {
        switch( errno )
        {
            case EINTR:
            case EAGAIN:
                break;

            default:
                msg_Err( p_access, "read failed: %s", heimdall_strerror_c(errno) );
                heimdall_dialog_display_error( p_access, _( "File reading failed" ),
                    _( "Heimdall could not read the file: %s" ),
                    heimdall_strerror(errno) );
                return 0;
        }
    }

    return i_ret;
}


/*****************************************************************************
 * Seek: seek to a specific location in a file
 *****************************************************************************/
static int Seek( stream_t *p_access, uint64_t i_pos )
{
    int *fdp = p_access->p_sys, fd = *fdp;

    if (lseek( fd, i_pos, SEEK_SET ) == (off_t)-1)
        return HEIMDALL_EGENERIC;
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( stream_t *p_access, int i_query, va_list args )
{
    int *fdp = p_access->p_sys, fd = *fdp;
    bool   *pb_bool;

    switch( i_query )
    {
        case STREAM_CAN_SEEK:
        case STREAM_CAN_FASTSEEK:
            pb_bool = va_arg( args, bool * );
            *pb_bool = true;
            break;

        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
            pb_bool = va_arg( args, bool * );
            *pb_bool = true;
            break;

        case STREAM_GET_SIZE:
        {
            uint64_t *s = va_arg( args, uint64_t * );
            struct stat st;
            if( fstat( fd, &st ) )
            {
                msg_Err( p_access, "fstat error: %s", heimdall_strerror_c(errno) );
                return HEIMDALL_EGENERIC;
            }
            *s = st.st_size;
            break;
        }

        case STREAM_GET_PTS_DELAY:
            *va_arg( args, heimdall_tick_t * ) =
                HEIMDALL_TICK_FROM_MS(var_InheritInteger( p_access, "file-caching" ));
            break;

        case STREAM_SET_PAUSE_STATE:
            /* Nothing to do */
            break;

        default:
            return HEIMDALL_EGENERIC;

    }
    return HEIMDALL_SUCCESS;
}
