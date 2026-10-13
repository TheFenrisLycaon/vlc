/*****************************************************************************
 * version.c: LibHeimdall version infos
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Derk-Jan Hartman <hartman at lycaria dot org>
 *          Rémi Denis-Courmont
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

/*****************************************************************************
 * Heimdall_CompileBy, Heimdall_CompileHost
 * Heimdall_Compiler, Heimdall_Changeset
 *****************************************************************************/
#define DECLARE_HEIMDALL_VERSION( func, var )                                    \
const char * HEIMDALL_##func ( void )                                            \
{                                                                           \
    return HEIMDALL_##var ;                                                      \
}

DECLARE_HEIMDALL_VERSION( CompileBy, COMPILE_BY )
DECLARE_HEIMDALL_VERSION( CompileHost, COMPILE_HOST )
DECLARE_HEIMDALL_VERSION( Compiler, COMPILER )
