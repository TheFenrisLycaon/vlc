/*****************************************************************************
 * time.cpp
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "time.hpp"


inline bool StreamTime::havePosition() const {

    float position = 0.0;
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );

    if( player )
    {
        heimdall_playlist_Lock( getPL() );
        position = heimdall_player_GetPosition( player );
        heimdall_playlist_Unlock( getPL() );
    }
    return player && (position != 0.0);
}


void StreamTime::set( float percentage, bool updateHeimdall )
{
    VarPercent::set( percentage );

    // Avoid looping forever...
    if( updateHeimdall )
    {
        heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
        heimdall_playlist_Lock( getPL() );
        heimdall_player_SetPosition( player, percentage );
        heimdall_playlist_Unlock( getPL() );
    }
}


std::string StreamTime::getAsStringPercent() const
{
    int value = (int)(100. * get());
    // 0 <= value <= 100, so we need 4 chars
    char str[4];
    snprintf( str, 4, "%d", value );
    return std::string(str);
}


std::string StreamTime::formatTime( int seconds, bool bShortFormat ) const
{
    char psz_time[MSTRTIME_MAX_SIZE];
    if( bShortFormat && (seconds < 60 * 60) )
    {
        snprintf( psz_time, MSTRTIME_MAX_SIZE, "%02d:%02d",
                  (int) (seconds / 60 % 60),
                  (int) (seconds % 60) );
    }
    else
    {
        snprintf( psz_time, MSTRTIME_MAX_SIZE, "%d:%02d:%02d",
                  (int) (seconds / (60 * 60)),
                  (int) (seconds / 60 % 60),
                  (int) (seconds % 60) );
    }
    return std::string(psz_time);
}


std::string StreamTime::getAsStringCurrTime( bool bShortFormat ) const
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    if( !havePosition() )
        return "-:--:--";

    heimdall_playlist_Lock( getPL() );
    heimdall_tick_t time = heimdall_player_GetTime( player );
    heimdall_playlist_Unlock( getPL() );
    return formatTime( SEC_FROM_HEIMDALL_TICK(time), bShortFormat );
}


std::string StreamTime::getAsStringTimeLeft( bool bShortFormat ) const
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    if( !havePosition() )
        return "-:--:--";

    heimdall_playlist_Lock( getPL() );
    heimdall_tick_t time = heimdall_player_GetTime( player );
    heimdall_tick_t duration = heimdall_player_GetLength( player );
    heimdall_playlist_Unlock( getPL() );
    return formatTime( SEC_FROM_HEIMDALL_TICK(duration - time), bShortFormat );
}


std::string StreamTime::getAsStringDuration( bool bShortFormat ) const
{
    heimdall_player_t *player = heimdall_playlist_GetPlayer( getPL() );
    if( !havePosition() )
        return "-:--:--";

    heimdall_playlist_Lock( getPL() );
    heimdall_tick_t duration = heimdall_player_GetLength( player );
    heimdall_playlist_Unlock( getPL() );
    return formatTime( SEC_FROM_HEIMDALL_TICK(duration), bShortFormat );
}
