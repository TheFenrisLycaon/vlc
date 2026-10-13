/*****************************************************************************
 * variables.h: Generic lua<->heimdall variables interface
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

#ifndef HEIMDALL_LUA_VARIABLES_H
#define HEIMDALL_LUA_VARIABLES_H

int heimdalllua_var_toggle_or_set( lua_State *, heimdall_object_t *, const char * );

#define heimdalllua_var_toggle_or_set( a, b, c ) \
        heimdalllua_var_toggle_or_set( a, HEIMDALL_OBJECT( b ), c )

#endif

