/*****************************************************************************
 * heimdall_clock.h: clock API
 *****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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

#ifndef HEIMDALL_CLOCK_H
#define HEIMDALL_CLOCK_H 1

/**
 * The Heimdall clock API.
 *
 * The publicly exposed clock API is for now very restraint. For now, only a
 * subset of the clock is exposed and simplified for stream output modules.
 *
 * The actual clock implementation is mostly private for now as no other use
 * case is found.
 */

/**
 * Opaques Heimdall Clock types.
 */
typedef struct heimdall_clock_main_t heimdall_clock_main_t;
typedef struct heimdall_clock_t heimdall_clock_t;

#endif
