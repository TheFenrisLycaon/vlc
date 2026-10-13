/*****************************************************************************
 * avparser.h
 *****************************************************************************
 * Copyright (C) 2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Denis Charmet <typx@videolan.org>
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

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_block.h>

#include "../codec/avcodec/avcodec.h"
#include "../codec/avcodec/avcommon.h"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
int  avparser_OpenPacketizer ( heimdall_object_t * );
void avparser_ClosePacketizer( heimdall_object_t * );

#define AVPARSER_MODULE \
    set_subcategory( SUBCAT_SOUT_PACKETIZER )           \
    set_description( N_("avparser packetizer") )        \
    set_capability( "video packetizer", 20 )            \
    set_callbacks( avparser_OpenPacketizer, avparser_ClosePacketizer )
