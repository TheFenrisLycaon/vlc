/*****************************************************************************
 * heimdall.h: global header for libheimdall
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Derk-Jan Hartman <hartman at lycaria dot org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
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

#ifndef HEIMDALL_HEIMDALL_H
#define HEIMDALL_HEIMDALL_H 1

/**
 * \file
 * This file defines libheimdall new external API
 */

# ifdef __cplusplus
extern "C" {
# endif

#include "libheimdall.h"
#include "libheimdall_renderer_discoverer.h"
#include "libheimdall_picture.h"
#include "libheimdall_media.h"
#include "libheimdall_media_player.h"
#include "libheimdall_media_list.h"
#include "libheimdall_media_list_player.h"
#include "libheimdall_media_discoverer.h"
#include "libheimdall_dialog.h"
#include "libheimdall_downloader.h"
#include "libheimdall_version.h"

# ifdef __cplusplus
}
# endif

#endif /* _HEIMDALL_HEIMDALL_H */
