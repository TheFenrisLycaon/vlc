/*****************************************************************************
 * cmd_snapshot.cpp
 *****************************************************************************
 * Copyright (C) 2006-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Olivier Teulière <ipkiss@via.ecp.fr>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "cmd_snapshot.hpp"
#include <heimdall_player.h>

void CmdSnapshot::execute()
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_vout_Snapshot( player );
}


void CmdToggleRecord::execute()
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_ToggleRecording( player );
}


void CmdNextFrame::execute()
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_NextVideoFrame( player );
}


