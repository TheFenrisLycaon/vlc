/*****************************************************************************
 * media_source.h
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
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

#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H

#include <heimdall_media_source.h>

heimdall_media_source_provider_t *
heimdall_media_source_provider_New(heimdall_object_t *parent);
#define heimdall_media_source_provider_New(obj) \
        heimdall_media_source_provider_New(HEIMDALL_OBJECT(obj))

void heimdall_media_source_provider_Delete(heimdall_media_source_provider_t *msp);

#endif
