/*****************************************************************************
 * cmd_input.cpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
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

#include "cmd_input.hpp"
#include "cmd_dialogs.hpp"
#include <heimdall_player.h>
#include <heimdall_playlist.h>

void CmdPlay::execute()
{
    heimdall_playlist_Lock( getPL() );
    bool b_empty = (heimdall_playlist_Count( getPL() ) == 0 );
    if( !b_empty )
    {
        heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
        if( heimdall_player_IsStarted( player ) )
            heimdall_playlist_Resume( getPL() );
        else
            heimdall_playlist_PlayAt( getPL(), 0 );
        heimdall_playlist_Unlock( getPL() );
    }
    else
    {
        heimdall_playlist_Unlock( getPL() );
        CmdDlgFile( getIntf() ).execute();
    }
}


void CmdPause::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_playlist_Pause( getPL() );
    heimdall_playlist_Unlock( getPL() );
}


void CmdStop::execute()
{
    heimdall_playlist_Lock( getPL() );
#if 0
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    if( heimdall_player_IsStarted( player ) )
        heimdall_playlist_Stop( getPL() );
    else
        heimdall_playlist_GoTo( getPL(), 0 );
#endif
    heimdall_playlist_Stop( getPL() );
    heimdall_playlist_GoTo( getPL(), -1 );

    heimdall_playlist_Unlock( getPL() );
}


void CmdSlower::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_DecrementRate( player );
    heimdall_playlist_Unlock( getPL() );
}


void CmdFaster::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_IncrementRate( player );
    heimdall_playlist_Unlock( getPL() );
}


void CmdMute::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_aout_ToggleMute( player );
    heimdall_playlist_Unlock( getPL() );
}


void CmdVolumeUp::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_aout_IncrementVolume( player, 1, NULL );
    heimdall_playlist_Unlock( getPL() );
}


void CmdVolumeDown::execute()
{
    heimdall_playlist_Lock( getPL() );
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    heimdall_player_aout_DecrementVolume( player, 1, NULL );
    heimdall_playlist_Unlock( getPL() );
}

